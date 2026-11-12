# Windows CI-only Mesa llvmpipe WGL software renderer, pinned by SHA256.
# Never install globally or copy into production source/install directories.
param([Parameter(Mandatory=$true)][ValidateSet('install','remove')][string]$Action)

$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_OS -ne 'Windows') {
    throw 'Mesa staging is restricted to ephemeral Windows GitHub Actions.'
}
$target = Join-Path $env:GITHUB_WORKSPACE 'build\bin\Release'
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

$uri = 'https://github.com/pal1000/mesa-dist-win/releases/download/26.2.6/mesa3d-26.2.6-release-msvc.7z'
$expected = 'b2e00615feaba45942343c265cc8c5f576142ca682c1ac6661922d4b72492877'
$archive = Join-Path $env:RUNNER_TEMP 'tonatiuhpp-mesa-msvc-26.2.6.7z'
$extract = Join-Path $env:RUNNER_TEMP 'tonatiuhpp-mesa-msvc-26.2.6'
if (-not (Test-Path $target -PathType Container)) {
    throw "Release GUI output directory absent: $target"
}
if (Test-Path $marker) {
    throw 'Mesa staging marker already exists; refusing to replace previous staged files.'
}
Write-Host "Downloading SHA-pinned Mesa3D 26.2.6 MSVC from $uri"
Invoke-WebRequest -Uri $uri -OutFile $archive -MaximumRedirection 5
$actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToLowerInvariant()
if ($actual -ne $expected) {
    throw "Mesa3D SHA256 mismatch: $actual"
}
Write-Host "Verified Mesa3D SHA256: $actual"
$sevenZip = (Get-Command 7z.exe -ErrorAction SilentlyContinue)
if ($sevenZip) { $exe = $sevenZip.Source }
elseif (Test-Path 'C:\Program Files\7-Zip\7z.exe') {
    $exe = 'C:\Program Files\7-Zip\7z.exe'
} else {
    throw '7-Zip is required on the Windows CI runner to unpack pinned Mesa.'
}
New-Item -ItemType Directory -Force -Path $extract | Out-Null
& $exe x $archive "-o$extract" -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Failed to extract verified Mesa3D archive' }

# Some upstream release archives wrap the architecture payload in a
# second archive. List and unpack only one nested level, preserving evidence
# when the distribution layout changes. Never assume a folder called "x64".
function DriverFiles {
    return @(Get-ChildItem -LiteralPath $extract -Recurse -File -ErrorAction Stop |
        Where-Object { $_.Name -ieq 'opengl32.dll' })
}
$ogl = @(DriverFiles)
if ($ogl.Count -eq 0) {
    $nested = @(Get-ChildItem -LiteralPath $extract -Recurse -File |
        Where-Object { $_.Extension -in @('.7z', '.zip') })
    foreach ($item in $nested) {
        $targetNested = Join-Path $extract ('nested-' + $item.BaseName)
        New-Item -ItemType Directory -Force -Path $targetNested | Out-Null
        & $exe x $item.FullName "-o$targetNested" -y | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "Unable to expand nested Mesa archive $($item.Name)" }
    }
    $ogl = @(DriverFiles)
}
if ($ogl.Count -eq 0) {
    Write-Host "Mesa driver archive inventory (first 100 files):"
    Get-ChildItem -LiteralPath $extract -File -Recurse | Select-Object -First 100 |
        ForEach-Object { Write-Host $_.FullName }
    throw 'No OpenGL32.dll found in SHA-verified Mesa release archive.'
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
$amd64 = @($ogl | Where-Object { IsAmd64Dll $_.FullName })
Write-Host ("Mesa OpenGL candidates: " + (($ogl | ForEach-Object { $_.FullName }) -join '; '))
if ($amd64.Count -ne 1) {
    throw "Expected exactly one AMD64 Mesa opengl32.dll, found $($amd64.Count)."
}
$dir = $amd64[0].DirectoryName
$gallium = Join-Path $dir 'libgallium_wgl.dll'
if (-not (Test-Path -LiteralPath $gallium -PathType Leaf)) {
    throw "Mesa WGL megadriver not found next to $($amd64[0].FullName)"
}
if (-not (IsAmd64Dll $gallium)) {
    throw 'Mesa libgallium_wgl.dll is not an AMD64 Windows DLL.'
}
$copyFiles = @($amd64[0], (Get-Item -LiteralPath $gallium))
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
Write-Host 'Mesa llvmpipe WGL is staged only for the native GUI CI test.'
