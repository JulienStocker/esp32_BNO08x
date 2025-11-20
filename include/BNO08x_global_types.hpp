/**
 * @file BNO08x_global_types.hpp
 * @author Myles Parfeniuk
 */
#pragma once

#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>

/// @brief Sensor accuracy returned during sensor calibration
enum class BNO08xAccuracy
{
    LOW = 1,
    MED,
    HIGH,
    UNDEFINED
};
using IMUAccuracy = BNO08xAccuracy; // legacy version compatibility

/// @brief Reason for previous IMU reset (returned by get_reset_reason())
enum class BNO08xResetReason
{
    UNDEFINED, ///< Undefined reset reason, this should never occur and is an error.
    POR,       ///< Previous reset was due to power on reset.
    INT_RST,   ///< Previous reset was due to internal reset.
    WTD,       ///< Previous reset was due to watchdog timer.
    EXT_RST,   ///< Previous reset was due to external reset.
    OTHER      ///< Previous reset was due to power other reason.
};
using IMUResetReason = BNO08xResetReason; // legacy version compatibility

/// @brief BNO08xActivity Classifier enable bits passed to enable_activity_classifier()
enum class BNO08xActivityEnable
{
    UNKNOWN = (1U << 0U),
    IN_VEHICLE = (1U << 1U),
    ON_BICYCLE = (1U << 2U),
    ON_FOOT = (1U << 3U),
    STILL = (1U << 4U),
    TILTING = (1U << 5U),
    WALKING = (1U << 6U),
    RUNNING = (1U << 7U),
    ON_STAIRS = (1U << 8U),
    ALL = 0x1FU
};

/// @brief BNO08xActivity states returned from get_activity_classifier()
enum class BNO08xActivity
{
    UNKNOWN = 0,    // 0 = unknown
    IN_VEHICLE = 1, // 1 = in vehicle
    ON_BICYCLE = 2, // 2 = on bicycle
    ON_FOOT = 3,    // 3 = on foot
    STILL = 4,      // 4 = still
    TILTING = 5,    // 5 = tilting
    WALKING = 6,    // 6 = walking
    RUNNING = 7,    // 7 = running
    ON_STAIRS = 8,  // 8 = on stairs
    UNDEFINED = 9   // used for unit tests
};

/// @brief BNO08xStability states returned from get_stability_classifier()
enum class BNO08xStability
{
    UNKNOWN = 0,    // 0 = unknown
    ON_TABLE = 1,   // 1 = on table
    STATIONARY = 2, // 2 = stationary
    UNDEFINED = 3   // used for unit tests
};

/// @brief IMU configuration settings passed into constructor
typedef struct bno08x_config_t
{
        spi_host_device_t spi_host;       ///<SPI peripheral to be used
        gpio_num_t io_mosi;               ///<MOSI GPIO pin (connects to BNO08x DI pin)
        gpio_num_t io_miso;               ///<MISO GPIO pin (connects to BNO08x SDA pin)
        gpio_num_t io_sclk;               ///<SCLK pin (connects to BNO08x SCL pin)
        gpio_num_t io_cs;                 ///<Chip select pin (connects to BNO08x CS pin)
        gpio_num_t io_int;                ///<Host interrupt pin (connects to BNO08x INT pin)
        gpio_num_t io_rst;                ///<Reset pin (connects to BNO08x RST pin)
        gpio_num_t io_wake;               ///<Wake pin (optional, connects to BNO08x P0)
        uint32_t sclk_speed;              ///<Desired SPI SCLK speed in Hz (max 3MHz)
        bool install_isr_service;         ///<Indicates whether the ISR service for the HINT should be installed at IMU initialization

        // ─────────────────────────────────────────────
        // NEW: Mux-aware CS support
        // ─────────────────────────────────────────────
        bool use_mux;          ///< true if external SN74HC138 controls CS
        gpio_num_t mux_pin_a;  ///< SN74HC138 A input
        gpio_num_t mux_pin_b;  ///< SN74HC138 B input
        gpio_num_t mux_pin_c;  ///< SN74HC138 C input
        uint8_t mux_channel;   ///< 0–7: Y0–Y7 output that goes to this IMU CS

        /// @brief IMU configuration settings constructor
        bno08x_config_t(
                spi_host_device_t host = SPI2_HOST,
                gpio_num_t mosi = GPIO_NUM_NC,
                gpio_num_t miso = GPIO_NUM_NC,
                gpio_num_t sclk = GPIO_NUM_NC,
                gpio_num_t cs   = GPIO_NUM_NC,
                gpio_num_t intr = GPIO_NUM_NC,
                gpio_num_t rst  = GPIO_NUM_NC,
                gpio_num_t wake = GPIO_NUM_NC,
                uint32_t sclk_hz = 1000000UL,
                bool install_isr = true,
                bool use_mux_    = false,
                gpio_num_t mux_a_ = GPIO_NUM_NC,
                gpio_num_t mux_b_ = GPIO_NUM_NC,
                gpio_num_t mux_c_ = GPIO_NUM_NC,
                uint8_t mux_ch_   = 0)
            : spi_host(host)
            , io_mosi(mosi)
            , io_miso(miso)
            , io_sclk(sclk)
            , io_cs(cs)
            , io_int(intr)
            , io_rst(rst)
            , io_wake(wake)
            , sclk_speed(sclk_hz)
            , install_isr_service(install_isr)
            , use_mux(use_mux_)
            , mux_pin_a(mux_a_)
            , mux_pin_b(mux_b_)
            , mux_pin_c(mux_c_)
            , mux_channel(mux_ch_)
        {
        }
} bno08x_config_t;

typedef bno08x_config_t imu_config_t; // legacy version compatibility