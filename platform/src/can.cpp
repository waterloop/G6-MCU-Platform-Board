#include "can.hpp"
#include "string.h"

static uint8_t messageMarkerGenerator = 0;
static constexpr uint8_t dlc_to_data_length[16] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    12,
    16,
    20,
    24,
    32,
    48,
    64
};

CanMessageFilter::CanMessageFilter()
    : filter{
        0,
        0,
        0,
        0,
        0,
        0,
    } {}

CanMessageFilter::CanMessageFilter(uint32_t filter_type,
                                   uint32_t filter_config,
                                   uint32_t filter_id1,
                                   uint32_t filter_id2)
    : filter{
        FDCAN_STANDARD_ID,
        0, //< Late INIT (Filter Index)
        filter_type,
        filter_config,
        filter_id1,
        filter_id2,
    } {}

CanMessage::CanMessage(CanMessageId id,
                            uint8_t *data,
                            uint32_t data_length)
    : identifier(id),
      message_marker(messageMarkerGenerator++),
      data(data),
      data_length(data_length) {}

void CanMessage::set_id(uint32_t id) {
    using Id = CanMessageId;
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

void CanMessage::set_id(CanMessageid id) { identifier = id; }

void CanMessage::set_ESI(uint32_t esi) {
    using ESI = CanMessage::ESI;
    switch (esi) {
    case FDCAN_ESI_ACTIVE:
        error_state_indicator = ESI::ERROR_ACTIVE;
        break;
    case FDCAN_ESI_PASSIVE:
        error_state_indicator = ESI::ERROR_PASSIVE;
        break;
    default:
        error_state_indicator = ESI::UNKNOWN_ERROR_STATE;
    }
};

RxCanMessage::RxCanMessage(uint8_t *data,
                            uint32_t dataLength)
                            : CanMessage(CanMessageId::DefaultRx, data, dataLength) {}


void CanDriver::initialize(OperatingMode initial_operating_mode) {
    /**
     * CRITICAL SECTION: This initializer should only be called before
     * starting any threaded activities. But in the case that it is not,
     * we have placed it in a critical section to prevent it from being
     * initialized twice.
    */
    __disable_irq();
    if (!initialized) {
        if (&can_handle == &hfdcan1) {
            MX_FDCAN1_Init();
        } else {
            Error_Handler();
        }

        initialized = true;
        /// Use the setter here so that we don't have to update the CubeMX
        /// Initializer function when ever we want to initialize into a different
        /// mode. This also allows us to store the enumerated value of OperatingMode
        /// so it is available when calling get_operating_mode();
        if (!set_operating_mode(initial_operating_mode)) {
            Error_Handler();
        }
    }
    __enable_irq();
}

void CanDriver::test_driver() {
    // Cache opmode then switch into internal loop back for test
    auto current_operating_mode = get_operating_mode();
    if (current_operating_mode != OperatingMode::InternalLoopback) {
        if (!set_operating_mode(OperatingMode::InternalLoopback)) {
            Error_Handler();
        }
    }
    // RUN Test
    uint8_t data[8];
    uint8_t rx_data[64];
    for (int i = 0; i < 8; i++) {
        data[i] = i;
        rx_data[i] = 0;
    }
    CanMessage msg(CanMessageId::RelayFaultDetected, data, 8);
    uint32_t id = write(msg);
    await_write(id);
    while (!read_ready(CanRxFifo::FIFO0));
    RxCanMessage rxmsg(rx_data, 64);
    if (!read(rxmsg)) { Error_Handler(); }
    for (int i = 0; i < 8; i++) {
        if (data[i] != rx_data[i]) {
            Error_Handler();
        }
    }

    // Check another message id
    for (int i = 0; i < 8; i++) {
        rx_data[i] = 0;
    }
    msg = CanMessage(CanMessageId::LVSensingFaultDetected, data, 8);
    id = write(msg);
    await_write(id);
    while (!read_ready(CanRxFifo::FIFO0));
    rxmsg = RxCanMessage(rx_data, 64);
    if (!read(rxmsg)) { Error_Handler(); }
    for (int i = 0; i < 8; i++) {
        if (data[i] != rx_data[i]) {
            Error_Handler();
        }
    }


    // Restore previous op mode
    if (current_operating_mode != OperatingMode::InternalLoopback) {
        if (!set_operating_mode(current_operating_mode)) {
            Error_Handler();
        }
    }
}

CanDriver::OperatingMode CanDriver::get_operating_mode() const {
    return operating_mode;
}

bool CanDriver::set_operating_mode(OperatingMode new_operating_mode) {
    HAL_FDCAN_Stop(&can_handle);
    /* Reset FDCAN Operation Mode */
    CLEAR_BIT(can_handle.Instance->CCCR, (FDCAN_CCCR_TEST | FDCAN_CCCR_MON | FDCAN_CCCR_ASM));
    CLEAR_BIT(can_handle.Instance->TEST, FDCAN_TEST_LBCK);
    switch (new_operating_mode) {
    case OperatingMode::RestrictedOperation:
        SET_BIT(can_handle.Instance->CCCR, FDCAN_CCCR_ASM);
        break;
    case OperatingMode::InternalLoopback:
        SET_BIT(can_handle.Instance->CCCR, FDCAN_CCCR_MON);
        SET_BIT(can_handle.Instance->CCCR, FDCAN_CCCR_TEST);
        SET_BIT(can_handle.Instance->TEST, FDCAN_TEST_LBCK);
        break;
    case OperatingMode::ExternalLoopback:
        SET_BIT(can_handle.Instance->CCCR, FDCAN_CCCR_TEST);
        SET_BIT(can_handle.Instance->TEST, FDCAN_TEST_LBCK);
        break;
    case OperatingMode::BusMonitoring:
        SET_BIT(can_handle.Instance->CCCR, FDCAN_CCCR_MON);

    case OperatingMode::Normal:
    default:
    break;
    }

    return HAL_FDCAN_Start(&can_handle) == HAL_OK;
}

uint32_t CanDriver::write(CanMessage &msg) {
    static uint8_t extra_buffer[64] = {0};

    auto dlc = get_data_length_code_from_byte_length(msg.data_length);
    FDCAN_TxHeaderTypeDef header = {
        .Identifier = (uint32_t)msg.identifier,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = dlc,
        .ErrorStateIndicator = (uint32_t)msg.error_state_indicator,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_FD_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS, // NOTE: For now. Don't store any tx events. May parameterize this later if needed.
        .MessageMarker = msg.message_marker
    };

    /**
     * @brief Since we cannot cover the full range of values from
     * 0 to 64 in the dlc, we have to round the length of the message
     * up to the next nearest size. To prevent reading past the end of
     * the buffer when we do this round up, we first copy the message into
     * a buffer which the HAL can safely read to the full length of the DLC.
     *
     */
    if (msg.data_length != dlc_to_data_length[dlc >> 16]) {
        memcpy(extra_buffer, msg.data, msg.data_length);
        HAL_FDCAN_AddMessageToTxFifoQ(&can_handle, &header, extra_buffer);
    }
    else {
        HAL_FDCAN_AddMessageToTxFifoQ(&can_handle, &header, msg.data);
    }
    return HAL_FDCAN_GetLatestTxFifoQRequestBuffer(&can_handle);
}

void CanDriver::await_write(uint32_t &txId) {
    while (HAL_FDCAN_IsTxBufferMessagePending(&can_handle, txId));
}

bool CanDriver::read_ready(CanRxFifo rxFifo ) {
    return HAL_FDCAN_GetRxFifoFillLevel(&can_handle, (uint32_t)rxFifo) > 0;
}

bool CanDriver::read(RxCanMessage &msg, CanRxFifo rxFifo ) {
    if (!read_ready(rxFifo)) { return false; }
    FDCAN_RxHeaderTypeDef rxHeader;
    if (HAL_FDCAN_GetRxMessage(&can_handle,
                               (uint32_t)rxFifo,
                               &rxHeader,
                               msg.data) != HAL_OK) {
        return false;
    }
    msg.data_length = dlc_to_data_length[rxHeader.DataLength >> 16];
    msg.set_id(rxHeader.Identifier);
    msg.set_ESI(rxHeader.ErrorStateIndicator);
    msg.filter_index = rxHeader.FilterIndex;
    return true;
}

bool CanDriver::match_all_ids() {
    if (HAL_FDCAN_Stop(&can_handle) != HAL_OK) { return false; }
    HAL_FDCAN_ConfigGlobalFilter(&can_handle,
    FDCAN_ACCEPT_IN_RX_FIFO0,
    FDCAN_ACCEPT_IN_RX_FIFO0,
    FDCAN_REJECT_REMOTE,
    FDCAN_REJECT_REMOTE);
    return HAL_FDCAN_Start(&can_handle) == HAL_OK;
}

bool CanDriver::push_filter(CanMessageFilter &filter) {
    if (num_filters == MAX_NUM_FILTERS) { return false; }
    filter.filter.FilterIndex = num_filters;
    message_filters[num_filters] = filter;
    auto result = HAL_FDCAN_ConfigFilter(&can_handle, &(filter.filter));
    num_filters++;
    return result == HAL_OK;
}

uint32_t CanDriver::get_data_length_code_from_byte_length(uint32_t byte_length) {
    if (byte_length <= 8) {
        return (byte_length << 16);
    }
    if (byte_length <= 12) {
        return FDCAN_DLC_BYTES_12;
    }
    if (byte_length <= 16) {
        return FDCAN_DLC_BYTES_16;
    }
    if (byte_length <= 20) {
        return FDCAN_DLC_BYTES_20;
    }
    if (byte_length <= 24) {
        return FDCAN_DLC_BYTES_24;
    }
    if (byte_length <= 32) {
        return FDCAN_DLC_BYTES_32;
    }
    if (byte_length <= 48) {
        return FDCAN_DLC_BYTES_48;
    }
    if (byte_length <= 64) {
        return FDCAN_DLC_BYTES_64;
    }
    return 0; // if the message is too big, then don't send anything
}

CanDriver::CanDriver()
    : can_handle(hfdcan1),
    operating_mode(OperatingMode::InternalLoopback),
    num_filters(0) {}