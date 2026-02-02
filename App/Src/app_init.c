#include "app_init.h"
#include "types.h"
#include "fdcan.h"
#include "tim.h"
#include <string.h>

// Internal Registry of all physical nodes
static const UID_Mapping_t Fleet_Table[] = {
    {{0x12345678, 0xABCDEF01, 0x55667788}, NODE_ID_FRONT_LEFT},
    {{0x87654321, 0x10FEDCBA, 0x99887766}, NODE_ID_FRONT_RIGHT},
    {{0x11223344, 0x55667788, 0x99AABBCC}, NODE_ID_REAR_LEFT},
    {{0x44332211, 0x88776655, 0xCCBBAA99}, NODE_ID_REAR_RIGHT},
    {{0x001E005F, 0x33335101, 0x32313831}, NODE_ID_NUCLEO_1},
    {{0x4D3C2B1A, 0x80706050, 0x40302010}, NODE_ID_NUCLEO_2},
    {{0xDEADBEEF, 0xCAFEBABE, 0xFEEDFACE}, NODE_ID_DASH}
};

NodeHardwareID_t self_node_id = NODE_ID_UNKNOWN;

void Identify_Self(void) {
    uint32_t current_uid[3];
    
    current_uid[0] = HAL_GetUIDw0();
    current_uid[1] = HAL_GetUIDw1();
    current_uid[2] = HAL_GetUIDw2();

    for (int i = 0; i < sizeof(Fleet_Table)/sizeof(UID_Mapping_t); i++) {
        if (current_uid[0] == Fleet_Table[i].uid[0] &&
            current_uid[1] == Fleet_Table[i].uid[1] &&
            current_uid[2] == Fleet_Table[i].uid[2]) {
            
            self_node_id = Fleet_Table[i].nodeType;
            return;
        }
    }
    
    self_node_id = NODE_ID_UNKNOWN; // Node not recognized
}


/**
 * @brief Configures the FDCAN hardware filters for Targeted and Broadcast IDs
 */
void Configure_FDCAN_Filters(void) {
    FDCAN_FilterTypeDef filter1;
    filter1.IdType       = FDCAN_EXTENDED_ID;
    filter1.FilterIndex  = 0;
    filter1.FilterType   = FDCAN_FILTER_MASK; // Use the standard HAL define
    filter1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

    FDCAN_FilterTypeDef filter2;
    filter2.IdType       = FDCAN_EXTENDED_ID;
    filter2.FilterIndex  = 1;
    filter2.FilterType   = FDCAN_FILTER_MASK;
    filter2.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;

    // The Mask: 5 bits set at position 21
    // 0x1F << 21 = 0x03E00000
    uint32_t targetIDMask = (0x1F << 21);

    // Filter for messages addressed specifically to this node
    // Priority, Command, and Source are 0 because the Mask will ignore them
    filter1.FilterID1 = (uint32_t)self_node_id << 21; 
    filter1.FilterID2 = targetIDMask;

    // Filter for Broadcast messages (ALL DAQ NODES)
    filter2.FilterID1 = (uint32_t)NODE_ID_ALL_NODES << 21;
    filter2.FilterID2 = targetIDMask;

    if (HAL_FDCAN_ConfigFilter(&hfdcan2, &filter1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_FDCAN_ConfigFilter(&hfdcan2, &filter2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief User Application Hardware Initialization Entry Point
 */
void App_Hardware_Init(void) {
    /* 1. Establish Identity first */
    Identify_Self(); 

    /* 2. Configure ID filtering */
    Configure_FDCAN_Filters();

    /* 3. Global Filter: Reject everything else to save CPU cycles */
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan2,
        FDCAN_REJECT, FDCAN_REJECT,
        FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) {
        Error_Handler();
    }

    /* 4. Start peripherals and notifications */
    if (HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        Error_Handler();
    }
}