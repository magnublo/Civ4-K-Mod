# Civ4 K-Mod

K-Mod is a gameplay, AI, bugfix, and performance overhaul for **Sid Meier's Civilization IV: Beyond the Sword (BtS)**.
It is designed as a close-to-vanilla replacement mod: familiar game feel, stronger AI, and a large number of under-the-hood fixes and improvements.

This README is written for both:
- Human contributors and modders
- AI coding agents that need reliable repo context before editing

## What Is In This Repository

- `Assets/`: data-driven mod content (XML, Python, art, text, config)
- `CvGameCoreDLL/`: C++ game DLL source (core game logic and AI)
- `PublicMaps/`: public map scripts/scenarios
- `K-Mod.ini`: mod metadata and Civ4 mod-loading flags
- `readme.txt`: original distribution instructions and high-level feature summary
- `changelog.txt`: release history (current text says version 1.46)

## Quick Start (Players)

From the included `readme.txt`:

1. Use Civilization IV: Beyond the Sword **3.19** (Windows).
2. Unzip this project into your BtS Mods folder so you get:
	 - `.../Beyond the Sword/Mods/K-Mod`
3. Start BtS and load the mod via:
	 - Advanced -> Load a Mod -> K-Mod

Optional auto-load target:
- Launch `Civ4BeyondSword.exe` with `mod="K-Mod"`.

Notes:
- `K-Mod.ini` sets `NoCustomAssets = 1`, which helps reduce user-environment drift.
- `AllowPublicMaps = 1` allows public maps while using this mod.

## Architecture Map

### 1) DLL layer (C++)

Main location: `CvGameCoreDLL/`

High-impact files:
- AI behavior:
	- `CvPlayerAI.cpp`
	- `CvCityAI.cpp`
	- `CvSelectionGroupAI.cpp`
	- `CvTeamAI.cpp`
	- `BetterBTSAI.cpp`
- Core game systems:
	- `CvGame.cpp`
	- `CvCity.cpp`
	- `CvUnit.cpp`
	- `CvPlot.cpp`
- DLL bootstrap and profiling hooks:
	- `CvGameCoreDLL.cpp`
	- `FProfiler.h`
	- `FAssert.h`

Project/build files:
- `CvGameCoreDLL/CvGameCoreDLL.vcproj`
- `CvGameCoreDLL/Makefile`

### 2) Data and scripting layer (Assets)

Main locations:
- XML data: `Assets/xml/`
- Python scripts: `Assets/Python/`

Important Python entrypoints:
- `Assets/Python/EntryPoints/CvEventInterface.py`
- `Assets/Python/EntryPoints/CvGameInterfaceFile.py`

Notable integration details:
- BUG mod components are included under `Assets/Python/BUG/`.
- `CvEventInterface.py` wires through BUG event management.
- `CvGameInterfaceFile.py` routes game utility hooks through BUG's dispatcher.
- `Assets/xml/PythonCallbackDefines.xml` contains callback toggles; many are set to `0` by default.

## Build Notes (DLL)

This is a legacy Civ4 SDK toolchain project.

From `CvGameCoreDLL/Makefile` and `CvGameCoreDLL.vcproj`:
- Win32 target
- NMake-driven build configurations: `Debug`, `Release`, `Profile`
- Legacy dependencies expected by default paths:
	- Microsoft Visual C++ Toolkit 2003
	- Windows SDK v7.1
	- Python 2.4 headers/libs (`Python24`)
	- Boost 1.32 (`boost_python-vc71-mt-1_32.lib`)

Typical flow (on a compatible Windows setup):

1. Open `CvGameCoreDLL/CvGameCoreDLL.vcproj` in a compatible Visual Studio environment, or run `nmake` in `CvGameCoreDLL/`.
2. Build one of:
	 - `nmake Debug`
	 - `nmake Release`
	 - `nmake Profile`
3. Copy resulting `CvGameCoreDLL.dll` into the mod's `Assets/` folder if your build system does not already do this.

Important caveat:
- Linux/macOS native DLL builds are not directly supported by this project setup.

## Safe Change Workflow

When making gameplay or AI changes:

1. Edit as narrowly as possible in one layer first:
	 - XML/Python if data/script-level is sufficient
	 - DLL only when behavior cannot be expressed through XML/Python
2. Keep semantics isolated per commit/change set:
	 - AI logic
	 - Balance values
	 - UI text
3. Validate in-game on at least one fresh game and one mid/late save when relevant.
4. If touching AI code, run enough turns to observe behavior (not just compile).

## Debugging and Validation Checklist

- Game launches and mod loads without immediate XML/Python errors
- New game creation works on at least one random map and one `PublicMaps/` map
- End-turn progression does not hang/crash for a representative test game
- No obvious UI regressions in advisor, city, diplomacy, and combat flows touched by change
- Multiplayer-sensitive changes reviewed carefully (K-Mod historically emphasizes OOS stability)

## AI-Agent Playbook

Use this section as operating constraints for automated code editors.

### Repo facts to assume

- This is a mixed C++ + XML + Python Civ4 BtS mod.
- Core behavior is concentrated in `CvGameCoreDLL/` and data/script behavior in `Assets/`.
- XML path uses lowercase `Assets/xml/` in this repo.
- BUG framework is integrated into Python entrypoints.

### Edit policy

- Prefer the smallest possible diff.
- Do not mass-reformat legacy C++ or Python 2 style code.
- Preserve function signatures, serialization behavior, and deterministic logic unless the task requires changing them.
- Avoid broad enum/order changes in XML unless all dependent references are updated.

### High-risk areas

- AI decision loops (`CvPlayerAI`, `CvCityAI`, `CvTeamAI`, group/unit AI)
- Turn processing and combat resolution
- Save/load related state and global caches
- Callback wiring between DLL and Python (`PythonCallbackDefines.xml`, entrypoint modules)

### Recommended investigation order

1. Search for existing logic in the most specific subsystem file.
2. Check whether behavior is already controlled by XML define/value.
3. Confirm whether a BUG/Python callback is in play.
4. Change one layer first, then only cross layers if needed.

## External Source Material (Modding Reference)

Primary general Civ4 modding hub:
- CivFanatics Civ4 Modding Tutorials & Reference:
	- https://forums.civfanatics.com/forums/civ4-modding-tutorials-reference.177/

Relevant K-Mod community/forum archive:
- CivFanatics "Civ 4 - K-Mod: Far Beyond the Sword":
	- https://forums.civfanatics.com/forums/civ-4-k-mod-far-beyond-the-sword.470/
	- Main thread example: https://forums.civfanatics.com/threads/k-mod-far-beyond-the-sword.407049/

Useful topic categories from the CivFanatics modding forum include:
- DLL compilation guides
- XML basics and modular XML patterns
- Python callback and event modding
- map script references and art pipeline tutorials

## Contribution Guidance

If you are preparing changes for review:

1. Describe exactly which layer you changed (DLL, XML, Python, or mixed).
2. Include gameplay intent and expected behavior changes.
3. Call out any balance-impacting constants changed.
4. Include manual test notes (map/speed/era/turn range, SP/MP context).

## Original Author Notes

The bundled `readme.txt` identifies Karadoc as original author and positions K-Mod as an unofficial content/balance patch with strong AI and technical improvements.

Source repository:
- https://github.com/karadoc/Civ4-K-Mod