# 📊 Presentasi: InfluxDB pada Smart Agriculture

> Dokumen penjelasan untuk presentasi project **Smart Agriculture & Predictive Farming**.

---

## 1. Apa itu InfluxDB?

**InfluxDB** = database khusus **time-series** (data deret waktu).

Artinya: database yang **dioptimalkan untuk menyimpan data yang berubah setiap waktu** — seperti suhu, kelembaban, cahaya yang dicatat terus-menerus.

**Kenapa tidak pakai MySQL/database biasa?**
| Database biasa (MySQL) | InfluxDB |
|---|---|
| Cocok untuk data transaksi (jual-beli, user) | Cocok untuk data sensor (deret waktu) |
| Query data waktu lambat kalau jutaan baris | Query waktu **sangat cepat** |
| Tidak ada fungsi agregasi waktu | Ada `mean`, `last`, `aggregateWindow` bawaan |
| — | Otomatis hapus data lama (retention) |

---

## 2. Yang Dipakai di Project Ini

| Komponen | Nilai | Keterangan |
|---|---|---|
| **Produk** | InfluxDB **v2.7.12** (OSS) | Open Source |
| **Cara jalan** | **Docker** (`influxdb:2.7`) | 1 perintah, langsung jalan |
| **Port** | **8086** | Default InfluxDB |
| **Organization** | `smart-agri` | "Folder" tingkat atas |
| **Bucket** | `agriculture` | Tempat data disimpan |
| **Retention** | **30 hari** (2.592.000 detik) | Data lama >30 hari otomatis dihapus |
| **Measurement** | `agriculture` | Nama "tabel"-nya |
| **Bahasa query** | **Flux** | Bahasa query InfluxDB v2 |
| **Dashboard** | Bawaan InfluxDB (7 panel) | Tanpa aplikasi tambahan |

---

## 3. Struktur Data (Schema)

InfluxDB menyimpan data dalam format **Line Protocol**. Struktur hierarkinya:

```
Bucket: agriculture
   └── Measurement: agriculture
          ├── Tag: device   = esp8266-garden-01
          ├── Tag: location = greenhouse-1
          ├── Field: soil_moisture = 62.5
          ├── Field: air_temperature = 28.4
          └── Field: ... (dan lain-lain)
```

### Contoh data nyata (Line Protocol):
```
agriculture,device=esp8266-garden-01,location=greenhouse-1 soil_moisture=62.5,soil_raw=473,air_temperature=30.7 1728895200000000000
```

### 🔑 Tag vs Field (PENTING untuk presentasi)

| | **Tag** | **Field** |
|---|---|---|
| Contoh | `device`, `location` | `soil_moisture`, `air_temperature` |
| Tipe | String | Angka/bool |
| Fungsi | **Pengelompokan** / filter | **Nilai** yang diukur |
| Di-index? | ✅ Ya (cepat difilter) | ❌ Tidak |
| Berubah-ubah? | Jarang (tetap) | Selalu berubah |

> 💡 **Analogi:** Tag = "label/nama", Field = "isi/angka".

---

## 4. 10 Field yang Disimpan

Data dari ESP8266 disimpan dalam **10 field**:

| No | Field | Satuan | Penjelasan |
|---|---|---|---|
| 1 | `soil_moisture` | % | Kelembaban tanah (hasil kalibrasi) |
| 2 | `soil_raw` | 0–1023 | Nilai ADC mentah sensor tanah |
| 3 | `soil_voltage` | Volt | Tegangan analog sensor tanah |
| 4 | `air_temperature` | °C | Suhu udara (DHT11) |
| 5 | `air_humidity` | % | Kelembaban udara (DHT11) |
| 6 | `light_lux` | lux | Intensitas cahaya (BH1750) |
| 7 | `predict_slope` | %/jam | Laju pengeringan tanah (prediksi) |
| 8 | `predict_hours_to_dry` | jam | Estimasi jam sampai tanah kering |
| 9 | `health_score` | 0–100 | Skor kesehatan tanaman |
| 10 | `irrigation_needed` | true/false | Apakah perlu disiram |

---

## 5. Alur Data (Data Flow)

```
┌──────────┐   Baca sensor    ┌──────────┐
│ Soil     │─────────────────►│          │
│ DHT11    │                  │ ESP8266  │
│ BH1750   │                  │          │
└──────────┘                  └────┬─────┘
                                   │
                     HTTP POST (Line Protocol)
                        setiap 60 detik
                                   │
                                   ▼
                          ┌────────────────┐
                          │   InfluxDB v2  │  ← Docker, port 8086
                          │ bucket:        │
                          │  agriculture   │
                          └───────┬────────┘
                                  │
                            Kueri Flux
                                  │
                                  ▼
                          ┌────────────────┐
                          │  Dashboard     │
                          │  (bawaan       │
                          │   InfluxDB)    │
                          └────────────────┘
```

**Penjelasan singkat:**
1. ESP8266 baca 3 sensor tiap 60 detik
2. Hitung prediksi & skor kesehatan
3. Kirim ke InfluxDB lewat **HTTP POST** (bukan MQTT)
4. InfluxDB simpan data
5. Dashboard ambil data pakai **kueri Flux** → tampilkan grafik

---

## 6. Cara Data Masuk (Write)

ESP8266 mengirim data pakai **HTTP POST** ke endpoint:

