// perf: sampling profiler, see SampleProfiler.h. Compiled to nothing unless SAMPLE_PROFILER is defined.

#include "CvGameCoreDLL.h"
#include "SampleProfiler.h"

#ifdef SAMPLE_PROFILER

#include <stdio.h>
#include <stdlib.h>

namespace
{
	// A sample's tag: bit 0 = the main thread is inside CvGame::update, bits 1-2 = SamplerPhase. The interturn
	// (what the human waits for) is SAMPLER_HUMAN_MOVES plus SAMPLER_AI_TURNS.
	enum { TAG_UPDATE = 1, NUM_TAGS = 2 * NUM_SAMPLER_PHASES };
	inline bool isInterturnTag(int iTag) { return (iTag >> 1) != SAMPLER_HUMAN_TURN; }

	struct Count { DWORD key; DWORD n; };
	struct Pair { DWORD a; DWORD b; DWORD n; };

	const int SELF_SIZE = 1 << 17; // per tag; key = instruction address / 16
	const int INCL_SIZE = 1 << 17; // interturn only; key = address / 16 of every DLL function on the stack
	const int EXT_SIZE = 1 << 16;  // interturn samples outside this DLL; (address / 4096, nearest DLL caller / 16)
	const int MAX_FRAMES = 64;
	const DWORD STACK_SCAN_BYTES = 32768;

	Count g_aSelf[NUM_TAGS][SELF_SIZE];
	const int DEFINE_SIZE = 1 << 12; // GlobalDefines lookups; key = address of the name (string literals)
	Count g_aDefines[DEFINE_SIZE];
	Count g_aIncl[INCL_SIZE];
	Pair g_aExt[EXT_SIZE];
	DWORD g_aiTotal[NUM_TAGS];
	DWORD g_iLost = 0;

	volatile LONG g_bInUpdate = 0;
	volatile LONG g_iPhase = SAMPLER_HUMAN_TURN;
	volatile LONG g_bStop = 0;
	HANDLE g_hMainThread = NULL;
	DWORD g_iStackBase = 0;
	DWORD g_iDllBase = 0;
	DWORD g_iDllEnd = 0;
	bool g_bStarted = false;
	char g_szOutPath[MAX_PATH];

	DWORD slot(DWORD key, int iSize)
	{
		return (key * 2654435761UL) & (iSize - 1);
	}

	void addCount(Count* aTable, int iSize, DWORD key)
	{
		DWORD i = slot(key, iSize);
		for (int iProbe = 0; iProbe < 256; iProbe++, i = (i + 1) & (iSize - 1))
		{
			if (aTable[i].key == key)
			{
				aTable[i].n++;
				return;
			}
			if (aTable[i].key == 0)
			{
				aTable[i].key = key;
				aTable[i].n = 1;
				return;
			}
		}
		g_iLost++;
	}

	void addPair(DWORD a, DWORD b)
	{
		DWORD i = slot(a * 31 + b, EXT_SIZE);
		for (int iProbe = 0; iProbe < 256; iProbe++, i = (i + 1) & (EXT_SIZE - 1))
		{
			if (g_aExt[i].a == a && g_aExt[i].b == b)
			{
				g_aExt[i].n++;
				return;
			}
			if (g_aExt[i].a == 0)
			{
				g_aExt[i].a = a;
				g_aExt[i].b = b;
				g_aExt[i].n = 1;
				return;
			}
		}
		g_iLost++;
	}

	// Does v look like a return address into this DLL, i.e. does a call instruction end just before it?
	bool isDllReturnAddress(DWORD v)
	{
		if (v < g_iDllBase + 0x1000 || v >= g_iDllEnd)
			return false;
		const BYTE* p = (const BYTE*)v;
		return p[-5] == 0xE8 // call rel32
			|| (p[-2] == 0xFF && (p[-1] & 0x38) == 0x10) // call reg / call [reg]
			|| (p[-3] == 0xFF && (p[-2] & 0x38) == 0x10) // call [reg+disp8], [sib]
			|| (p[-4] == 0xFF && (p[-3] & 0x38) == 0x10) // call [sib+disp8]
			|| (p[-6] == 0xFF && (p[-5] & 0x38) == 0x10) // call [disp32], [reg+disp32]
			|| (p[-7] == 0xFF && (p[-6] & 0x38) == 0x10); // call [sib+disp32]
	}

