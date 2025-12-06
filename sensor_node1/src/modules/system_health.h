#ifndef system_health_h
#define system_health_h

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <stdint.h>

#define MAX_DRIFT_THRESHOLD     5.0f
#define TEMP_MIN_VALID          -40
#define TEMP_MAX_VALID          80
#define HUMIDITY_MIN_VALID      0
#define HUMIDITY_MAX_VALID      100

typedef enum {
    HEALTH_OK = 0,
    VALUE_DRIFT, // 1 Difference b/w Sensor A readings and Sensor B readings is more than MAX_DRIFT_THRESHOLD.
    SENSOR_SDASCL_FAIL, // 2 Issue with SDA or SCL Wire (Swapped or Not Plugged).
    SENSOR_NOT_READY, // 3 Issue with the SDA or SCL Wire.
    SENSOR_VCC_FAIL, // 4 VCC or GND Wire is not connected.
    SENSOR_FETCH_FAIL, // 5 Issue with VDD or GND Wires.
    TEMP_VALUE_GET_FAIL, // 6 Internal issue, not with wires.
    HUMI_VALUE_GET_FAIL, // 7 Internal issue, not with wires.
    VALUES_GET_FAIL, // 8 Internal issue, not with wires.
    TEMPERATURE_VAL_OUT_OF_RANGE, // 9 Temp greater than TEMP_MIN_VALID or lesser than TEMP_MAX_VALID.
    HUMIDITIY_VAL_OUT_OF_RANGE, // 10 Humi greater than HUMIDITY_MAX_VALID or lesser than HUMIDITY_MIN_VALID.
    VALUES_OUT_OF_RANGE, //11 Both Temp and Humi out of their MIN_VALID and MAX_VALID.

} health_status_code_t;

typedef struct {
    const struct device *sensor_A;
    const struct device *sensor_B;
    health_status_code_t status_code;

} system_health_t;

void check_system_health(const struct device *sensor_A, const struct device *sensor_B, health_status_code_t *status);

#endif