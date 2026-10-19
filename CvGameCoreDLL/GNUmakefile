#### CvGameCoreDLL.dll on Linux: MSVC 2003 Toolkit under Wine ####
# GNU make reads this file before "Makefile", which stays the nmake build for Windows.
#
#   make -j$(nproc)                 Release build -> Release/CvGameCoreDLL.dll
#   make -j$(nproc) TARGET=LogAI    Release plus LOG_AI: every AI decision goes to Logs/BBAI.log -> LogAI/CvGameCoreDLL.dll
#   make -j$(nproc) TARGET=Debug    unoptimised, asserts and LOG_AI -> Debug/CvGameCoreDLL.dll
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
ifeq ($(filter $(TARGET),Release LogAI Debug),)
$(error TARGET must be Release, LogAI or Debug)
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

OUT := $(TARGET)
DLL := $(OUT)/CvGameCoreDLL.dll
PCH := $(OUT)/CvGameCoreDLL.pch
PCH_OBJ := $(OUT)/_precompile.obj

# CvTextScreens.cpp is not part of the DLL (the nmake build blacklists it too).
SOURCES := $(filter-out _precompile.cpp CvTextScreens.cpp,$(wildcard *.cpp))
OBJS := $(SOURCES:%.cpp=$(OUT)/%.obj)
# Nearly every header is in the precompiled header, so any header change rebuilds everything.
HEADERS := $(wildcard *.h)

GLOBAL_CFLAGS := /nologo /GR /Gy /W3 /EHsc /Gd /Gm- /DWIN32 /D_WINDOWS /D_USRDLL /DCVGAMECOREDLL_EXPORTS
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
else
CFLAGS := /MD /O2 /Oy /Oi /Og /G7 /DNDEBUG /DFINAL_RELEASE $(GLOBAL_CFLAGS)
LDFLAGS := /OPT:REF /OPT:ICF
EXTRA_LIBS :=
endif

INCS := /I$(call winpath,$(BOOST)/include) /I$(call winpath,$(PYTHON)/include) \
	/I$(call winpath,$(TOOLKIT)/include) \
	/I$(call winpath,$(WINSDK)/include/um) /I$(call winpath,$(WINSDK)/include/shared)
LIBPATHS := /LIBPATH:$(call winpath,$(TOOLKIT)/lib) /LIBPATH:$(call winpath,$(WINSDK)/lib/um/x86) \
	/LIBPATH:$(call winpath,$(PYTHON)/libs) /LIBPATH:$(call winpath,$(BOOST)/libs)
LIBS := boost_python-vc71-mt-1_32.lib winmm.lib user32.lib $(EXTRA_LIBS)

.PHONY: all clean
all: $(DLL)

$(OUT):
	mkdir -p $@

$(PCH) $(PCH_OBJ) &: _precompile.cpp $(HEADERS) | $(OUT)
	$(CL) $(CFLAGS) $(INCS) /YcCvGameCoreDLL.h /Fp$(PCH) /Fo$(PCH_OBJ) /c _precompile.cpp

$(OUT)/%.obj: %.cpp $(PCH) | $(OUT)
	$(CL) $(CFLAGS) $(INCS) /YuCvGameCoreDLL.h /Fp$(PCH) /Fo$@ /c $<

$(DLL): $(PCH_OBJ) $(OBJS)
	printf '%s\n' $(PCH_OBJ) $(OBJS) > $(OUT)/objects.rsp
	$(LINK) /nologo /DLL /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /TLBID:1 /INCREMENTAL:NO $(LDFLAGS) \
		/out:$@ /IMPLIB:$(OUT)/CvGameCoreDLL.lib $(LIBPATHS) $(LIBS) @$(OUT)/objects.rsp

clean:
	rm -rf -- $(OUT)
