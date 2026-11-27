# canpute on the ev27-can main bus

canpute now speaks ev27-can (the generated CAN database) next to its old DFR
29-bit commands and the ezfd bootloader protocol.

What it does:

- `ev27_node` from ev27-can, used as is: Status every 500 ms at 0x700 + address,
  CM5_Command (Mute, Unmute, RebootToBootloader, Identify), CM5_TimeSync car time.
- Imu at 0x400 + slot: the average of each 5-sample FIFO batch (about 83 Hz),
  gyro bias removed (MotionGC).
- TireTemp at 0x420 + slot: the 16 sensor columns averaged over the 12 rows.

Where it hooks in:

| File | Change |
| --- | --- |
| `App/ev27/ev27_daq.c` | ev27_port_* for ev27_node, address/slot mapping, Imu/TireTemp packing, 1 MHz TIM2 |
| `App/ev27/ev27_daq_config.h` | firmware version, RX IDs, sensor bits, bench slot |
| `App/can/can_utils.c` | RX interrupt stamps each frame with the µs timer and its length/ID type; TX mutex shared with ev27 |
| `App/can/can_task.c` | 10 ms TX loop (Status, Identify LED, 1 Hz ezfd heartbeat); RX splits 11-bit (ev27) from 29-bit (old/ezfd) |
| `App/common/app_init.c` | standard-ID filters for TimeSync and Command |
| `Core/Src/fdcan.c` | `StdFiltersNbr = 2` |
| `CMakeLists.txt` | builds `ev27_main.c` and `ev27_node.c` from `../ev27-can`, git hash into Status |

Addresses: a corner in `Fleet_Table` gets its ev27 address (FL 0x10 ... RR 0x13)
and slot. Any other board keeps its own address (0x1F on the bench) and sends
data as the FL slot (`EV27_BENCH_SLOT`).

Watch out:

- **CubeMX:** `StdFiltersNbr = 2` is a hand edit in `fdcan.c`. Set "Std Filters
  Nbr" to 2 in `canpute.ioc` too, or the next regeneration resets it to 0 and
  `ev27_daq_config_filters()` ends in `Error_Handler()` at boot.
- **ev27-can location:** the build expects a checkout at `../ev27-can`
  (`-DEV27_CAN_DIR=...` to change). A `lib/ev27-can` submodule is the
  long-term plan from ev27-can's README.
- **IMU axes** are as mounted, not the vehicle frame (no config record yet).
  The gyro is set to +-2000 dps but the message holds +-286 dps, so it is clamped.
- **Tire zone order:** column 0 is sent as the inner edge. Flip in
  `tire_temp_task.c` if the sensor is mounted the other way.
- **Bootloader address:** the ezfd bootloader only knows the Nucleo UIDs, so a
  real corner board is 0x1F in the bootloader but 0x10-0x13 in the app. Put the
  corner UIDs in the bootloader's table with their ev27 addresses before
  flashing corners on the car.
- **Data bit rate:** ev27-can lists the main bus at 1M/1M (TBD); canpute and the
  bootloader run 1M/5M. Frames are sent with BRS on.
