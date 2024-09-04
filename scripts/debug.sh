# Run both gdb and rum in seperate windows to debug
start ./rum.exe

PID=$(tasklist | grep -Fa "rum.exe" | awk '{print $2}')
start gdb -p $PID