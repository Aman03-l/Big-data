# 🗄️ Setup InfluxDB v2 + Dashboard Bawaan

## 1. Jalankan InfluxDB v2

### Opsi A — Docker (paling cepat)

```bash
cd influxdb
docker compose up -d
```

InfluxDB berjalan di **http://localhost:8086**.

### Opsi B — Install native

Unduh dari https://portal.influxdata.com/downloads/ lalu jalankan `influxd`.

## 2. Inisialisasi org / bucket / token

### Windows (PowerShell)

```powershell
cd influxdb
./setup_influx.ps1 -Org smart-agri -Bucket agriculture -Retention 30d
```

### Linux / macOS

```bash
cd influxdb
chmod +x setup_influx.sh
./setup_influx.sh
```

Script akan membuat:
- **Organization**: `smart-agri`
- **Bucket**: `agriculture` (retensi 30 hari)
- **Token** dengan izin write — **salin token ini ke `config.h`**.

> Kalau memakai **Docker**, inisialisasi otomatis sudah diatur di
> `docker-compose.yml` (user `admin` / password `admin12345`, token
> `smart-agri-dev-token`). Untuk produksi, ganti token!

## 3. Sesuaikan konfigurasi di firmware

Firmware: `firmware/SmartAgriculture_ESP8266/SmartAgriculture_ESP8266.ino`

```cpp
// ----- WiFi -----
static const WifiCred WIFI_LIST[] = {
  { "Aaa", "" },                // SSID, password ("" = terbuka)
  { "Desiot", "1sampai5" },     // cadangan
};

// ----- InfluxDB v2 -----
#define INFLUX_URL       "http://10.4.75.52:8086"  // IP PC/server InfluxDB (cek: ipconfig)
#define INFLUX_ORG       "smart-agri"
#define INFLUX_BUCKET    "agriculture"
#define INFLUX_TOKEN     "smart-agri-dev-token"

#define DEVICE_ID        "esp8266-garden-01"
#define LOCATION_TAG     "greenhouse-1"
```

> Gunakan **IP LAN** (bukan `localhost`) karena ESP8266 harus menjangkau server.
> Cek IP PC dengan `ipconfig` (lihat bagian Wi-Fi / IPv4).

## 4. Import Dashboard

### Opsi A — Otomatis (sudah disiapkan)

```powershell
cd influxdb
powershell -ExecutionPolicy Bypass -File .\deploy_dashboard.ps1
```

Script ini otomatis membuat dashboard + variabel `bucket` di InfluxDB.
Output akan menampilkan URL dashboard, mis.:

```
Buka : http://localhost:8086/orgs/<orgID>/dashboards/<dashID>
```

### Opsi B — Import manual

1. Buka InfluxDB UI → http://localhost:8086
2. Menu **Dashboards** → **Create Dashboard** → **Import** (tombol ⤒)
3. Pilih file `influxdb/dashboard_smart_agriculture.json`
4. Buat variabel `bucket` bernilai `agriculture` bila diminta → **Import**

Dashboard berisi 7 panel:
- Kelembaban Tanah (%) — gauge
- Skor Kesehatan Tanaman — single stat
- Prediksi & Status Irigasi — tabel
- Suhu Udara (°C) — grafik
- Kelembaban Udara (%) — grafik
- Intensitas Cahaya (lux) — grafik
- Riwayat Kelembaban Tanah — grafik

> Jika import JSON gagal (perbedaan versi), buat panel manual dan tempel kueri
> dari `influxdb/flux_queries.flux`.

## 5. Cek Data Masuk

Data Explorer → bucket `agriculture` → measurement `agriculture`.
Atau lewat CLI:

```bash
influx query 'from(bucket:"agriculture") |> range(start:-1h) |> limit(n:5)'
```

## Struktur Data (Line Protocol)

Measurement: **`agriculture`**
Tags: `device`, `location`

| Field | Tipe | Satuan |
|---|---|---|
| `soil_moisture` | float | % |
| `soil_raw` | int | ADC 0–1023 (ESP8266) |
| `soil_voltage` | float | V |
| `air_temperature` | float | °C |
| `air_humidity` | float | % |
| `light_lux` | float | lux |
| `predict_slope` | float | %/jam |
| `predict_hours_to_dry` | float | jam |
| `health_score` | float | 0–100 |
| `irrigation_needed` | bool | true/false |

## Troubleshooting

| Masalah | Solusi |
|---|---|
| ESP8266 gagal kirim (HTTP 401) | Token/org/bucket salah |
| HTTP 404 | `INFLUX_URL` salah atau path beda versi |
| Timeout / connect fail | ESP8266 & server beda jaringan, cek firewall port 8086 |
| Timestamp 1970 | NTP gagal — cek akses internet/UDP 123 |
| Nilai lux 0 terus | BH1750 tak terdeteksi — cek I²C (SDA D2 / SCL D1) |
| Soil selalu 0% atau 100% | Kalibrasi `SOIL_ADC_DRY`/`SOIL_ADC_WET` belum benar |
