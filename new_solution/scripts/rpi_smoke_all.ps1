param(
    [Parameter(Mandatory = $true)]
    [string]$RpiIp,
    [string]$DeviceId = "FS-001",
    [int]$Port = 8000
)

$BaseUrl = "http://${RpiIp}:${Port}"

Write-Host "=== Floating Sensor – RPi smoke check sequence ==="
Write-Host "Target: $BaseUrl"
Write-Host ""

$scripts = @(
    @{ name = "status";   script = "check_api.ps1";       args = @{BaseUrl = $BaseUrl} },
    @{ name = "seed";     script = "seed_api_telemetry.ps1"; args = @{BaseUrl = $BaseUrl; DeviceId = $DeviceId} },
    @{ name = "latest";   script = "check_api_latest.ps1"; args = @{BaseUrl = $BaseUrl; DeviceId = $DeviceId} },
    @{ name = "history";  script = "check_api_history.ps1"; args = @{BaseUrl = $BaseUrl; DeviceId = $DeviceId} }
)

$failed = 0
foreach ($entry in $scripts) {
    $scriptPath = Join-Path $PSScriptRoot $entry.script
    Write-Host "--- [$($entry.name)] ---"
    & $scriptPath @($entry.args)
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAIL: $($entry.name)"
        $failed++
    }
    Write-Host ""
}

if ($failed -eq 0) {
    Write-Host "All smoke checks passed."
    exit 0
} else {
    Write-Host "FAILED: $failed check(s) did not pass."
    exit 1
}