	int scanStack(DWORD iEsp, DWORD* aiFrames)
	{
		int iFound = 0;
		if (iEsp >= g_iStackBase || iEsp + 16 * 1024 * 1024 < g_iStackBase)
			return 0; // not on the main thread's own stack
		__try
		{
			DWORD iEnd = std::min(iEsp + STACK_SCAN_BYTES, g_iStackBase);
			for (const DWORD* p = (const DWORD*)iEsp; (DWORD)p < iEnd && iFound < MAX_FRAMES; ++p)
			{
				if (isDllReturnAddress(*p))
					aiFrames[iFound++] = *p;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return iFound;
	}

	DWORD WINAPI samplerThread(LPVOID)
	{
		CONTEXT ctx;
		DWORD aiFrames[MAX_FRAMES];
		while (!g_bStop)
		{
			Sleep(1);
			int iTag = (g_bInUpdate ? TAG_UPDATE : 0) | (g_iPhase << 1);
			if (SuspendThread(g_hMainThread) == (DWORD)-1)
				continue;
			ctx.ContextFlags = CONTEXT_CONTROL;
			if (!GetThreadContext(g_hMainThread, &ctx))
			{
				ResumeThread(g_hMainThread);
				continue;
			}
			int iFrames = isInterturnTag(iTag) ? scanStack(ctx.Esp, aiFrames) : 0;
			ResumeThread(g_hMainThread);

			DWORD iEip = ctx.Eip;
			addCount(g_aSelf[iTag], SELF_SIZE, iEip >> 4);
			g_aiTotal[iTag]++;
			if (isInterturnTag(iTag))
			{
				bool bInDll = iEip >= g_iDllBase && iEip < g_iDllEnd;
				if (bInDll)
					addCount(g_aIncl, INCL_SIZE, iEip >> 4);
				else
					addPair(iEip >> 12, iFrames > 0 ? aiFrames[0] >> 4 : 0);
				for (int i = 0; i < iFrames; i++)
				{
					// each return address once per sample (recursion would count twice)
					bool bSeen = false;
					for (int j = 0; j < i && !bSeen; j++)
						bSeen = aiFrames[j] == aiFrames[i];
					if (!bSeen)
						addCount(g_aIncl, INCL_SIZE, aiFrames[i] >> 4);
				}
			}
		}
		return 0;
	}

	void start()
	{
		g_bStarted = true;
		const char* szDir = getenv("CIV4_HARNESS_DIR");
		if (szDir == NULL || strlen(szDir) > MAX_PATH - 20)
			return;
		_snprintf(g_szOutPath, MAX_PATH, "%s\\sampler.txt", szDir);

		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery((LPCVOID)&SampleProfilerUpdateBegin, &mbi, sizeof(mbi));
		g_iDllBase = (DWORD)mbi.AllocationBase;
		const IMAGE_DOS_HEADER* pDos = (const IMAGE_DOS_HEADER*)g_iDllBase;
		const IMAGE_NT_HEADERS* pNt = (const IMAGE_NT_HEADERS*)(g_iDllBase + pDos->e_lfanew);
		g_iDllEnd = g_iDllBase + pNt->OptionalHeader.SizeOfImage;

		DWORD iStackBase;
		__asm
		{
			mov eax, fs:[4]
			mov iStackBase, eax
		}
		g_iStackBase = iStackBase;

		if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &g_hMainThread,
			THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, 0))
			return;
		HANDLE hThread = CreateThread(NULL, 0, samplerThread, NULL, 0, NULL);
		if (hThread != NULL)
		{
			SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
			CloseHandle(hThread);
		}
	}

	typedef BOOL (WINAPI *GetModuleHandleExA_t)(DWORD, LPCSTR, HMODULE*);

