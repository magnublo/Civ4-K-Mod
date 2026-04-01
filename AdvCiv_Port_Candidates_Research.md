# AdvCiv / AdvCiv-SAS Non-Gameplay Port Candidate Research (for K-Mod baseline)

## Scope and Method
- **Baseline:** original K-Mod behavior and architecture.
- **Target:** this repository (`magnublo/Civ4-K-Mod`).
- **Upstreams inspected:**
  - `f1rpo/AdvCiv`
  - `wonderingabout/AdvCiv-SAS`
- **Method:** reviewed upstream commit history and individual commit diffs (not only current file state), then classified items as:
  - **Safe candidate** (likely gameplay-neutral)
  - **Mixed / needs separation**
  - **Reject** (gameplay-altering or too entangled)

---

## Performance

### 1) Scoreboard lag fix (avoid slow repeated UI text updates)
- **Category:** Performance / UI
- **Upstream source:** AdvCiv
- **Commit(s):**
  - `7f19709ef56cf0974960b5c5461cceb0e8db8380`
  - `79b002888ad509c3a48c87d4b6fa73da498c8379`
- **Affected files:**
  - `Assets/Python/BUG/Scoreboard.py`
  - `Assets/Python/Screens/CvMainInterface.py`
- **Affected systems/functions:** scoreboard refresh/hide-show path, `CyGInterfaceScreen.setText` call frequency
- **What changed:** refined prior scoreboard fix to avoid expensive redraw patterns.
- **Why:** explicit upstream note that `setText` was slow and causing lag symptoms.
- **Assessment:** **Safe candidate**
- **Porting notes:** self-contained Python-side; likely minimal conflict unless local scoreboard logic diverged.

### 2) AI control check micro-optimization in hot paths
- **Category:** Performance / AI infrastructure
- **Upstream source:** AdvCiv
- **Commit(s):** `791a45ea07b51d153832d8b34474dd2ec88f1114`
- **Affected files:**
  - `CvGameCoreDLL/CvSelectionGroup.cpp/.h`
  - `CvGameCoreDLL/CvSelectionGroupAI.cpp/.h`
  - `CvGameCoreDLL/CvMap.cpp`
  - `CvGameCoreDLL/CvUnitAI.cpp`
  - `CvGameCoreDLL/GroupPathFinder.cpp`
  - `CvGameCoreDLL/CvVirtualWrappers.cpp`
- **Affected systems/functions:** `AI_isControlled` -> `isAIControlled` move/nonvirtualization
- **What changed:** moved frequent control-check function to base class and removed virtual overhead in common calls.
- **Why:** reduce per-call overhead in often-used pathfinding/AI loops.
- **Assessment:** **Safe candidate**
- **Porting notes:** mechanical but cross-file; check wrapper and naming collisions.

### 3) `ArrayEnumMap<bool>` memory packing fix
- **Category:** Performance / Technical cleanup
- **Upstream source:** AdvCiv
- **Commit(s):** `5104e569eb814997df286681e3f322f977dcfba7`
- **Affected files:**
  - `CvGameCoreDLL/EnumMap.h`
  - touch points in `CvCity.cpp`, `CvGame.cpp`, `CvPlayer.cpp`, `CvTeam.cpp`, `CvUnit.cpp`
- **Affected systems/functions:** dynamic enum-map storage for bool values
- **What changed:** fixed bool storage from byte-block-like behavior to bit-level packing in dynamic mode.
- **Why:** memory efficiency and correctness of container implementation.
- **Assessment:** **Safe candidate**
- **Porting notes:** mostly utility-layer; verify template behavior where this repo instantiates dynamic bool maps.

### 4) Minor map-area/shelf computation perf tweaks
- **Category:** Performance
- **Upstream source:** AdvCiv
- **Commit(s):** `a12ed4ce5ade2f935869f0934edc1d3e76ae2a53`
- **Affected files:** `CvGameCoreDLL/CvMap.cpp`
- **Affected systems/functions:** map shelf/area calculation paths
- **What changed:** small optimization/refactor in map math.
- **Why:** reduce overhead in repeated map calculations.
- **Assessment:** **Safe candidate**
- **Porting notes:** self-contained; review for assumptions already changed in this repo.

