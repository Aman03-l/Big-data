# 🌱 Smart Agriculture & Predictive Farming

Sistem monitoring & prediksi pertanian pintar berbasis **ESP8266**, dengan penyimpanan
data di **InfluxDB v2** dan visualisasi memakai **dashboard bawaan InfluxDB**.

## Fitur

| Fitur | Keterangan |
|---|---|
| Kelembaban tanah | **Capacitive Soil Moisture Sensor** (analog, ADC) |
| Suhu & kelembaban udara | **DHT22** (digital, one-wire) |
| Intensitas cahaya | **BH1750** (digital, I²C) |
| Otak sistem | **ESP8266** (NodeMCU / Wemos D1 mini) |
| Database | **InfluxDB v2** (time-series) |
| Dashboard | **InfluxDB UI bawaan** (import template JSON) |
| Predictive farming | Estimasi waktu tanah mengering, skor kesehatan, saran irigasi |

## Struktur Proyek

```
SmartAgriculture/
├── firmware/
│   └── SmartAgriculture_ESP8266/  # ⭐ Firmware ESP8266 (NodeMCU / Wemos D1 mini)
│       └── SmartAgriculture_ESP8266.ino
├── influxdb/
│   ├── docker-compose.yml         # Jalankan InfluxDB v2 dengan Docker
│   ├── deploy_dashboard.ps1       # Buat dashboard + variabel otomatis
│   ├── setup_influx.ps1           # Setup org/bucket/token (Windows)
│   ├── setup_influx.sh            # Setup org/bucket/token (Linux/macOS)
│   ├── flux_queries.flux          # Kueri Flux siap pakai
│   └── dashboard_smart_agriculture.json  # Template dashboard
├── tools/
│   ├── simulate_data.py           # Simulator data (uji tanpa hardware)
│   ├── CekWiFi_Aaa/               # Sketch cek koneksi WiFi (ESP8266)
│   └── WiFiScan_ESP8266/          # Sketch diagnosa WiFi (ESP8266)
└── docs/
    ├── WIRING.md                  # Skema wiring & pinout
    ├── SETUP_INFLUXDB.md          # Langkah setup database & dashboard
    └── CALIBRATION.md             # Cara kalibrasi sensor kelembaban tanah
```

## Quick Start

1. **Jalankan InfluxDB** → `cd influxdb; docker compose up -d`
2. **Deploy dashboard** → `powershell -ExecutionPolicy Bypass -File influxdb\deploy_dashboard.ps1`
3. **Wiring** → lihat [`docs/WIRING.md`](docs/WIRING.md)
4. **Kalibrasi sensor tanah** → lihat [`docs/CALIBRATION.md`](docs/CALIBRATION.md)
5. **Upload firmware** ke ESP8266 → buka
   `firmware/SmartAgriculture_ESP8266/SmartAgriculture_ESP8266.ino`
   (board: **"NodeMCU 1.0 (ESP-12E Module)"**)
6. **Setup InfluxDB detail** → lihat [`docs/SETUP_INFLUXDB.md`](docs/SETUP_INFLUXDB.md)

> Belum punya hardware? Uji dulu pakai simulator:
> `python tools/simulate_data.py --interval 1 --count 60`

## Cara Kerja Singkat

```
┌─────────┐   analog   ┌──────────┐
│ Soil v1.2├──────────►│          │
└─────────┘            │          │   WiFi/HTTP   ┌───────────┐   Flux   ┌──────────────┐
┌─────────┐  one-wire  │ ESP8266  ├──────────────►│ InfluxDB  ├─────────►│ Dashboard UI │
│  DHT22  ├──────────►│          │  line proto   │   v2      │          │  (bawaan)    │
└─────────┘            │          │               └───────────┘          └──────────────┘
┌─────────┐    I²C     │          │
│ BH1750  ├──────────►│          │
└─────────┘            └──────────┘
```

Setiap `SAMPLE_INTERVAL_MS` (default 60 detik) ESP8266 membaca ketiga sensor,
menghitung prediksi, lalu mengirim satu titik data ke InfluxDB dengan measurement
`agriculture`.
