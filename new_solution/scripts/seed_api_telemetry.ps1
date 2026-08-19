param(
    [string]$BaseUrl = "http://localhost:8000",
    [string]$DeviceId = "FS-001",
    [int]$TimeoutSec = 5
)

$endpoint = "$BaseUrl/api/v1/dev/ingest"
$payload = @{
    device_id   = $DeviceId
    timestamp   = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    temperature = 22.4
    humidity    = 48.2
    pressure    = 1008.4
}

try {
    $json = $payload | ConvertTo-Json -Depth 4
    $response = Invoke-RestMethod -Uri $endpoint -Method Post -ContentType "application/json" -Body $json -TimeoutSec $TimeoutSec

    if ($response.status -ne "success") {
        Write-Host "ERROR: Seed failed: expected status='success', got '$($response.status)'"
        exit 1
    }

    Write-Host "OK: seeded telemetry for '$DeviceId' via $endpoint"
    exit 0
}
catch {
    Write-Host "ERROR: Seed failed: $($_.Exception.Message)"
    exit 1
}