### 5) Sevopedia grouping re-sort elimination
- **Category:** Performance / UI
- **Upstream source:** AdvCiv-SAS
- **Commit(s):** `53469ab2618d1c9ef6f4e8ad673f6eff7a2ef17d`
- **Affected files:**
  - `Assets/Python/Contrib/Sevopedia/SevoPediaMain.py`
  - `Assets/Python/Contrib/Sevopedia/_sevopedia_main_groupings.py`
- **Affected systems/functions:** Sevopedia list grouping/sorting pipeline
- **What changed:** removed redundant second sorting when list already sorted upstream.
- **Why:** avoid repeated work on pedia category construction.
- **Assessment:** **Safe candidate**
- **Porting notes:** only relevant if this repo has equivalent Sevopedia grouping helpers.

### 6) CvCityAI cache-heavy optimization series
- **Category:** Performance / AI
- **Upstream source:** AdvCiv-SAS
- **Commit(s):**
  - `3e072ae4096baea2eb9a2657ae47f3aae702f82e`
  - `1f9b99fd955e07d83cd3984be22b61bad3bc4220`
- **Affected files:** primarily `CvGameCoreDLL/CvCityAI.cpp` (plus many unrelated files in second commit)
- **Affected systems/functions:** `CvCityAI::AI_buildingValue` and related repeated lookups
- **What changed:** aggressive caching of repeated per-turn/per-player/per-team computations.
- **Why:** reduce expensive recomputation.
- **Assessment:** **Mixed / needs separation**
- **Porting notes:** second commit is heavily entangled (docs/UI/other code). Candidate only after isolating narrow DLL-only cache blocks and validating no behavior drift.

---

## UI Improvements

### 7) F-key advisor switching fix
- **Category:** UI bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `1616a5522446f208b1f5b7b5d179a51241c1b62e`
- **Affected files:** `CvGameCoreDLL/CvEnums.h`
- **Affected systems/functions:** advisor hotkey/screen enum mapping
- **What changed:** corrected enum ordering/mapping issue that broke advisor switching via function keys.
- **Why:** restore expected UI navigation.
- **Assessment:** **Safe candidate**
- **Porting notes:** confirm local enum set/order before cherry-picking logic.

### 8) Civics screen heading layout fix
- **Category:** UI bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `6b1e29d27251fcb5ab3f8a075f90df1e86564d74`
- **Affected files:** `Assets/Python/Screens/CvCivicsScreen.py`
- **Affected systems/functions:** Civics screen text placement
- **What changed:** adjusted coordinates/sizing to correct heading misplacement.
- **Why:** fix layout regression.
- **Assessment:** **Safe candidate**
- **Porting notes:** likely conflict-light Python patch.

### 9) Plot list sizing/position fix (BUG draw method)
- **Category:** UI bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `0cc9e1c4aa26b904e6c7d8265032a1124323c183`
- **Affected files:** `Assets/Python/BUG/BugUnitPlot.py`
- **Affected systems/functions:** unit plot list rendering bounds
- **What changed:** corrected size/position math.
- **Why:** fix post-overhaul UI alignment bug.
- **Assessment:** **Safe candidate**
- **Porting notes:** easy if this repo uses same BUG module revision.

### 10) City automation button state display fix
- **Category:** UI bug fix / QoL
- **Upstream source:** AdvCiv
- **Commit(s):** `8e0b92c3a1ee9b0957c8f3ddb014512c8f4f5ea8`
- **Affected files:** `Assets/Python/Screens/CvMainInterface.py`
- **Affected systems/functions:** city automation toggle visuals
- **What changed:** fixed on/off status indicators not updating correctly.
- **Why:** remove misleading city-screen feedback.
- **Assessment:** **Safe candidate**
- **Porting notes:** localized Python-side.

### 11) GP bar / advisor button overlap fix
- **Category:** UI bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `ed1c1b30337866eff186b526f19adfe3208e8885`
- **Affected files:** `Assets/Python/Screens/CvMainInterface.py`
- **Affected systems/functions:** main HUD layout under scaled GP bars
- **What changed:** adjusted offsets to prevent overlap.
- **Why:** improve readability and clickability.
- **Assessment:** **Safe candidate**
- **Porting notes:** verify against any local custom HUD scaling.

### 12) Domestic advisor dynamic column/font sizing
- **Category:** UI improvement
- **Upstream source:** AdvCiv
- **Commit(s):** `07bd8466ad81b4b846621d9691f708b9520a43b3`
- **Affected files:**
  - `Assets/Python/Screens/CvDomesticAdvisor.py`
  - `Assets/Python/Screens/CvCustomizableDomesticAdvisor.py`
