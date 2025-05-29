west build -p -b thingy52/nrf52832 <filepath> <br>

To build with bootloader:
`west build -p -b thingy52/nrf52832 --sysbuild <filepath>`<br>
`west sign -t imgtool -- --key <path to key.pem>`