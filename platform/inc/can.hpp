#pragma once
#include "fdcan.h"
#include "stdint.h"
#include <type_traits>
#include "can_messages.h"
#include "utils.hpp"

enum class CanFilterConfiguration : uint32_t {
    Disable = FDCAN_FILTER_DISABLE,
    RxFIFO0 = FDCAN_FILTER_TO_RXFIFO0,
    RxFIFO1 = FDCAN_FILTER_TO_RXFIFO1,
    Reject = FDCAN_FILTER_REJECT,
    HighPriority = FDCAN_FILTER_HP,
    HighPriorityRxFIFO0 = FDCAN_FILTER_TO_RXFIFO0_HP,
    HighPriorityRxFIFO1 = FDCAN_FILTER_TO_RXFIFO1_HP,
};

enum class CanRxFifo : uint32_t {
    FIFO0 = FDCAN_RX_FIFO0,
    FIFO1 = FDCAN_RX_FIFO1,
};

// We may want to parameterize this in the future, but for now just use rxfifo 0
constexpr CanFilterConfiguration DEFAULT_FILTER_CONFIG = CanFilterConfiguration::RxFIFO0;
constexpr CanRxFifo DEFAULT_RX_FIFO = CanRxFifo::FIFO0;
constexpr uint32_t MAX_FILTER_ID = 0x7FF;
constexpr uint32_t MAX_NUM_FILTERS = 28;
 constexpr size_t CAN_MAX_DATA_LENGTH = 64;

class CanDriver;

class CanMessageFilter {
    friend class CanDriver; // gives CanDriver access to the filter;
    FDCAN_FilterTypeDef filter;

    CanMessageFilter();
    CanMessageFilter(uint32_t filter_type,
                     uint32_t filter_config,
                     uint32_t filter_id1,
                     uint32_t filter_id2);
public:
    /**
     * @brief Create a Range Filter
     * Creates a filter which matches messages with ids
     * id_range_start to id_range_end, including these two ids
     *
     * @param id_range_start
     * @param id_range_end
     * @return CanMessageFilter
     */
    static CanMessageFilter RangeFilter(uint32_t id_range_start,
                                        uint32_t id_range_end,
                                        CanFilterConfiguration config
                                            = DEFAULT_FILTER_CONFIG) {
        return CanMessageFilter(FDCAN_FILTER_RANGE,
                                (uint32_t)config,
                                id_range_start,
                                id_range_end);
    }

    /**
     * @brief Create a Dual Filter
     * Creates a filter which matches on either id_1 or id_2
     *
     * @param id_1
     * @param id_2
     * @return CanMessageFilter
     */
    static CanMessageFilter DualFilter(uint32_t id_1,
                                       uint32_t id_2,
                                       CanFilterConfiguration config
                                            = DEFAULT_FILTER_CONFIG) {
        return CanMessageFilter(FDCAN_FILTER_RANGE,
                                (uint32_t)config,
                                id_1,
                                id_2);
    }

    /**
     * @brief Creates a Mask Filter
     * Matches a message_id according to the following relation
     * (filter & mask) == (message_id & mask)
     *
     * @note a mask of 0x00 will match all messages, regardless of filter
     *
     * @param filter
     * @param mask
     * @return CanMessageFilter
     */
    static CanMessageFilter MaskFilter(uint32_t filter, uint32_t mask) {
        return CanMessageFilter(FDCAN_FILTER_MASK,
                                (uint32_t)DEFAULT_FILTER_CONFIG,
                                filter,
                                mask);
    }
};

struct CanMessage {
    enum class ESI : uint32_t {
        ERROR_ACTIVE  = FDCAN_ESI_ACTIVE,
        ERROR_PASSIVE = FDCAN_ESI_PASSIVE,

        UNKNOWN_ERROR_STATE = UINT32_MAX,
    };

    CanMessage(CanMessageId id, uint8_t *data, uint32_t data_length=CAN_MAX_DATA_LENGTH);

