#!/usr/bin/env bash
# Build and flash over ST-Link using STM32CubeCLT.
set -e
cd "$(dirname "$0")"
CLT=/opt/st/stm32cubeclt_1.22.0
cmake --build build/Debug
"$CLT/STM32CubeProgrammer/bin/STM32_Programmer_CLI" -c port=SWD mode=UR sn=002900064142501820353451 -w build/Debug/canpute.elf -v -rst
