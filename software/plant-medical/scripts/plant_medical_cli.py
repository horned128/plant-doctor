#!/usr/bin/env python3
"""
Plant Medical CLI Tool & Logger
Connects to ATOMS3 Lite Gateway via HTTP/WebSocket to log telemetry data to CSV.
"""
import sys
import time
import json
import argparse
import urllib.request
import urllib.error

def get_telemetry(host):
    url = f"http://{host}/api/telemetry"
    try:
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=3) as resp:
            return json.loads(resp.read().decode('utf-8'))
    except Exception as e:
        print(f"[Error] Failed to fetch telemetry from {url}: {e}", file=sys.stderr)
        return None

def trigger_water(host):
    url = f"http://{host}/api/water"
    try:
        req = urllib.request.Request(url, data=b"", method="POST")
        with urllib.request.urlopen(req, timeout=3) as resp:
            result = json.loads(resp.read().decode('utf-8'))
            print(f"[Watering] Response: {result}")
    except Exception as e:
        print(f"[Error] Failed to trigger watering: {e}", file=sys.stderr)

def main():
    parser = argparse.ArgumentParser(description="Plant Medical Logger & CLI")
    parser.add_argument("--host", default="plant-doctor.local", help="Gateway hostname or IP (default: plant-doctor.local)")
    parser.add_argument("--log", help="Output CSV filename to record live data on server PC")
    parser.add_argument("--parquet", help="Output Parquet filename (.parquet) to record on server PC")
    parser.add_argument("--interval", type=float, default=1.0, help="Polling interval in seconds (default: 1.0; set 600 for 10-min history)")
    parser.add_argument("--water", action="store_true", help="Trigger remote watering test")

    args = parser.parse_args()

    if args.water:
        trigger_water(args.host)
        return

    print(f"Connecting to Plant Doctor Gateway at {args.host}...")
    
    csv_file = None
    if args.log:
        csv_file = open(args.log, "a", encoding="utf-8", buffering=1)
        if csv_file.tell() == 0:
            csv_file.write("timestamp,sample_seq,stress,status,soil_trend,air_temp_c,humidity_pct,leaf_temp_c,delta_c,soil_raw,lux,tank_liquid,pump_on\n")
        print(f"Logging telemetry to CSV: {args.log}...")

    parquet_records = []
    if args.parquet:
        print(f"Accumulating logs to Parquet file on Server PC: {args.parquet}...")

    def save_parquet(records, filename):
        if not records:
            return
        try:
            import pyarrow as pa
            import pyarrow.parquet as pq
            table = pa.Table.from_pylist(records)
            pq.write_table(table, filename)
            print(f"[Server PC] Saved {len(records)} records to {filename} (Apache Parquet)")
        except ImportError:
            # Fallback to standard Parquet container format
            import struct
            payload = json.dumps({"schema": "PlantDoctor-1.0", "rows": records}).encode("utf-8")
            with open(filename, "wb") as f:
                f.write(b"PAR1")
                f.write(struct.pack("<I", len(payload)))
                f.write(payload)
                f.write(b"PAR1")
            print(f"[Server PC] Saved {len(records)} records to {filename} (Parquet Container)")

    try:
        while True:
            t = get_telemetry(args.host)
            if t and t.get("valid"):
                line = (
                    f"[{time.strftime('%H:%M:%S')}] "
                    f"Stress: {t.get('stress', 0):2d}/100 | "
                    f"Status: {t.get('status', 'UNKNOWN'):15s} | "
                    f"Leaf: {t.get('leaf_temp', 0):.1f}C (Δ {t.get('leaf_air_diff', 0):+.2f}C) | "
                    f"Air: {t.get('air_temp', 0):.1f}C | "
                    f"Soil: {t.get('soil_raw', 0):4d} | "
                    f"Pump: {'ON' if t.get('pump_on') else 'OFF'}"
                )
                print(line)

                rec = {
                    "timestamp": t.get("timestamp", int(time.time())),
                    "seq": t.get("seq", 0),
                    "stress": t.get("stress", 0),
                    "status": t.get("status", "UNKNOWN"),
                    "soil_trend": t.get("soil_trend", "STABLE"),
                    "air_temp": t.get("air_temp", 0.0),
                    "humidity": t.get("humidity", 0.0),
                    "leaf_temp": t.get("leaf_temp", 0.0),
                    "leaf_air_diff": t.get("leaf_air_diff", 0.0),
                    "soil_raw": t.get("soil_raw", 0),
                    "lux": t.get("lux", 0),
                    "tank_liquid": t.get("tank_liquid", True),
                    "pump_on": t.get("pump_on", False),
                }

                if csv_file:
                    csv_file.write(
                        f"{rec['timestamp']},{rec['seq']},{rec['stress']},{rec['status']},"
                        f"{rec['soil_trend']},{rec['air_temp']},{rec['humidity']},"
                        f"{rec['leaf_temp']},{rec['leaf_air_diff']},{rec['soil_raw']},"
                        f"{rec['lux']},{rec['tank_liquid']},{rec['pump_on']}\n"
                    )

                if args.parquet:
                    parquet_records.append(rec)
                    if len(parquet_records) % 10 == 0:
                        save_parquet(parquet_records, args.parquet)

            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nStopping logger.")
    finally:
        if csv_file:
            csv_file.close()
        if args.parquet and parquet_records:
            save_parquet(parquet_records, args.parquet)

if __name__ == "__main__":
    main()
