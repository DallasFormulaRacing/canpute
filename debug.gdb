# Run ./gdbserver.sh in one terminal, then ./gdb.sh in another
file build/Debug/canpute.elf
target extended-remote :61234
load
monitor reset
