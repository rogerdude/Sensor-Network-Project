west build -b m5stack_core2/esp32/procpu -s m2core --pristine


chmod +x modules/hal/espressif/tools/esptool_py/esptool.py


west blobs fetch hal_espressif


west flash


west espressif monitor


mosquitto_sub -h 127.0.0.1 -p 1883 -t m5core2/geo_severity -v
mosquitto_pub -h 127.0.0.1 -p 1883 -t m5core2/geo_severity -m "{lon:153.0210,lat:-27.4705,s:2}"
