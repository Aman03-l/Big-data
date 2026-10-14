# =====================================================================
#  Setup InfluxDB v2 - Linux / macOS
#  Membuat organization, bucket, dan token (read/write).
#  Butuh CLI `influx` (bagian dari InfluxDB 2.x).
# =====================================================================
#!/usr/bin/env bash
set -e

INFLUX_HOST="${INFLUX_HOST:-http://localhost:8086}"
INFLUX_USER="${INFLUX_USER:-admin}"
INFLUX_PASS="${INFLUX_PASS:-admin12345}"
ORG="${ORG:-smart-agri}"
BUCKET="${BUCKET:-agriculture}"
RETENTION="${RETENTION:-30d}"

echo ">> Setup InfluxDB di $INFLUX_HOST"

# 1. Setup awal (org + bucket + admin token)
influx setup \
  --host "$INFLUX_HOST" \
  --username "$INFLUX_USER" \
  --password "$INFLUX_PASS" \
  --org "$ORG" \
  --bucket "$BUCKET" \
  --retention "$RETENTION" \
  --force || echo "(setup sudah pernah dilakukan, lanjut...)"

# 2. Buat token read/write khusus ESP32
echo ">> Membuat token untuk ESP32..."
influx auth create \
  --host "$INFLUX_HOST" \
  --org "$ORG" \
  --description "esp32-smart-agriculture" \
  --read-bucket "$(influx bucket list --host "$INFLUX_HOST" --org "$ORG" --name "$BUCKET" --hide-headers | awk '{print $1}')" \
  --write-bucket "$(influx bucket list --host "$INFLUX_HOST" --org "$ORG" --name "$BUCKET" --hide-headers | awk '{print $1}')" \
  --json | tee /tmp/influx_token.json

echo ""
echo "===================================================="
echo "  Org    : $ORG"
echo "  Bucket : $BUCKET"
echo "  Token  : (lihat /tmp/influx_token.json -> .token)"
echo "  -> Salin token ke config.h (INFLUX_TOKEN)"
echo "===================================================="
