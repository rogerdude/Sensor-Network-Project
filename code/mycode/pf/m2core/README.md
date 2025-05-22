west build -b m5stack_core2/esp32/procpu -s m2core --pristine


chmod +x modules/hal/espressif/tools/esptool_py/esptool.py


west blobs fetch hal_espressif


west flash


west espressif monitor


