#ifndef __CAN_DRV_H
#define __CAN_DRV_H

#include "main.h"

#define CAN_MSG_RING_SIZE 16

typedef struct {
    uint32_t id;
    uint8_t ide;
    uint8_t rtr;
    uint8_t dlc;
    uint8_t data[8];
} CAN_Msg_t;

uint8_t CAN_DRV_Init(uint32_t baudrate);
uint8_t CAN_DRV_SendMsg(CAN_Msg_t *msg);
uint8_t CAN_DRV_RecvMsg(CAN_Msg_t *msg, uint32_t timeout_ms);
uint8_t CAN_DRV_AvailMsg(void);

extern CAN_HandleTypeDef hcan;

#endif
