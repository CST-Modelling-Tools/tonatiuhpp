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

$ogl = @(Get-ChildItem -LiteralPath $extract -File -Recurse -Filter opengl32.dll |
    Where-Object { $_.FullName -match '(?i)(\\|/)x64(\\|/)' })
if ($ogl.Count -ne 1) {
    throw "Expected exactly one Mesa x64 opengl32.dll, found $($ogl.Count): $($ogl.FullName)"
}
$dir = $ogl[0].DirectoryName
$gallium = Join-Path $dir 'libgallium_wgl.dll'
if (-not (Test-Path -LiteralPath $gallium -PathType Leaf)) {
    throw "Mesa WGL megadriver not found next to $($ogl[0].FullName)"
}
$copyFiles = @($ogl[0], (Get-Item -LiteralPath $gallium))
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