- **Affected systems/functions:** domestic advisor layout adaptation to available screen width
- **What changed:** larger, more readable columns/fonts when space permits.
- **Why:** usability on larger resolutions.
- **Assessment:** **Safe candidate**
- **Porting notes:** medium-size Python diff; ensure no collision with local advisor customizations.

### 13) Tech chooser + Sevopedia build-hover fixes
- **Category:** UI bug fix
- **Upstream source:** AdvCiv-SAS
- **Commit(s):** `63c976964cb516b1dd7d37d73defd74f47fff2c6`
- **Affected files:**
  - `Assets/Python/Screens/CvTechChooser.py`
  - `Assets/Python/Contrib/Sevopedia/SevoPediaBuild.py`
  - `Assets/Python/Contrib/Sevopedia/SevoPediaFeature.py`
  - `Assets/Python/Contrib/Sevopedia/SevoPediaImprovement.py`
  - `Assets/Python/Contrib/Sevopedia/SevoPediaIndex.py`
  - `Assets/Python/Contrib/Sevopedia/SevoPediaMain.py`
- **Affected systems/functions:** hover tooltip routing and indexing in pedia/tech chooser
- **What changed:** corrected hover behavior and related index lookups.
- **Why:** fix user-facing tooltips/navigation defects.
- **Assessment:** **Safe candidate** (if Sevopedia codebase roughly matches)
- **Porting notes:** validate each sub-change; some files may have drifted heavily in SAS.

### 14) Left-click GP bar opens corresponding city
- **Category:** UI / QoL
- **Upstream source:** AdvCiv-SAS
- **Commit(s):** `4ccd0353ee8f1b64afb2af6cc7dcf3bfdafe1731`
- **Affected files:** `Assets/Python/Screens/CvMainInterface.py`
- **Affected systems/functions:** GP progress bar click handler
- **What changed:** click action now navigates to producing city.
- **Why:** removes manual city hunt.
- **Assessment:** **Safe candidate**
- **Porting notes:** self-contained, but requires equivalent city lookup context in handler.

### 15) Diplomacy screen icon enhancements
- **Category:** UI improvement
- **Upstream source:** AdvCiv-SAS
- **Commit(s):**
  - `0656c16e6dc5c308911268e5ab7a3a10003ea1af`
  - `42823ae07df0f8e19602467de9df7faa1b5a2fbe`
  - `2078e7e8e9acc04ebf59a13dbfec0fa775a0723d`
- **Affected files:**
  - `CvGameCoreDLL/CvPlayer.cpp`
  - `Assets/Python/Screens/CvExoticForeignAdvisor.py`
  - `Assets/XML/Text/AdvCiv-SAS_main.xml`
- **Affected systems/functions:** trade item string rendering (`CvPlayer::getItemTradeString`), foreign advisor presentation
- **What changed:** added icons and tab label/presentation improvements.
- **Why:** faster parsing of diplomatic information.
- **Assessment:** **Mixed / needs separation**
- **Porting notes:** icon/text parts may be safe; advisor-tab behavior changes include content choices and may need selective extraction.

---

## Quality of Life (Gameplay-Neutral)

### 16) On-screen message timing fix
- **Category:** QoL / UI behavior
- **Upstream source:** AdvCiv
- **Commit(s):** `7a6bce539f23e8e6d9588ccc25ed71248a684e19`
- **Affected files:** `CvGameCoreDLL/CvDLLInterfaceIFaceBase.cpp`
- **Affected systems/functions:** message queue timing/display
- **What changed:** corrected logic causing some messages to appear only after delay/turn boundary.
- **Why:** timely player feedback.
- **Assessment:** **Safe candidate**
- **Porting notes:** tiny DLL-side patch, likely low risk.

### 17) Unload button lingering-state fix
- **Category:** QoL / UI bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `b560a83419ac8ec9e220d03049fb2186c8c6c4ab`
- **Affected files:** `CvGameCoreDLL/CvUnit.cpp`
- **Affected systems/functions:** action button availability after unload/group changes
- **What changed:** corrected stale unload button visibility.
- **Why:** prevents misleading command UI.
- **Assessment:** **Safe candidate**
- **Porting notes:** localized; validate with group unload edge cases.

