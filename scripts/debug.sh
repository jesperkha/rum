# Run both gdb and rum in seperate windows to debug
start ./rum.exe

PID=$(tasklist | grep -Fa "rum.exe" | awk '{print $2}')
start gdb -p $PID --tui -ex "break main" -ex "set print pretty on"