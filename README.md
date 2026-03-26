# CoopAndreas
[![Made in Ukraine](https://img.shields.io/badge/made_in-ukraine-ffd700.svg?labelColor=0057b7)](https://stand-with-ukraine.pp.ua)

Videos, pictures, news, suggestions, and communication can be found here:

[![Discord](https://img.shields.io/badge/Discord%20Server%20(most%20active)-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/Z3ugSgFJMU)
[![YouTube](https://img.shields.io/badge/YouTube-FF0000?style=for-the-badge&logo=youtube&logoColor=white)](https://www.youtube.com/@CoopAndreas)
[![X](https://img.shields.io/badge/X-000000?style=for-the-badge&logo=x&logoColor=white)](https://x.com/CoopAndreas)
[![Telegram](https://img.shields.io/badge/Telegram-2CA5E0?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/coopandreas)
## Disclaimer
This mod is an unofficial modification for **Grand Theft Auto: San Andreas** and requires a legitimate copy of the game to function. No original game files or assets from Rockstar Games are included in this repository, and all content provided is independently developed. The project is not affiliated with Rockstar Games or Take-Two Interactive. All rights to the original game, its assets, and intellectual property belong to Rockstar Games and Take-Two Interactive. This mod is created solely for educational and non-commercial purposes. Users must comply with the terms of service and license agreements of Rockstar Games.


## Building (Windows)

### Beginner-friendly: one command (recommended)

If you are new and just want this to work, run this command from PowerShell in the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\bootstrap_and_build.ps1
```

What `bootstrap_and_build.ps1` does for you:
- Installs missing build tools via `winget` (`git`, `cmake`, `xmake`).
- Installs/checks Visual Studio 2022 Build Tools with the C++ workload.
- Clones `plugin-sdk` into `third_party\plugin-sdk` (if missing) and checks out the required commit.
- Runs `setup_and_build.ps1` to build `client`, `server`, and `proxy`, then deploy DLLs into your GTA:SA folder.

Optional flags:

```powershell
# Skip build and only install prerequisites + plugin-sdk
powershell -ExecutionPolicy Bypass -File .\scripts\windows\bootstrap_and_build.ps1 -SkipBuild

# Pre-fill paths to avoid prompts
powershell -ExecutionPolicy Bypass -File .\scripts\windows\bootstrap_and_build.ps1 -GtaSaDir "C:\Games\GTA San Andreas" -PluginSdkDir "D:\dev\plugin-sdk"
```

### Guided setup script (advanced/manual)

Use this if you already have dependencies installed and only want validation + build + deploy:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\windows\setup_and_build.ps1
```

What the script does:
- Checks required tooling: `xmake`, `cmake`, and Visual Studio 2022 C++ workload.
- Prompts for (or reuses) `GTA_SA_DIR` and `PLUGIN_SDK_DIR`, validates them, and stores them for the current user.
- Verifies `plugin-sdk` is on commit `050d18b6e1770477deab81a40028a40277583d97`.
- Runs build steps in order: `xmake f -m release`, then `client`, `server`, and `proxy` targets.
- Ensures `CoopAndreasSA.dll` is in `%GTA_SA_DIR%` and installs proxy as `%GTA_SA_DIR%\eax.dll` (backing up original to `eax_orig.dll` when needed).
- Prints next steps for compiling `main.scm` when `%GTA_SA_DIR%\CoopAndreas\main.scm` is missing.

### Client & Server

1. Make sure you have the **C++ package** installed in **Visual Studio 2022** and **[xmake](https://xmake.io/)** (needed to build the project).

2. Download **[this version of plugin-sdk](https://github.com/DK22Pac/plugin-sdk/tree/050d18b6e1770477deab81a40028a40277583d97)** and install it using **[this instruction](https://github.com/DK22Pac/plugin-sdk/wiki/Set-up-plugin-sdk)**.  
   Set up your GTA-SA and plugin-sdk folders, and make sure `GTA_SA_DIR` and `PLUGIN_SDK_DIR` environment variables are set.

3. Open a terminal in the repository root and build the projects:

```bash
# Build client DLL
xmake --build client

# Build server binary
xmake --build server
```
If `GTA_SA_DIR` is set, the client DLL (`CoopAndreasSA.dll`) will automatically be placed in your game directory.

---

### Proxy (DLL Loader)

The proxy is required to load (inject) the main DLL (`CoopAndreasSA.dll`) into the game executable.

1. Go to your game directory and rename `eax.dll` to `eax_orig.dll`.

2. Build the **Proxy** project:

```bash
xmake --build proxy
```

3. Rename the output DLL to `eax.dll` and copy it to your game folder.

---

### `main.scm`

1. Download and install **[Sanny Builder 4](https://github.com/sannybuilder/dev/releases)**.

2. Copy all files from the `sdk/Sanny Builder 4/` directory to the **Sanny Builder 4 installation folder**.  
   This will add **CoopAndreas opcodes** to the compiler.

3. Open `scm/main.txt` in **Sanny Builder 4**, compile it, and copy all generated files to  
   `${GTA_SA_DIR}/CoopAndreas/`.

---

### Running

1. Run `server.exe` it will start accepting client connections.
2. Run `gta_sa.exe`, press **Start Game**, enter your nickname, then provide the server IP and port.
   - If the server is running on the same machine, use `127.0.0.1`
   - Default port: `6767`


## Building (GNU/Linux)

### Server

Fast path (recommended):

```bash
./scripts/linux/setup_and_build.sh
```

What it does:
- Installs required packages (`build-essential`, `clang`, `cmake`, `git`, `xmake`).
- Configures xmake in `release` mode.
- Builds the `server` target.
- Prints the server binary location.

Optional behavior:

```bash
# Build debug mode
MODE=debug ./scripts/linux/setup_and_build.sh

# Build and launch server immediately
RUN_SERVER=1 ./scripts/linux/setup_and_build.sh
```

Manual build (if you prefer step-by-step):

```bash
sudo apt-get update
sudo apt-get install -y build-essential clang cmake xmake
xmake f -m release
xmake --build server
./build/linux/x86_64/release/server
```

Linux build notes:
- The server bundles ENet sources from `third_party/enet` and links pthreads for threading support.
- Linux socket APIs come from glibc (no separate socket library is required, unlike Windows `ws2_32`).

### Root CMake configure (canonical for CMake-based tooling)

If your tooling expects a root `CMakeLists.txt`, configure from the repository root:

```bash
cmake -S . -B build
cmake --build build -j2
```

This CMake entry point delegates to `cmake/CMakeLists.txt` and provides optional `xmake` bridge targets (for example: `cmake --build build --target coopandreas-xmake`).


## Developer note: mission sync state

`CMissionSyncState` centralizes mission/cutscene transient state on the client side.

- **Opcode pre-execution stage:** specific opcodes are routed through a small handler table (`HandleOpCodePreExecute`). `0x0701` clears skip HUD text immediately; `0x02E7` (start cutscene) is deferred until cutscene assets are loaded.
- **Frame update stage:** `Main.cpp` now calls `ProcessDeferredCutsceneStart()` and `ProcessMissionAudioLoading()` every networked frame instead of inline checks.
- **Host behavior:** host does not auto-play deferred mission audio/cutscene start from this state module; it remains authoritative and only syncs the opcode/events.
- **Non-host behavior:** non-host clients wait for cutscene load status/audio slot readiness, then trigger `START_CUTSCENE` / `PlayLoadedMissionAudio` exactly once and clear the pending flags.
- **Task sequence status:** sequence processing status is tracked in this module and toggled by `CTaskSequenceSync` around replayed opcode execution.

## Donate
https://send.monobank.ua/jar/8wPrs73MBa

USDT TRC20: `TNdTwiy9JM2zUe8qgBoMJoAExKf4gs5vGA`

BTC: `bc1qwsl8jv2gyvry75j727qkktr5vgcmqm5e69qt2t`

ETH: `0xE7aE0448A147844208C9D51b0Ac673Bafbe2a35c`

PayPal `kirilltymoshchenko59@gmail.com`

*If you need another way to donate, please dm me on discord: `@tornamic`*

## Project backlog

Detailed tracking for gameplay sync, launcher work, mission parity, and optional script content now lives in **`docs/ROADMAP.md`**.

- Current focus milestones: **core sync stability**, **mission parity**, **launcher UX**, **optional content**.
- Every item in the roadmap is tagged with **priority** (`P0/P1/P2`) and **effort** (`S/M/L`).
- All previously completed checklist items from the old README TODO sections were removed from active tracking so only open work remains.

➡️ See: [docs/ROADMAP.md](docs/ROADMAP.md)