    void set_id(uint32_t id);
    void set_id(CanMessageId id);
    void set_ESI(uint32_t esi);

    CanMessageId identifier;
    ESI error_state_indicator = ESI::ERROR_PASSIVE;
    uint8_t message_marker;
    uint8_t* data;
    uint32_t data_length;
};

struct RxCanMessage : public CanMessage {
    RxCanMessage(uint8_t *data, uint32_t data_length=CAN_MAX_DATA_LENGTH);
    uint32_t filter_index=0;

};

struct CanDriver {

    enum class OperatingMode : uint32_t {
        Normal = FDCAN_MODE_NORMAL,
        RestrictedOperation,
        BusMonitoring,
        InternalLoopback,
        ExternalLoopback
    };


    /**
     * @brief Initalized the Can Driver
     * This function should only be called once in the setup of the
     * application.
     *
     */
    void initialize(OperatingMode initial_operating_mode=OperatingMode::InternalLoopback);

    /**
     * @brief Test the CAN Driver
     * A diagnostic tool for testing the can driver.
     * Disconnects the Device from the CAN bus, runs some tests,
     * and then places the device back on the CAN bus.
     */
    void test_driver();

    /**
     * @brief Get the Single Instance of the Can Driver
     * The CAN Driver is designed with a singleton pattern due
     * to it's association with physical hardware.
     *
     * @return CanDriver*
     */
    static CanDriver &get_driver() {
        static CanDriver can_driver;
        return can_driver;
    }

    /**
     * @brief Get the operating mode
     * Determine the current operating mode of the CAN Driver
     *
     * @return OperatingMode
     */
    OperatingMode get_operating_mode() const;

    /**
     * @brief Set the Operating Mode of the can bus
     * takes the device off of the can bus, switches operating modes to the mode
     * specified, then places the device back on the can bus.
     *
     * @param new_operating_mode
     * @return true
     * @return false
     */
    [[nodiscard]] bool set_operating_mode(OperatingMode new_operating_mode);

    /**
     * @brief A non blocking write to the CAN bus
     *
     * @return uint32_t
     * The returned txId can be used with await_write to
     * block until the message is sent on the bus.
     */
    uint32_t write(CanMessage &msg);

    /**
     * @brief Block until the message specified by txId
     * is sent.
     *
     * @param txId
     */
    void await_write(uint32_t &txId);

    [[nodiscard]] bool read_ready(CanRxFifo rxFifo = DEFAULT_RX_FIFO);

    [[nodiscard]] bool read(RxCanMessage &msg, CanRxFifo rxFifo = DEFAULT_RX_FIFO);


    /**
     * @brief Add Filters for Can Messages
     *
     * @tparam Filter
     * @param filter
     * @return true
     * @return false
     */
    template<typename... Filter>
    [[nodiscard]] bool push_filters(Filter... filter) {
        static_assert(is_type<CanMessageFilter, Filter...>(),
        "Filter should be of type CanMessageFilter");
        auto result = false;
        auto status = HAL_FDCAN_Stop(&can_handle);
        if (status != HAL_OK) { Error_Handler(); }
        result = (push_filter(filter) && ...);
        status = HAL_FDCAN_Start(&can_handle);
        if (status != HAL_OK) { Error_Handler(); }

        return result;
    }

    /**
     * @brief Disable Filters and capture every message on the BUS
     *
     * @return true
     * @return false
     */
    [[nodiscard]] bool match_all_ids();

protected:
    CanDriver();

    [[nodiscard]] bool push_filter(CanMessageFilter &filter);

    [[nodiscard]] uint32_t get_data_length_code_from_byte_length(uint32_t byte_length);

private:
    FDCAN_HandleTypeDef &can_handle;
    bool initialized;
    OperatingMode operating_mode;
    uint32_t num_filters;
    CanMessageFilter message_filters[MAX_NUM_FILTERS];
};