```
POST http://<server>:8086/api/v2/write?org=smart-agri&bucket=agriculture&precision=ns
Header: Authorization: Token <TOKEN>
Body:   agriculture,device=esp8266-garden-01,location=greenhouse-1 soil_moisture=62.5,...
```

**Ciri khas InfluxDB:** data dikirim dalam format **Line Protocol** (baris teks), bukan JSON. Ini membuat pengiriman sangat ringan & cepat.

---

## 7. Cara Baca Data (Query - Flux)

InfluxDB v2 pakai bahasa **Flux**. Contoh kueri yang dipakai dashboard:

### Contoh 1 — Kelembaban tanah terbaru:
```flux
from(bucket: "agriculture")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "soil_moisture")
  |> last()
```
**Artinya:** Ambil `soil_moisture` dari 1 jam terakhir, tampilkan nilai terakhir.

### Contoh 2 — Rata-rata per jendela waktu (untuk grafik):
```flux
from(bucket: "agriculture")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "air_temperature")
  |> aggregateWindow(every: 5m, fn: mean)
```
**Artinya:** Ambil suhu 24 jam terakhir, rata-ratakan per 5 menit.

### Penjelasan tiap fungsi Flux:

| Fungsi | Arti |
|---|---|
| `from(bucket:)` | Pilih bucket (tempat data) |
| `range(start:)` | Rentang waktu |
| `filter(fn:)` | Saring data (measurement, field) |
| `last()` | Ambil nilai terakhir |
| `mean()` | Hitung rata-rata |
| `aggregateWindow(every:)` | Kelompokkan per jendela waktu |
| `pivot()` | Ubah baris jadi kolom (untuk tabel) |

---

## 8. Dashboard (Bawaan InfluxDB)

Dashboard dibuat **otomatis** lewat script `deploy_dashboard.ps1`, berisi **7 panel**:

| Panel | Tipe | Menampilkan |
|---|---|---|
| Kelembaban Tanah | Gauge | % kelembaban (0–100) |
| Skor Kesehatan Tanaman | Single Stat | Skor 0–100 |
| Prediksi & Status Irigasi | Tabel | Jam sampai kering + status |
| Suhu Udara | Grafik garis | °C |
| Kelembaban Udara | Grafik garis | % |
| Intensitas Cahaya | Grafik garis | lux |
| Riwayat Kelembaban Tanah | Grafik garis | Tren % |

---

## 9. Keunggulan Pakai InfluxDB (untuk presentasi)

1. **Khusus time-series** → query data waktu sangat cepat
2. **Retention otomatis** → data lama dihapus sendiri (hemat storage)
3. **Dashboard bawaan** → tidak perlu aplikasi tambahan (Grafana, dll)
4. **Line Protocol** → ringan untuk perangkat IoT (ESP8266)
5. **Bahasa Flux** → fleksibel, bisa agregasi/transformasi
6. **Mudah di-deploy** → 1 file `docker-compose.yml`
7. **Open source & gratis**

---

## 10. Perintah Penting (Demo saat presentasi)

```bash
# 1. Jalankan InfluxDB
cd influxdb
docker compose up -d

# 2. Cek jalan
docker ps

# 3. Deploy dashboard
powershell -ExecutionPolicy Bypass -File .\deploy_dashboard.ps1

# 4. Buka UI
# http://localhost:8086  (login: admin / admin12345)
```

---

## 11. Istilah Kunci (Glossary)

| Istilah | Arti |
|---|---|
| **Time-series** | Data yang berubah tiap waktu |
| **Bucket** | Wadah penyimpanan (seperti "database") |
| **Measurement** | Nama data (seperti "tabel") |
| **Tag** | Label/index (device, location) |
| **Field** | Nilai yang diukur |
| **Line Protocol** | Format kirim data InfluxDB |
| **Flux** | Bahasa query InfluxDB v2 |
| **Retention** | Aturan hapus data lama |
| **Token** | Kunci akses (autentikasi) |
| **Organization** | Grup/tenant tingkat atas |

---

## 12. Ringkasan 1 Kalimat (untuk pembuka presentasi)

> **"Project ini menggunakan InfluxDB v2 (time-series database) yang berjalan di
> Docker, dengan bucket `agriculture` menyimpan 10 field dari sensor ESP8266,
> di-query pakai bahasa Flux, dan divisualisasikan lewat dashboard bawaan
> InfluxDB dengan 7 panel."**

---

## 13. Contoh Data Nyata (dari sistem yang berjalan)

Data terakhir yang tersimpan di InfluxDB:

| Field | Nilai |
|---|---|
| `soil_moisture` | 0 % |
| `soil_raw` | 1024 |
| `soil_voltage` | 3.303 V |
| `air_temperature` | 23.9 °C |
| `air_humidity` | 48 % |
| `light_lux` | 238.3 lux |
| `predict_slope` | 0 |
| `predict_hours_to_dry` | -1 (tidak ada tren kering) |
| `health_score` | 45 / 100 |
| `irrigation_needed` | true |

**Info penyimpanan:**
- Total data: **120 titik** (field `soil_moisture`)
- Device: `esp8266-garden-01`
- Lokasi: `greenhouse-1`
- Interval: 1 data / 60 detik
- Retention: 30 hari

> 💡 **Catatan presentasi:** nilai `soil_moisture = 0` & `soil_raw = 1024` berarti
> sensor tanah sedang **di udara kering** (raw mentok maksimum). Ini normal —
> kalau sensor ditancap ke tanah basah, nilainya naik.


