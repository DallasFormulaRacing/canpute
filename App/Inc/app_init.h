/* App/Inc/app_init.h */
#ifndef APP_INIT_H
#define APP_INIT_H

#include "stm32h5xx_hal.h"
#include "types.h"

/**
 * @brief Global identity of the current node, established during boot.
 * This is defined in app_init.c and used throughout the application.
 */
extern NodeHardwareID_t self_node_id;

/**
 * @brief Entry point for all non-RTOS hardware logic and identity discovery.
 * This should be called in main.c within the USER CODE BEGIN 2 block, 
 * after peripheral MX_Init functions but before osKernelStart().
 */
void App_Hardware_Init(void);

/**
 * @brief Configures FDCAN hardware filters based on the discovered identity.
 * Sets up a dual-ID filter to accept messages for self_node_id and 
 * the NODE_ID_ALL_NODES (broadcast).
 */
void Configure_FDCAN_Filters(void);

/**
 * @brief Reads the STM32 unique ID and maps it to a predefined NodeHardwareID_t.
 * If no match is found, self_node_id is set to NODE_ID_UNKNOWN.
 */
void Identify_Self(void);

#endif /* APP_INIT_H */