### 18) Tile hover owner attribution fix
- **Category:** QoL / UI text correctness
- **Upstream source:** AdvCiv
- **Commit(s):** `e2686d498434d098dedbab4ed31bfb3ba6c721b1`
- **Affected files:** `CvGameCoreDLL/CvGameTextMgr.cpp`
- **Affected systems/functions:** unit stack hover text ownership resolution
- **What changed:** corrected owner shown in specific mixed-rival stack layouts.
- **Why:** improves hover accuracy.
- **Assessment:** **Safe candidate**
- **Porting notes:** minimal DLL diff.

---

## AI Tuning / AI Behavior (Non-rule-changing)

### 19) Pathfinder danger-avoidance correctness series
- **Category:** AI behavior correctness / stability
- **Upstream source:** AdvCiv
- **Commit(s):**
  - `2eddf22d1761aa426ebe581908a3143069ce542c` (workaround / early catch)
  - `b3de867844ccdf0937797e6a6375e22816d042f6` (fuller fix)
- **Affected files:**
  - `CvGameCoreDLL/CvSelectionGroup.cpp`
  - `CvGameCoreDLL/FAStarFunc.cpp`
  - `CvGameCoreDLL/GroupPathFinder.cpp/.h`
  - `CvGameCoreDLL/KmodPathFinder.h`
  - `CvGameCoreDLL/KmodPathFinderLegacy.cpp/.h`
  - `CvGameCoreDLL/TeamPathFinder.h`
- **Affected systems/functions:** pathfinder cost/evaluation consistency around danger checks
- **What changed:** fixed edge-case inconsistency that could generate bad paths / rare loops.
- **Why:** improve AI movement robustness and prevent pathological updates.
- **Assessment:** **Mixed / needs separation**
- **Porting notes:** broad pathfinder touch set; port only as tested mini-series, not isolated hunks.

### 20) Rare plot-to-working-city assignment bug fix
- **Category:** AI/engine correctness
- **Upstream source:** AdvCiv
- **Commit(s):** `8f92e0843d86a3312c0bb115a66d130f53f9f492`
- **Affected files:**
  - `CvGameCoreDLL/CvPlot.cpp/.h`
  - `CvGameCoreDLL/CvSelectionGroupAI.cpp`
  - `CvGameCoreDLL/CvStructs.h`
  - `CvGameCoreDLL/CvUnit.cpp/.h`
- **Affected systems/functions:** `IDInfo` / working-city assignment lifetime/use patterns
- **What changed:** adjusted IDInfo handling to avoid rare mis-assignment bug.
- **Why:** correctness in city work-plot linkage.
- **Assessment:** **Safe candidate** (engine correctness)
- **Porting notes:** multi-file but still focused; verify save/load invariants and IDInfo assumptions.

---

## Technical Bug Fixes / Engine Cleanups

### 21) OOS logger locale compatibility
- **Category:** Technical bug fix / MP support
- **Upstream source:** AdvCiv
- **Commit(s):** `6b3ece079dab0c0acf9fa89b23e002ffdd421d0f`
- **Affected files:** `Assets/Python/Contrib/OOSLogger.py`
- **Affected systems/functions:** OOS log file/string handling for non-Latin locales
- **What changed:** encoding/format handling made locale-safe.
- **Why:** avoid failures on non-Latin environments.
- **Assessment:** **Safe candidate**
- **Porting notes:** pure Python, low conflict risk.

### 22) OOS fix on human group bombard/splits
- **Category:** Technical bug fix / MP stability
- **Upstream source:** AdvCiv
- **Commit(s):** `9e70b989b233e1cd6352d67cb695581b883099f8`
- **Affected files:** `CvGameCoreDLL/CvSelectionGroup.cpp`
- **Affected systems/functions:** group split/bombard move synchronization
- **What changed:** fixed state mismatch causing OOS in specific split scenarios.
- **Why:** multiplayer determinism.
- **Assessment:** **Safe candidate**
- **Porting notes:** localized patch with high practical value for MP-heavy users.

### 23) BUG options first-run persistence fix
- **Category:** Technical bug fix / QoL
- **Upstream source:** AdvCiv
- **Commit(s):** `2f14174118899095e1f4b3c64638e4da907c73b8`
- **Affected files:** `Assets/Python/BUG/configobj.py`
- **Affected systems/functions:** initial BUG config write path
- **What changed:** ensured options persist on first run.
- **Why:** prevent confusing “settings didn’t save” behavior.
- **Assessment:** **Safe candidate**
- **Porting notes:** tiny upstream patch; likely directly portable.

