#!/usr/bin/env bash
# Raspberry Pi 4 – setup script for the Floating Sensor IoT Core MVP stack.
# Run once on a fresh Raspberry Pi OS 64-bit installation.
# Usage: bash rpi_setup.sh

set -e

REPO_URL="${REPO_URL:-}"
PROJECT_DIR="${HOME}/floating_sensor"
SRC_DIR="${PROJECT_DIR}/new_solution/src"

echo "=== Floating Sensor – Raspberry Pi 4 setup ==="

# ── 1. Docker ──────────────────────────────────────────────────────────────
echo "[1/5] Installing Docker..."
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker "$USER"
echo "NOTE: Docker group change takes effect after re-login."
echo "      If docker compose fails, log out and run this script again."

# ── 2. Clone repo ─────────────────────────────────────────────────────────
echo "[2/5] Cloning repository..."
if [ -z "$REPO_URL" ]; then
    echo "ERROR: Set REPO_URL before running, e.g.:"
    echo "  REPO_URL=https://github.com/<user>/<repo>.git bash rpi_setup.sh"
    exit 1
fi

if [ -d "$PROJECT_DIR" ]; then
    echo "  Directory exists, pulling latest..."
    git -C "$PROJECT_DIR" pull
else
    git clone "$REPO_URL" "$PROJECT_DIR"
fi

# ── 3. Python venv ────────────────────────────────────────────────────────
echo "[3/5] Setting up Python virtual environment..."
cd "$SRC_DIR"
python3 -m venv .venv
# shellcheck source=/dev/null
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
deactivate

# ── 4. Start Docker services (Mosquitto + InfluxDB) ───────────────────────
echo "[4/5] Starting Docker services..."
cd "$SRC_DIR"
docker compose up -d

echo "      Waiting for services to be ready..."
sleep 5
docker compose ps

# ── 5. Run smoke tests ────────────────────────────────────────────────────
echo "[5/5] Running pytest smoke tests..."
cd "$SRC_DIR"
source .venv/bin/activate
python -m pytest tests/test_smoke.py -q
deactivate

echo ""
echo "=== Setup complete ==="
echo ""
echo "Start the IoT Core API:"
echo "  cd ${SRC_DIR}"
echo "  source .venv/bin/activate"
echo "  python -m uvicorn api.main:app --host 0.0.0.0 --port 8000"
echo ""
echo "Verify from dev machine (replace <rpi-ip>):"
echo "  .\\scripts\\check_api.ps1 -BaseUrl http://<rpi-ip>:8000"
echo "  .\\scripts\\seed_api_telemetry.ps1 -BaseUrl http://<rpi-ip>:8000"
echo "  .\\scripts\\check_api_latest.ps1 -BaseUrl http://<rpi-ip>:8000"
