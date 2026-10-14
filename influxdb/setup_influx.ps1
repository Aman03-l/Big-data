# =====================================================================
#  Setup InfluxDB v2 - Windows PowerShell
#  Membuat organization, bucket, dan token (read/write).
#  Butuh CLI `influx` (bagian dari InfluxDB 2.x) di PATH.
#
#  Contoh:
#    ./setup_influx.ps1 -Org smart-agri -Bucket agriculture -Retention 30d
# =====================================================================
param(
    [string]$InfluxHost = "http://localhost:8086",
    [string]$User       = "admin",
    [string]$Pass       = "admin12345",
    [string]$Org        = "smart-agri",
    [string]$Bucket     = "agriculture",
    [string]$Retention  = "30d"
)

Write-Host ">> Setup InfluxDB di $InfluxHost" -ForegroundColor Cyan

# 1. Setup awal (org + bucket + admin token)
try {
    influx setup `
        --host $InfluxHost `
        --username $User `
        --password $Pass `
        --org $Org `
        --bucket $Bucket `
        --retention $Retention `
        --force
} catch {
    Write-Host "(setup sudah pernah dilakukan, lanjut...)" -ForegroundColor Yellow
}

# 2. Ambil ID bucket
Write-Host ">> Mengambil ID bucket '$Bucket'..."
$bucketId = (influx bucket list --host $InfluxHost --org $Org --name $Bucket --hide-headers | Out-String).Trim().Split("`t")[0]
Write-Host "   Bucket ID: $bucketId"

# 3. Buat token read/write khusus ESP8266
Write-Host ">> Membuat token untuk ESP8266..." -ForegroundColor Cyan
$tokenJson = influx auth create `
    --host $InfluxHost `
    --org $Org `
    --description "esp8266-smart-agriculture" `
    --read-bucket $bucketId `
    --write-bucket $bucketId `
    --json

$tokenJson | Out-File -FilePath "$PSScriptRoot\influx_token.json" -Encoding utf8
$token = ($tokenJson | ConvertFrom-Json).token

Write-Host ""
Write-Host "====================================================" -ForegroundColor Green
Write-Host "  Org    : $Org"
Write-Host "  Bucket : $Bucket"
Write-Host "  Token  : $token"
Write-Host "  -> Salin token di atas ke config.h (INFLUX_TOKEN)"
Write-Host "  -> Token juga tersimpan di influx_token.json"
Write-Host "====================================================" -ForegroundColor Green
