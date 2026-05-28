$ErrorActionPreference = "Stop"

Set-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)

$env:PYTHONIOENCODING = "utf-8"
$env:PLATFORMIO_CORE_DIR = "$env:USERPROFILE\.platformio"
$env:PLATFORMIO_BUILD_DIR = "D:\Codex\.platformio-build\M5StickS3PersonalConsole"
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = "No"
New-Item -ItemType Directory -Force -Path $env:PLATFORMIO_BUILD_DIR | Out-Null

$python = "D:\Codex\pio-py311\Scripts\python.exe"
if (-not (Test-Path $python)) {
    C:\Users\ccgg6\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe -m venv "D:\Codex\pio-py311"
}

$platformioOk = $false
try {
    & $python -m platformio --version *> $null
    $platformioOk = ($LASTEXITCODE -eq 0)
} catch {
    $platformioOk = $false
}

if (-not $platformioOk) {
    Write-Host "PlatformIO not found. Installing with pip..."
    & $python -m pip install platformio
}

Write-Host "PlatformIO Core: $env:PLATFORMIO_CORE_DIR"
Write-Host "Build dir:       $env:PLATFORMIO_BUILD_DIR"
$portsText = & $python -m serial.tools.list_ports -v
$uploadPort = $null
foreach ($line in $portsText) {
    if ($line -match '^(COM\d+)\s*$') {
        $candidatePort = $Matches[1]
        continue
    }
    if ($line -match 'VID:PID=303A:1001' -and $candidatePort) {
        $uploadPort = $candidatePort
        break
    }
}
if (-not $uploadPort) {
    foreach ($line in $portsText) {
        if ($line -match '^(COM\d+)\s*$') {
            $candidatePort = $Matches[1]
            continue
        }
        if ($line -match 'VID:PID=303A:' -and $candidatePort) {
            $uploadPort = $candidatePort
            break
        }
    }
}
if (-not $uploadPort) {
    Write-Error "No ESP32-S3 serial port found. Put StickS3 in download mode and try again."
    exit 1
}
Write-Host "Upload port:     $uploadPort"
& $python -m platformio run
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $python -m platformio run -t uploadfs --upload-port $uploadPort
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $python -m platformio run -t upload --upload-port $uploadPort
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
