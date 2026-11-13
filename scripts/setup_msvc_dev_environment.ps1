# Initialize the MSVC x64 developer environment for subsequent GitHub Actions steps.
# Uses the Visual Studio installation already supplied by the Windows runner.
$ErrorActionPreference = "Stop"

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio locator not found: $vswhere"
}

$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vsInstall) {
    throw "Visual Studio with MSVC x64 tools was not found."
}

$vsDevCmd = Join-Path $vsInstall "Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path -LiteralPath $vsDevCmd)) {
    throw "Visual Studio developer command file not found: $vsDevCmd"
}

# A batch file can modify only its own process environment. Capture the
# resulting variables and propagate them to subsequent Actions steps.
$batch = Join-Path $env:RUNNER_TEMP "tonatiuhpp-msvc-dev-env.cmd"
@"
@echo off
call "$vsDevCmd" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 exit /b %errorlevel%
set
"@ | Set-Content -LiteralPath $batch -Encoding ascii

$before = [Environment]::GetEnvironmentVariables()
$output = & cmd.exe /d /c $batch
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio developer environment initialization failed ($LASTEXITCODE)."
}

$written = 0
foreach ($line in $output) {
    # cmd.exe also prints pseudo-variables such as =C:=...; omit these.
    if ($line -notmatch '^([^=]+)=(.*)$') { continue }
    $name = $matches[1]
    $value = $matches[2]
    if ($name -match '^(GITHUB_|RUNNER_|ACTIONS_)') { continue }
    if ($before.Contains($name) -and [string]$before[$name] -ceq $value) { continue }
    # Visual Studio environment values are single-line; fail closed otherwise.
    if ($value.Contains("`n") -or $value.Contains("`r")) {
        throw "Unexpected newline in MSVC environment variable $name."
    }
    "$name=$value" | Out-File -LiteralPath $env:GITHUB_ENV -Append -Encoding utf8
    $written++
}

if ($written -eq 0) { throw "MSVC developer environment yielded no changed variables." }
Write-Host "Configured MSVC x64 toolchain from $vsInstall ($written environment updates)."
