param(
    [string]$BaseUrl = "http://localhost:8000",
    [string]$DeviceId = "FS-001",
    [int]$TimeoutSec = 5,
    [switch]$AllowNotFound
)

$endpoint = "$BaseUrl/api/v1/telemetry/latest?device_id=$DeviceId"

try {
    $response = Invoke-WebRequest -Uri $endpoint -Method Get -TimeoutSec $TimeoutSec
    $payload = $response.Content | ConvertFrom-Json

    if ($payload.status -ne "success") {
        Write-Host "ERROR: Smoke check failed: expected status='success', got '$($payload.status)'"
        exit 1
    }

    if (-not $payload.data) {
        Write-Host "ERROR: Smoke check failed: missing data payload"
        exit 1
    }

    $requiredFields = @("device_id", "timestamp")
    foreach ($field in $requiredFields) {
        if (-not $payload.data.PSObject.Properties.Name.Contains($field)) {
            Write-Host "ERROR: Smoke check failed: missing required field '$field' in response"
            exit 1
        }
    }

    Write-Host "OK: telemetry/latest endpoint returned valid payload for device '$DeviceId'"
    exit 0
}
catch {
    $exception = $_.Exception

    if ($exception.Response -and $exception.Response.StatusCode.value__ -eq 404) {
        if ($AllowNotFound) {
            Write-Warning "telemetry/latest returned 404. No telemetry in registry yet for '$DeviceId'."
            Write-Host "Hint: run pipeline ingestion first, then retry this check."
            exit 0
        }

        Write-Host "ERROR: Smoke check failed: telemetry/latest returned 404 for '$DeviceId'."
        Write-Host "ERROR: Precondition not met. Registry must contain recent telemetry for the device."
        Write-Host "TIP: use -AllowNotFound for startup smoke, or ingest telemetry in the same API process before strict check."
        exit 1
    }

    Write-Host "ERROR: Smoke check failed: $($exception.Message)"
    exit 1
}
