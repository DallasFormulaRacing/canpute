#ifndef NODE_TASKS_H
#define NODE_TASKS_H

#include "types.h"

void start_canfd_tx(void *argument);
void start_canfd_rx(void *argument);
void start_wheel_speed(void *argument);
void start_tire_temp(void *argument);
void StandaloneTimer_Callback(void *argument);

#endif