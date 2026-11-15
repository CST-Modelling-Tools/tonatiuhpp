# Tonatiuh++ v3 diagnostics wrapper: native Windows Performance Monitor logging.
# Never changes C++ code, benchmark sources, or the underlying harness.
[CmdletBinding()]
param(
    [string]$Repo = 'C:\OpenSource\tonatiuhpp',
    [string]$BaselineSource = 'C:\Users\mblanco\TonatiuhPP-Performance-Baselines\e2914aca7843',
    [string]$BaselineBuild = 'C:\Users\mblanco\TonatiuhPP-Performance-Baselines\build-e2914aca7843',
    [long]$Rays = 5000000,
    [int]$WarmupPairs = 2,
    [int]$AbbaBlocks = 2,
    [double]$ThresholdPercent = 2.0,
    [double]$StabilityLimitPercent = 3.0,
    [double]$AgreementLimitPercentagePoints = 2.0,
    [string]$OutputDir = '',
    [string[]]$CounterPaths = @(),
    [switch]$ProbeOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($env:OS -ne 'Windows_NT') { throw 'This script requires Windows.' }
foreach ($name in @('logman.exe','Get-Counter')) {
    if ($name -eq 'Get-Counter') {
        if (-not (Get-Command Get-Counter -ErrorAction SilentlyContinue)) { throw 'Get-Counter is unavailable. Use Windows PowerShell 5.1 or a PowerShell installation with the Microsoft.PowerShell.Diagnostics module.' }
    } elseif (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
        throw "Required native Windows tool not found: $name"
    }
}

function Get-AvailableCounterPaths {
    param([string[]]$Requested)
    if ($Requested.Count -gt 0) {
        return @($Requested | Sort-Object -Unique)
    }
    $sets = @(Get-Counter -ListSet '*' -ErrorAction Stop)
    $desired = @(
        '(?i)^% Processor Time$|^% tiempo de procesador$',
        '(?i)^% Processor Utility$|^% utilidad del procesador$',
        '(?i)^% Processor Performance$|^% rendimiento del procesador$',
        '(?i)^% of Maximum Frequency$|^% de frecuencia m[aá]xima$',
        '(?i)^Processor Frequency$|^Frecuencia del procesador$',
        '(?i)^% Privileged Time$|^% tiempo privilegiado$'
    )
    $selected = New-Object 'System.Collections.Generic.List[string]'
    foreach ($set in $sets) {
        $group = [string]$set.CounterSetName
        if ($group -notmatch '(?i)^(Processor Information|Processor|Informaci[oó]n del procesador|Procesador)$') { continue }
        if ($group -match '(?i)^Processor$|^Procesador$' -and
            @($sets | Where-Object { $_.CounterSetName -match '(?i)^(Processor Information|Informaci[oó]n del procesador)$' }).Count -gt 0) {
            continue # Prefer topology-aware Processor Information over legacy Processor.
        }
        foreach ($path in @($set.Paths)) {
            if ($path -notmatch '\(\*\)\\([^\\]+)$') { continue }
            $counter = $Matches[1]
            foreach ($pattern in $desired) {
                if ($counter -match $pattern) {
                    if (-not $selected.Contains([string]$path)) { $selected.Add([string]$path) }
                    break
                }
            }
        }
    }
    foreach ($set in $sets) {
        if ([string]$set.CounterSetName -notmatch '(?i)^System$|^Sistema$') { continue }
        foreach ($path in @($set.Paths)) {
            if ($path -match '(?i)\\(Processor Queue Length|Longitud de la cola del procesador)$' -and -not $selected.Contains([string]$path)) {
                $selected.Add([string]$path)
            }
        }
    }
    return @($selected.ToArray())
}

# Probe discovered paths individually. Some counters are advertised but unavailable
# on particular hardware or under restricted accounts.
$requested = @(Get-AvailableCounterPaths -Requested $CounterPaths)
$working = New-Object 'System.Collections.Generic.List[string]'
$unavailable = New-Object 'System.Collections.Generic.List[string]'
foreach ($path in $requested) {
    try {
        $null = Get-Counter -Counter $path -MaxSamples 1 -ErrorAction Stop
        $working.Add($path)
    } catch {
        $unavailable.Add("${path}: $($_.Exception.Message)")
    }
}
Write-Host "Windows counter paths verified: $($working.Count)"
foreach ($path in $working) { Write-Host "  $path" }
foreach ($errorPath in $unavailable) { Write-Warning "Unavailable: $errorPath" }
if ($working.Count -eq 0) {
    throw 'No usable counters found. In Windows PowerShell, inspect Get-Counter -ListSet *; then pass explicit -CounterPaths to this script.'
}
if ($ProbeOnly) {
    Write-Host 'PASS: Counter discovery completed. No benchmark or collector started.' -ForegroundColor Green
    return
}

$baselineExe = Join-Path $BaselineBuild 'bin\Release\tonatiuhpp-cli.exe'
$candidateExe = Join-Path $Repo 'build\bin\Release\tonatiuhpp-cli.exe'
$baselineScene = Join-Path $BaselineSource 'examples\benchmarks\benchmark_heliostat_field_v1.tnhpp'
$candidateScene = Join-Path $Repo 'examples\benchmarks\benchmark_heliostat_field_v1.tnhpp'
$harness = Join-Path $Repo 'scripts\performance\Compare-TonatiuhPP-Performance.ps1'
foreach ($file in @($baselineExe,$candidateExe,$baselineScene,$candidateScene,$harness)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing required file: $file" }
}
if ((Get-FileHash -LiteralPath $baselineScene -Algorithm SHA256).Hash -ne
    (Get-FileHash -LiteralPath $candidateScene -Algorithm SHA256).Hash) {
    throw 'Benchmark scene SHA-256 mismatch. Benchmark cancelled.'
}
if ($Rays -le 0 -or $WarmupPairs -lt 0 -or $AbbaBlocks -lt 1) { throw 'Invalid rays or repetition counts.' }
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $env:USERPROFILE ('TonatiuhPP-Performance-Comparisons\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$monitorDir = $OutputDir + '-windows-monitor'
if ((Test-Path -LiteralPath $OutputDir) -or (Test-Path -LiteralPath $monitorDir)) {
    throw 'Refusing to reuse an existing comparison or monitoring directory.'
}
$null = New-Item -ItemType Directory -Path $monitorDir
$collector = 'TonatiuhPP-' + $PID + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$csvPath = Join-Path $monitorDir 'hardware-counters.csv'
$started = $false
$created = $false
$comparisonError = $null
$collectorErrors = New-Object 'System.Collections.Generic.List[string]'
$beginUtc = [DateTime]::UtcNow
try {
    $createArgs = @('create','counter',$collector,'-f','csv','-si','00:00:01','-o',$csvPath,'-c') + $working.ToArray()
    & logman.exe @createArgs | Out-File -LiteralPath (Join-Path $monitorDir 'logman-create.txt') -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw "logman create failed with code $LASTEXITCODE; see logman-create.txt. Try a PowerShell window with administrator privileges if access was denied." }
    $created = $true
    & logman.exe start $collector | Out-File -LiteralPath (Join-Path $monitorDir 'logman-start.txt') -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw "logman start failed with code $LASTEXITCODE; see logman-start.txt." }
    $started = $true
    $beginUtc = [DateTime]::UtcNow
    Write-Host "Collector started: $collector"
    Write-Host "Performance result: $OutputDir"
    Write-Host "Hardware counters: $monitorDir"
    & $harness -Repo $Repo -BaselineExe $baselineExe -CandidateExe $candidateExe `
        -BaselineScene $baselineScene -CandidateScene $candidateScene `
        -Rays $Rays -WarmupPairs $WarmupPairs -AbbaBlocks $AbbaBlocks `
        -ThresholdPercent $ThresholdPercent -StabilityLimitPercent $StabilityLimitPercent `
        -AgreementLimitPercentagePoints $AgreementLimitPercentagePoints -OutputDir $OutputDir
} catch {
    $comparisonError = $_
} finally {
    $endUtc = [DateTime]::UtcNow
    if ($started) {
        try {
            & logman.exe stop $collector | Out-File -LiteralPath (Join-Path $monitorDir 'logman-stop.txt') -Encoding utf8
            if ($LASTEXITCODE -ne 0) { $collectorErrors.Add("logman stop returned $LASTEXITCODE") }
        } catch { $collectorErrors.Add("logman stop: $($_.Exception.Message)") }
    }
    if ($created) {
        try {
            & logman.exe delete $collector | Out-File -LiteralPath (Join-Path $monitorDir 'logman-delete.txt') -Encoding utf8
            if ($LASTEXITCODE -ne 0) { $collectorErrors.Add("logman delete returned $LASTEXITCODE") }
        } catch { $collectorErrors.Add("logman delete: $($_.Exception.Message)") }
    }
    # Exact subprocess boundaries from instrumented Python harness; never
    # manufacture approximated NTFS timestamps as if they were process events.
    $eventFile = Join-Path $OutputDir 'run-events.jsonl'
    $eventBoundaryStatus = if (Test-Path -LiteralPath $eventFile -PathType Leaf) {
        'exact subprocess call boundaries recorded by harness'
    } else {
        'missing; exact time-window correlation unavailable'
    }
    $coverageScript = Join-Path $PSScriptRoot 'analyze_monitor_coverage.py'
    $coverageStatus = 'not attempted'
    if ((Test-Path -LiteralPath $eventFile -PathType Leaf) -and
        (Test-Path -LiteralPath $coverageScript -PathType Leaf)) {
        try {
            $pythonCommand = Get-Command python.exe -ErrorAction SilentlyContinue
            if (-not $pythonCommand) { throw 'python.exe not available for coverage analyzer' }
            & $pythonCommand.Source $coverageScript --monitor-dir $monitorDir --comparison-dir $OutputDir
            if ($LASTEXITCODE -ne 0) { throw "coverage analyzer exit $LASTEXITCODE" }
            $coverageStatus = 'created sample-coverage.md and sample-coverage.json'
        } catch {
            $coverageStatus = "unavailable: $($_.Exception.Message)"
            Write-Warning $coverageStatus
        }
    }
    $meta = [ordered]@{
        schema = 'tonatiuhpp.windows-counter-monitor.v2'
        collector = $collector
        counter_log_directory = $monitorDir
        comparison_directory = $OutputDir
        collection_start_utc = $beginUtc.ToString('o')
        collection_stop_utc = $endUtc.ToString('o')
        sample_interval_seconds = 1
        selected_counters = @($working.ToArray())
        unavailable_counters = @($unavailable.ToArray())
        run_boundaries = $eventBoundaryStatus
        coverage_status = $coverageStatus
        caveat = 'Counters report OS-visible utilization/performance, not necessarily true per-core effective clocks, CPU package temperature or thermal throttling.'
        collector_errors = @($collectorErrors.ToArray())
        comparison_error = if ($null -ne $comparisonError) { $comparisonError.Exception.Message } else { $null }
    }
    $meta | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $monitorDir 'monitor-metadata.json') -Encoding UTF8
    Write-Host "Monitoring saved to: $monitorDir"
}
if ($collectorErrors.Count -gt 0) {
    foreach ($err in $collectorErrors) { Write-Warning $err }
}
if ($null -ne $comparisonError) { throw $comparisonError }
Write-Host 'PASS: Comparison and hardware monitoring completed.' -ForegroundColor Green
