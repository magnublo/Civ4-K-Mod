# Civ4 K-Mod

K-Mod is a gameplay, AI, bugfix, and performance overhaul for **Sid Meier's Civilization IV: Beyond the Sword (BtS)**.
It is designed as a close-to-vanilla replacement mod: familiar game feel, stronger AI, and a large number of under-the-hood fixes and improvements.

This README is written for both:
- Human contributors and modders
- AI coding agents that need reliable repo context before editing

## What Is In This Repository

- `Assets/`: data-driven mod content (XML, Python, art, text, config)
- `CvGameCoreDLL/`: C++ game DLL source (core game logic and AI)
- `PublicMaps/`: public map scripts/scenarios (includes ported Earth/Europe maps)
- `Settings/`: configuration templates (unit naming, domestic advisor)
- `K-Mod.ini`: mod metadata and Civ4 mod-loading flags
- `K-Mod, more resources.ini`: optional alternative launcher profile
- `readme.txt`: original distribution instructions and high-level feature summary
- `changelog.txt`: release history (current text says version 1.46)

## Features Added In This Fork (DLL)

Search the sources for the tags in brackets.

- **48 civilizations** (`CIV_PLAYER_CAP` in `CvDefines.h`; K-Mod's default is 18). Unused civ slots cost little in this DLL: the same 24-civ game (Huge, 200 turns of AI autoplay, identical on both) took 2% longer with the 48-civ DLL than with a 24-civ one. So the CI builds only the 48-civ DLL (artifact `CvGameCoreDLL-48civs`, cap checked with `.github/scripts/check_civ_cap.py`). Other caps build locally: `make CIVS=24` → `Release-24civs/`; nmake: `nmake "CUSTOM_CFLAGS=/DCIV_PLAYER_CAP=24" Release`. Saves only load in a DLL with the same cap.
- **Fewer early AI wars** [`early war`]: the AI starts no war plans of its own before turn 50 (K-Mod: 30), and until turn 150 its war plans are less likely: a third of the usual chance at turn 50, back to normal by turn 150. Both are Normal-speed turns, scaled by the game speed's `iBarbPercent` (Marathon and Marathon Accelerated ×4: turns 200 and 600). Early rushes ("dagger" strategy) are damped the same way. Unlike K-Mod, a "crowded" world no longer ends the no-war period early (cities don't scale with game speed). Tune it in `Assets/xml/GlobalDefinesAlt.xml`: `AI_EARLY_WAR_GATE_TURNS`, `AI_EARLY_WAR_CROWDED_WORLD` (1 = K-Mod's exception), `AI_EARLY_WAR_RAMP_TURNS`, `AI_EARLY_WAR_RAND_PERCENT` (100 turns the damping off).
- **Pre-chop** [`pre-chop`]: workers on a forest get a second chop button (axe with a pause badge). They chop until one worker-turn is left and stop, so a normal chop later finishes in one turn and delivers the hammers then. Ctrl+click on any build button does the same for that build. The flag rides in `MissionData::iData2` (`BUILD_MISSION_STOP_SHORT`).
- **Pre-build** [`pre-build`]: a toggle at the start of the city screen's wonder row (or the project row) marks the first wonder or project in the queue. The city builds it until one turn of production is left, then pauses it with the hammers stored and moves on. Queue it again to finish it in one turn. The queue shows flagged items in yellow. The flag rides in `OrderData::iData2` (`ORDER_DATA_PREBUILD`) and is set through `TASK_PREBUILD`. Whipping or buying a flagged wonder clears the flag.

Texts: `Assets/xml/Text/CIV4GameText_PreChopPreBuild.xml`. Buttons: `Assets/Art/Interface/Buttons/Actions/PreChop.dds`, `PreBuild.dds` and `PreBuildOn.dds`. UI: `Assets/Python/Screens/CvMainInterface.py`. None of this changes the savegame format.

## Faster Turns (DLL) [`perf`]

Written by Claude (2026-10) after profiling late-game saves. Unless noted, each change leaves every game decision as it was: a save played with and without it ends in the same game state (checked with a full fingerprint of all units, cities and players after 8 turns of a 14-civ Huge save). Search the sources for `perf`.

- **Trade networks** (`CvPlotGroup`): checking whether a network split used the exe's pathfinder; it is now a plain search in the DLL (`countConnectedPlots`). A resource gained or lost by a network now updates the owner's cities in it instead of walking every plot of the network (`changeNumBonuses`). Trade-network updates had been a quarter of AI turn time in big games, mostly set off by ships blockading coasts.
- **Pathfinder** (`KmodPathFinder`): a reset no longer clears the whole node map (about 1 MB on a Huge map); nodes are cleared when first used.
- **GlobalDefines by name** (`CvGlobals::getDefineINT/FLOAT`): the result is remembered per name; the cache is emptied whenever defines are set (XML loading, `setDefine*`).
- **Buildings and resources** (`CvCity::getBonusEffectBuildings`): when a city gains or loses a resource, only buildings with an effect for that resource are summed (the list is made from the building XML when it loads). `updateBuildingCommerce` checks each building once.
- **Python**: `onUnitMove` (sent for every step of every unit) and `getExtraCost` (asked very often by the AI) are behind switches in `PythonCallbackDefines.xml`, `USE_ON_UNIT_MOVE_CALLBACK` and `USE_GET_EXTRA_COST_CALLBACK`, both 0 as BtS's handlers do nothing. Python events get the Alt/Ctrl/Shift state once per engine update instead of three system calls per event. The scoreboard ranking calls the Python score function once per player instead of once per player per rank.
- **Map symbols** (`CvPlot::updateSymbolsIfYieldsChanged`): the yield icons of a plot are only rebuilt when the yields they show change.
- **Automated units move together** (`AUTOMATED_UNITS_MOVE_TOGETHER` in `GlobalDefinesAlt.xml`, 1): at the end of your turn all automated units (workers, explorers, missionaries) act in the same quarter-second time slice instead of one per slice, so their moves are shown at the same time. Same decisions, same work done (measured), but the order of events changes, so games don't replay exactly as with 0. See the comment in `CvGame::updateMoves`.
- **Build**: Release builds use whole-program optimization (`/GL`, `/LTCG`), in the `GNUmakefile` (`make WPO=0` without), the nmake `Makefile` and so the CI: about 10% shorter AI turns. It rounds some floating-point intermediate results differently, so AI evaluations can differ in close calls and games don't replay exactly like a `WPO=0` build. Links take longer: use `WPO=0` while iterating. New targets for this work: `make TARGET=Sample` (sampling profiler), `TARGET=Profile` (K-Mod's profiler), `VERIFY=1` (runs the original code next to the new one and logs differences).

