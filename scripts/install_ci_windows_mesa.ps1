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

# Try the previously established MSVC release first. The inspected
# 26.2.6 MSVC archive lacks the OpenGL WGL DLLs altogether, so do not use
# it. Use the current MinGW build as a SHA-pinned fallback if necessary.
$packages = @(
    @{ version = '25.1.9'; toolchain = 'msvc';
       sha256 = 'dd83c3c4cc4d7706c9e21b277388447a2fc5367b0912f9ee45621e36aeb3b7d2' },
    @{ version = '26.2.6'; toolchain = 'mingw';
       sha256 = 'da0f71d9181b672c1918f3c342f11118c687f6059a46b16502845160733cc388' }
)
if (-not (Test-Path $target -PathType Container)) {
    throw "Release GUI output directory absent: $target"
}
if (Test-Path $marker) {
    throw 'Mesa staging marker already exists; refusing to replace previous staged files.'
}
$sevenZip = (Get-Command 7z.exe -ErrorAction SilentlyContinue)
if ($sevenZip) { $exe = $sevenZip.Source }
elseif (Test-Path 'C:\Program Files\7-Zip\7z.exe') { $exe = 'C:\Program Files\7-Zip\7z.exe' }
else { throw '7-Zip required on hosted Windows to unpack pinned Mesa.' }
# Some upstream release archives wrap the architecture payload in a
# second archive. List and unpack only one nested level, preserving evidence
# when the distribution layout changes. Never assume a folder called "x64".
function DriverFiles([string]$directory) {
    return @(Get-ChildItem -LiteralPath $directory -Recurse -File -ErrorAction Stop |
        Where-Object { $_.Name -ieq 'opengl32.dll' })
}
$ogl = @()
$chosen = ''
foreach ($package in $packages) {
    $fileName = 'mesa3d-' + $package.version + '-release-' + $package.toolchain + '.7z'
    $uri = "https://github.com/pal1000/mesa-dist-win/releases/download/$($package.version)/$fileName"
    $archive = Join-Path $env:RUNNER_TEMP $fileName
    $extract = Join-Path $env:RUNNER_TEMP ('tonatiuhpp-ci-mesa-' + $package.version + '-' + $package.toolchain)
    Write-Host "Downloading pinned Mesa package $uri"
    Invoke-WebRequest -Uri $uri -OutFile $archive -MaximumRedirection 5
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToLowerInvariant()
    if ($actual -ne $package.sha256) {
        throw "Mesa3D SHA256 mismatch ($fileName): $actual"
    }
    Write-Host "Verified SHA256 ($fileName): $actual"
    New-Item -ItemType Directory -Force -Path $extract | Out-Null
    & $exe x $archive "-o$extract" -y | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Extraction failed for verified $fileName" }
    $ogl = @(DriverFiles $extract)
    if ($ogl.Count -eq 0) {
        $nested = @(Get-ChildItem -LiteralPath $extract -Recurse -File |
            Where-Object { $_.Extension -in @('.7z','.zip') })
        foreach ($item in $nested) {
            $targetNested = Join-Path $extract ('nested-' + $item.BaseName)
            New-Item -ItemType Directory -Force -Path $targetNested | Out-Null
            & $exe x $item.FullName "-o$targetNested" -y | Out-Null
            if ($LASTEXITCODE -ne 0) { throw "Extraction failed for nested $($item.Name)" }
        }
        $ogl = @(DriverFiles $extract)
    }
    if ($ogl.Count -eq 0) {
        Write-Host "No WGL opengl32.dll in $fileName (package inventory):"
        Get-ChildItem -LiteralPath $extract -Recurse -File | Select-Object -First 40 |
            ForEach-Object { Write-Host $_.FullName }
        continue
    }
    $chosen = $fileName
    break
}
if ($ogl.Count -eq 0) {
    throw 'No OpenGL WGL runtime in either SHA-verified Mesa package.'
}
Write-Host "Selected Mesa package: $chosen"
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
Write-Host ('Mesa llvmpipe WGL is staged only for native GUI CI: ' + $chosen)
