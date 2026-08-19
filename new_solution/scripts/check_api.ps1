param(
    [string]$BaseUrl = "http://localhost:8000",
    [int]$TimeoutSec = 5
)

$endpoint = "$BaseUrl/api/v1/status"

try {
    $response = Invoke-RestMethod -Uri $endpoint -Method Get -TimeoutSec $TimeoutSec

    if ($response.status -ne "success") {
        Write-Error "Smoke check failed: expected status='success', got '$($response.status)'"
        exit 1
    }

    if (-not $response.data) {
        Write-Error "Smoke check failed: missing data payload"
        exit 1
    }

    if ($response.data.iot_core -ne "ok") {
        Write-Error "Smoke check failed: expected data.iot_core='ok', got '$($response.data.iot_core)'"
        exit 1
    }

    Write-Host "OK: API status endpoint is healthy at $endpoint"
    exit 0
}
catch {
    Write-Error "Smoke check failed: $($_.Exception.Message)"
    exit 1
}
