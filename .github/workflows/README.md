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
2. Provision the toolkit and CRT libs (msvcrt.lib, msvcrtd.lib, msvcprt.lib) in CI.
3. Build the DLL through nmake targets defined in the Civ4 makefile structure.

## What Was Adapted For This Repo

The workflow is not a literal port of appveyor.yml. It was adapted to the current K-Mod repository and to GitHub Actions:

1. CI platform migration:
   - AppVeyor model -> GitHub Actions job on a Windows runner (windows-2022).
2. Build entrypoint alignment:
   - Uses this repository's existing nmake Release target from CvGameCoreDLL/Makefile.
3. VC++ Toolkit 2003 provisioning:
   - The original InstallShield installer cannot run on modern Windows Server runners (blocked by OS version checks and Windows Installer policy).
   - The toolkit files were extracted from the original installer's embedded MSI and packaged as a zip archive.
   - The zip (`vc2003-toolkit.zip`) is hosted as a GitHub release asset on this repository under the `build-deps/v1` tag, along with the CRT libs (`msvcrt.lib`, `msvcrtd.lib`, `msvcprt.lib`).
   - The workflow downloads and extracts the zip — no installer execution needed.
4. Dependency provisioning for this repo layout:
   - K-Mod does not store Boost-1.32.0 and Python24 under CvGameCoreDLL/.
   - Workflow fetches them when needed from FinalFrontierPlus/CvGameCoreDLL.
5. Cache strategy:
   - Cache the extracted VC++ Toolkit 2003 directory.
   - Cache Boost-1.32.0 and Python24 folders.
   - The toolkit install step is skipped entirely on cache hit.
6. Windows SDK compatibility:
   - The Civ4 Makefile expects a flat SDK layout (Include/windows.h, Lib/kernel32.lib, Bin/rc.exe).
   - The workflow first checks for legacy SDK installs (v7.0A, v7.1, etc.).
   - If none are found (typical on windows-2022 runners), it builds a shim directory from the Windows 10 SDK by flattening the versioned include/lib paths into the expected layout.
7. Artifact handling:
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

1. Runner image drift:
   - Hosted runner filesystem and bundled SDKs may change over time.
   - The Windows 10 SDK shim logic may need updating if Microsoft changes the SDK directory layout.
2. Legacy binary dependency source:
   - Boost-1.32.0 and Python24 currently come from an external Civ4 repository snapshot (FinalFrontierPlus/CvGameCoreDLL).
3. Build dependency hosting:
   - The VC++ Toolkit 2003 zip and CRT libs are hosted as GitHub release assets under the `build-deps/v1` tag on this repository.
   - If the release is deleted or the repository changes ownership, the `BUILD_DEPS_URL` env var in the workflow must be updated.

If any of those break, update the relevant URLs/paths in the workflow.

## Credits

1. TC01 for documenting the original automation pattern in the CivFanatics thread.
2. Civ4 modding community contributors in that thread and related SDK guides.
3. FinalFrontierPlus maintainers for preserving a repository snapshot with buildable legacy dependency folders.