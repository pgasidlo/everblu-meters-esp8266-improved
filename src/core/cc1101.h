/**
 * @file cc1101.h
 * @brief CC1101 radio driver for Everblu Cyble water/gas meter communication
 *
 * This header defines the interface for communicating with Everblu Cyble
 * water and gas meters using the CC1101 sub-GHz radio transceiver and the RADIAN protocol.
 * Supports adaptive frequency tracking and historical data extraction.
 */

#ifndef __CC1101_H__
#define __CC1101_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @struct detected_meter
 * @brief Structure to hold detected meter identification from sniffer mode
 *
 * Contains meter identification extracted from trigger (interrogation) frames
 * captured in passive sniffer mode.
 */
struct detected_meter
{
  uint8_t meter_year;    // 2-digit year from trigger frame (e.g., 17 for 2017)
  uint32_t meter_serial; // 24-bit serial number from trigger frame
  int8_t rssi_dbm;       // Signal strength in dBm
  uint8_t lqi;           // Link Quality Indicator (0-255)
};

/**
 * @struct tmeter_data
 * @brief Meter data structure containing current readings and metadata
 *
 * Contains all data extracted from an Everblu Cyble water/gas meter reading,
 * including current consumption, historical data, signal quality metrics,
 * and battery information.
 */
struct tmeter_data
{
  int volume;             // Current consumption reading in liters (water) or cubic meters (gas)
  int reads_counter;      // Number of times meter has been read (wraps around 255→1)
  int battery_left;       // Estimated battery life remaining in months
  int time_start;         // Reading window start time (24-hour format, e.g., 8 = 8am)
  int time_end;           // Reading window end time (24-hour format, e.g., 18 = 6pm)
  int rssi;               // Radio Signal Strength Indicator (raw value)
  int rssi_dbm;           // RSSI converted to dBm
  int lqi;                // Link Quality Indicator (0-255, higher is better)
  int8_t freqest;         // Frequency offset estimate from CC1101 for adaptive tracking
  uint32_t history[13];   // Monthly historical readings (13 months), index 0 = oldest, 12 = most recent
  bool history_available; // True if historical data was successfully extracted
};

/**
 * @brief Set the CC1101 radio frequency in MHz
 *
 * Configures the CC1101 transceiver to operate at the specified frequency.
 * Used for fine-tuning frequency to match meter transmissions or for
 * adaptive frequency tracking.
 *
 * @param mhz Frequency in MHz (typically around 433.82 MHz for Cyble meters)
 */
void setMHZ(float mhz);

/**
 * @brief Initialize the CC1101 radio transceiver
 *
 * Performs complete initialization of the CC1101 radio including:
 * - SPI communication setup
 * - Register configuration for RADIAN protocol
 * - Frequency calibration
 * - Power amplifier configuration
 *
 * @param freq Initial operating frequency in MHz
 * @return true if initialization succeeded, false on failure
 */
bool cc1101_init(float freq);

/**
 * @brief Put CC1101 radio into receive (RX) mode
 *
 * Configures the radio to listen for incoming meter transmissions.
 * Must be called after initialization or frequency changes to enable reception.
 */
void cc1101_rec_mode(void);

/**
 * @brief Set target meter identification for queries
 *
 * Sets the meter year and serial number to use when sending interrogation frames.
 * This allows runtime configuration of the target meter, overriding compile-time
 * METER_YEAR and METER_SERIAL defaults.
 *
 * Call this before get_meter_data() to query a specific meter.
 *
 * @param year 2-digit year from meter label (e.g., 23 for 2023)
 * @param serial Meter serial number (up to 24 bits / 8 digits)
 */
void set_meter_target(uint8_t year, uint32_t serial);

/**
 * @brief Read data from Everblu Cyble water/gas meter
 *
 * Performs a complete read cycle:
 * 1. Transmits RADIAN protocol request frame to meter
 * 2. Waits for meter response
 * 3. Decodes received data including current reading and history
 * 4. Validates CRC and data integrity
 * 5. Extracts signal quality metrics (RSSI, LQI, frequency offset)
 *
 * Uses meter year/serial set by set_meter_target(), or falls back to
 * compile-time METER_YEAR/METER_SERIAL if not set.
 *
 * This is a blocking operation that may take several seconds to complete.
 *
 * @return tmeter_data structure containing all extracted meter data
 */
struct tmeter_data get_meter_data(void);

/**
 * @brief Configure CC1101 for sniffer mode (passive listening for trigger frames)
 *
 * Sets up the radio to listen for trigger (interrogation) frames sent by
 * other devices. In this mode, no transmissions occur - the device only listens.
 *
 * Call this once after cc1101_init() to enter sniffer mode.
 */
void sniffer_configure_rx(void);

/**
 * @brief Listen for trigger frames in sniffer mode
 *
 * Non-blocking function that checks for incoming trigger frames.
 * If a valid trigger frame is detected, extracts meter_year and meter_serial.
 *
 * Trigger frame structure (pre-encoding, 19 bytes):
 * - Byte [4]: meter_year (2-digit year)
 * - Bytes [5-7]: meter_serial (24-bit, big-endian)
 *
 * @param detected Output structure to store detected meter info
 * @param timeout_ms Maximum time to wait for a frame (milliseconds)
 * @return true if a valid trigger frame was detected, false otherwise
 */
bool sniffer_listen(struct detected_meter *detected, int timeout_ms);

#endif // __CC1101_H__