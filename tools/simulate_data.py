#!/usr/bin/env python3
"""
Simulator data Smart Agriculture -> InfluxDB v2
================================================
Mengirim data sensor sintetis ke InfluxDB agar dashboard bisa diuji
TANPA hardware ESP8266.

Pemakaian:
    pip install influxdb-client
    python simulate_data.py --url http://localhost:8086 \
        --token smart-agri-dev-token --org smart-agri --bucket agriculture \
        --interval 5 --count 200

Tekan Ctrl+C untuk berhenti.
"""
import argparse
import math
import random
import time
from datetime import datetime, timezone

try:
    from influxdb_client import InfluxDBClient, Point, WritePrecision
    from influxdb_client.client.write_api import SYNCHRONOUS
except ImportError:
    raise SystemExit("Install dulu: pip install influxdb-client")


def parse_args():
    p = argparse.ArgumentParser(description="Simulator data Smart Agriculture")
    p.add_argument("--url", default="http://localhost:8086")
    p.add_argument("--token", default="smart-agri-dev-token")
    p.add_argument("--org", default="smart-agri")
    p.add_argument("--bucket", default="agriculture")
    p.add_argument("--device", default="esp8266-garden-01")
    p.add_argument("--location", default="greenhouse-1")
    p.add_argument("--interval", type=float, default=5.0, help="detik antar titik")
    p.add_argument("--count", type=int, default=0, help="0 = tak terbatas")
    return p.parse_args()


def make_point(args, t, i):
    # Simulasi siklus harian: soil mengering lalu "disiram" tiap ~40 titik.
    cycle = i % 40
    soil = 75.0 - cycle * 1.6 + random.uniform(-1.5, 1.5)
    soil = max(5.0, min(100.0, soil))

    hour = (i * args.interval / 60.0) % 24
    air_temp = 24.0 + 6.0 * math.sin((hour - 6) / 24 * 2 * math.pi) + random.uniform(-0.5, 0.5)
    air_hum = 60.0 - 12.0 * math.sin((hour - 6) / 24 * 2 * math.pi) + random.uniform(-2, 2)
    light = max(0.0, 50000.0 * max(0.0, math.sin((hour - 6) / 12 * math.pi)) + random.uniform(-2000, 2000))

    # Prediksi sederhana
    slope = -1.6 * (60.0 / args.interval) / 60.0  # %/jam kira-kira
    hours_to_dry = (soil - 30.0) / -slope if slope < 0 and soil > 30 else -1.0
    score = 100.0
    if soil < 20: score -= 45
    elif soil < 30: score -= 25
    elif soil > 80: score -= 15
    if air_temp > 35: score -= 15
    if light < 5000: score -= 10
    score = max(0.0, min(100.0, score))
    irrig = soil < 30.0 or (0 <= hours_to_dry < 6.0)

    return (
        Point("agriculture")
        .tag("device", args.device)
        .tag("location", args.location)
        .field("soil_moisture", round(soil, 2))
        # ESP8266: ADC 10-bit (0-1023). soil_raw turun saat basah.
        .field("soil_raw", int(725 - soil * 4))
        .field("soil_voltage", round((725 - soil * 4) / 1023 * 3.3, 3))
        .field("air_temperature", round(air_temp, 2))
        .field("air_humidity", round(air_hum, 2))
        .field("light_lux", round(light, 1))
        .field("predict_slope", round(slope, 3))
        .field("predict_hours_to_dry", round(hours_to_dry, 2))
        .field("health_score", round(score, 1))
        .field("irrigation_needed", bool(irrig))
        .time(t, WritePrecision.NS)
    )


def main():
    args = parse_args()
    client = InfluxDBClient(url=args.url, token=args.token, org=args.org)
    write_api = client.write_api(write_options=SYNCHRONOUS)

    print(f"Kirim ke {args.url} org={args.org} bucket={args.bucket}")
    print("Tekan Ctrl+C untuk berhenti.\n")

    i = 0
    try:
        while True:
            now = datetime.now(timezone.utc)
            point = make_point(args, now, i)
            write_api.write(bucket=args.bucket, record=point)
            soil = 75.0 - (i % 40) * 1.6
            print(f"[{i:04d}] {now:%H:%M:%S} soil~{soil:5.1f}% terkirim")
            i += 1
            if args.count and i >= args.count:
                break
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nDihentikan.")
    finally:
        client.close()


if __name__ == "__main__":
    main()