Seconds from ending the turn to having the next one, on late-game saves (Huge, Marathon Accelerated, under Wine; "K-Mod 1.45" is the 48-civ DLL these games were played with, a build with assertions; "fork" is this repository before these changes):

| Save | K-Mod 1.45 | fork | with these changes |
|---|---|---|---|
| AD 1320, 14 civs, 268 cities, 4739 units (7 turns) | 15.2 | 13.7 | 6.8 |
| AD 1448, 8 civs, 333 cities, 4336 units (2 turns) | 54.2 | 37.3 | 12.2 |
| AD 1748, 17 civs, 281 cities, 3795 units (1 turn) | 41.1 | 15.7 | 6.3 |

## Building On Linux (Wine)

`CvGameCoreDLL/GNUmakefile` builds the DLL with the MSVC 2003 Toolkit under Wine: `cd CvGameCoreDLL && make -j$(nproc)` → `Release/CvGameCoreDLL.dll`. `make TARGET=LogAI` adds `LOG_AI`: the AI's decisions (war plans, war rolls, strategies, production) go to `Logs/BBAI.log` when `LoggingEnabled = 1`. It expects the toolkit (from this repo's `build-deps/v1` release, plus `msvcrt.lib` and `msvcprt.lib`), the Windows SDK as unpacked by `xwin --arch x86 splat`, and the BtS SDK's `Boost-1.32.0` and `Python24` under `~/.local/share/civ4-build/`. All paths can be overridden: see the file's header. The Windows build (nmake `Makefile`, used by the CI workflow) has the same Release flags.

