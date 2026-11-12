# Investigate native GUI crashes only on ephemeral GitHub Windows CI.
# Keep WER dumps outside the public build artifact: process memory is private.
param([Parameter(Mandatory=$true)][ValidateSet('before','after')][string]$Phase)

$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_OS -ne 'Windows') {
    throw 'Windows GUI crash triage is restricted to GitHub Actions Windows runners.'
}
$build = Join-Path $env:GITHUB_WORKSPACE 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$diagnosticLog = Join-Path $build 'gui-windows-host-diagnostics.txt'
$dumpsDir = Join-Path $env:RUNNER_TEMP 'tonatiuhpp_gui_wer_dumps'
$werKey = 'HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\tonatiuhpp-gui.exe'
$marker = Join-Path $env:RUNNER_TEMP 'tonatiuhpp_gui_created_wer_key'
function Log([string]$message) {
    $line = "[GUI_WINDOWS_$Phase] $message"
    Write-Host $line
    Add-Content -Path $diagnosticLog -Value $line -Encoding utf8
}
function Attempt([string]$label, [scriptblock]$command) {
    try { & $command } catch { Log ($label + ' failed: ' + $_.Exception.Message) }
}
if ($Phase -eq 'before') {
    Log ('Windows: ' + [Environment]::OSVersion.VersionString)
    Log ("Identity: user=$env:USERNAME; session_name=$env:SESSIONNAME; interactive=$([Environment]::UserInteractive)")
    Log ("PowerShell session_id=$((Get-Process -Id $PID).SessionId)")
    Attempt 'Windows screen probe' {
        Add-Type -AssemblyName System.Windows.Forms
        foreach ($screen in [System.Windows.Forms.Screen]::AllScreens) {
            Log ("Screen: $($screen.DeviceName); bounds=$($screen.Bounds); primary=$($screen.Primary)")
        }
    }
    Attempt 'Window station and desktop probe' {
        Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class TonatiuhWindowsDesktopProbe {
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] public static extern IntPtr GetThreadDesktop(uint threadId);
    [DllImport("user32.dll")] public static extern IntPtr GetProcessWindowStation();
    [DllImport("user32.dll")] public static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool GetUserObjectInformation(
        IntPtr obj, int index, StringBuilder buffer, int length, out int needed);
}
'@
        $desktop = [TonatiuhWindowsDesktopProbe]::GetThreadDesktop(
            [TonatiuhWindowsDesktopProbe]::GetCurrentThreadId())
        $station = [TonatiuhWindowsDesktopProbe]::GetProcessWindowStation()
        foreach ($entry in @(@('window-station', $station), @('thread-desktop', $desktop))) {
            $buffer = New-Object System.Text.StringBuilder(256)
            $needed = 0
            $ok = [TonatiuhWindowsDesktopProbe]::GetUserObjectInformation(
                [IntPtr]$entry[1], 2, $buffer, 512, [ref]$needed)
            Log ("$($entry[0]): $($buffer.ToString()); found=$ok")
        }
        $width = [TonatiuhWindowsDesktopProbe]::GetSystemMetrics(0)
        $height = [TonatiuhWindowsDesktopProbe]::GetSystemMetrics(1)
        Log ("User32 desktop size: " + $width + 'x' + $height)
    }
    Attempt 'GPU adapter probe' {
        Get-CimInstance Win32_VideoController | ForEach-Object {
            Log ("GPU: $($_.Name); driver=$($_.DriverVersion); resolution=$($_.CurrentHorizontalResolution)x$($_.CurrentVerticalResolution)")
        }
    }
    Attempt 'WER minidump setup' {
        New-Item -ItemType Directory -Force -Path $dumpsDir | Out-Null
        if (Test-Path $werKey) {
            Log 'Application WER key already exists; leaving existing policy untouched.'
        } else {
            New-Item -Path $werKey -Force | Out-Null
            New-ItemProperty -Path $werKey -Name DumpFolder -PropertyType ExpandString -Value $dumpsDir -Force | Out-Null
            New-ItemProperty -Path $werKey -Name DumpType -PropertyType DWord -Value 1 -Force | Out-Null
            New-ItemProperty -Path $werKey -Name DumpCount -PropertyType DWord -Value 3 -Force | Out-Null
            Set-Content -Path $marker -Value 'created' -Encoding ascii
            Log 'WER minidumps enabled outside build artifact; dumps will never be uploaded.'
        }
    }
    Log 'Crash evidence collection prepared.'
} else {
    Attempt 'Native Qt acceptance crash stages' {
        $root = Join-Path $build 'tests/gui_native_acceptance'
        Get-ChildItem -Path $root -Recurse -Filter native_report.json -File -ErrorAction SilentlyContinue |
            ForEach-Object {
                $report = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
                Log ("Native mode=$($report.mode) status=$($report.status) stage=$($report.stage) " +
                     "ray_dialog_accepted=$($report.ray_dialog_accepted)")
            }
    }
    Attempt 'Windows Application Error events' {
        $since = (Get-Date).AddMinutes(-25)
        $events = @(Get-WinEvent -FilterHashtable @{
            LogName = 'Application'; Id = @(1000,1001,1002); StartTime = $since
        } -ErrorAction Stop | Where-Object { $_.Message -match '(?i)tonatiuhpp-gui[.]exe' } |
            Select-Object -First 10)
        Log ("Matching WER events: $($events.Count)")
        foreach ($event in $events) {
            $message = $event.Message -replace '\s+', ' '
            Log ("Event $($event.Id) $($event.TimeCreated): " +
                 $message.Substring(0, [Math]::Min(1700, $message.Length)))
        }
    }
    Attempt 'Inspect local WER minidumps' {
        $dumps = @(Get-ChildItem -Path $dumpsDir -Filter '*.dmp' -File -ErrorAction SilentlyContinue)
        Log ("WER minidump count: $($dumps.Count)")
        $cdb = Get-Command cdb.exe -ErrorAction SilentlyContinue
        if (-not $cdb) {
            foreach ($candidate in @(
                'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe',
                'C:\Program Files\Windows Kits\10\Debuggers\x64\cdb.exe')) {
                if (Test-Path $candidate) { $cdb = $candidate; break }
            }
        }
        if ($cdb -and $dumps.Count -gt 0) {
            $path = if ($cdb -is [string]) { $cdb } else { $cdb.Source }
            Log ("CDB available; analyzing $($dumps[0].Name) stack")
            foreach ($line in ((& $path -z $dumps[0].FullName -c '.ecxr; kv; q' 2>&1) |
                               Select-Object -Last 95)) {
                Log ([string]$line)
            }
        } elseif ($dumps.Count -gt 0) {
            Log 'No CDB debugger found; preserving only fault metadata, not dump bytes.'
        }
        foreach ($dump in $dumps) { Remove-Item -LiteralPath $dump.FullName -Force }
    }
    if (Test-Path $marker) {
        Attempt 'Restore WER registry' {
            Remove-Item -LiteralPath $werKey -Force -Recurse
            Remove-Item -LiteralPath $marker -Force
            Log 'Removed our temporary per-image WER registry configuration.'
        }
    }
}
