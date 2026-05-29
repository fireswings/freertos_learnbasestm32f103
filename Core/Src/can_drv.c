#include "can_drv.h"
#include "cmsis_os.h"

CAN_HandleTypeDef hcan;

static CAN_TxHeaderTypeDef   tx_header;
static CAN_RxHeaderTypeDef   rx_header;
static CAN_Msg_t             can_msg_ring[CAN_MSG_RING_SIZE];
static volatile uint8_t      can_msg_head = 0, can_msg_tail = 0;

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA12 = CAN_TX */
    GPIO_InitStruct.Pin = CAN_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CAN_TX_GPIO_Port, &GPIO_InitStruct);

    /* PA11 = CAN_RX */
    GPIO_InitStruct.Pin = CAN_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(CAN_RX_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

static uint8_t CAN_CalcTiming(uint32_t baudrate, CAN_InitTypeDef *init)
{
    /* APB1=36MHz, Prescaler=BRP+1 即有效分频值(1-1024) */
    static const struct {
        uint32_t baud;
        uint16_t prescaler;
        uint32_t bs1;
        uint32_t bs2;
    } table[] = {
        /* BaudRate=36M/(Prescaler*(1+BS1+BS2)), SP=(1+BS1)/(1+BS1+BS2) */
        {1000000,  2, CAN_BS1_12TQ, CAN_BS2_5TQ},   /* 36M/2/18=1M,   SP=72.2% */
        {500000,   4, CAN_BS1_12TQ, CAN_BS2_5TQ},   /* 36M/4/18=500k, SP=72.2% */
        {250000,   8, CAN_BS1_12TQ, CAN_BS2_5TQ},   /* 36M/8/18=250k, SP=72.2% */
        {125000,  16, CAN_BS1_12TQ, CAN_BS2_5TQ},   /* 36M/16/18=125k,SP=72.2% */
    };

    for (uint8_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if (table[i].baud == baudrate)
        {
            init->Prescaler = table[i].prescaler;
            init->SyncJumpWidth = CAN_SJW_1TQ;
            init->TimeSeg1 = table[i].bs1;
            init->TimeSeg2 = table[i].bs2;
            return 1;
        }
    }
    return 0;
}

uint8_t CAN_DRV_Init(uint32_t baudrate)
{
    CAN_InitTypeDef can_init = {0};
    CAN_FilterTypeDef can_filter = {0};

    hcan.Instance = CAN1;

    if (!CAN_CalcTiming(baudrate, &can_init))
        return 0;

    can_init.Mode = CAN_MODE_NORMAL;
    can_init.AutoBusOff = ENABLE;
    can_init.AutoWakeUp = DISABLE;
    can_init.AutoRetransmission = ENABLE;
    can_init.ReceiveFifoLocked = DISABLE;
    can_init.TimeTriggeredMode = DISABLE;

    if (HAL_CAN_Init(&hcan) != HAL_OK)
        return 0;

    /* 过滤器: 接收所有消息 */
    can_filter.FilterIdHigh = 0;
    can_filter.FilterIdLow = 0;
    can_filter.FilterMaskIdHigh = 0;
    can_filter.FilterMaskIdLow = 0;
    can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    can_filter.FilterBank = 0;
    can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter.FilterActivation = ENABLE;
    can_filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan, &can_filter) != HAL_OK)
        return 0;

    /* 使能 RX FIFO0 中断 */
    if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
        return 0;

    if (HAL_CAN_Start(&hcan) != HAL_OK)
        return 0;

    return 1;
}

uint8_t CAN_DRV_SendMsg(CAN_Msg_t *msg)
{
    uint32_t mailbox;

    tx_header.StdId = (msg->ide == CAN_ID_STD) ? msg->id : 0;
    tx_header.ExtId = (msg->ide == CAN_ID_EXT) ? msg->id : 0;
    tx_header.IDE = msg->ide;
    tx_header.RTR = msg->rtr;
    tx_header.DLC = msg->dlc;
    tx_header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(&hcan, &tx_header, msg->data, &mailbox) != HAL_OK)
        return 0;

    return 1;
}

uint8_t CAN_DRV_RecvMsg(CAN_Msg_t *msg, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (can_msg_head == can_msg_tail)
    {
        if (HAL_GetTick() - start > timeout_ms)
            return 0;
        osDelay(1);
    }

    __disable_irq();
    msg->id   = can_msg_ring[can_msg_tail].id;
    msg->ide  = can_msg_ring[can_msg_tail].ide;
    msg->rtr  = can_msg_ring[can_msg_tail].rtr;
    msg->dlc  = can_msg_ring[can_msg_tail].dlc;
    for (uint8_t i = 0; i < msg->dlc; i++)
        msg->data[i] = can_msg_ring[can_msg_tail].data[i];
    can_msg_tail = (can_msg_tail + 1) % CAN_MSG_RING_SIZE;
    __enable_irq();

    return 1;
}

uint8_t CAN_DRV_AvailMsg(void)
{
    return (can_msg_head != can_msg_tail);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    uint8_t data[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, data) != HAL_OK)
        return;

    uint8_t next = (can_msg_head + 1) % CAN_MSG_RING_SIZE;
    if (next == can_msg_tail) return;  /* 缓冲区满，丢弃 */

    can_msg_ring[can_msg_head].id  = (rx_header.IDE == CAN_ID_STD)
                                     ? rx_header.StdId : rx_header.ExtId;
    can_msg_ring[can_msg_head].ide = rx_header.IDE;
    can_msg_ring[can_msg_head].rtr = rx_header.RTR;
    can_msg_ring[can_msg_head].dlc = rx_header.DLC;
    for (uint8_t i = 0; i < rx_header.DLC; i++)
        can_msg_ring[can_msg_head].data[i] = data[i];

    can_msg_head = next;
}
