#pragma once

// perf: sampling profiler for turn-time work (make TARGET=Sample). A background thread samples the main thread's
// instruction pointer about once per millisecond and counts where it is: in this DLL, Python, the exe, Wine/graphics.
// For samples outside this DLL it also records the nearest return address into this DLL found on the stack, which
// shows which DLL code called the time-consuming Python or exe function. Results go to sampler.txt in the folder
// named by the CIV4_HARNESS_DIR environment variable (the Wine test harness), so release builds and normal games
// never run it. Read the file with tools/wine/sampler-report.py and the build's map file.
// It also counts GlobalDefines lookups by name (getDefineINT etc. look the name up in a string hash map each time).
// In every other build target these macros are empty.

#ifndef CvSampleProfiler_h
#define CvSampleProfiler_h

// Phases of a turn as the human sees it: the human's own turn, the human's end-of-turn moves (after ending the
// turn, until the human's units are done), and the rest of the interturn (AI players, CvGame::doTurn).
enum SamplerPhase { SAMPLER_HUMAN_TURN, SAMPLER_HUMAN_MOVES, SAMPLER_AI_TURNS, NUM_SAMPLER_PHASES };

#ifdef SAMPLE_PROFILER
void SampleProfilerUpdateBegin(SamplerPhase ePhase);
void SampleProfilerUpdateEnd();
void SampleProfilerSetPhase(SamplerPhase ePhase);
void SampleProfilerDump();
void SampleProfilerCountDefine(const char* szName);
#define SAMPLER_UPDATE_BEGIN(ePhase) SampleProfilerUpdateBegin(ePhase)
#define SAMPLER_UPDATE_END() SampleProfilerUpdateEnd()
#define SAMPLER_SET_PHASE(ePhase) SampleProfilerSetPhase(ePhase)
#define SAMPLER_DUMP() SampleProfilerDump()
#define SAMPLER_COUNT_DEFINE(szName) SampleProfilerCountDefine(szName)
#else
#define SAMPLER_UPDATE_BEGIN(ePhase)
#define SAMPLER_UPDATE_END()
#define SAMPLER_SET_PHASE(ePhase)
#define SAMPLER_DUMP()
#define SAMPLER_COUNT_DEFINE(szName)
#endif

#endif
