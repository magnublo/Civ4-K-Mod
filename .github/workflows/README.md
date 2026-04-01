# Civ4 DLL CI Workflow Notes

This folder contains the GitHub Actions pipeline that builds the K-Mod DLL:

- build-civ4-dll.yml

The goal of this document is to record where the workflow design came from, which parts were copied as known-good patterns, and which parts were adapted for this repository.

## Background and Sources

Primary learning source:

- CivFanatics thread: Scripting a build of the DLL!  
  https://forums.civfanatics.com/threads/scripting-a-build-of-the-dll.567819/

Key author and approach:

- TC01 documented an AppVeyor setup for legacy Civ4 DLL builds.
- The thread pattern installs Microsoft Visual C++ Toolkit 2003 non-interactively and downloads msvcrt.lib, msvcrtd.lib, and msvcprt.lib from Kael-hosted files.
- The thread also mentions the afxres.h issue in resource compilation and a windows.h workaround.

Additional reference material:

- Civ4 SDK setup guide (Modiki):  
  http://modiki.civfanatics.com/index.php/How_to_Install_the_SDK#Installing_the_Tools
- The Civ4-compatible makefile and project conventions in this repository:
  - CvGameCoreDLL/Makefile
  - CvGameCoreDLL/CvGameCoreDLL.vcproj
- A repository that includes the legacy dependency folders used by many Civ4 SDK builds:
  - https://github.com/FinalFrontierPlus/CvGameCoreDLL

## What Was Copied As Pattern

The following ideas are directly inherited from the CivFanatics/AppVeyor-era process:

1. Use the legacy toolchain (VC++ Toolkit 2003) instead of modern MSVC for binary compatibility with the Civ4 DLL build ecosystem.
2. Install the toolkit in automation with a silent installer invocation.
3. Download the missing runtime library files into the toolkit lib directory.
4. Build the DLL through nmake targets defined in the Civ4 makefile structure.

## What Was Adapted For This Repo

The workflow is not a literal port of appveyor.yml. It was adapted to the current K-Mod repository and to GitHub Actions:

1. CI platform migration:
   - AppVeyor model -> GitHub Actions job on a Windows runner.
2. Build entrypoint alignment:
   - Uses this repository's existing nmake Release target from CvGameCoreDLL/Makefile.
3. Dependency provisioning for this repo layout:
   - K-Mod does not currently store Boost-1.32.0 and Python24 under CvGameCoreDLL/.
   - Workflow fetches them when needed from FinalFrontierPlus/CvGameCoreDLL.
4. Cache strategy:
   - Cache the VC++ Toolkit 2003 install directory.
   - Cache Boost-1.32.0 and Python24 folders.
   - This reduces repeated cold-start download and install cost.
5. Windows SDK robustness:
   - The job checks several SDK candidate paths before building and exports the selected path.
6. Artifact handling:
   - Uploads built DLL and PDB artifacts.
   - Also stages a DLL copy into Assets for convenience in downstream workflows or manual download.

## Why These Choices

1. Reliability over novelty:
   - Civ4 DLL builds rely on old assumptions; preserving known-compatible toolchain behavior is safer than rewriting build logic.
2. Fast reruns:
   - Most runtime cost is in old dependency setup; caching targets those expensive parts first.
3. Explicit provenance:
   - Legacy Civ4 modding knowledge is scattered across forum posts and old docs, so source attribution is recorded here.

## Known Risks and Maintenance Notes

1. External host dependency:
   - Kael-hosted binaries are external and could become unavailable.
2. Runner image drift:
   - Hosted runner filesystem and bundled SDKs may change over time.
3. Legacy binary dependency source:
   - Boost-1.32.0 and Python24 currently come from an external Civ4 repository snapshot.

If any of those break, mirror artifacts in a stable location and update workflow URLs/paths accordingly.

## Credits

1. TC01 for documenting the original automation pattern in the CivFanatics thread.
2. Civ4 modding community contributors in that thread and related SDK guides.
3. FinalFrontierPlus maintainers for preserving a repository snapshot with buildable legacy dependency folders.