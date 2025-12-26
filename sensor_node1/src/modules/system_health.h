/**
 * @file system_health.h
 * @brief System Health Monitoring Module
 * * This module implements a comprehensive diagnostic engine that validates
 * the physical integrity of the sensor node. It checks for:
 * - Wiring faults (VCC/GND/SDA/SCL)
 * - Sensor initialization failures
 * - Data validity ranges
 * - Drift between redundant sensors
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#ifndef system_health_h
#define system_health_h

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <stdint.h>

/**
 * @brief Maximum allowable difference between Sensor A and Sensor B.
 * If the difference exceeds this value (in degrees C or % RH), 
 * a DRIFT warning is generated.
 */
#define MAX_DRIFT_THRESHOLD     5.0f

// --- Safe Operating Limits ---
#define TEMP_MIN_VALID          -40
#define TEMP_MAX_VALID          80
#define HUMIDITY_MIN_VALID      0
#define HUMIDITY_MAX_VALID      100

/**
 * @brief Diagnostic Status Codes.
 * Maps specific return errors to physical hardware issues.
 */
typedef enum {
    /** @brief System is healthy and operating normally. */
    HEALTH_OK = 0,
    
    /** @brief Sensors disagree by more than MAX_DRIFT_THRESHOLD. */
    VALUE_DRIFT,            // 1
    
    /** @brief Communication Bus Error. Often caused by swapped SDA/SCL wires. */
    SENSOR_SDASCL_FAIL,     // 2
    
    /** @brief Device not found on bus. Check if wires are plugged in. */
    SENSOR_NOT_READY,       // 3
    
    /** @brief Power Failure. VCC (3.3V) or GND wire disconnected. */
    SENSOR_VCC_FAIL,        // 4
    
    /** @brief Sample Fetch Failed. Sensor acknowledges address but returns no data. */
    SENSOR_FETCH_FAIL,      // 5
    
    /** @brief Internal Driver Error: Failed to decode Temperature. */
    TEMP_VALUE_GET_FAIL,    // 6
    
    /** @brief Internal Driver Error: Failed to decode Humidity. */
    HUMI_VALUE_GET_FAIL,    // 7
    
    /** @brief Internal Driver Error: Both channels failed. */
    VALUES_GET_FAIL,        // 8
    
    /** @brief Temperature value is physically impossible (e.g., > 80C). */
    TEMPERATURE_VAL_OUT_OF_RANGE, // 9
    
    /** @brief Humidity value is physically impossible (e.g., > 100%). */
    HUMIDITIY_VAL_OUT_OF_RANGE,   // 10
    
    /** @brief Both sensors reporting garbage data. */
    VALUES_OUT_OF_RANGE     // 11

} health_status_code_t;

/**
 * @brief Container for health context (optional usage).
 */
typedef struct {
    const struct device *sensor_A;
    const struct device *sensor_B;
    health_status_code_t status_code;

} system_health_t;

/**
 * @brief Main diagnostic function.
 * Validates connectivity, power, and data integrity for both sensors.
 * * @param sensor_A Device struct for the first DHT20 sensor.
 * @param sensor_B Device struct for the second DHT20 sensor.
 * @param status Array of size 2 to store the resulting status codes.
 */
void check_system_health(const struct device *sensor_A, const struct device *sensor_B, health_status_code_t *status);

#endif