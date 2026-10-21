# K-Mod-Magnus

K-Mod-Magnus is a fork of [K-Mod](https://github.com/karadoc/Civ4-K-Mod) 1.46 by karadoc, the gameplay, AI, bugfix and performance mod for **Sid Meier's Civilization IV: Beyond the Sword (BtS) 3.19**. It plays like K-Mod and adds:

- room for 48 civilizations (K-Mod: 18);
- the Marathon Accelerated game speed;
- more Stone, Marble and seafood on generated maps, and other starting techs for five civilizations;
- pre-chop and pre-build buttons, and a "prechopped" sign on each forest you pre-chop;
- a "Workers" tab in the BUG options: which improvements your automated workers build (at first all but watermills, windmills and forest preserves) and which they replace;
- automated production that builds your list of buildings in your order, then Wealth or Research;
- fewer early AI wars;
- turns 1.6 to 2.5 times faster than K-Mod 1.46 in big late games;
- the Planet Generator map script, reworked so that its options hold, with more coast on request;
- Blue Marble terrain textures;
- the unit, building and city graphics of the Realism Invictus mod, each civilization in its own look (an option, on by default).

Everything that differs from K-Mod is listed under [Changes from K-Mod 1.46](#changes-from-k-mod-146). The mod's folder is called `K-Mod-Magnus`; it used to be `K-Mod, more resources`.

This README is for players installing the mod, for modders, and for coding agents that need the repository's context before editing.

## Installing on Windows

No compiler is needed. You need Beyond the Sword patched to **3.19** (the Steam and GOG versions are); K-Mod doesn't run on the Mac version.

1. **Get the mod files.** On this repository's GitHub page, click **Code → Download ZIP** (about 300 MB, most of it the Realism Invictus art) and unpack it. Rename the unpacked folder `Civ4-K-Mod-main` (the one that contains `Assets`) to `K-Mod-Magnus` and move it into the `Mods` folder of your Beyond the Sword installation, so that you have `...\Beyond the Sword\Mods\K-Mod-Magnus\Assets`. On Steam you find the installation with Library → right-click the game → Manage → Browse local files.
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
    - **"prechopped" signs** [`pre-chop sign`]: when a pre-chop stops on a forest or jungle (also a Ctrl+click build that would clear it, such as a mine on a forested hill), the plot gets one of your map signs reading "prechopped". The DLL reports the stop to `Assets/Python/Contrib/PreChopSigns.py`, which places the sign. It goes away when no chop is waiting there any more: at once when a build removes the forest (the chop finished, or a farm built over it), otherwise at the start of the next turn (an event, global warming). A plot that already has one of your signs keeps it and gets no second one, and signs that don't read "prechopped" are never removed. The game saves these signs like your own.
- **Pre-build** [`pre-build`]: a toggle at the start of the city screen's wonder row (or the project row) marks the first wonder or project in the queue. The city builds it until one turn of production is left, then pauses it with the hammers stored and moves on. Queue it again to finish it in one turn. The queue shows flagged items in yellow. The flag rides in `OrderData::iData2` (`ORDER_DATA_PREBUILD`) and is set through `TASK_PREBUILD`. Whipping or buying a flagged wonder clears the flag.
- **What your automated workers build and replace** [`automated workers`]: the BUG options (Ctrl+Alt+O) have a **Workers** tab with two lists of checkboxes, one entry per improvement in the XML (an improvement added to the XML gets its own). They apply to the workers you automate (build improvements, improve the nearest city, build a trade network; also work boats):
    - **They build**: at first all checked except Watermill, Windmill and Forest Preserve. For an unchecked improvement, a city planning its tiles leaves it out and picks the best of the rest, weighing its emphasis (food, production, commerce) as before. In a test on a late-game save (8 turns, 51 automated workers), the workers set out to replace 9 mines with windmills and 5 workshops with watermills when they could build mills; when they couldn't, those tiles kept their mines and workshops. Forest preserves were the most common job in that save (up to 18 workers at a time, 12 preserves built in the 8 turns). With preserves unchecked, no worker built one; more of them worked on farms, workshops and cottages, and they chopped no more forest than before (86 forests at the end either way). A tile that already has an unchecked improvement is planned as in K-Mod: keeping it stays a choice, so the workers replace it only where K-Mod would have.
    - **They replace**: at first none checked. On a tile one of your cities works or can work, a checked improvement counts as none when the city plans the tile, so your automated workers replace it with the best other improvement there, also with "Automated Workers Leave Old Improvements" on. They never build it either. It stays where it connects a resource of its tile (a farm on wheat), and where nothing else is worth building (a watermill on snow, where only a fort fits). In the test save, with farms checked and Leave Old Improvements on, the 7 farms on resources stayed, the 6 other farms in city radii were planned as cottages and workshops (4 done within 6 turns), and no other improvement was touched.
    - A worker that is already building an improvement it may no longer build stops and picks a new job. The worker recommendations (the circled tile and the highlighted build button) follow the same plans.

    The choices are kept on your computer, like all BUG options (`Automated Workers.ini` in BUG's settings folder), not in the save. The game hands them to the DLL at the start of each of your turns and whenever you change one, as a network message, so multiplayer games stay in sync. Not affected: the AI's workers, AI autoplay and the orders you give a worker yourself.
- **Automated production builds your list, in your order** [`automated production`]: a city whose production you hand to the governor (the city screen's automate-production button) builds the first building of `AUTOMATED_PRODUCTION_ORDER` (`Assets/xml/GlobalDefinesAlt.xml`) that it can build, and nothing else: no units, no wonders, no buildings that aren't on the list. A civilization's unique building takes the place of its class (a Holy Roman city builds a Rathaus for the Courthouse). When nothing on the list can be built, the city makes Wealth or Research, whichever the AI rates higher, and starts the next listed building as soon as one becomes possible. It chooses again every turn, so it moves up the list at once (the hammers of a building it leaves stay with that building, and an item with three turns or less to go is finished first). Power plants: a city that has clean power builds none, and a dirty one (Coal Plant) only where the city has no power at all, so the list's "Hydro Plant, Coal Plant" means a hydro plant if possible, else a coal plant, and a hydro plant later where a river city got a coal plant first. The list as it comes:

    Courthouse, Granary, Forge, Factory, Hydro Plant or Coal Plant, Levee, Industrial Park, Lighthouse, Airport, Harbor, Grocer, Supermarket, Aqueduct, Hospital, Public Transportation, Recycling Center, Jail, the seven temples, Colosseum, Broadcast Tower, Market, Theatre, Bank, Library, the seven monasteries, University, Observatory, Laboratory, Intelligence Agency, Security Bureau, Customs House, Drydock, Barracks, Bunker, Bomb Shelter.

    The cathedrals (Stupa, Cathedral, Academy, Mandir, Mosque, Synagogue, Pagoda) and the Nuclear Plant are left out, so automated cities never build them. Edit the list in the XML (building classes from `Buildings/CIV4BuildingClassInfos.xml`); a name that isn't a building class gives an "XML Error" message while the mod loads, and an empty list gives K-Mod's governor back. Tested on a late-game save with all 99 cities automated: every city not in disorder built the first listed building it could; a city 4 turns into a Customs House that lost its Rathaus built a new Rathaus the next turn; and with Plastics, the 9 river cities that had only a coal plant built hydro plants at once. Not affected: cities whose production you choose yourself, and the AI's cities.
- **Faster AI turns** [`perf`]: see [Faster turns](#faster-turns).

Pre-chop and pre-build texts: `Assets/xml/Text/CIV4GameText_PreChopPreBuild.xml`. Buttons: `Assets/Art/Interface/Buttons/Actions/PreChop.dds`, `PreBuild.dds` and `PreBuildOn.dds`. UI: `Assets/Python/Screens/CvMainInterface.py`; the "prechopped" signs: `Assets/Python/Contrib/PreChopSigns.py` and `Assets/Config/Pre-chop Signs.xml`. Pre-chop, pre-build, the signs, the early-war rules, the Workers tab, the production order and the faster turns leave the savegame format as it is; only the civ cap changes it (see [Saves](#saves)).

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
- **Switches**: `GlobalDefinesAlt.xml` adds the early-war settings and the automated production order; `PythonCallbackDefines.xml` adds `USE_ON_UNIT_MOVE_CALLBACK` and `USE_GET_EXTRA_COST_CALLBACK` (see [Faster turns](#faster-turns)).
- **Texts** for the new buttons: `Text/CIV4GameText_PreChopPreBuild.xml`; for the Workers tab: `Text/CIV4GameText_AutomatedWorkers.xml`.

### Interface and art

- `Assets/Python/Screens/CvMainInterface.py`: the pre-chop button, the pre-build toggle and the yellow queue entries.
- `Assets/Python/Contrib/PreChopSigns.py` (loaded through `Assets/Config/Pre-chop Signs.xml`): the "prechopped" signs.
- The Workers tab: `Assets/Python/BUG/Tabs/BugWorkersOptionsTab.py`; its options are made by `Assets/Python/Contrib/AutomatedWorkers.py` (defaults in `Assets/Config/Automated Workers.xml`), which sends them to the DLL (`CvPlayer::getAutomatedWorkers`) as a ModNetMessage.
- `Assets/Python/BUG/BugOptions.py`: changes to a BUG settings file that was created in the same session are now saved (before, only from the next start of the game).
- Buttons: `Assets/Art/Interface/Buttons/Actions/PreChop.dds`, `PreBuild.dds`, `PreBuildOn.dds`.
- **Blue Marble terrain**: terrain, water and wave textures (`Assets/Art/Textures/`, `Assets/Art/Water/`, `Assets/Art/Waves/`) from the Blue Marble terrain mod by ColdFever and civilized.de ([CivFanatics thread](https://forums.civfanatics.com/threads/blue-marble-v4-0-gold.295876/)).
- **Realism Invictus graphics** [`RI art`]: units, buildings and cities look as in the Realism Invictus mod, each civilization with its own soldiers, ships and planes, and its own houses, walls and versions of many buildings. On by default; switched in the BUG options (Ctrl+Alt+O), Map tab, "Realism Invictus graphics". See [Realism Invictus graphics](#realism-invictus-graphics).

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
- `tools/ri_art/port_ri_art.py`: writes the Realism Invictus art and its XML into `Assets/` (see [Realism Invictus graphics](#realism-invictus-graphics)); `tools/ri_art/ported.md` is its table of what each unit and building gets.
- `AdvCiv_Port_Candidates_Research.md`: notes on AdvCiv features that could be ported.

## Realism Invictus graphics

[Realism Invictus](https://forums.civfanatics.com/threads/realism-invictus.411799/) (RI) is a large BtS mod whose graphics give each civilization its own look: Roman legions and Greek hoplites, Prussian riflemen, Pz.III and T-28 tanks, Chinese junks, Arabian souks, Roman barracks. K-Mod-Magnus shows that art for its own units, buildings and cities, taken from RI 3.81 (2026-02-08); RI's rules, units, buildings and civilizations are not part of it. Tagged [`RI art`] in the sources.

**The option**: BUG options (Ctrl+Alt+O), Map tab, "Realism Invictus graphics", on by default. It takes effect at once (the units and cities on the map are redrawn) and is a setting of each computer (`Realism Invictus Art.ini` in BUG's settings folder). Only the looks change: a unit shows as many figures as in K-Mod (RI's figure groups are fitted to K-Mod's group sizes) and battles are planned from K-Mod's art, so the game plays out the same with the option on or off, saves don't store it, and players of one multiplayer game may set it differently. Measured on a late-game save (14 civs, AD 1320, 4,700 units) under Wine, which draws without a GPU: with RI art the AI turns took about 3% longer (0.3 s of 10 s) and played out identically, the game used a little less memory (2.0 GB instead of 2.1 GB after looking at every city) and drew 4–5% more frames per second; the save loaded in the same 31 s.

**What gets RI art** (the full table: `tools/ri_art/ported.md`):

- 114 of the 123 units. Each civilization gets RI's version of the unit for its RI counterpart, with RI's art for each era (e.g. Roman warriors are "Warrior (Mediterranean)", German riflemen "Rifleman (Prussian)", Japan's battleships "Battleship (Japanese)"). Unique units get RI's unit of the same name, or what that civilization builds instead in RI: the Praetorian is RI's "Legion (Imperial)", the Keshik RI's "Horse Archer (Keshik)", the Cataphract RI's Byzantine "Knight (Late Kataphraktoi)", the Panzer RI's "Medium Tank (Pz.III)", the Navy SEAL RI's American marines, the Redcoat RI's English riflemen. Units whose RI class has another name: Tank → RI's Medium Tank, Ship of the Line → Line, Stealth Destroyer → Modern Destroyer, Missile Cruiser → RI's Missile Cruiser (Aegis), Airship → Zeppelin. Unit buttons on the map and in the city screen follow the art.
- 140 of the 159 buildings, with RI's regional versions of ordinary buildings for the civilizations that have one (souk or bazaar for the market, Roman barracks, Chinese provincial government for the courthouse, palaces, ...). Wonders only take RI's versions of the same wonder (e.g. each civilization's Heroic Epic), not RI's other landmarks.
- Cities: RI's 15 city styles (houses by era and region, walls, RI's layouts of each building), from RI's city layout.
- Civilizations RI doesn't have take another's look: Byzantium RI's Greece (which turns Byzantine), the Holy Roman Empire Austria, Native America the Iroquois, Sumeria Babylon (RI's Mesopotamia).
- Keep K-Mod's look (RI has nothing that fits): executives and corporations, the Confucian missionary and buildings (no Confucianism in RI), SAM Infantry (a vehicle in RI), the Roman Forum, Madrassa, Sacrificial Altar, Ziggurat, Incan Terrace, Industrial Park, Public Transportation and Mausoleum of Maussollos.
- Not ported: RI's sounds (sound scripts K-Mod lacks are left out), leaders, terrain, improvements and interface.

**Files**: `Assets/xml/Art/RI_CIV4ArtDefines_Unit.xml` and `RI_CIV4ArtDefines_Building.xml` (RI's art defines, types prefixed `RI_`), `Assets/xml/Art/RI_CIV4ArtInfos.xml` (which RI art each unit and building gets, per civilization, and each civilization's RI city style; schema `RI_CIV4ArtSchema.xml`), `Assets/xml/buildings/CIV4CityLSystem.xml` (BtS's city layout, with BtS's houses limited to BtS's six city styles, plus RI's entries under `RI_` names) and `Assets/xml/GlobalTypes.xml` (BtS's, plus RI's city styles `RI_ARTSTYLE_*`). The art is in `Assets/Art/RI/` (RI's `Art/X` as `Art/RI/X`) and `Assets/Art/Structures/cities/RI/` (RI's houses: the game takes houses from `Art/Structures/Cities`), about 300 MB in 10,400 files. Nothing in these is used with the option off. RI files that would replace a file of the game (RI changed some of BtS's textures and animations) are only in these folders, together with the game's files the RI version is used with (e.g. the model an animation file names, which the unit picture in the interface loads).

**How**: `CvGameCoreDLL/CvRIArt.h` reads `RI_CIV4ArtInfos.xml` and holds the switch. The DLL answers the exe's art questions from it while the option is on: `CvUnit::getArtInfo`, `getGroupDefinitions`, `getUnitGroupRequired`, `getAnimationMaxSpeed`, `getAnimationPadTime` (units on the map), `CvPlayer::getUnitArtInfo` and `getUnitButton` (unit pictures and buttons in the interface), `CvCity::getBuildingArtInfo` and `CvGame::getBuildingArtInfo` (buildings) and `CvInitCore::getArtStyle` (the city style; `setArtStyle` never stores an RI style). Python: `gc.setRealismInvictusArt(bool)`, `gc.isRealismInvictusArt()`, `gc.isRealismInvictusArtAvailable()`; the option is `Assets/Config/Realism Invictus Art.xml` with `Assets/Python/Contrib/RealismInvictusArt.py`. In a mod folder without the RI files (this DLL in a K-Mod folder, say) nothing is loaded, no error is shown, and the option does nothing.

**Regenerating** (e.g. for a newer RI): download RI's unpacked installer (CivFanatics, "Realism Invictus (unpacked version)", a SourceForge file such as `Realism Invictus 3.81 (2026-02-08) Setup (Unpacked).exe`), extract it with `7z x` (no need to run it; the mod is the folder `$_4_`), then

```
python3 tools/ri_art/port_ri_art.py --ri "<extracted>/$_4_" --civ4 "<Civilization IV folder>" --ri-version "3.81 (2026-02-08)" --report tools/ri_art/ported.md
```

`--civ4` is the folder with `Assets`, `Warlords` and `Beyond the Sword` (the script reads the game's art to tell which RI files differ from it). The civilization and class mappings are at the top of the script.

## Faster turns

In big late games, the wait from ending your turn to having the next one is 1.6 to 2.5 times shorter than with K-Mod 1.46. The rules and the AI's logic stay as they were. Claude made these changes in October 2026 after profiling late-game saves; they are tagged [`perf`] in the source.

### How much faster than K-Mod 1.46

Seconds from ending your turn to having the next one, on three late-game saves (Huge map, Marathon Accelerated):

| Save | K-Mod 1.46 | K-Mod-Magnus | Faster |
|---|---|---|---|
| AD 1320: 14 civs, 268 cities, 4,739 units | 13.68 | 8.71 | 1.6× |
| AD 1448: 8 civs, 333 cities (243 of them yours), 4,336 units | 16.45 | 8.93 | 1.8× |
| AD 1748: 17 civs, 281 cities, 3,795 units | 15.40 | 6.16 | 2.5× |

The gain grows with the game. In a new game (Huge, 24 civs, Normal speed, AI autoplay), turns 0 to 198 took 303 s with K-Mod 1.46 and 231 s with K-Mod-Magnus, 24% less: 6% less in turns 0–49, 26% less in turns 150–198. The exact changes alone (see [Why](#why)), which play the very same game as K-Mod 1.46, took 247 s.

How this was measured:

- "K-Mod 1.46" is karadoc's code (tag `v1.46`) built with K-Mod's own release settings; only its civ cap is raised to 48, which these saves need. Unused civ slots cost little (about 2%, see [Rules and AI](#rules-and-ai-dll)).
- Under Wine on an i9-9900K without a GPU (software rendering), with logs and autosaves off; means of two runs, which differed by at most 1.1%. AD 1320: the 7 turns after the first one after loading, which is slower with every DLL. AD 1448: the 2 turns after loading; AD 1748: 1 turn (both games end in victory soon after).
- The mod folder used for these tests lacks this fork's early-war settings and the Realism Invictus art, so K-Mod-Magnus played by K-Mod's rules there, with K-Mod's graphics, and the games can be compared.

### Why

The changes are of two kinds. Measured one after the other, in seconds per turn (in brackets the part spent moving your units after you end the turn):

| | AD 1320 | AD 1448 | AD 1748 |
|---|---|---|---|
| K-Mod 1.46 | 13.7 (4.5) | 16.5 (8.1) | 15.4 (5.7) |
| Exact changes | 9.3 (3.9) | 10.1 (5.7) | 7.1 (1.5) |
| … and whole-program optimization: K-Mod-Magnus | 8.7 (3.7) | 8.9 (5.4) | 6.2 (1.2) |

- **Exact changes** compute the same results with less work, so the game plays out exactly as with K-Mod 1.46. Checked with a fingerprint of all units, cities and players: on the three saves after the measured turns, and in the new 24-civ game after each of its 199 turns. Test builds that run the original code next to the new one (`make VERIFY=1`) found no difference either.
- **Whole-program optimization** lets the compiler optimize across source files. It doesn't change the rules or how any decision is made, but it rounds some floating-point results differently, so a game doesn't replay exactly as with K-Mod 1.46. Saves load either way.

Where K-Mod 1.46 spent the time, and what changed. The shares are of the time per turn on AD 1320, from profiles of K-Mod 1.46's code; the two items marked "later" are shares of what was left after the others:

- **Trade networks, about a quarter** (`CvPlotGroup`, `CvUnit::updatePlunder`). A ship blockading a coast, a culture flip or a deal makes K-Mod rebuild every player's trade networks: each plot leaves its network and joins again. Along the way, the game engine's pathfinder checked whether a network had split (16,669 times in 7 turns on this save), and each resource a network gained or lost walked all its plots (about 8,000 in these games) to find its cities. Now the split check is a plain search in the DLL (`countConnectedPlots`). It is skipped after a blockade when the blockade took a plot out of the network, because the network can't still be whole. Resource changes go straight to the owner's cities in the network (`changeNumBonuses`).
- **Unit pathfinding, about 15%** (`KmodPathFinder`; about 15,000 path searches per turn on this save). A search that couldn't continue the previous one began by clearing the whole node map, about 1 MB on a Huge map: a third of the pathfinding time. Now nodes are cleared when first used. Each step's movement cost was computed twice; now it is computed once per search. The search itself stays A*: the other algorithms don't fit Civ4's movement rules (costs that depend on the unit, turn boundaries), or they would make units take other paths.
- **Settings looked up by name, 5–7%** (`CvGlobals::getDefineINT/FLOAT`): about 5.6 million lookups per turn, each building a string and hashing it. Each value is now remembered under its name, and names written in the code are recognized by their address. The cache is emptied whenever defines are set (XML loading, `setDefine*`).
- **Python, 3–4%**. Every Python event read the Alt, Ctrl and Shift keys, three system calls that are slow under Wine; now the keys are read once per engine update. `onUnitMove` (sent for every step of every unit) and `getExtraCost` (asked very often by the AI) are behind switches in `PythonCallbackDefines.xml`, `USE_ON_UNIT_MOVE_CALLBACK` and `USE_GET_EXTRA_COST_CALLBACK`. Both are 0, as BtS's handlers do nothing. The scoreboard ranking asks Python for each player's score once instead of once per rank.
- **Cities** (`CvCity`):
    - When a city gains or loses a resource, only the buildings with an effect for that resource are summed, not every building type; the lists are made from the building XML when it loads.
    - `updateBuildingCommerce` looks at each of the city's buildings once for all commerce types, and skips those with no commerce of their own.
    - Corporation bonuses are counted in arrays on the stack instead of new heap vectors per call.
    - `allUpgradesAvailable` follows each unit's own upgrades instead of trying every unit class at every step.
    - The maintenance distance (to the nearest palace or other government center) is remembered per city until a city is added or removed or a government center changes.
- **Map symbols** (`CvPlot::updateSymbolsIfYieldsChanged`): a plot's yield icons are rebuilt only when the yields they show change.
- **Later: AI loops over all units, about 5%** (`CvUnitAI::AI_shadow`, `AI_omniGroup`, `AI_guardBonus`, `CvPlot::isFriendlyCity`). For each candidate, the expensive test came first: is the plot a friendly city, which looks through all the units in the city for enemies. The cheap ones (the unit's AI type, cargo, ownership) came after it. Now the cheap ones come first.
- **Later: units, cities and networks by ID, 3–4%** (`FFreeListTrashArray.h`, `CvPlayer::getUnit` and the like): every lookup, and every step of a loop over a player's units or cities, was a virtual function call the compiler can't inline. Now they are direct calls.
- **Whole-program optimization** (`/GL`, `/LTCG`): Release builds use it in the `GNUmakefile` (`make WPO=0` builds without it), in the nmake `Makefile` and so in the CI. It rounds some floating-point intermediate results differently, so AI evaluations can come out the other way in close calls, and games don't replay exactly like a `WPO=0` build.

Build options made for this work: `make TARGET=Sample` (sampling profiler), `TARGET=Profile` (K-Mod's profiler), `VERIFY=1` (runs the original code next to the new one and logs differences).

### What is left

Much of the remaining wait is not the DLL computing:

- **Your automated units.** At the end of your turn, K-Mod moves your automated units (workers, explorers, missionaries) one group per engine update, and the engine updates four times a second. With dozens of automated workers that adds seconds to every turn, mostly spent waiting for the next update (it is part of the bracketed times above). Letting all of them act in the same update was tried and saved about 3 s per turn on AD 1320 and 4 s on AD 1448, but your units then choose their moves in another order, which changes the course of your game, so it was left out.
- **The game engine's own work** between its updates: mostly drawing frames, plus its own unit and interface work. The engine is the exe, which a mod can't change. In this test copy, without a GPU, a frame takes about 120 ms; with a graphics card, much less. Drawing fewer frames during AI turns would make the screen refresh less often, so that trade-off was left out (AdvCiv offers it as an option).

The DLL's own time is spread thin: no function takes more than about 2% of a turn.

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
- `tools/`: `save_mod_name.py`; `ri_art/`, the port of the Realism Invictus art
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
- **Realism Invictus** 3.81 by Walter Hawkwood, Takofloppa and the Realism Invictus team, and the many artists credited in its manual ("Credits and thanks"): the unit, building and city art and the city layouts of [Realism Invictus graphics](#realism-invictus-graphics) ([CivFanatics thread](https://forums.civfanatics.com/threads/realism-invictus.411799/), [SourceForge](https://sourceforge.net/projects/civ4mods/)). RI's `License.txt` allows "full and unrestricted use" of the RI team's assets; the other files belong to their authors, mostly CivFanatics artists, and RI's manual names other games some of its models come from.
- This fork: magnublo, with GitHub Copilot and Claude as coding agents (the turn-time work is Claude's).
