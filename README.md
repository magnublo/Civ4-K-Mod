# K-Mod-Magnus

K-Mod-Magnus is a fork of [K-Mod](https://github.com/karadoc/Civ4-K-Mod) 1.46 by karadoc, the gameplay, AI, bugfix and performance mod for **Sid Meier's Civilization IV: Beyond the Sword (BtS) 3.19**. It plays like K-Mod and adds:

- room for 48 civilizations (K-Mod: 18);
- the Marathon Accelerated game speed;
- more Stone, Marble and seafood on generated maps, and other starting techs for five civilizations;
- pre-chop and pre-build buttons;
- fewer early AI wars;
- AI turns about twice as fast in big late games;
- the Planet Generator map script, reworked so that its options hold, with more coast on request;
- Blue Marble terrain textures.

Everything that differs from K-Mod is listed under [Changes from K-Mod 1.46](#changes-from-k-mod-146). The mod's folder is called `K-Mod-Magnus`; it used to be `K-Mod, more resources`.

This README is for players installing the mod, for modders, and for coding agents that need the repository's context before editing.

## Installing on Windows

No compiler is needed. You need Beyond the Sword patched to **3.19** (the Steam and GOG versions are); K-Mod doesn't run on the Mac version.

1. **Get the mod files.** On this repository's GitHub page, click **Code → Download ZIP** and unpack it. Rename the unpacked folder `Civ4-K-Mod-main` (the one that contains `Assets`) to `K-Mod-Magnus` and move it into the `Mods` folder of your Beyond the Sword installation, so that you have `...\Beyond the Sword\Mods\K-Mod-Magnus\Assets`. On Steam you find the installation with Library → right-click the game → Manage → Browse local files.
2. **Get the DLL.** The repository holds the DLL's source code, not the DLL itself; GitHub builds it after every change.
    1. Sign in to GitHub (build outputs can't be downloaded without an account).
    2. Open **Actions → Build Civ4 K-Mod DLL** and click the newest run on `main` with a green check mark.
    3. Under **Artifacts**, download `CvGameCoreDLL-48civs` and unpack it.
    4. Copy its `Assets\CvGameCoreDLL.dll` into `Mods\K-Mod-Magnus\Assets\`.

    Without this DLL the game falls back to BtS's own, which doesn't match this mod's XML and Python: expect errors while loading. GitHub deletes build outputs after 90 days; when the newest run is older, the repository's owner can start a new build with **Run workflow** on the same page.
3. **Play.** Start Beyond the Sword and choose **Advanced → Load a Mod → K-Mod-Magnus**; the game restarts with the mod. To start it directly, make a shortcut to `"...\Beyond the Sword\Civ4BeyondSword.exe" mod="K-Mod-Magnus"`.

The game reads the mod's settings from the `.ini` file named like the folder, `K-Mod-Magnus.ini`. If you rename the folder, rename that file too, and see [Saves](#saves).

**Updating:** download the files and the DLL again, delete the old `K-Mod-Magnus` folder and install the new one. Settings are kept elsewhere, and saves keep working unless a change says otherwise.

**Uninstalling:** delete the `K-Mod-Magnus` folder. BUG's settings (options screen, unit naming) are in `Documents\My Games\Beyond the Sword\K-Mod\`, which this mod shares with K-Mod: delete that only if you don't play K-Mod either.

### Saves

- A save only loads in a DLL built for the same number of civ slots. This mod has 48: saves from 48-civ K-Mod builds load (tested with late-game saves made with a 48-civ K-Mod 1.45 DLL); saves from standard K-Mod (18 slots) or unmodded BtS don't.
- A save remembers the folder name of its mod, and loading it restarts the game in that folder. If that folder is gone, the game reports "an invalid mod directory" and asks to restart. So a save made in `Mods\K-Mod, more resources` opens there, not in `K-Mod-Magnus`. The mod's `tools\save_mod_name.py` (needs [Python 3](https://www.python.org/downloads/)) writes a copy of such a save that opens in `K-Mod-Magnus`. Open a command prompt in `Documents\My Games\Beyond the Sword\Saves\single` (single-player saves) and run, with your path to the mod:

    ```
    py "...\Beyond the Sword\Mods\K-Mod-Magnus\tools\save_mod_name.py" "My game.CivBeyondSwordSave" K-Mod-Magnus "My game (K-Mod-Magnus).CivBeyondSwordSave"
    ```

    With only a save as argument it prints the folder that save asks for. A K-Mod 1.45 save converted this way loaded with this mod's DLL and XML and played on (tested under Wine).
- In multiplayer every computer needs the same mod files and the same DLL.

## Changes from K-Mod 1.46

Compared with karadoc's K-Mod 1.46 (branch `master` of github.com/karadoc/Civ4-K-Mod); everything not listed is K-Mod as released. Search the sources for the tags in brackets.

### Rules and AI (DLL)

- **48 civilizations** (`CIV_PLAYER_CAP` in `CvDefines.h`; K-Mod: 18). Unused civ slots cost little in this DLL: the same 24-civ game (Huge, 200 turns of AI autoplay, identical on both) took 2% longer with the 48-civ DLL than with a 24-civ one. So the CI builds only the 48-civ DLL (artifact `CvGameCoreDLL-48civs`, cap checked with `.github/scripts/check_civ_cap.py`). Other caps build locally: `make CIVS=24` → `Release-24civs/`; nmake: `nmake "CUSTOM_CFLAGS=/DCIV_PLAYER_CAP=24" Release`. Saves only load in a DLL with the same cap.
- **Fewer early AI wars** [`early war`]: the AI starts no war plans of its own before turn 50 (K-Mod: 30), and until turn 150 its war plans are less likely: a third of the usual chance at turn 50, back to normal by turn 150. Both are Normal-speed turns, scaled by the game speed's `iBarbPercent` (Marathon and Marathon Accelerated ×4: turns 200 and 600). Early rushes ("dagger" strategy) are damped the same way. Unlike K-Mod, a "crowded" world no longer ends the no-war period early (cities don't scale with game speed). Tune it in `Assets/xml/GlobalDefinesAlt.xml`: `AI_EARLY_WAR_GATE_TURNS`, `AI_EARLY_WAR_CROWDED_WORLD` (1 = K-Mod's exception), `AI_EARLY_WAR_RAMP_TURNS`, `AI_EARLY_WAR_RAND_PERCENT` (100 turns the damping off).
- **Pre-chop** [`pre-chop`]: workers on a forest get a second chop button (axe with a pause badge). They chop until one worker-turn is left and stop, so a normal chop later finishes in one turn and delivers the hammers then. Ctrl+click on any build button does the same for that build. The flag rides in `MissionData::iData2` (`BUILD_MISSION_STOP_SHORT`).
- **Pre-build** [`pre-build`]: a toggle at the start of the city screen's wonder row (or the project row) marks the first wonder or project in the queue. The city builds it until one turn of production is left, then pauses it with the hammers stored and moves on. Queue it again to finish it in one turn. The queue shows flagged items in yellow. The flag rides in `OrderData::iData2` (`ORDER_DATA_PREBUILD`) and is set through `TASK_PREBUILD`. Whipping or buying a flagged wonder clears the flag.
- **Faster AI turns** [`perf`]: see [Faster turns](#faster-turns).

Pre-chop and pre-build texts: `Assets/xml/Text/CIV4GameText_PreChopPreBuild.xml`. Buttons: `Assets/Art/Interface/Buttons/Actions/PreChop.dds`, `PreBuild.dds` and `PreBuildOn.dds`. UI: `Assets/Python/Screens/CvMainInterface.py`. Pre-chop, pre-build, the early-war rules and the faster turns leave the savegame format as it is; only the civ cap changes it (see [Saves](#saves)).

### Game data (XML, under `Assets/xml/`)

- **Marathon Accelerated** game speed, from the Marathon Accelerated 1.1 mod, unchanged (its `GameInfo/CIV4GameSpeedInfo.xml`, `CIV4ProcessInfo.xml` and `CIV4CultureLevelInfo.xml`). A fifth speed (`GAMESPEED_MARATHONACCELERATED`, first in the list) with Marathon's 1500-turn calendar, growth, cottage growth and barbarian and war timing (`iBarbPercent = 400`), but Normal-speed costs for units, buildings and workers:
    - `iTrainPercent = 100`, `iConstructPercent = 100`
    - `iBuildPercent = 100` (worker actions take Normal-speed turns, e.g. a road takes 2 turns)
    - `iCreatePercent = 400` (projects)
    - `iResearchPercent = 560`
    - `iFeatureProductionPercent = 400` (chop yield)
    - `iInflationPercent = 11`
    - Culture thresholds Refined 1750, Influential 17500, Legendary 225000 (the mod's own readme says 2100/21000/210000, which is wrong).
    - Build Research (`PROCESS_RESEARCH`) comes with Mysticism instead of Alphabet.

    Standard Marathon and the other speeds are as in BtS. If you edit the speed list, keep `CIV4CultureLevelInfo.xml` in step: each culture level needs one `SpeedThreshold` per speed (a missing one counts as 0), and listing more speeds than exist gives "XML Error" popups while loading.
- **More resources** (`Terrain/CIV4BonusInfos.xml`, otherwise the same as the game's own file): Stone `iConstAppearance` 200 and `iPlayer` 100 (BtS: 100 and 50), Marble `iPlayer` 100 (50), Clam, Crab and Fish `iConstAppearance` 260 (50). How many a map gets grows with both values (`CvMapGenerator::calculateNumBonusesToAdd`): about 3.8× the Stone, 2× the Marble and 3.8× the seafood, as far as there are plots that can take them. Maps placed by hand (scenarios) are unaffected.
- **Starting techs** (`Civilizations/CIV4CivilizationInfos.xml`, otherwise BtS's file):

    | Civilization | K-Mod-Magnus | BtS and K-Mod |
    |---|---|---|
    | America | Fishing, Mining | Fishing, Agriculture |
    | Holy Roman Empire | Fishing, Mining | Mysticism, Hunting |
    | Khmer | Mysticism, Mining | Hunting, Mining |
    | Netherlands | Mysticism, Fishing | Agriculture, Fishing |
    | Rome | Agriculture, Mining | Fishing, Mining |
- **Sid's Sushi** (`buildings/CIV4BuildingInfos.xml`, `BUILDING_CORPORATION_2`): founded with Medicine as in BtS (K-Mod: Refrigeration).
- **Switches**: `GlobalDefinesAlt.xml` adds the early-war settings above and `AUTOMATED_UNITS_MOVE_TOGETHER`; `PythonCallbackDefines.xml` adds `USE_ON_UNIT_MOVE_CALLBACK` and `USE_GET_EXTRA_COST_CALLBACK` (see [Faster turns](#faster-turns)).
- **Texts** for the new buttons: `Text/CIV4GameText_PreChopPreBuild.xml`.

### Interface and art

- `Assets/Python/Screens/CvMainInterface.py`: the pre-chop button, the pre-build toggle and the yellow queue entries.
- Buttons: `Assets/Art/Interface/Buttons/Actions/PreChop.dds`, `PreBuild.dds`, `PreBuildOn.dds`.
- **Blue Marble terrain**: terrain, water and wave textures (`Assets/Art/Textures/`, `Assets/Art/Water/`, `Assets/Art/Waves/`) from the Blue Marble terrain mod by ColdFever and civilized.de ([CivFanatics thread](https://forums.civfanatics.com/threads/blue-marble-v4-0-gold.295876/)).

### Map script: Planet Generator

`PublicMaps/Planet_Generator_0_68.py` is Nercury's Planet Generator 0.68 (continents grown plot by plot from many shape templates), reworked for this mod so that its options do what they say (the change log at the top of the file has the details):

- **Percentage of ocean** is exact and leaves the islands out: 70% means the continents cover exactly 30% of the map, and the islands (those around the continents and the lonely ones) come on top, as if they were ocean. The original missed the percentage (about 76% water for 70%, and 63-65% for anything from 0 to 50%). The game's lake step afterwards may turn a few land plots into lakes.
- **Continents** stay separate and whole: the chosen separation holds between continents and islands, each continent is one landmass, and the islands of "Continent scattering" sit off its coast instead of being torn-off parts of it (which were often as big as continents). One stuck continent no longer stops the others from growing, and a crash in continent growth is fixed.
- **Lonely islands**: more choices (no islands; 1 to 8, 10, 12, 15, 20, 25 or 30 islands; 10, 25, 50 or 75% of the empty ocean; every empty area), and the islands are as big as the chosen size. "Use every empty area in ocean" now fills every gap (big islands first, smaller ones in the gaps between); a number of islands also goes into smaller gaps when the open sea runs out, if an island of the chosen size still fits there. Islands stay smaller than continents, and nobody starts on them (the game's own choice could put a civ on a big island).
- **Coastline** (new): Rugged, Very rugged or Maximum coast reshape the continents for more coast plots (fjords and peninsulas open to the sea, no new lakes), keeping each continent in one piece, its size and the separation. On a Standard map with 70% ocean, "Maximum coast" about doubles the coast plots.
- **Minimum island size** (new, replaces "One tile islands allowed"): smaller islands grow to it, or go if there is no room.
- **Rival civs on your continent** (new): none; 1 to 10, 12, 15 or 20; a random number from a range (1-2, 2-3, 4-5, 6-10, 11-15 or 16-20, picked when the map is made); or all. With "By player count" sizes your continent is made for you and that many rivals.
- **Every resource at least twice** (new; the map options are drop-downs, so it is No/Yes): every resource is placed at least twice whatever the climate; where no plot can take it, a plot is changed to fit (e.g. grassland and jungle for bananas on a cold map).

Your last choices are remembered as before (`planet_generator_0_60.cfg` in the Beyond the Sword folder); choices saved by the original script carry over.

### Mod files and settings

- `K-Mod-Magnus.ini` is the mod's settings file. It is K-Mod's `K-Mod.ini` with another name and description and `NoCustomAssets = 0` (K-Mod: 1), so files in `Documents\My Games\Beyond the Sword\CustomAssets` are loaded too; set it to 1 if old files there cause trouble. `K-Mod.ini` is still there but is only read by a folder named `K-Mod`.
- `Settings/`: BUG's defaults for unit naming (`Adv Unit Naming.ini`) and the Customizable Domestic Advisor's layouts (`CustomDomAdv/`). BUG copies them to `Documents\My Games\Beyond the Sword\K-Mod\Settings` when that folder doesn't exist yet.

### Build, CI and tools

- `CvGameCoreDLL/GNUmakefile`: builds the DLL on Linux under Wine (see [Building the DLL](#building-the-dll)).
- `CvGameCoreDLL/Makefile` (nmake): `CUSTOM_CFLAGS` can be given on the command line; Release and Profile compile with whole-program optimization.
- `.github/workflows/build-civ4-dll.yml`: the GitHub Actions build of the 48-civ DLL; `.github/scripts/check_civ_cap.py` checks the cap compiled into it; notes in `.github/workflows/README.md`.
- `CvGameCoreDLL/SampleProfiler.h`, `.cpp`: a sampling profiler, compiled only with `make TARGET=Sample`.
- `tools/save_mod_name.py`: shows or changes the mod folder a save asks for (see [Saves](#saves)).
- `AdvCiv_Port_Candidates_Research.md`: notes on AdvCiv features that could be ported.

## Faster turns

Written by Claude (2026-10) after profiling late-game saves; tagged [`perf`]. Unless noted, each change leaves every game decision as it was: a save played with and without it ends in the same game state (checked with a full fingerprint of all units, cities and players after 8 turns of a 14-civ Huge save).

- **Trade networks** (`CvPlotGroup`): checking whether a network split used the exe's pathfinder; it is now a plain search in the DLL (`countConnectedPlots`). A resource gained or lost by a network now updates the owner's cities in it instead of walking every plot of the network (`changeNumBonuses`). Trade-network updates had been a quarter of AI turn time in big games, mostly set off by ships blockading coasts.
- **Pathfinder** (`KmodPathFinder`): a reset no longer clears the whole node map (about 1 MB on a Huge map); nodes are cleared when first used.
- **GlobalDefines by name** (`CvGlobals::getDefineINT/FLOAT`): the result is remembered per name; the cache is emptied whenever defines are set (XML loading, `setDefine*`).
- **Buildings and resources** (`CvCity::getBonusEffectBuildings`): when a city gains or loses a resource, only buildings with an effect for that resource are summed (the list is made from the building XML when it loads). `updateBuildingCommerce` checks each building once.
- **Python**: `onUnitMove` (sent for every step of every unit) and `getExtraCost` (asked very often by the AI) are behind switches in `PythonCallbackDefines.xml`, `USE_ON_UNIT_MOVE_CALLBACK` and `USE_GET_EXTRA_COST_CALLBACK`, both 0 as BtS's handlers do nothing. Python events get the Alt/Ctrl/Shift state once per engine update instead of three system calls per event. The scoreboard ranking calls the Python score function once per player instead of once per player per rank.
- **Map symbols** (`CvPlot::updateSymbolsIfYieldsChanged`): the yield icons of a plot are only rebuilt when the yields they show change.
- **Automated units move together** (`AUTOMATED_UNITS_MOVE_TOGETHER` in `GlobalDefinesAlt.xml`, 1): at the end of your turn all automated units (workers, explorers, missionaries) act in the same quarter-second time slice instead of one per slice, so their moves are shown at the same time. Same decisions, same work done (measured), but the order of events changes, so games don't replay exactly as with 0. See the comment in `CvGame::updateMoves`.
- **Build**: Release builds use whole-program optimization (`/GL`, `/LTCG`), in the `GNUmakefile` (`make WPO=0` without), the nmake `Makefile` and so the CI: about 10% shorter AI turns. It rounds some floating-point intermediate results differently, so AI evaluations can differ in close calls and games don't replay exactly like a `WPO=0` build. New targets for this work: `make TARGET=Sample` (sampling profiler), `TARGET=Profile` (K-Mod's profiler), `VERIFY=1` (runs the original code next to the new one and logs differences).

Seconds from ending the turn to having the next one, on late-game saves (Huge, Marathon Accelerated, under Wine; "K-Mod 1.45" is the 48-civ DLL these games were played with, a build with assertions; "fork" is this repository before these changes):

| Save | K-Mod 1.45 | fork | with these changes |
|---|---|---|---|
| AD 1320, 14 civs, 268 cities, 4739 units (7 turns) | 15.2 | 13.7 | 6.8 |
| AD 1448, 8 civs, 333 cities, 4336 units (2 turns) | 54.2 | 37.3 | 12.2 |
| AD 1748, 17 civs, 281 cities, 3795 units (1 turn) | 41.1 | 15.7 | 6.3 |

## Building the DLL

Like every BtS DLL, it is 32-bit Windows code built with the Microsoft Visual C++ Toolkit 2003 (MSVC 7.1) against the BtS SDK's Boost 1.32 (`boost_python-vc71-mt-1_32.lib`) and Python 2.4 (`Python24`).

- **GitHub Actions** (`.github/workflows/build-civ4-dll.yml`) builds the Release DLL on every push that changes `CvGameCoreDLL/`, checks its civ cap and uploads it as `CvGameCoreDLL-48civs`. It fetches the toolkit from this repository's `build-deps/v1` release and Boost and Python from FinalFrontierPlus/CvGameCoreDLL; see `.github/workflows/README.md`.
- **Windows**: in `CvGameCoreDLL/`, `nmake Release` (or `Debug`, `Profile`), or open `CvGameCoreDLL.vcproj` in Visual Studio. The Makefile expects the toolkit and the Windows SDK v7.1 at their default paths (otherwise `nmake TOOLKIT=... PSDK=...`) and `Boost-1.32.0` and `Python24` inside `CvGameCoreDLL/`. Copy `Release\CvGameCoreDLL.dll` into `Assets\`.
- **Linux (Wine)**: `cd CvGameCoreDLL && make -j$(nproc)` → `Release/CvGameCoreDLL.dll`, using the `GNUmakefile` (GNU make reads it before the nmake `Makefile`). It expects the toolkit (from this repo's `build-deps/v1` release, plus `msvcrt.lib` and `msvcprt.lib`), the Windows SDK as unpacked by `xwin --arch x86 splat`, and the BtS SDK's `Boost-1.32.0` and `Python24` under `~/.local/share/civ4-build/`; all paths can be overridden (see the file's header). `TARGET=LogAI` adds `LOG_AI`: the AI's decisions (war plans, war rolls, strategies, production) go to `Logs/BBAI.log` when `LoggingEnabled = 1`. Also `TARGET=Debug` (assertions), `TARGET=Profile`, `TARGET=Sample`, `CIVS=24`, `WPO=0`, `VERIFY=1`.

Whole-program optimization makes builds slower: with 16 jobs under Wine (i9-9900K) a clean build takes 170 s instead of 36 s and a one-file change 150 s instead of 15 s, and the CI build 5 minutes instead of 2. Use `make WPO=0` while iterating.

## For contributors and coding agents

### What is in this repository

- `Assets/`: the mod's content (XML, Python, art, text, BUG configuration)
- `CvGameCoreDLL/`: the DLL's C++ source and makefiles
- `PublicMaps/`: K-Mod's map script `not_too_Big_or_Small.py` and the reworked Planet Generator
- `Settings/`: BUG's default settings (unit naming, domestic advisor)
- `tools/`: `save_mod_name.py`
- `K-Mod-Magnus.ini`: the mod's settings file; `K-Mod.ini`: K-Mod's, read only by a folder named `K-Mod`
- `readme.txt`, `changelog.txt`: K-Mod's readme and changelog (1.46)
- `.github/`: the CI workflow and its notes

### Architecture map

DLL layer (C++), in `CvGameCoreDLL/`:
- AI behavior: `CvPlayerAI.cpp`, `CvCityAI.cpp`, `CvSelectionGroupAI.cpp`, `CvTeamAI.cpp`, `BetterBTSAI.cpp`
- Core game systems: `CvGame.cpp`, `CvCity.cpp`, `CvUnit.cpp`, `CvPlot.cpp`
- DLL bootstrap and profiling hooks: `CvGameCoreDLL.cpp`, `FProfiler.h`, `FAssert.h`
- Build files: `CvGameCoreDLL.vcproj`, `Makefile` (nmake), `GNUmakefile` (Linux/Wine)

Data and scripting layer, in `Assets/`:
- XML data: `Assets/xml/` (lowercase in this repo)
- Python scripts: `Assets/Python/`; entry points `Assets/Python/EntryPoints/CvEventInterface.py` (wires through BUG's event manager) and `CvGameInterfaceFile.py` (routes game utility hooks through BUG's dispatcher)
- BUG is included under `Assets/Python/BUG/`.
- `Assets/xml/PythonCallbackDefines.xml` holds the callback switches; many are 0 by default.

### Safe change workflow

1. Edit as narrowly as possible in one layer first:
    - XML/Python if data/script-level is sufficient
    - DLL only when behavior cannot be expressed through XML/Python
2. Keep semantics isolated per commit/change set:
    - AI logic
    - Balance values
    - UI text
3. Validate in-game on at least one fresh game and one mid/late save when relevant.
4. If touching AI code, run enough turns to observe behavior (not just compile).

### Debugging and validation checklist

- Game launches and mod loads without immediate XML/Python errors
- New game creation works on at least one random map and one `PublicMaps/` map
- End-turn progression does not hang/crash for a representative test game
- No obvious UI regressions in advisor, city, diplomacy, and combat flows touched by change
- Multiplayer-sensitive changes reviewed carefully (K-Mod historically emphasizes OOS stability)

### Agent playbook

Repo facts to assume:
- This is a mixed C++ + XML + Python Civ4 BtS mod.
- Core behavior is concentrated in `CvGameCoreDLL/` and data/script behavior in `Assets/`.
- XML path uses lowercase `Assets/xml/` in this repo.
- BUG framework is integrated into Python entrypoints.

Edit policy:
- Prefer the smallest possible diff.
- Do not mass-reformat legacy C++ or Python 2 style code.
- Preserve function signatures, serialization behavior, and deterministic logic unless the task requires changing them.
- Avoid broad enum/order changes in XML unless all dependent references are updated.

High-risk areas:
- AI decision loops (`CvPlayerAI`, `CvCityAI`, `CvTeamAI`, group/unit AI)
- Turn processing and combat resolution
- Save/load related state and global caches
- Callback wiring between DLL and Python (`PythonCallbackDefines.xml`, entrypoint modules)

Recommended investigation order:
1. Search for existing logic in the most specific subsystem file.
2. Check whether behavior is already controlled by XML define/value.
3. Confirm whether a BUG/Python callback is in play.
4. Change one layer first, then only cross layers if needed.

### Contribution guidance

If you are preparing changes for review:

1. Describe exactly which layer you changed (DLL, XML, Python, or mixed).
2. Include gameplay intent and expected behavior changes.
3. Call out any balance-impacting constants changed.
4. Include manual test notes (map/speed/era/turn range, SP/MP context).

### Modding references

- CivFanatics Civ4 Modding Tutorials & Reference: https://forums.civfanatics.com/forums/civ4-modding-tutorials-reference.177/ (DLL compilation guides, XML and modular XML, Python callbacks and events, map scripts, art)
- CivFanatics K-Mod forum: https://forums.civfanatics.com/forums/civ-4-k-mod-far-beyond-the-sword.470/; main thread: https://forums.civfanatics.com/threads/k-mod-far-beyond-the-sword.407049/

## Credits

- **karadoc**: K-Mod, an unofficial content and balance patch with a much stronger AI ([source](https://github.com/karadoc/Civ4-K-Mod); see `readme.txt` and `changelog.txt`). It includes BUG and BULL, the Unofficial Patch and Actual Quotes.
- The **Marathon Accelerated 1.1** game-speed mod.
- **Blue Marble** terrain by ColdFever and civilized.de.
- **Planet Generator** map script by Nercury (Mac support by AlanH).
- This fork: magnublo, with GitHub Copilot and Claude as coding agents (the turn-time work is Claude's).
