# Requires Windows PowerShell 5.1+ or PowerShell 7+; does not change or commit a Git repository.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BaselineExe,
    [Parameter(Mandatory = $true)][string]$CandidateExe,
    [Parameter(Mandatory = $true)][string]$BaselineScene,
    [Parameter(Mandatory = $true)][string]$CandidateScene,
    [string]$Repo = (Get-Location).Path,
    [long]$Rays = 5000000,
    [int]$AbbaBlocks = 2,
    [double]$ThresholdPercent = 2.0,
    [switch]$Warmup,
    [int]$WarmupPairs = 2,
    [double]$StabilityLimitPercent = 3.0,
    [double]$AgreementLimitPercentagePoints = 2.0,
    [switch]$BaselineLegacy,
    [switch]$CandidateLegacy,
    [string]$OutputDir = ""
)
$ErrorActionPreference = 'Stop'
if ($Rays -le 0 -or $AbbaBlocks -le 0 -or $ThresholdPercent -le 0 -or $WarmupPairs -lt 0 -or $StabilityLimitPercent -le 0 -or $AgreementLimitPercentagePoints -le 0) {
    throw 'Rays, AbbaBlocks and ThresholdPercent must be positive.'
}
foreach ($path in @($BaselineExe, $CandidateExe, $BaselineScene, $CandidateScene)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$runner = Join-Path $scriptDir 'compare_working_tree.py'
if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) { throw "Python harness missing: $runner" }
$venvPython = Join-Path $Repo '.venv\Scripts\python.exe'
if (Test-Path -LiteralPath $venvPython -PathType Leaf) {
    $pythonExe = $venvPython
} else {
    $pythonCommand = Get-Command python.exe -ErrorAction SilentlyContinue
    if (-not $pythonCommand) { throw 'Python not found: use the Tonatiuh++ .venv or put python.exe in PATH.' }
    $pythonExe = $pythonCommand.Source
}
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
    $OutputDir = Join-Path $env:USERPROFILE "TonatiuhPP-Performance-Comparisons\$stamp"
}
if (Test-Path -LiteralPath $OutputDir) { throw "Output directory already exists: $OutputDir" }
$parent = Split-Path -Parent $OutputDir
if (-not (Test-Path -LiteralPath $parent)) {
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
}
$arguments = @(
    $runner,
    '--baseline-exe', (Resolve-Path -LiteralPath $BaselineExe).Path,
    '--candidate-exe', (Resolve-Path -LiteralPath $CandidateExe).Path,
    '--baseline-scene', (Resolve-Path -LiteralPath $BaselineScene).Path,
    '--candidate-scene', (Resolve-Path -LiteralPath $CandidateScene).Path,
    '--repo', $Repo,
    '--rays', [string]$Rays,
    '--abba-blocks', [string]$AbbaBlocks,
    '--warmup-pairs', [string]$WarmupPairs,
    '--stability-limit-percent', $StabilityLimitPercent.ToString([System.Globalization.CultureInfo]::InvariantCulture),
    '--agreement-limit-percentage-points', $AgreementLimitPercentagePoints.ToString([System.Globalization.CultureInfo]::InvariantCulture),
    '--threshold-percent', $ThresholdPercent.ToString([System.Globalization.CultureInfo]::InvariantCulture),
    '--output-dir', $OutputDir
)
if ($Warmup) { $arguments += '--warmup' }
if ($BaselineLegacy) { $arguments += '--baseline-legacy' }
if ($CandidateLegacy) { $arguments += '--candidate-legacy' }
Write-Host "Performance results directory: $OutputDir"
& $pythonExe @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Performance comparison returned exit code $LASTEXITCODE. Inspect $OutputDir"
}
