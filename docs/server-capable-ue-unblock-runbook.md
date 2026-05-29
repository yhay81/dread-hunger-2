# Server-Capable UE 5.7 Unblock Runbook

Use this runbook to obtain or point `UE_ROOT` at a server-capable Unreal Engine 5.7
installation so that `AbyssLockServer.exe` can be built, removing the current GP-02 blocker.

---

## Current Blocker (as of 2026-05-29)

The only Unreal Engine on the local Windows machine is the Launcher distribution installed at
`C:\Program Files\Epic Games\UE_5.7`. This distribution hard-rejects server targets in UBT
with:

```
Server targets are not currently supported from this engine distribution
```

UBT exits with code 6. The gate command

```powershell
cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server
```

records the `build_server` step as `blocked` (not `fail`). No project code change will fix
this; it is an engine-distribution constraint.

The `frostwake-tools` resolver checks `UE_ROOT`, then `UNREAL_ENGINE_ROOT`, then
`%ProgramFiles%\Epic Games\UE_5.7`, then `C:\Program Files\Epic Games\UE_5.7` — in that
order. A Launcher install found at any of those paths still blocks server targets.

---

## Option A: Source Build from Epic GitHub (recommended for full server support)

This is the only confirmed path to a UE 5.7 Windows distribution that supports Server targets.

### Prerequisites

- A GitHub account linked to an Epic Games account (Epic's developer agreement accepted at
  [unrealengine.com](https://www.unrealengine.com/en-US/ue-on-github)). Epic grants access to
  the private `EpicGames/UnrealEngine` repository once the agreement is accepted.
- At least 200 GB of free disk space for the source checkout and build.
- Visual Studio 2022 (version 17.14 or later, Current channel) with the
  **Game development with C++** workload, MSVC 14.44-compatible tooling, and a Windows 10/11
  SDK installed. Do not use Visual Studio 2026 / MSVC 14.50 until Epic marks it supported for
  UE 5.7.
- Approximately 2-4 hours of build time on typical developer hardware.

### Steps

1. **Obtain source access.** Accept the Unreal Engine End User License Agreement on
   [unrealengine.com](https://www.unrealengine.com/en-US/ue-on-github), then link your GitHub
   account in your Epic account settings. You will receive an invitation to the
   `EpicGames/UnrealEngine` repository within minutes.

2. **Clone the 5.7 branch.**

   ```powershell
   git clone --branch 5.7 --depth 1 https://github.com/EpicGames/UnrealEngine.git D:\UE_5.7_Source
   cd D:\UE_5.7_Source
   ```

   A shallow clone (`--depth 1`) is sufficient for a local build. Adjust the destination path
   (`D:\UE_5.7_Source`) to a drive with sufficient space.

3. **Run the setup scripts.** From the repository root:

   ```powershell
   .\Setup.bat
   .\GenerateProjectFiles.bat
   ```

   `Setup.bat` downloads binary dependencies (several GB). `GenerateProjectFiles.bat` creates
   the `UE5.sln` solution file.

4. **Build the engine.** Open `UE5.sln` in Visual Studio 2022 and build the
   `Development Editor` configuration for `Win64`, or run from the command line:

   ```powershell
   msbuild UE5.sln /p:Configuration="Development Editor" /p:Platform=Win64 /m
   ```

   This produces `Engine\Binaries\Win64\UnrealEditor.exe` and the full toolchain including
   server-target support.

5. **Set `UE_ROOT`** to the source build root before running any Frostwake tooling:

   ```powershell
   $env:UE_ROOT = "D:\UE_5.7_Source"
   ```

   To persist this for the current user session, add it to your PowerShell profile or Windows
   environment variables. The `frostwake-tools` resolver picks up `UE_ROOT` first; the Launcher
   install at `C:\Program Files\Epic Games\UE_5.7` is still present but will not be used.

---

## Option B: Alternative Server-Capable Distribution

Epic may provide server-capable binary distributions (for example, a "Development Server"
prebuilt package distributed separately from the Launcher) in future 5.7.x releases. Check
[Epic's developer documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/dedicated-servers-in-unreal-engine)
for any such distribution before undertaking a full source build.

As of the current evidence (cycle 65/70/78, 2026-05-26), no such pre-built alternative is
known to be available. A source build (Option A) is the confirmed path.

---

## Verification

After pointing `UE_ROOT` at a server-capable build, run the following from the repository root.
Use an isolated Cargo target directory to avoid locking the user's own build:

```powershell
$env:CARGO_TARGET_DIR = "target\loop-gp02"
$env:UE_ROOT = "D:\UE_5.7_Source"   # adjust to the actual source-build root
cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server
```

Expected output: all checks green, specifically `build_server: pass` and
`Binaries\Win64\AbyssLockServer.exe` present in the working tree.

If `build_server` still shows `blocked`, UBT output will include the engine-distribution
rejection string. Confirm `UE_ROOT` resolves to the source build (not the Launcher path) and
that `Setup.bat` completed successfully.

---

## After the Build Gate Passes

Once `AbyssLockServer.exe` exists, continue with the steps in
`docs/windows-dedicated-server-runbook.md`:

1. Create the local server config:

   ```powershell
   New-Item -ItemType Directory -Force Saved\Config | Out-Null
   Copy-Item Tools\ops\server_config.example.json Saved\Config\server_config.local.json
   ```

2. Validate the config:

   ```powershell
   cargo run -p frostwake-tools -- server-config-check Saved\Config\server_config.local.json
   ```

3. Run the dedicated server boot probe:

   ```powershell
   .\Tools\windows\run_dedicated_server_validation.ps1
   ```

4. Run the dedicated client join probe:

   ```powershell
   .\Tools\windows\run_dedicated_client_join_validation.ps1
   ```

5. Run the 8-player ready-lobby probe:

   ```powershell
   .\Tools\windows\run_dedicated_ready_validation.ps1
   ```

6. Run the Phase 2 entry orchestration wrapper:

   ```powershell
   .\Tools\windows\run_phase2_entry_validation.ps1 -SkipGenerate
   ```

   A passing run writes `Saved\Phase2EntryValidation\<timestamp>\manifest.json` with
   `decision=pass`.

Record all evidence per the standard in `docs/subprojects/sp-02-windows-dedicated-server-foundation.md`
§Evidence Standard: dated doc, command, platform, `UE_ROOT` value, build target, log path,
JSONL event path, player count, pass/fail status.

---

## Notes

- The Launcher UE limitation is not a project code issue. All Frostwake C++ and scripts are
  already correct for server-target builds; only the engine distribution needs to change.
- The listen-server path (Editor/Game builds, `run-local-smoke`, `ready8`) continues to work
  with the Launcher UE and must remain covered while this blocker stands (charter boundary).
- Do not start Steam server browser or SDR implementation before `AbyssLockServer.exe` is
  verified (charter boundary from `docs/subprojects/sp-02-windows-dedicated-server-foundation.md`).
- GP-04 (Steam lobby implementation) is also gated behind this lane's M1 unblock.
