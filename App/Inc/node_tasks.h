#ifndef NODE_TASKS_H
#define NODE_TASKS_H

#include "types.h"

void Start_canfdTXTask(void *argument);
void Start_rpmEvalTask(void *argument);
void Update_Simulated_Sensors(void);

#endif