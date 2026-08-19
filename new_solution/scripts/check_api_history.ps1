param(
    [string]$BaseUrl = "http://localhost:8000",
    [string]$DeviceId = "FS-001",
    [string]$From = "2026-08-05T10:00:00Z",
    [string]$To = "2026-08-05T11:00:00Z",
    [int]$Limit = 500,
    [switch]$RequirePoints,
    [int]$TimeoutSec = 5
)

$endpoint = "$BaseUrl/api/v1/telemetry/history?device_id=$DeviceId&from=$From&to=$To&limit=$Limit"

try {
    $response = Invoke-RestMethod -Uri $endpoint -Method Get -TimeoutSec $TimeoutSec

    if ($response.status -ne "success") {
        Write-Host "ERROR: Smoke check failed: expected status='success', got '$($response.status)'"
        exit 1
    }

    if (-not $response.data) {
        Write-Host "ERROR: Smoke check failed: missing data payload"
        exit 1
    }

    $requiredFields = @("device_id", "points", "count")
    foreach ($field in $requiredFields) {
        if (-not $response.data.PSObject.Properties.Name.Contains($field)) {
            Write-Host "ERROR: Smoke check failed: missing required field '$field' in response"
            exit 1
        }
    }

    if ($response.data.device_id -ne $DeviceId) {
        Write-Host "ERROR: Smoke check failed: expected device_id='$DeviceId', got '$($response.data.device_id)'"
        exit 1
    }

    if ($RequirePoints -and [int]$response.data.count -lt 1) {
        Write-Host "ERROR: Smoke check failed: expected at least one history point but got count=$($response.data.count)"
        Write-Host "TIP: seed telemetry first using .\scripts\seed_api_telemetry.ps1 -DeviceId \"$DeviceId\""
        exit 1
    }

    Write-Host "OK: telemetry/history endpoint returned valid payload for device '$DeviceId' (count=$($response.data.count))"
    exit 0
}
catch {
    Write-Host "ERROR: Smoke check failed: $($_.Exception.Message)"
    exit 1
}
