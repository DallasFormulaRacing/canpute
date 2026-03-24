#ifndef NODE_TASKS_H
#define NODE_TASKS_H

#include "types.h"

void Start_canfdTXTask(void *argument);
void Start_rpmEvalTask(void *argument);
void StartTireTempTask(void *argument);
void StandaloneTimer_Callback(void *argument);

#endif