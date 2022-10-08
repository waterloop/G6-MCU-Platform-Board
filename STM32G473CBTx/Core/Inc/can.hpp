#pragma once
#include "stm32g473xx.h"
#include "stm32g4xx_hal_fdcan.h"

struct CanDriver {
    CanDriver();

    enum class FDCANOperatingMode : uint32_t {
        Normal = FDCAN_MODE_NORMAL,
        RestrictedOperation,
        BusMonitoring,
        InternalLoopback,
        ExternalLoopback
    };


    struct Message {
        static constexpr size_t MAX_DATA_LENGTH = 64;
        static constexpr uint32_t MAX_UINT32 = 0xffffffff;

        using Marker = uint8_t;

        enum class Id : uint32_t {
            RelayFaultDetected      = 0x00,
            BmsFaultDetected        = 0x01,
            McFaultDetected         = 0x02,
            LVSensingFaultDetected  = 0x03,


            // TODO Fill out with the rest of the values
            DefaultRx = MAX_UINT32,
        };

        enum class ESI : uint32_t {
            ERROR_ACTIVE  = FDCAN_ESI_ACTIVE,
            ERROR_PASSIVE = FDCAN_ESI_PASSIVE,

            UNKNOWN_ERROR_STATE = MAX_UINT32,
        };

        Message(Id id, uint8_t *data, uint32_t dataLength=MAX_DATA_LENGTH);

        void setId(uint32_t id);
        void setESI(uint32_t esi);

        Id identifier;
        ESI errorStateIndicator = ESI::ERROR_PASSIVE;
        Marker messageMarker;
        uint8_t* data;
        uint32_t dataLength;
    };

    struct RxMessage : public Message {
        RxMessage(uint8_t *data, uint32_t dataLength=MAX_DATA_LENGTH);
        uint32_t filterIndex=0;

    };


    /**
     * @brief Set the Operating Mode of the can bus
     * takes the device off of the can bus, switches operating modes to the mode
     * specified, then places the device back on the can bus.
     *
     * @param newOperatingMode
     * @return true
     * @return false
     */
    bool setOperatingMode(FDCANOperatingMode newOperatingMode);

    /**
     * @brief A non blocking write to the CAN bus
     *
     * @return uint32_t
     * The returned txId can be used with awaitWrite to
     * block until the message is sent on the bus.
     */
    uint32_t write(Message &msg);

    /**
     * @brief Block until the message specified by txId
     * is sent.
     *
     * @param txId
     */
    void awaitWrite(uint32_t &txId);

    bool canRead(uint32_t rxFifo = FDCAN_RX_FIFO0);

    bool read(RxMessage &msg, uint32_t rxFifo = FDCAN_RX_FIFO0);
private:
    FDCAN_HandleTypeDef &canHandle;
};
