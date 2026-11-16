[CmdletBinding()]
param(
    [string]$BaselineBuild = 'C:\Users\mblanco\TonatiuhPP-Performance-Baselines\build-e2914aca7843',
    [string]$CandidateBuild = 'C:\OpenSource\tonatiuhpp\build',
    [string]$OutputDir = '',
    [switch]$ProbeOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Find-Dumpbin {
    $command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $candidates = @(Get-ChildItem 'C:\Program Files\Microsoft Visual Studio' -Filter dumpbin.exe -File -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\Hostx64\\x64\\' })
    $candidates += @(Get-ChildItem 'C:\Program Files (x86)\Microsoft Visual Studio' -Filter dumpbin.exe -File -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\Hostx64\\x64\\' })
    if ($candidates.Count -eq 0) { throw 'MSVC dumpbin.exe not found; use a Visual Studio Developer PowerShell.' }
    return ($candidates | Sort-Object FullName -Descending | Select-Object -First 1).FullName
}

$dumpbin = Find-Dumpbin
Write-Host "Using dumpbin: $dumpbin"
if ($ProbeOnly) { Write-Host 'PASS: dumpbin available. No disassembly generated.'; return }
if (-not $OutputDir) {
    $OutputDir = Join-Path $env:USERPROFILE ('TonatiuhPP-MachineCode-Comparisons\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
if (Test-Path -LiteralPath $OutputDir) { throw "Refusing existing output directory: $OutputDir" }
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
$manifest = [ordered]@{ schema = 'tonatiuhpp.machine-code-inspection.v1'; dumpbin = $dumpbin; builds = @{} }
foreach ($item in @(@{ Label = 'baseline'; Build = $BaselineBuild }, @{Label = 'candidate'; Build = $CandidateBuild})) {
    $label = $item.Label
    $build = $item.Build
    $dll = Join-Path $build 'bin\Release\TonatiuhppKernel.dll'
    if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) { throw "Missing kernel DLL: $dll" }
    # Several targets can compile RayTracer.cpp in a development build.
    # Only the TonatiuhppKernel Release object is the relevant scientific DLL code.
    $obj = @(Get-ChildItem -LiteralPath $build -Filter RayTracer.obj -File -Recurse -ErrorAction Stop |
        Where-Object { $_.FullName -match '[\\/]TonatiuhppKernel\.dir[\\/]Release[\\/]RayTracer\.obj$' })
    if ($obj.Count -ne 1) {
        $matches = @(Get-ChildItem -LiteralPath $build -Filter RayTracer.obj -File -Recurse -ErrorAction Stop |
            ForEach-Object { $_.FullName })
        $detail = if ($matches.Count -eq 0) { '(none)' } else { $matches -join [Environment]::NewLine }
        throw "Expected one TonatiuhppKernel Release RayTracer.obj under $build; found $($obj.Count). All RayTracer.obj paths:$([Environment]::NewLine)$detail"
    }
    $objFile = $obj[0].FullName
    $out = Join-Path $OutputDir $label
    New-Item -ItemType Directory -Path $out | Out-Null
    $symbols = Join-Path $out 'RayTracer-symbols.txt'
    $assembly = Join-Path $out 'RayTracer-disassembly.txt'
    $exports = Join-Path $out 'TonatiuhppKernel-exports.txt'
    & $dumpbin /SYMBOLS "/OUT:$symbols" $objFile
    if ($LASTEXITCODE -ne 0) { throw "dumpbin /SYMBOLS failed for $label" }
    & $dumpbin /DISASM "/OUT:$assembly" $objFile
    if ($LASTEXITCODE -ne 0) { throw "dumpbin /DISASM failed for $label" }
    & $dumpbin /EXPORTS "/OUT:$exports" $dll
    if ($LASTEXITCODE -ne 0) { throw "dumpbin /EXPORTS failed for $label" }
    $symbolMatches = @(Select-String -LiteralPath $symbols -Pattern 'RayTracer.*(traceRays|operator|NewPrimitiveRay)' | Select-Object -First 40)
    $symbolMatches | ForEach-Object { $_.Line } | Set-Content -LiteralPath (Join-Path $out 'relevant-symbols.txt') -Encoding UTF8
    $manifest.builds[$label] = [ordered]@{
        kernel_dll = $dll; kernel_sha256 = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash
        raytracer_object = $objFile; object_sha256 = (Get-FileHash -LiteralPath $objFile -Algorithm SHA256).Hash
        object_bytes = (Get-Item -LiteralPath $objFile).Length
        symbols_file = $symbols; disassembly_file = $assembly; exports_file = $exports
        relevant_symbol_lines = $symbolMatches.Count
    }
    Write-Host "PASS: $label disassembled ($($symbolMatches.Count) relevant symbol lines)."
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir 'manifest.json') -Encoding UTF8
Write-Host "Machine-code inspection saved to: $OutputDir"
Write-Host 'Interpretation requires care: object-level code size and textual disassembly are not a runtime speed measurement.'