	// Module containing an address: writes "MOD base end name" the first time a module is seen.
	struct ModuleCache
	{
		DWORD aiBase[512];
		DWORD aiEnd[512];
		int iNum;
		GetModuleHandleExA_t pGetModuleHandleExA;
		FILE* pFile;

		void note(DWORD iAddress)
		{
			for (int i = 0; i < iNum; i++)
			{
				if (iAddress >= aiBase[i] && iAddress < aiEnd[i])
					return;
			}
			HMODULE hModule = NULL;
			if (pGetModuleHandleExA == NULL || iNum >= 512
				|| !pGetModuleHandleExA(4 | 2, (LPCSTR)iAddress, &hModule) || hModule == NULL) // FROM_ADDRESS | UNCHANGED_REFCOUNT
				return;
			DWORD iBase = (DWORD)hModule;
			const IMAGE_DOS_HEADER* pDos = (const IMAGE_DOS_HEADER*)iBase;
			const IMAGE_NT_HEADERS* pNt = (const IMAGE_NT_HEADERS*)(iBase + pDos->e_lfanew);
			char szName[MAX_PATH];
			if (GetModuleFileNameA(hModule, szName, MAX_PATH) == 0)
				strcpy(szName, "?");
			aiBase[iNum] = iBase;
			aiEnd[iNum] = iBase + pNt->OptionalHeader.SizeOfImage;
			iNum++;
			fprintf(pFile, "MOD %08lx %08lx %s\n", iBase, aiEnd[iNum - 1], szName);
		}
	};
}

void SampleProfilerUpdateBegin(SamplerPhase ePhase)
{
	if (!g_bStarted)
		start();
	g_iPhase = ePhase;
	g_bInUpdate = 1;
}

void SampleProfilerUpdateEnd()
{
	g_bInUpdate = 0;
}

void SampleProfilerCountDefine(const char* szName)
{
	if (g_iPhase != SAMPLER_HUMAN_TURN) // main thread only, so no race with the sampler thread
		addCount(g_aDefines, DEFINE_SIZE, (DWORD)szName);
}

void SampleProfilerSetPhase(SamplerPhase ePhase)
{
	g_iPhase = ePhase;
}

// Writes the counts so far (cumulative since the game was started).
void SampleProfilerDump()
{
	if (g_hMainThread == NULL)
		return;
	FILE* pFile = fopen(g_szOutPath, "w");
	if (pFile == NULL)
		return;
	ModuleCache modules;
	modules.iNum = 0;
	modules.pFile = pFile;
	modules.pGetModuleHandleExA = (GetModuleHandleExA_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetModuleHandleExA");
	fprintf(pFile, "DLL %08lx %08lx\n", g_iDllBase, g_iDllEnd);
	fprintf(pFile, "LOST %lu\n", g_iLost);
	for (int iTag = 0; iTag < NUM_TAGS; iTag++)
	{
		fprintf(pFile, "TOTAL %d %lu\n", iTag, g_aiTotal[iTag]);
		for (int i = 0; i < SELF_SIZE; i++)
		{
			if (g_aSelf[iTag][i].key != 0)
			{
				DWORD iAddress = g_aSelf[iTag][i].key << 4;
				modules.note(iAddress);
				fprintf(pFile, "SELF %d %08lx %lu\n", iTag, iAddress, g_aSelf[iTag][i].n);
			}
		}
	}
	for (int i = 0; i < INCL_SIZE; i++)
	{
		if (g_aIncl[i].key != 0)
			fprintf(pFile, "INCL %08lx %lu\n", g_aIncl[i].key << 4, g_aIncl[i].n);
	}
	for (int i = 0; i < EXT_SIZE; i++)
	{
		if (g_aExt[i].a != 0)
			fprintf(pFile, "EXT %08lx %08lx %lu\n", g_aExt[i].a << 12, g_aExt[i].b << 4, g_aExt[i].n);
	}
	for (int i = 0; i < DEFINE_SIZE; i++)
	{
		if (g_aDefines[i].key != 0)
			fprintf(pFile, "DEFINE %lu %s\n", g_aDefines[i].n, (const char*)g_aDefines[i].key);
	}
	fclose(pFile);
}

#endif
