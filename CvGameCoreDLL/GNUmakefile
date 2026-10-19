#### CvGameCoreDLL.dll on Linux: MSVC 2003 Toolkit under Wine ####
# GNU make reads this file before "Makefile", which stays the nmake build for Windows.
#
#   make -j$(nproc)                 Release build -> Release/CvGameCoreDLL.dll
#   make -j$(nproc) TARGET=LogAI    Release plus LOG_AI: every AI decision goes to Logs/BBAI.log -> LogAI/CvGameCoreDLL.dll
#   make -j$(nproc) TARGET=Debug    unoptimised, asserts and LOG_AI -> Debug/CvGameCoreDLL.dll
#   make -j$(nproc) TARGET=Profile  Release plus K-Mod's internal profiler (PROFILE_FUNC etc.): one table per AI
#                                   interturn and per human-turn update in Logs/IFP_log.txt -> Profile/CvGameCoreDLL.dll
#   make -j$(nproc) TARGET=Sample   Release plus the sampling profiler (SampleProfiler.h) and a linker map file
#                                   -> Sample/CvGameCoreDLL.dll, Sample/CvGameCoreDLL.map
#   make -j$(nproc) CIVS=24         civ cap 24 instead of 48 (any TARGET) -> Release-24civs/CvGameCoreDLL.dll
#   make -j$(nproc) VERIFY=1        also run the replaced original code next to each speed change marked PERF_VERIFY
#                                   and log differences to Logs/perf_verify.log (any TARGET) -> Release-verify/...
#   make -j$(nproc) WPO=0           without whole-program optimization (/GL, /LTCG), which the optimized targets use
#   make clean                      removes the TARGET's build folder
#
# Paths can be overridden on the command line (make TOOLKIT=...) or in the environment.
#   TOOLKIT   Visual C++ Toolkit 2003 (bin/cl.exe, include/, lib/ plus msvcrt.lib and msvcprt.lib)
#   WINSDK    Windows SDK as unpacked by xwin "splat" (include/um, include/shared, lib/um/x86)
#   BOOST     Boost-1.32.0 and PYTHON Python24 from the BtS SDK (Beyond the Sword/CvGameCoreDLL)
#   WINEPREFIX  32-bit prefix used only for compiling
# Setup: see "Building On Linux (Wine)" in the repository's README.md.

CIV4_BUILD ?= $(HOME)/.local/share/civ4-build
TOOLKIT ?= $(CIV4_BUILD)/vc2003
WINSDK ?= $(CIV4_BUILD)/winsdk/sdk
BOOST ?= $(CIV4_BUILD)/sdk-deps/Boost-1.32.0
PYTHON ?= $(CIV4_BUILD)/sdk-deps/Python24
WINEPREFIX ?= $(HOME)/.local/share/wineprefixes/civ4-build
WINE ?= wine32
TARGET ?= Release
WPO ?= 1
CIVS ?=
ifeq ($(filter $(TARGET),Release LogAI Debug Profile Sample),)
$(error TARGET must be Release, LogAI, Debug, Profile or Sample)
endif

# cl.exe and link.exe are console programs; keep Wine away from the X display and quiet.
export WINEPREFIX
export WINEARCH := win32
export WINEDEBUG := -all
export DISPLAY :=

# Linux path -> path Wine programs understand (Z: is the root of the Linux file system).
winpath = Z:$(1)

CL := $(WINE) $(TOOLKIT)/bin/cl.exe
LINK := $(WINE) $(TOOLKIT)/bin/link.exe

OUT := $(TARGET)$(if $(CIVS),-$(CIVS)civs)$(if $(VERIFY),-verify)$(if $(filter 0,$(WPO)),-nowpo)
DLL := $(OUT)/CvGameCoreDLL.dll
PCH := $(OUT)/CvGameCoreDLL.pch
PCH_OBJ := $(OUT)/_precompile.obj

# CvTextScreens.cpp is not part of the DLL (the nmake build blacklists it too).
SOURCES := $(filter-out _precompile.cpp CvTextScreens.cpp,$(wildcard *.cpp))
OBJS := $(SOURCES:%.cpp=$(OUT)/%.obj)
# Nearly every header is in the precompiled header, so any header change rebuilds everything.
HEADERS := $(wildcard *.h)

