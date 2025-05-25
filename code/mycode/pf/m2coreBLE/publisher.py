#!/usr/bin/env python3
import time
import json
import random               # just for demo
import paho.mqtt.client as mqtt
import math
CENTER_LAT      = -27.499488
LON_DEG_PER_M   = 1.0 / (111000.0 * math.cos(math.radians(CENTER_LAT)))
CENTER_LON      = 153.014495
GRID_SPAN_M     = 100.0
LAT_DEG_PER_M   = 1.0 / 111000.0

BROKER = "127.0.0.1"
PORT   = 1883
TOPIC  = "m5core2/geo_severity"
QOS    = 1

def get_sensor_data():
    # pick a random offset in metres within ±100 m
    lat_off_m = random.uniform(-GRID_SPAN_M, GRID_SPAN_M)
    lon_off_m = random.uniform(-GRID_SPAN_M, GRID_SPAN_M)

    return {
        "latitude":  CENTER_LAT  + lat_off_m * LAT_DEG_PER_M,
        "longitude": CENTER_LON  + lon_off_m * LON_DEG_PER_M,
        "severity":  random.randint(0,2)  # 0=green,1=yellow,2=red
    }

def main():
    client = mqtt.Client()
    client.connect(BROKER, PORT, keepalive=60)

    try:
        while True:
            data = get_sensor_data()
            payload = json.dumps(data)
            client.publish(TOPIC, payload, qos=QOS)
            print(f"Published: {payload}")
            time.sleep(1)
    except KeyboardInterrupt:
        print("Exiting…")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
