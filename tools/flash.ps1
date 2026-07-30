param(
    [string]$UploadPort = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = "No"

function Find-PlatformIO {
    foreach ($commandName in @("pio", "platformio")) {
        $command = Get-Command $commandName -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }
    }

    $userInstall = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
    if (Test-Path -LiteralPath $userInstall) {
        return $userInstall
    }

    throw "PlatformIO was not found. Install PlatformIO Core or the PlatformIO IDE extension first."
}

$platformio = Find-PlatformIO

if (-not $UploadPort) {
    $deviceJson = & $platformio device list --json-output
    if ($LASTEXITCODE -ne 0) {
        throw "PlatformIO could not enumerate serial devices."
    }

    $devices = $deviceJson | ConvertFrom-Json
    $device = $devices |
        Where-Object { $_.hwid -match "VID:PID=303A:" } |
        Select-Object -First 1

    if (-not $device) {
        $device = $devices |
            Where-Object { $_.port -match "^COM\d+$" } |
            Select-Object -First 1
    }

    if (-not $device) {
        throw "No ESP32-S3 serial port was found. Connect the device or pass -UploadPort COMx."
    }

    $UploadPort = $device.port
}

Write-Host "PlatformIO: $platformio"
Write-Host "Upload port: $UploadPort"

& $platformio run
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $platformio run -t uploadfs --upload-port $UploadPort
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $platformio run -t upload --upload-port $UploadPort
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
