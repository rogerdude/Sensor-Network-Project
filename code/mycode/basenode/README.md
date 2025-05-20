### Task 3: Command Line Interface Shell
This task creates a command-line interface shell accessible via a serial terminal. The shell provides commands for controlling LEDs and displaying system uptime.  
- **Functionality Achieved**:
  - A shell command `led` allows users to set or toggle LEDs using a bitmask.
  - Another shell command `time` displays the system uptime in seconds or a formatted `hh:mm:ss` format.
  - The shell is implemented using Zephyr's shell subsystem.


## User Instructions
1- Compile using west build -b disco_l475_iot1 mycode/apps/prac1/task2 --pristine=always --sysbuild -Dmcuboot_EXTRA_DTC_OVERLAY_FILE="/home/brain/repo/mycode/apps/prac1/task2/boards/disco_l475_iot1.overlay"

2 - Flash using  west flash --runner jlink

3 - Input commands via screen /dev/ttyACM0 115200
 
