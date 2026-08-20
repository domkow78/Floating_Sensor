#!/usr/bin/env bash
# Smoke checks run locally on the RPi against the IoT Core API.
# Usage: bash rpi_smoke_all.sh [base_url] [device_id]

set -e

BASE_URL="${1:-http://localhost:8000}"
DEVICE_ID="${2:-FS-001}"
FAILED=0

check_json_field() {
    local json="$1" field="$2" expected="$3"
    local value
    value=$(echo "$json" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d$field)" 2>/dev/null)
    if [ "$value" != "$expected" ]; then
        echo "  FAIL: expected $field='$expected', got='$value'"
        return 1
    fi
    return 0
}

echo "=== Floating Sensor – RPi smoke check sequence ==="
echo "Target: $BASE_URL  Device: $DEVICE_ID"
echo ""

# ── 1. status ────────────────────────────────────────────────────────────
echo "--- [status] ---"
response=$(curl -sf "$BASE_URL/api/v1/status") || { echo "FAIL: no response"; FAILED=$((FAILED+1)); }
if check_json_field "$response" "['status']" "success" && \
   check_json_field "$response" "['data']['iot_core']" "ok"; then
    echo "OK: status"
else
    FAILED=$((FAILED+1))
fi
echo ""

# ── 2. seed telemetry ─────────────────────────────────────────────────────
echo "--- [seed] ---"
seed_payload=$(python3 -c "
import json, datetime
print(json.dumps({
    'device_id': '$DEVICE_ID',
    'timestamp': datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ'),
    'temperature': 22.4,
    'humidity': 48.2,
    'pressure': 1008.4
}))
")
response=$(curl -sf -X POST "$BASE_URL/api/v1/dev/ingest" \
    -H "Content-Type: application/json" \
    -d "$seed_payload") || { echo "FAIL: no response"; FAILED=$((FAILED+1)); }
if check_json_field "$response" "['status']" "success"; then
    echo "OK: seeded telemetry for $DEVICE_ID"
else
    FAILED=$((FAILED+1))
fi
echo ""

# ── 3. telemetry/latest ───────────────────────────────────────────────────
echo "--- [latest] ---"
response=$(curl -sf "$BASE_URL/api/v1/telemetry/latest?device_id=$DEVICE_ID") || { echo "FAIL: no response"; FAILED=$((FAILED+1)); }
if check_json_field "$response" "['status']" "success" && \
   check_json_field "$response" "['data']['device_id']" "$DEVICE_ID"; then
    echo "OK: telemetry/latest"
else
    FAILED=$((FAILED+1))
fi
echo ""

# ── 4. telemetry/history ──────────────────────────────────────────────────
echo "--- [history] ---"
FROM="2026-08-01T00:00:00Z"
TO="2026-12-31T23:59:59Z"
response=$(curl -sf "$BASE_URL/api/v1/telemetry/history?device_id=$DEVICE_ID&from=$FROM&to=$TO&limit=10") || { echo "FAIL: no response"; FAILED=$((FAILED+1)); }
if check_json_field "$response" "['status']" "success" && \
   check_json_field "$response" "['data']['device_id']" "$DEVICE_ID"; then
    echo "OK: telemetry/history (count=$(echo "$response" | python3 -c "import sys,json; print(json.load(sys.stdin)['data']['count'])"))"
else
    FAILED=$((FAILED+1))
fi
echo ""

# ── summary ───────────────────────────────────────────────────────────────
if [ "$FAILED" -eq 0 ]; then
    echo "All smoke checks passed."
    exit 0
else
    echo "FAILED: $FAILED check(s) did not pass."
    exit 1
fi
