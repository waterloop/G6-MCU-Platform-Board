#include "can.hpp"
#include "string.h"

#define CHECK_MASK(bitset, mask) (((bitset) & (mask)) == (mask))

static uint8_t messageMarkerGenerator = 0;

static CanDriverLocks can_driver_locks;

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
        identifier = Id::RelayFaultDetectedId;
        break;
    case 0x01:
        identifier = Id::BmsFaultDetectedId;
        break;
    case 0x02:
        identifier = Id::McFaultDetectedId;
        break;
    case 0x03:
        identifier = Id::LVSensingFaultDetectedId;
        break;
    default:
        identifier = Id::DefaultRx;
    }
}

void CanMessage::set_id(CanMessageId id) { identifier = id; }

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


void FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);
void FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs);

void CanDriver::initialize(OperatingMode initial_operating_mode) {
    /**
     * CRITICAL SECTION: This initializer should only be called before
     * starting any threaded activities. But in the case that it is not,
     * we have placed it in a critical section to prevent it from being
     * initialized twice.
    */
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

        auto status = HAL_FDCAN_Stop(&can_handle);
        if (status != HAL_OK) { Error_Handler(); }
        if (HAL_FDCAN_RegisterRxFifo0Callback(&can_handle, &FDCAN_RxFifo0Callback) != HAL_OK) {
            Error_Handler();
        }
        if (HAL_FDCAN_RegisterRxFifo1Callback(&can_handle, &FDCAN_RxFifo1Callback) != HAL_OK) {
            Error_Handler();
        }
        status = HAL_FDCAN_Start(&can_handle);
        if (status != HAL_OK) { Error_Handler(); }
    }
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
    CanMessage msg(CanMessageId::RelayFaultDetectedId, data, 8);
    write(msg);
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
    msg = CanMessage(CanMessageId::LVSensingFaultDetectedId, data, 8);
    write(msg);
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

    uint32_t returnValue;
    if (!driver_locks.tx_lock.criticalSection([&]() {
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
        returnValue = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(&can_handle);
    })) { Error_Handler(); };

    return returnValue;
}

void CanDriver::await_write(uint32_t &txId) {
    while (HAL_FDCAN_IsTxBufferMessagePending(&can_handle, txId));
}

bool CanDriver::read(RxCanMessage &msg, CanRxFifo rxFifo, uint32_t timeout ) {
    switch (rxFifo) {
    case CanRxFifo::APP_FIFO0:
        if (!driver_locks.rx_fifo0.acquire(timeout)) {
            Error_Handler();
        }
        break;
    case CanRxFifo::PLATFORM_FIFO1:
        if (driver_locks.rx_fifo1.acquire(timeout)) {
            Error_Handler();
        }
        break;
    }
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

bool CanDriver::enable_interrupts() {
    driver_locks.rx_fifo0 = Semaphore::New(3, 0);
    driver_locks.rx_fifo1 = Semaphore::New(3, 0);
    driver_locks.tx_lock  = Mutex::New();
    if (!driver_locks.rx_fifo0.isInitialized() || !driver_locks.rx_fifo1.isInitialized()) {
        return false;
    }

    uint32_t interrupts = 0;
    interrupts |= FDCAN_IT_RX_FIFO0_MESSAGE_LOST;
    interrupts |= FDCAN_IT_RX_FIFO1_MESSAGE_LOST;
    interrupts |= FDCAN_IT_RX_FIFO0_NEW_MESSAGE;
    interrupts |= FDCAN_IT_RX_FIFO1_NEW_MESSAGE;
    auto status = HAL_FDCAN_ActivateNotification(&can_handle, interrupts, 0);
    return status == HAL_OK;
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
    driver_locks(can_driver_locks),
    operating_mode(OperatingMode::InternalLoopback),
    num_filters(0) {}


void FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs) {
#ifdef DEBUG
#if DEBUG > 0
    if (!can_driver_locks.rx_fifo1.isInitialized()) {
        Error_Handler();
    }
#endif
#endif
    if (CHECK_MASK(RxFifo1ITs, FDCAN_IT_RX_FIFO1_MESSAGE_LOST)) {
        Error_Handler();
    }
    if (CHECK_MASK(RxFifo1ITs, FDCAN_IT_RX_FIFO1_NEW_MESSAGE)) {
        if(!can_driver_locks.rx_fifo1.release()) {
            Error_Handler();
        }
    }
}

void FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
#ifdef DEBUG
#if DEBUG > 0
    if (!can_driver_locks.rx_fifo0.isInitialized()) {
        Error_Handler();
    }
#endif
#endif

    if (CHECK_MASK(RxFifo0ITs, FDCAN_IT_RX_FIFO0_MESSAGE_LOST)) {
        Error_Handler();
    }
    if (CHECK_MASK(RxFifo0ITs, FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) {
        if (!can_driver_locks.rx_fifo0.release()) {
            Error_Handler();
        }
    }
}
