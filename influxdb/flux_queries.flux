# =====================================================================
#  Kueri Flux - Smart Agriculture
#  Tempel di InfluxDB UI > Data Explorer / Dashboard Cell (Query Editor)
#  Sesuaikan variabel bucket & range sesuai kebutuhan.
# =====================================================================

// ---------- 1. Kelembaban tanah (%) ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "soil_moisture")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "soil_moisture")

// ---------- 2. Suhu udara (°C) ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "air_temperature")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "air_temperature")

// ---------- 3. Kelembaban udara (%) ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "air_humidity")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "air_humidity")

// ---------- 4. Intensitas cahaya (lux) ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "light_lux")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "light_lux")

// ---------- 5. Prediksi jam sampai tanah kering ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "predict_hours_to_dry")
  |> last()
  |> yield(name: "hours_to_dry")

// ---------- 6. Skor kesehatan tanaman (0-100) ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "health_score")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "health_score")

// ---------- 7. Status irigasi terakhir ----------
from(bucket: "agriculture")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "irrigation_needed")
  |> last()
  |> yield(name: "irrigation_needed")

// ---------- 8. Nilai terbaru semua sensor (tabel ringkas) ----------
from(bucket: "agriculture")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "agriculture")
  |> last()
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> yield(name: "latest_all")

// ---------- 9. Peringatan: kelembaban tanah di bawah batas ----------
from(bucket: "agriculture")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "agriculture" and r._field == "soil_moisture")
  |> filter(fn: (r) => r._value < 30.0)
  |> yield(name: "low_soil_alert")