## Included Merge: Marathon Accelerated

The Marathon Accelerated 1.1 game-speed mod is included unchanged (the same content as its three XML files).

Integrated behavior:
- Adds a fifth game speed, Marathon Accelerated (`GAMESPEED_MARATHONACCELERATED`, first entry in the XML). It keeps Marathon's 1500-turn calendar, growth, cottage growth and barbarian/war timing (`iBarbPercent = 400`), but uses Normal-speed unit, building and worker costs:
	- `iTrainPercent = 100`
	- `iConstructPercent = 100`
	- `iBuildPercent = 100` (worker actions take Normal-speed turns, e.g. a road takes 2 turns)
	- `iCreatePercent = 400`
	- `iResearchPercent = 560`
	- `iFeatureProductionPercent = 400` (chop yield)
	- `iInflationPercent = 11`
- Standard Marathon (`GAMESPEED_MARATHON`) and the other speeds are unchanged from BtS.
- Marathon Accelerated culture thresholds are Refined 1750, Influential 17500 and Legendary 225000. The numbers in the mod's own readme (2100/21000/210000) are wrong.
- Build Research (`PROCESS_RESEARCH`) is unlocked at `TECH_MYSTICISM` (instead of `TECH_ALPHABET`).

Merged files are in:
- `Assets/xml/GameInfo/CIV4GameSpeedInfo.xml`
- `Assets/xml/GameInfo/CIV4ProcessInfo.xml`
- `Assets/xml/GameInfo/CIV4CultureLevelInfo.xml` (also listed under the old personal snapshot below; it is the mod's file)

If you edit the speed list, keep `CIV4CultureLevelInfo.xml` in step. Each culture level needs one `SpeedThreshold` per speed, because a missing one defaults to 0. Listing more speeds than exist triggers "XML Error" popups while loading.

## Included Merge: Old Personal Snapshot ("K-Mod, more resources")

Additional content ported from an older personal K-Mod v1.45 snapshot. All shared files kept the newer v1.46 repo versions; only old-snapshot-only additions were ported.

### XML Data Files

- `Assets/xml/Civilizations/CIV4CivilizationInfos.xml` — full civilization definitions (standard BtS set)
- `Assets/xml/GameInfo/CIV4CultureLevelInfo.xml` — culture level thresholds
- `Assets/xml/Terrain/CIV4BonusInfos.xml` — bonus resource definitions

### Blue Marble Art Assets

Terrain and water textures from the [Blue Marble](https://civilization.fandom.com/wiki/Blue_Marble_(Civ4)) mod:
- `Assets/Art/Textures/` — terrain blend/detail/grid textures (Coast, Desert, Grass, Hill, Ice, Ocean, Peak, Plains, Tundra)
- `Assets/Art/Water/` — water surface and coast blend textures
- `Assets/Art/Waves/` — wave animation models (.nif) and textures

Blue Marble is a graphics mod by **GT** (GeoTestudo) that replaces Civ4's terrain visuals with satellite-imagery-based textures. Original mod page: https://civilization.fandom.com/wiki/Blue_Marble_(Civ4)

### Settings Templates

- `Settings/Adv Unit Naming.ini`
- `Settings/CustomDomAdv/` — Custom Domestic Advisor presets (default, German, Italian, widescreen)

### Optional Launcher Profile

- `K-Mod, more resources.ini` — alternative mod launcher profile (does not replace default `K-Mod.ini`)

Decision manifest: `gitignore/merge-decision-manifest.md`

## Gameplay XML Quick Reference

- Corporation founding requirements are defined in:
	- `Assets/xml/buildings/CIV4BuildingInfos.xml`
	- Search for `BUILDING_CORPORATION_*` and edit that entry's `<PrereqTech>`.
- Sid's Sushi (`BUILDING_CORPORATION_2`) uses:
	- `<PrereqTech>TECH_MEDICINE</PrereqTech>` (matches original BtS behavior).

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