### 24) Python crash fix for corp/no-religion city path
- **Category:** Technical bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `5ba43d46b67eef24dbcff90d4f925bd2c2ec38ce`
- **Affected files:**
  - `Assets/Python/Screens/CvMainInterface.py`
  - `Assets/Python/Screens/LayoutDict.py`
- **Affected systems/functions:** city-screen UI data path for corporation/religion assumptions
- **What changed:** added guards against missing religion data with corporations.
- **Why:** prevent Python exception/crash path.
- **Assessment:** **Safe candidate**
- **Porting notes:** small Python guard change; good low-risk pickup.

### 25) Potential memory leak fix in replay info
- **Category:** Technical bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `fc41848ceac8e16d81a9f06d127a84be94ccfa6b`
- **Affected files:** `CvGameCoreDLL/CvReplayInfo.cpp`
- **Affected systems/functions:** replay info resource management
- **What changed:** corrected cleanup/ownership path.
- **Why:** eliminate possible leak.
- **Assessment:** **Safe candidate**
- **Porting notes:** very contained C++ patch.

### 26) Cached commerce/culture consistency + savegame recalc set
- **Category:** Technical bug fix / consistency
- **Upstream source:** AdvCiv
- **Commit(s):**
  - `a23c3c3d92de97a343e0d170a1eb20105c232b8b`
  - `b9858b0878b8adb43454661b326c933b1cfc818b`
  - `31f10c9e7536a230d0d160195bdf986a633052b7`
  - `eb30bf1d446a9380801c690b1f5c838aae4930e0`
  - `532ed697efb7e5264b21511b7a789be93d1b00e2`
- **Affected files:**
  - `CvGameCoreDLL/CvPlayer.cpp/.h`
  - `CvGameCoreDLL/CvCity.cpp`
  - `CvGameCoreDLL/CyPlayer.cpp/.h`
  - `CvGameCoreDLL/CyPlayerInterface1.cpp`
  - `Assets/Python/Screens/EconomicsAdvisor.py`
- **Affected systems/functions:** special-commerce and culture caches, Python exposure, savegame recalc
- **What changed:** fixed cache correctness in edge states and added recovery on load for old saves.
- **Why:** prevent stale/incorrect financial breakdowns and cache state drift.
- **Assessment:** **Safe candidate** (as a small series)
- **Porting notes:** port as a coherent bundle; single commits depend on prior cache assertions and recalc logic.

### 27) Wide/narrow string conversion bugfix
- **Category:** Technical cleanup / bug fix
- **Upstream source:** AdvCiv
- **Commit(s):** `20003c613157a7d0fbb7e084d14d514694665db2`
- **Affected files:** `CvGameCoreDLL/CvString.cpp`
- **Affected systems/functions:** assignment between `CvString` and `CvWString`
- **What changed:** fixed ineffective cross-width string assignment operators.
- **Why:** correctness and hardening against subtle text bugs.
- **Assessment:** **Safe candidate**
- **Porting notes:** self-contained and broadly useful.

### 28) UI theme load diagnostics and path hardening
- **Category:** Technical diagnostics / robustness
- **Upstream source:** AdvCiv
- **Commit(s):**
  - `c0ed55ab48ec30fc14f1959f3c69646565a529c7`
  - `9597346ec577c5def40ce8cbeaaaa56c8f8d1f6d`
  - `dba4f59c35dccf724125fab59d17db9cd03ddc9f`
- **Affected files:**
  - `CvGameCoreDLL/CvArtFileMgr.cpp/.h`
  - `CvGameCoreDLL/CvGlobals.cpp/.h`
  - `CvGameCoreDLL/ModName.cpp`
  - `CvGameCoreDLL/CvDLLButtonPopup.cpp`
- **Affected systems/functions:** UI theme loading and warning/error output
- **What changed:** better diagnostics plus path capitalization robustness.
- **Why:** easier troubleshooting and fewer path-related false failures.
- **Assessment:** **Safe candidate**
- **Porting notes:** medium-sized but independent of gameplay systems.

### 29) SAS crash fixes tied to known base-AdvCiv defects
- **Category:** Technical crash fixes
- **Upstream source:** AdvCiv-SAS
- **Commit(s):**
  - `d2500dc40107815d6a0afef5b481b9d2073c1743` (CvSelectionGroup::plot crash)
  - `24cfc0803db7251f521f0b9b6df1c620109ae3a8` (CvCity::getProductionBarPercentages crash)
  - `feaaae6ce35340be2b9d0e68dc5e4137329e728c` (variant of same crash)
  - `abaf980a63b6cd7ca128b732ce60366f6d44a4fb` (Sevopedia/main-menu crash path)
