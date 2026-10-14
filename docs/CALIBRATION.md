# 🎯 Kalibrasi Sensor Kelembaban Tanah

Sensor kapasitif mengeluarkan **tegangan analog** yang **turun** saat tanah makin
basah. Nilai ADC mentah perlu dikalibrasi ke persen (0–100%).

## ⚠️ Rentang ADC ESP8266

ESP8266 memakai ADC **10-bit** dengan rentang **0–1023** (beda dengan ESP32
yang 12-bit/0–4095). Kalau Anda sebelumnya pakai nilai ESP32, harus diganti.

## Nilai Default

Firmware: `firmware/SmartAgriculture_ESP8266/SmartAgriculture_ESP8266.ino`

```cpp
#define SOIL_ADC_DRY     1022    // nilai ADC di udara (0%)  <- hasil kalibrasi
#define SOIL_ADC_WET     473     // nilai ADC di air (100%)  <- hasil kalibrasi
#define SOIL_ADC_MAX     1023
```

> ⚠️ Nilai di atas adalah **hasil kalibrasi sensor spesifik Anda**. Kalau ganti
> sensor, kalibrasi ulang (lihat langkah di bawah).

Rumus konversi:
```
persen = (SOIL_ADC_DRY - raw) / (SOIL_ADC_DRY - SOIL_ADC_WET) * 100
```

## Cara Kalibrasi Manual

1. **Nyalakan board** dengan sensor belum ditancap, buka **Serial Monitor
   (115200 baud)**. Setiap pembacaan akan menampilkan `soil_raw=...`.
2. **Ukur titik kering**:
   - Biarkan sensor di **udara terbuka** (kering).
   - Catat nilai `soil_raw` yang stabil → jadikan `SOIL_ADC_DRY`.
3. **Ukur titik basah**:
   - Celup sensor sampai garis batas ke dalam **gelas berisi air**.
   - Catat nilai `soil_raw` yang stabil → jadikan `SOIL_ADC_WET`.
4. Update `SOIL_ADC_DRY` dan `SOIL_ADC_WET` di kode, lalu upload ulang.

## Contoh Hasil (ESP8266, sensor Anda)

| Kondisi | soil_raw | Persen |
|---|---|---|
| Udara kering | 1022 | 0 % |
| Tanah agak kering | 900 | 22 % |
| Tanah lembab | 750 | 50 % |
| Tanah basah | 600 | 77 % |
| Dalam air | 473 | 100 % |

## Tips

- Setiap unit sensor sedikit berbeda — kalibrasi per-unit untuk akurasi terbaik.
- **Jangan** merendam sensor melewati garis putih/batas elektronik.
- Untuk hasil lebih stabil, tambahkan `SOIL_ADC_SAMPLES` (default 16).
- Kalau nilai ADC tampak acak, cek: pin benar, VCC 3V3, dan GND tersambung.
- **ESP8266**: pastikan tegangan A0 tidak melebihi batas board (lihat `docs/WIRING.md`).

## Ambang Batas Irigasi

```cpp
#define SOIL_MOISTURE_MIN  30.0f   // di bawah ini → butuh irigasi
#define SOIL_MOISTURE_MAX  80.0f
#define SOIL_CRITICAL      20.0f   // kritis / sangat kering
```
