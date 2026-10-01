#!/usr/bin/env bash
# ST-Link GDB server on :61234. Leave running, then `./gdb.sh` in another terminal.
CLT=/opt/st/stm32cubeclt_1.22.0
"$CLT/STLink-gdb-server/bin/ST-LINK_gdbserver" -p 61234 -d -s -cp "$CLT/STM32CubeProgrammer/bin" -m 1 -i 002900064142501820353451
