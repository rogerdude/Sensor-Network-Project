#!/usr/bin/env python3
import time
import json
import random               # just for demo
import paho.mqtt.client as mqtt

BROKER = "10.0.0.3"
PORT   = 1883
TOPIC  = "m5core2/geo_severity"
QOS    = 1

def get_sensor_data():
    """
    Replace this with your real data-acquisition logic.
    Here we just generate random coords + severity.
    """
    return {
        "longitude": 153.0 + random.random()*0.01,
        "latitude": -27.4  + random.random()*0.01,
        "severity": random.randint(0,5)
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
