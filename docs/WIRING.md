# 🔌 Wiring & Pinout ESP8266

> **Board: ESP8266 (NodeMCU / Wemos D1 mini)** — di Arduino IDE pilih
> **"NodeMCU 1.0 (ESP-12E Module)"**.
> Firmware: `firmware/SmartAgriculture_ESP8266/SmartAgriculture_ESP8266.ino`.

## Tabel Koneksi

| Sensor | Pin Sensor | Pin ESP8266 | Label NodeMCU | Catatan |
|---|---|---|---|---|
| **Capacitive Soil Moisture** | AOUT | **A0** | A0 | Satu-satunya pin analog |
| | VCC | 3V3 | 3V3 | |
| | GND | GND | GND | |
| | DO | *(tidak disambung)* | — | Opsional, lihat catatan bawah |
| **DHT11** | DATA | **GPIO14** | **D5** | pull-up 10 kΩ |
| | VCC | 3V3 | 3V3 | |
| | GND | GND | GND | |
| **BH1750** | SDA | **GPIO4** | **D2** | I2C data |
| | SCL | **GPIO5** | **D1** | I2C clock |
| | VCC | 3V3 | 3V3 | |
| | GND | GND | GND | |
| | ADDR | GND | — | Alamat I2C `0x23` (ke VCC → `0x5C`) |

> **DHT11 vs DHT22**: di firmware, `DHT_TYPE` harus disesuaikan.
> Untuk DHT11 → `#define DHT_TYPE DHT11`. Untuk DHT22 → `DHT22`.


### ⚠️ PENTING soal A0 (analog) ESP8266

| Board | Tegangan maks A0 | Perlu pembagi tegangan? |
|---|---|---|
| **NodeMCU v3 / Wemos D1 mini** | **0–3.3V** | ❌ Tidak (sudah ada di board) |
| **ESP-12 / ESP-01 telanjang** | **0–1.0V** | ✅ **YA — wajib** |

**Kalau pakai modul telanjang (0–1.0V)**, buat pembagi tegangan:
```
Soil AOUT ──[ R1 220kΩ ]──┬── A0 (ADC)
                          │
                     [ R2 100kΩ ]
                          │
                         GND
```
(R1/R2 ini menurunkan 3.3V → ~1.0V agar A0 tidak rusak.)

### Pin NodeMCU → GPIO (referensi)

| Label | GPIO |
|---|---|
| D0 | GPIO16 |
| **D1** | **GPIO5** |
| **D2** | **GPIO4** |
| D3 | GPIO0 |
| D4 | GPIO2 |
| **D5** | **GPIO14** |
| D6 | GPIO12 |
| D7 | GPIO13 |
| D8 | GPIO15 |
| A0 | ADC (0–1023) |

## Diagram

```
                  NodeMCU / Wemos D1 mini
                    ┌───────────────┐
   Soil AOUT ───────┤ A0            │
   DHT11 DATA ──────┤ D5 (GPIO14)   │
   BH1750 SDA ──────┤ D2 (GPIO4)    │
   BH1750 SCL ──────┤ D1 (GPIO5)    │
             3V3 ───┤ 3V3           │
             GND ───┤ GND           │
                    └───────────────┘
```

## Catatan Hardware

- **Soil sensor kapasitif** (bukan resistif) jauh lebih tahan korosi untuk
  pemakaian jangka panjang di dalam tanah.
- ADC ESP8266 agak *noisy*, sudah di-handle dengan rata-rata 16 sampel di firmware.
- Untuk DHT11 gunakan resistor pull-up 10 kΩ antara DATA dan VCC bila memakai
  sensor telanjang (bukan modul).
- Panjang kabel I²C disarankan < 1 m. Untuk kabel panjang, tambahkan pull-up 4.7 kΩ.

> ℹ️ **Tentang pin DO sensor soil**: DO (Digital Output) adalah output on/off
> opsional yang dikendalikan potensiometer di modul. **Tidak perlu disambung** —
> sensor tetap bekerja normal memakai AOUT (analog). Kita pakai AOUT karena
> memberi nilai persen (0–100%) untuk grafik & prediksi.

## Daftar Komponen

| No | Komponen | Jumlah |
|---|---|---|
| 1 | ESP8266 — NodeMCU v3 / Wemos D1 mini | 1 |
| 2 | Capacitive Soil Moisture Sensor v1.2/v2.0 | 1 |
| 3 | DHT11 (suhu & kelembaban udara) | 1 |
| 4 | BH1750 (GY-302) | 1 |
| 5 | Resistor 10 kΩ (opsional utk DHT22) | 1 |
| 6 | Resistor 220 kΩ + 100 kΩ (hanya jika modul telanjang) | 2 |
| 7 | Kabel jumper & breadboard | secukupnya |

