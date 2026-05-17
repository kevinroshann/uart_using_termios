Demonstrates Linux UART serial communication using the POSIX termios API.
Configures UART settings with 115200 8N1 and RAW communication mode.
Supports UART data transmission and reception with timeout-based select() handling.
Includes basic error handling for reliable serial communication.
Build using gcc index.c -o index and run with ./index /dev/ttyUSB0.

