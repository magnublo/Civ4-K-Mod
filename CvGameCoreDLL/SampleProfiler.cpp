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
	// Raw interturn stacks (stacks.bin next to sampler.txt): per sample a header (0x5A000000 | tag << 8 | number of
	// addresses), the instruction pointer, then every value on the stack that looks like a return address into this
	// DLL or the exe, innermost first. tools/wine/sampler-stacks.py keeps the frames whose call instruction targets the
	// function of the frame inside it, which drops stale return addresses that a plain scan counts.
	const int RAW_DWORDS = 6 * 1024 * 1024;

	Count g_aSelf[NUM_TAGS][SELF_SIZE];
	const int DEFINE_SIZE = 1 << 12; // GlobalDefines lookups; key = address of the name (string literals)
	Count g_aDefines[DEFINE_SIZE];
	Count g_aIncl[INCL_SIZE];
	Pair g_aExt[EXT_SIZE];
	DWORD g_aiTotal[NUM_TAGS];
	unsigned __int64 g_aiCounter[NUM_SAMPLER_COUNTERS]; // (not double: Direct3D runs the FPU at single precision)
	int g_aiCounterMax[NUM_SAMPLER_COUNTERS];
	const char* g_aszCounter[NUM_SAMPLER_COUNTERS] = { "pf_searches", "pf_reused_tree", "pf_found", "pf_expansions",
		"pf_open_scanned", "pf_edges", "pf_new_nodes", "pf_path_steps", "pf_expansions_found", "pf_fresh",
		"pf_expansions_fresh", "pf_weight_tiny" };
	DWORD g_iLost = 0;

	volatile LONG g_bInUpdate = 0;
	volatile LONG g_iPhase = SAMPLER_HUMAN_TURN;
	volatile LONG g_bStop = 0;
	HANDLE g_hMainThread = NULL;
	DWORD g_iStackBase = 0;
	DWORD g_iDllBase = 0;
	DWORD g_iDllEnd = 0;
	DWORD g_iExeBase = 0;
	DWORD g_iExeEnd = 0;
	DWORD* g_aiRaw = NULL;
	volatile LONG g_iRawUsed = 0;
	LONG g_iRawWritten = 0;
	DWORD g_iRawLost = 0;
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

	// Does v look like a return address, i.e. does a call instruction end just before it? (v is in this DLL or the exe)
	bool isCallReturn(DWORD v)
	{
		const BYTE* p = (const BYTE*)v;
		return p[-5] == 0xE8 // call rel32
			|| (p[-2] == 0xFF && (p[-1] & 0x38) == 0x10) // call reg / call [reg]
			|| (p[-3] == 0xFF && (p[-2] & 0x38) == 0x10) // call [reg+disp8], [sib]
			|| (p[-4] == 0xFF && (p[-3] & 0x38) == 0x10) // call [sib+disp8]
			|| (p[-6] == 0xFF && (p[-5] & 0x38) == 0x10) // call [disp32], [reg+disp32]
			|| (p[-7] == 0xFF && (p[-6] & 0x38) == 0x10); // call [sib+disp32]
	}

	bool isDllReturnAddress(DWORD v)
	{
		return v >= g_iDllBase + 0x1000 && v < g_iDllEnd && isCallReturn(v);
	}

	bool isExeReturnAddress(DWORD v)
	{
		return v >= g_iExeBase + 0x1000 && v < g_iExeEnd && isCallReturn(v);
	}

	// DLL return addresses on the stack into aiFrames; DLL and exe ones into aiRaw (if not NULL), innermost first.
	int scanStack(DWORD iEsp, DWORD* aiFrames, DWORD* aiRaw, int* piRaw)
	{
		int iFound = 0;
		int iRaw = 0;
		if (iEsp >= g_iStackBase || iEsp + 16 * 1024 * 1024 < g_iStackBase)
			return 0; // not on the main thread's own stack
		__try
		{
			DWORD iEnd = std::min(iEsp + STACK_SCAN_BYTES, g_iStackBase);
			for (const DWORD* p = (const DWORD*)iEsp; (DWORD)p < iEnd && (iFound < MAX_FRAMES || iRaw < MAX_FRAMES); ++p)
			{
				if (isDllReturnAddress(*p))
				{
					if (iFound < MAX_FRAMES)
						aiFrames[iFound++] = *p;
					if (aiRaw != NULL && iRaw < MAX_FRAMES)
						aiRaw[iRaw++] = *p;
				}
				else if (aiRaw != NULL && iRaw < MAX_FRAMES && isExeReturnAddress(*p))
					aiRaw[iRaw++] = *p;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		if (piRaw != NULL)
			*piRaw = iRaw;
		return iFound;
	}

	void addRaw(int iTag, DWORD iEip, const DWORD* aiRaw, int iRaw)
	{
		LONG iUsed = g_iRawUsed;
		if (g_aiRaw == NULL || iUsed + 2 + iRaw > RAW_DWORDS)
		{
			g_iRawLost++;
			return;
		}
		g_aiRaw[iUsed] = 0x5A000000 | (iTag << 8) | iRaw;
		g_aiRaw[iUsed + 1] = iEip;
		for (int i = 0; i < iRaw; i++)
			g_aiRaw[iUsed + 2 + i] = aiRaw[i];
		InterlockedExchange(&g_iRawUsed, iUsed + 2 + iRaw); // publish after the record is complete
	}

	DWORD WINAPI samplerThread(LPVOID)
	{
		CONTEXT ctx;
		DWORD aiFrames[MAX_FRAMES];
		DWORD aiRaw[MAX_FRAMES];
		int iRaw = 0;
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
			iRaw = 0;
			int iFrames = isInterturnTag(iTag) ? scanStack(ctx.Esp, aiFrames, aiRaw, &iRaw) : 0;
			ResumeThread(g_hMainThread);
			if (isInterturnTag(iTag))
				addRaw(iTag, ctx.Eip, aiRaw, iRaw);

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
		g_iExeBase = (DWORD)GetModuleHandleA(NULL);
		const IMAGE_DOS_HEADER* pExeDos = (const IMAGE_DOS_HEADER*)g_iExeBase;
		const IMAGE_NT_HEADERS* pExeNt = (const IMAGE_NT_HEADERS*)(g_iExeBase + pExeDos->e_lfanew);
		g_iExeEnd = g_iExeBase + pExeNt->OptionalHeader.SizeOfImage;
		g_aiRaw = (DWORD*)VirtualAlloc(NULL, RAW_DWORDS * sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

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

void SampleProfilerCount(SamplerCounter eCounter, int iAmount)
{
	if (g_iPhase != SAMPLER_HUMAN_TURN) // main thread only
	{
		g_aiCounter[eCounter] += iAmount;
		if (iAmount > g_aiCounterMax[eCounter])
			g_aiCounterMax[eCounter] = iAmount;
	}
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
	fprintf(pFile, "EXE %08lx %08lx\n", g_iExeBase, g_iExeEnd);
	fprintf(pFile, "LOST %lu\n", g_iLost);
	fprintf(pFile, "RAWLOST %lu\n", g_iRawLost);
	for (int iC = 0; iC < NUM_SAMPLER_COUNTERS; iC++)
		fprintf(pFile, "COUNTER %s %I64u %d\n", g_aszCounter[iC], g_aiCounter[iC], g_aiCounterMax[iC]);
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

	// raw stacks recorded since the last dump, appended to stacks.bin
	LONG iUsed = g_iRawUsed;
	if (g_aiRaw != NULL && iUsed > g_iRawWritten)
	{
		char szRawPath[MAX_PATH];
		strcpy(szRawPath, g_szOutPath);
		char* pSlash = strrchr(szRawPath, '\\');
		strcpy(pSlash != NULL ? pSlash + 1 : szRawPath, "stacks.bin");
		FILE* pRaw = fopen(szRawPath, "ab");
		if (pRaw != NULL)
		{
			fwrite(g_aiRaw + g_iRawWritten, sizeof(DWORD), iUsed - g_iRawWritten, pRaw);
			fclose(pRaw);
			g_iRawWritten = iUsed;
		}
	}
}

#endif
