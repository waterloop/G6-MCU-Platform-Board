#include "can.hpp"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx.h"

#include "main.h"
static FDCAN_HandleTypeDef hfdcan1;
static uint8_t messageMarkerGenerator = 0;

CanDriver::CanDriver()
    : canHandle(hfdcan1) {
    canHandle.Instance = FDCAN1;
    canHandle.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    canHandle.Init.FrameFormat = FDCAN_FRAME_FD_NO_BRS;
    canHandle.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
    canHandle.Init.AutoRetransmission = DISABLE;
    canHandle.Init.TransmitPause = DISABLE;
    canHandle.Init.ProtocolException = DISABLE;
    canHandle.Init.NominalPrescaler = 1;
    canHandle.Init.NominalSyncJumpWidth = 1;
    canHandle.Init.NominalTimeSeg1 = 2;
    canHandle.Init.NominalTimeSeg2 = 2;
    canHandle.Init.DataPrescaler = 1;
    canHandle.Init.DataSyncJumpWidth = 1;
    canHandle.Init.DataTimeSeg1 = 1;
    canHandle.Init.DataTimeSeg2 = 1;
    canHandle.Init.StdFiltersNbr = 0;
    canHandle.Init.ExtFiltersNbr = 0;
    canHandle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&canHandle) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_FDCAN_Start();
}

bool CanDriver::setOperatingMode(FDCANOperatingMode newOperatingMode) {
    HAL_FDCAN_Stop(&canHandle);
    /* Reset FDCAN Operation Mode */
    CLEAR_BIT(canHandle.Instance->CCCR, (FDCAN_CCCR_TEST | FDCAN_CCCR_MON | FDCAN_CCCR_ASM));
    CLEAR_BIT(canHandle.Instance->TEST, FDCAN_TEST_LBCK);
    switch (newOperatingMode) {
    case FDCANOperatingMode::RestrictedOperation:
        SET_BIT(canHandle.Instance->CCCR, FDCAN_CCCR_ASM);
        break;
    case FDCANOperatingMode::InternalLoopback:
        SET_BIT(canHandle.Instance->CCCR, FDCAN_CCCR_MON);
        SET_BIT(canHandle.Instance->CCCR, FDCAN_CCCR_TEST);
        SET_BIT(canHandle.Instance->TEST, FDCAN_TEST_LBCK);
        break;
    case FDCANOperatingMode::ExternalLoopback:
        SET_BIT(canHandle.Instance->CCCR, FDCAN_CCCR_TEST);
        SET_BIT(canHandle.Instance->TEST, FDCAN_TEST_LBCK);
        break;
    case FDCANOperatingMode::BusMonitoring:
        SET_BIT(canHandle.Instance->CCCR, FDCAN_CCCR_MON);

    case FDCANOperatingMode::Normal:
    default:
    break;
    }

    return HAL_FDCAN_Start(&canHandle) == HAL_OK;
}

uint32_t CanDriver::write(Message &msg) {
    FDCAN_TxHeaderTypeDef header = {
        .Identifier = msg.identifier,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = msg.dataLength,
        .ErrorStateIndicator = msg.errorStateIndicator,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_FD_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS, // NOTE: For now. Don't store any tx events. May parameterize this later if needed.
        .MessageMarker = msg.messageMarker
    };
    HAL_FDCAN_AddMessageToTxFifoQ(&canHandle, &header, msg.data);
    return HAL_FDCAN_GetLatestTxFifoQRequestBuffer(&canHandle);
}

void CanDriver::awaitWrite(uint32_t &txId) {
    while (HAL_FDCAN_IsTxBufferMessagePending(&canHandle, txId));
}

bool CanDriver::canRead(uint32_t rxFifo ) {
    return HAL_FDCAN_GetRxFifoFillLevel(&canHandle, rxFifo) > 0;
}

bool CanDriver::read(RxMessage &msg, uint32_t rxFifo ) {
    if (!canRead(rxFifo)) { return false; }
    FDCAN_RxHeaderTypeDef rxHeader;
    HAL_FDCAN_GetRxMessage(&canHandle, rxFifo, &rxHeader, msg.data);
    msg.dataLength = rxHeader.DataLength;
    msg.identifier = rxHeader.Identifier;
    msg.errorStateIndicator = rxHeader.ErrorStateIndicator;
    msg.filterIndex = rxHeader.FilterIndex;
    return true;
}

CanDriver::Message::Message(CanDriver::Message::Id id,
                            uint8_t *data,
                            uint32_t dataLength)
    : identifier(id),
      messageMarker(messageMarkerGenerator++),
      data(data),
      dataLength(dataLength) {}

void CanDriver::Message::setId(uint32_t id) {
    using Id = CanDriver::Message::Id;
    switch (id) {
    case 0x00:
        identifier = Id::RelayFaultDetected;
        break;
    case 0x01:
        identifier = Id::BmsFaultDetected;
        break;
    case 0x02:
        identifier = Id::McFaultDetected;
        break;
    case 0x03:
        identifier = Id::LVSensingFaultDetected;
        break;
    default:
        identifier = Id::DefaultRx;
    }
}

void CanDriver::Message::setESI(uint32_t esi) {
    using ESI = CanDriver::Message::ESI;
    switch (esi) {
    case FDCAN_ESI_ACTIVE:
        errorStateIndicator = ESI::ERROR_ACTIVE;
        break;
    case FDCAN_ESI_PASSIVE:
        errorStateIndicator = ESI::ERROR_PASSIVE;
        break;
    default:
        errorStateIndicator = ESI::UNKNOWN_ERROR_STATE;
    }
};

CanDriver::RxMessage::RxMessage(uint8_t *data,
                            uint32_t dataLength)
                            : Message(Id::DefaultRx, data, dataLength) {}

