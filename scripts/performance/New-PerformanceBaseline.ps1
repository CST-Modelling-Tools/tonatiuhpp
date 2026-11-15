# Creates a detached, clean baseline worktree at HEAD without touching the working tree.
# Run from repo root; never commits, resets, cleans, or deletes anything.
[CmdletBinding()]
param(
    [string]$Repo = (Get-Location).Path,
    [string]$BaselineDir = ""
)
$ErrorActionPreference = 'Stop'
$repoRoot = (& git -C $Repo rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or -not $repoRoot) { throw "Not a Git repository: $Repo" }
$sha = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve HEAD.' }
if ([string]::IsNullOrWhiteSpace($BaselineDir)) {
    $BaselineDir = Join-Path $env:USERPROFILE "TonatiuhPP-Performance-Baselines\$($sha.Substring(0,12))"
}
if (Test-Path -LiteralPath $BaselineDir) {
    throw "Baseline directory already exists. Nothing changed: $BaselineDir"
}
$parent = Split-Path -Parent $BaselineDir
if (-not (Test-Path -LiteralPath $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}
& git -C $repoRoot worktree add --detach $BaselineDir HEAD
if ($LASTEXITCODE -ne 0) { throw 'Could not create baseline worktree.' }
Write-Host "BASELINE: $BaselineDir"
Write-Host "HEAD: $sha"
Write-Host 'Build baseline separately, using the SAME toolchain, Qt version, and Release configuration as candidate.'
Write-Host 'Never build either version into the other version source tree.'
