# ✅ Status & Langkah Selanjutnya

## Yang SUDAH selesai (otomatis)

- [x] **InfluxDB v2.7.12 jalan** di `http://localhost:8086` (Docker)
- [x] Org `smart-agri`, bucket `agriculture` (retensi 30 hari), token valid
- [x] **Dashboard "Smart Agriculture & Predictive Farming"** + 7 panel dibuat
- [x] Variabel dashboard `bucket` = `agriculture` dibuat
- [x] Jalur tulis & baca data **terbukti bekerja** (diuji dengan data contoh)
- [x] Konfigurasi firmware sudah diisi IP PC Anda: `10.4.75.52` (cek ulang: `ipconfig`)

**Buka dashboard:** http://localhost:8086 → login `admin` / `admin12345`

---

## Yang PERLU Anda lakukan

### 1. (Opsional) Buat firewall rule agar ESP8266 bisa akses InfluxDB
Buka **PowerShell sebagai Administrator**, lalu:
```powershell
New-NetFirewallRule -DisplayName "InfluxDB 8086" -Direction Inbound -Protocol TCP -LocalPort 8086 -Action Allow -Profile Private
```

### 2. Isi WiFi di kode ESP8266
Firmware ESP8266 ada di: **`firmware/SmartAgriculture_ESP8266/SmartAgriculture_ESP8266.ino`**
- WiFi sudah terisi (`Aaa` — **terbuka/tanpa password**). Board: **"NodeMCU 1.0 (ESP-12E Module)"**

> ESP8266 & PC harus di **jaringan WiFi yang sama**.

### 3. Rakit hardware (lihat `docs/WIRING.md`)

**ESP8266 (NodeMCU / Wemos D1 mini):**
| Sensor | Pin ESP8266 | Label NodeMCU |
|---|---|---|
| Soil Moisture (analog) | **A0** | A0 |
| Soil Moisture (DO) | *(tidak disambung)* | — |
| DHT22 (digital) | **GPIO14** | **D5** |
| BH1750 SDA | **GPIO4** | **D2** |
| BH1750 SCL | **GPIO5** | **D1** |

### 4. Upload firmware
- Buka **Arduino IDE**
- Buka file `SmartAgriculture_ESP8266.ino`
- Pilih board **"NodeMCU 1.0 (ESP-12E Module)"**
- Install library: **DHT sensor library**, **Adafruit Unified Sensor**, **BH1750**
- Pilih **Port COM** yang muncul → klik **Upload**
- ⚠️ **Tutup Serial Monitor sebelum upload** (kalau tidak, muncul "Access denied")
- Buka **Serial Monitor** (115200 baud) untuk lihat nilai sensor

### 5. Kalibrasi sensor tanah (WAJIB untuk akurasi)
Ikuti `docs/CALIBRATION.md`:
1. Lihat nilai `soil_raw` di Serial Monitor
2. Catat nilai di **udara** → `SOIL_ADC_DRY`
3. Catat nilai di **air** → `SOIL_ADC_WET`
4. Update di file `.ino` → upload ulang

> ⚠️ **ESP8266**: ADC 10-bit (0–1023), bukan 0–4095 seperti ESP32. Nilai
> `SOIL_ADC_DRY`/`SOIL_ADC_WET` di kode masih **perkiraan** — wajib dikalibrasi.


---

## Uji tanpa hardware (sekarang bisa langsung)
```powershell
cd tools
python simulate_data.py --interval 5 --count 100
```
Lalu buka dashboard — grafik akan bergerak.

## Buka lagi InfluxDB setelah restart PC
```powershell
cd influxdb
docker compose up -d
```

## Perintah berguna
| Tujuan | Perintah |
|---|---|
| Cek InfluxDB jalan | `docker ps` |
| Lihat log InfluxDB | `docker logs influxdb-smart-agri` |
| Hentikan InfluxDB | `cd influxdb; docker compose down` |
| Hapus InfluxDB + data | `cd influxdb; docker compose down -v` |
| Redeploy dashboard | `cd influxdb; powershell -ExecutionPolicy Bypass -File .\deploy_dashboard.ps1` |
| Cek IP PC | `ipconfig` (lihat IPv4 Wi-Fi) |
