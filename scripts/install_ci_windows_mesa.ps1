# Windows CI-only Mesa llvmpipe WGL software renderer, pinned by SHA256.
# Never install globally or copy into production source/install directories.
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('install','remove')]
    [string]$Action,
    [string]$TargetBin = ''
)

$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_OS -ne 'Windows') {
    throw 'Mesa staging is restricted to ephemeral Windows GitHub Actions.'
}
# During Phase D, stage beside the build GUI; during Phase E, beside an
# explicitly supplied *disposable* relocated install under RUNNER_TEMP.
$defaultBin = [IO.Path]::GetFullPath((Join-Path $env:GITHUB_WORKSPACE 'build\bin\Release'))
$target = if ($TargetBin) { [IO.Path]::GetFullPath($TargetBin) } else { $defaultBin }
$allowedTemp = [IO.Path]::GetFullPath($env:RUNNER_TEMP).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
if (-not [string]::Equals($target, $defaultBin, [StringComparison]::OrdinalIgnoreCase) -and
    -not $target.StartsWith($allowedTemp, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing Mesa staging outside build output or runner temp: $target"
}
$marker = Join-Path $env:RUNNER_TEMP 'tonatiuhpp_ci_mesa_staged_files.txt'
if ($Action -eq 'remove') {
    if (Test-Path $marker) {
        foreach ($file in (Get-Content -LiteralPath $marker)) {
            if (-not $file.StartsWith($target + [IO.Path]::DirectorySeparatorChar,
                                      [StringComparison]::OrdinalIgnoreCase)) {
                throw "Refusing Mesa cleanup outside build bin: $file"
            }
            if (Test-Path -LiteralPath $file) {
                Remove-Item -LiteralPath $file -Force
                Write-Host "Removed CI-only Mesa DLL $file"
            }
        }
        Remove-Item -LiteralPath $marker -Force
    }
    return
}

# The exact Mesa MSVC release below is proven to contain a functional
# 64-bit WGL opengl32.dll; newer Mesa 26.2.6 MSVC packaging omits it.
# Pin version, archive name and digest together; do not silently fall back
# to a different toolchain or software renderer.
$version = '25.1.9'
$name = "mesa3d-$version-release-msvc.7z"
$url = "https://github.com/pal1000/mesa-dist-win/releases/download/$version/$name"
$expectedSha256 = 'dd83c3c4cc4d7706c9e21b277388447a2fc5367b0912f9ee45621e36aeb3b7d2'
$archive = Join-Path $env:RUNNER_TEMP $name
$extract = Join-Path $env:RUNNER_TEMP 'tonatiuhpp-ci-mesa-25.1.9-msvc'

if (-not (Test-Path -LiteralPath $target -PathType Container)) {
    throw "Release GUI output directory absent: $target"
}
if (Test-Path -LiteralPath $marker) {
    throw 'Mesa staging marker already exists; refusing to replace staged files.'
}
$sevenZip = Get-Command 7z.exe -ErrorAction SilentlyContinue
if ($sevenZip) { $exe = $sevenZip.Source }
elseif (Test-Path 'C:\Program Files\7-Zip\7z.exe') {
    $exe = 'C:\Program Files\7-Zip\7z.exe'
} else {
    throw '7-Zip is required to unpack pinned Mesa on hosted Windows.'
}

Write-Host "Downloading pinned Mesa $name"
Invoke-WebRequest -Uri $url -OutFile $archive -MaximumRedirection 5
$actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToLowerInvariant()
if ($actual -ne $expectedSha256) {
    throw "SHA256 mismatch for Mesa ($name): $actual"
}
Write-Host "Verified Mesa $name SHA256: $actual"
New-Item -ItemType Directory -Path $extract -Force | Out-Null
& $exe x $archive "-o$extract" -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Verified Mesa archive extraction failed' }
$dir = Join-Path $extract 'x64'
$opengl = Join-Path $dir 'opengl32.dll'
$gallium = Join-Path $dir 'libgallium_wgl.dll'
$software = Join-Path $dir 'pipe_swrast.dll'
foreach ($dll in @($opengl, $gallium, $software)) {
    if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) {
        throw "Required Mesa software WGL file missing: $dll"
    }
}
# Verify the actual DLL's PE Machine field: IMAGE_FILE_MACHINE_AMD64=0x8664.
# This avoids accidentally choosing an x86 OpenGL library when archive
# directory names change.
function IsAmd64Dll([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try {
        $reader = [IO.BinaryReader]::new($stream)
        $stream.Position = 0x3c
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0x40 -or $peOffset -gt ($stream.Length - 6)) { return $false }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { return $false }
        return $reader.ReadUInt16() -eq 0x8664
    } finally { $stream.Dispose() }
}
foreach ($filePath in @($opengl, $gallium, $software)) {
    if (-not (IsAmd64Dll $filePath)) {
        throw "Mesa runtime is not AMD64: $filePath"
    }
}
$copyFiles = @(Get-ChildItem -LiteralPath $dir -Filter '*.dll' -File)
foreach ($item in $copyFiles) {
    $dest = Join-Path $target $item.Name
    if (Test-Path -LiteralPath $dest) {
        throw "Refusing to overwrite existing runtime: $dest"
    }
}
$staged = @()
try {
    foreach ($item in $copyFiles) {
        $dest = Join-Path $target $item.Name
        Copy-Item -LiteralPath $item.FullName -Destination $dest -ErrorAction Stop
        $staged += $dest
        Write-Host "Staged Mesa software WGL library: $dest"
    }
    Set-Content -LiteralPath $marker -Value $staged -Encoding ascii
} catch {
    foreach ($path in $staged) { Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue }
    throw
}
Write-Host "Mesa $name software WGL is staged only for native GUI CI."