GLOBAL_CFLAGS := /nologo /GR /Gy /W3 /EHsc /Gd /Gm- /DWIN32 /D_WINDOWS /D_USRDLL /DCVGAMECOREDLL_EXPORTS
ifneq ($(CIVS),)
GLOBAL_CFLAGS += /DCIV_PLAYER_CAP=$(CIVS)
endif
ifneq ($(VERIFY),)
GLOBAL_CFLAGS += /DPERF_VERIFY
endif
ifeq ($(TARGET),Debug)
# /Z7 instead of /Zi: debug info goes into each .obj, so parallel compiles don't fight over one PDB.
CFLAGS := /MD /Z7 /Od /D_DEBUG /DLOG_AI /RTC1 $(GLOBAL_CFLAGS)
LDFLAGS := /DEBUG /PDB:$(OUT)/CvGameCoreDLL.pdb
EXTRA_LIBS := msvcprt.lib
else ifeq ($(TARGET),LogAI)
# BBAI's AI decision logging (BetterBTSAI.h: all log levels 3) without the Debug build's slowness and assert popups
CFLAGS := /MD /O2 /Oy /Oi /Og /G7 /DNDEBUG /DFINAL_RELEASE /DLOG_AI $(GLOBAL_CFLAGS)
LDFLAGS := /OPT:REF /OPT:ICF
EXTRA_LIBS :=
else ifeq ($(TARGET),Profile)
# The same flags as the nmake Makefile's Profile target; the profiler's timers make every PROFILE'd call slower.
CFLAGS := /MD /O2 /Oy /Oi /Og /G7 /DNDEBUG /DFINAL_RELEASE /DFP_PROFILE_ENABLE /DUSE_INTERNAL_PROFILER $(GLOBAL_CFLAGS)
LDFLAGS := /OPT:REF /OPT:ICF
EXTRA_LIBS :=
else ifeq ($(TARGET),Sample)
# Release code plus the sampler; the map file lets tools/wine/sampler-report.py name the sampled functions.
CFLAGS := /MD /O2 /Oy /Oi /Og /G7 /DNDEBUG /DFINAL_RELEASE /DSAMPLE_PROFILER $(GLOBAL_CFLAGS)
# No /OPT:ICF: folding identical functions would give several functions one address and name in the report.
LDFLAGS := /OPT:REF /OPT:NOICF /MAP:$(OUT)/CvGameCoreDLL.map
EXTRA_LIBS :=
else
CFLAGS := /MD /O2 /Oy /Oi /Og /G7 /DNDEBUG /DFINAL_RELEASE $(GLOBAL_CFLAGS)
LDFLAGS := /OPT:REF /OPT:ICF
EXTRA_LIBS :=
endif

# perf: whole-program optimization lets the compiler inline across source files (AdvCiv measured ~10% shorter
# AI turns with the same toolkit). It makes linking slower and incremental builds pointless, so Debug goes without.
ifneq ($(WPO),0)
ifneq ($(TARGET),Debug)
CFLAGS += /GL
LDFLAGS += /LTCG
endif
endif

INCS := /I$(call winpath,$(BOOST)/include) /I$(call winpath,$(PYTHON)/include) \
	/I$(call winpath,$(TOOLKIT)/include) \
	/I$(call winpath,$(WINSDK)/include/um) /I$(call winpath,$(WINSDK)/include/shared)
LIBPATHS := /LIBPATH:$(call winpath,$(TOOLKIT)/lib) /LIBPATH:$(call winpath,$(WINSDK)/lib/um/x86) \
	/LIBPATH:$(call winpath,$(PYTHON)/libs) /LIBPATH:$(call winpath,$(BOOST)/libs)
LIBS := boost_python-vc71-mt-1_32.lib winmm.lib user32.lib $(EXTRA_LIBS)

.PHONY: all clean FORCE
all: $(DLL)

$(OUT):
	mkdir -p $@

# The compiler and linker flags of the last build: when they change, everything is rebuilt.
FLAGS_FILE := $(OUT)/build-flags.txt
$(FLAGS_FILE): FORCE | $(OUT)
	@echo '$(CFLAGS) $(LDFLAGS)' | cmp -s - $@ || echo '$(CFLAGS) $(LDFLAGS)' >$@

$(PCH) $(PCH_OBJ) &: _precompile.cpp $(HEADERS) $(FLAGS_FILE) | $(OUT)
	$(CL) $(CFLAGS) $(INCS) /YcCvGameCoreDLL.h /Fp$(PCH) /Fo$(PCH_OBJ) /c _precompile.cpp

$(OUT)/%.obj: %.cpp $(PCH) | $(OUT)
	$(CL) $(CFLAGS) $(INCS) /YuCvGameCoreDLL.h /Fp$(PCH) /Fo$@ /c $<

$(DLL): $(PCH_OBJ) $(OBJS) $(FLAGS_FILE)
	printf '%s\n' $(PCH_OBJ) $(OBJS) > $(OUT)/objects.rsp
	$(LINK) /nologo /DLL /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /TLBID:1 /INCREMENTAL:NO $(LDFLAGS) \
		/out:$@ /IMPLIB:$(OUT)/CvGameCoreDLL.lib $(LIBPATHS) $(LIBS) @$(OUT)/objects.rsp

clean:
	rm -rf -- $(OUT)
