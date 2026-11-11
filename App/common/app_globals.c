#include "app_globals.h"

NodeHardwareID_t self_node_id = NODE_ID_UNKNOWN;
TemperatureSpeedData_t temperatureSpeedData;

volatile uint32_t can_tx_status;
volatile uint32_t can_tx_error;
volatile uint32_t can_tx_free;
volatile uint32_t debug_rx_level = 0U;
volatile uint32_t debug_notify_status = 0U;