- **Affected files:**
  - `CvGameCoreDLL/CvCity.cpp`
  - `CvGameCoreDLL/CvGameInterface.cpp`
  - `CvGameCoreDLL/CvGameTextMgr.cpp`
  - `CvGameCoreDLL/GroupPathFinder.cpp`
  - `CvGameCoreDLL/RiseFall.cpp`
  - `CvGameCoreDLL/CvGame.cpp`, `CvDllTranslator.cpp` (in one large mixed commit)
- **Affected systems/functions:** city bar %, selection-group plot access, Sevopedia interface access guards
- **What changed:** defensive guards and crash-prevention checks in known fault paths.
- **Why:** stability hardening.
- **Assessment:** **Mixed / needs separation**
- **Porting notes:** each commit includes extra unrelated edits in some cases; manually isolate minimal crash-fix hunks before porting.

---

## Mixed or Probably Gameplay-Altering (Do Not Directly Port as “non-gameplay”)

### 30) Lake-generation overflow fix commit includes map-generation behavior change
- **Category:** Mixed (engine bug + gameplay/map behavior)
- **Upstream source:** AdvCiv
- **Commit(s):** `1b2adf4aea769c23269ac862d0c2f64c488f5b35`
- **Affected files:** `CvMap.cpp`, `CvMapGenerator.cpp`, `CvPlot.cpp`
- **Why mixed:** includes legitimate overflow/perf fixes **and** explicit “lakes can no longer replace peaks” behavior change.
- **Assessment:** **Mixed / needs separation**
- **Porting notes:** only port arithmetic/overflow safety parts after isolating from worldgen-rule changes.

### 31) Large SAS AI “performance” commits with balance/behavior retuning
- **Category:** Mixed / gameplay-altering
- **Upstream source:** AdvCiv-SAS
- **Commit(s):** e.g. `1f9b99fd955e07d83cd3984be22b61bad3bc4220` and nearby series
- **Affected files:** broad, including AI valuation, UI, docs, XML/text
- **Why mixed:** combines caching with strategic behavior changes and many unrelated edits.
- **Assessment:** **Reject as direct port candidate**
- **Porting notes:** only use as idea source; require fresh reimplementation of tiny proven-neutral pieces.

### 32) SAS diplomacy/foreign-advisor visual overhauls with semantic/UI-content shifts
- **Category:** Mixed
- **Upstream source:** AdvCiv-SAS
- **Commit(s):** `2078e7e8e9acc04ebf59a13dbfec0fa775a0723d` (and related)
- **Affected files:** `CvExoticForeignAdvisor.py`, DLL trade string text, XML text
- **Why mixed:** mostly UI, but tab semantics and shown content changed.
- **Assessment:** **Mixed / needs separation**
- **Porting notes:** iconography and string rendering may be safe; screen-content decisions should be reviewed as design choices.

### 33) SAS startup/widget constant fix likely partly target-specific
- **Category:** Mixed / environment-dependent
- **Upstream source:** AdvCiv-SAS
- **Commit(s):** `6f94679507a14828421cdc06eeb033a436f1e890`
- **Affected files:** `Assets/Python/BUG/BugPath.py`, `Assets/Python/Screens/CvTechChooser.py`
- **Why mixed:** addresses specific binding/path inconsistencies in certain DLL/EXE setups.
- **Assessment:** **Mixed / needs validation**
- **Porting notes:** investigate whether this repository has the same missing binding and BUG path behavior before porting.

---

## Suggested Porting Order (shortlist)
1. **Low-risk, high-value:** #23, #24, #25, #21, #22, #16, #18
2. **UI polish/fixes:** #7, #8, #9, #10, #11, #12, #14
3. **Performance-infrastructure:** #1, #2, #3, #4
4. **Bundle candidates requiring careful sequencing:** #26, #19
5. **Manual archaeology first (mixed):** #5, #6, #15, #29, #30, #33

## Notes for follow-up implementation agents
- Prefer cherry-picking **single safe commits** first where upstream commit is narrow and self-contained.
- For mixed commits, diff-split by subsystem and port only gameplay-neutral hunks with explicit validation.
- For any DLL changes touching pathfinding or cached values, do AI Auto Play smoke tests plus MP/OOS sanity checks before acceptance.
