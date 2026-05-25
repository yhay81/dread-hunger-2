# Windows Tools

These helpers are for the Windows-only development path. Run them from a fresh Windows clone after Git LFS, Python, Unreal Engine, and Visual Studio are installed.

## First Run

From the repository root:

```powershell
.\Tools\windows\check_prereqs.ps1
.\Tools\windows\run_first_validation.ps1 -SkipGenerate
```

If Unreal is not installed in a default Epic path, set `UE_ROOT` first:

```powershell
$env:UE_ROOT = "D:\Epic Games\UE_5.7"
.\Tools\windows\run_first_validation.ps1 -UeRoot $env:UE_ROOT -SkipGenerate
```

## Optional Smoke

After Editor, Game, and Server validation are understood:

```powershell
.\Tools\windows\run_first_validation.ps1 -SkipGenerate -IncludeSmoke
```

Use `-IncludeHeavySmoke` only after the quick smoke path is stable.

## Output

Validation output is written under ignored `Saved\WindowsValidation\`. Copy the key pass/fail lines into `docs/windows-validation-template.md` or a new cycle record. Do not commit generated logs.
