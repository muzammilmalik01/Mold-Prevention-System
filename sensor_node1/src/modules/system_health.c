
#include "system_health.h"
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(system_health, LOG_LEVEL_INF);

/**
 * @brief Validates a single sensor's health and retrieves values.
 * Returns status code and puts valid values into final_temp/final_humi pointers.
 */
static health_status_code_t check_sensor(const struct device *sensor, float* final_temp, float *final_humi) {
    int return_code = device_is_ready(sensor);
    if (return_code != 1){
        if (return_code == 0) {
            LOG_ERR("Sensor %s SDA/SCL Wires not working. Return Code: %d", sensor->name, return_code);
            return SENSOR_SDASCL_FAIL;
        } 
        else {
            LOG_ERR("Sensor %s not ready, check SDA/SCL Wires.", sensor->name);
            return SENSOR_NOT_READY;
        } 
    } 
    
    return_code = sensor_sample_fetch(sensor);
    if (return_code != 0)
        {
            if (return_code == -5){
                LOG_ERR("Sensor %s VCC (Power) issue. ", sensor->name);
                return SENSOR_VCC_FAIL;
            } else {
                LOG_ERR("Sensor %s Fetch Fail. ", sensor->name);
                return SENSOR_FETCH_FAIL;
            }
        }

    struct sensor_value temperature;
    struct sensor_value humidity;
    int temp_value = sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    int humi_value = sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &humidity);
    if (temp_value != 0 && humi_value != 0) return VALUES_GET_FAIL;
    if (temp_value != 0 && humi_value == 0) return TEMP_VALUE_GET_FAIL;
    if (humi_value != 0 && temp_value == 0) return HUMI_VALUE_GET_FAIL;
    if (temperature.val1 < TEMP_MIN_VALID || temperature.val1 > TEMP_MAX_VALID) return TEMPERATURE_VAL_OUT_OF_RANGE;
    if (humidity.val1 < HUMIDITY_MIN_VALID || humidity.val1 > HUMIDITY_MAX_VALID) return HUMIDITIY_VAL_OUT_OF_RANGE;
    if ((temperature.val1 < TEMP_MIN_VALID || temperature.val1 > TEMP_MAX_VALID) && (humidity.val1 < HUMIDITY_MIN_VALID || humidity.val1 > HUMIDITY_MAX_VALID)) return VALUES_OUT_OF_RANGE;
    *final_temp = sensor_value_to_double(&temperature);
    *final_humi = sensor_value_to_double(&humidity);
    LOG_DBG("%s: T=%.2f, H=%.2f", sensor->name, *final_temp, *final_humi);
    printk("%s: T=%.2f, H=%.2f", sensor->name, *final_temp, *final_humi);
    return HEALTH_OK;
}


/**
 * @brief Checks if two sensor values have drifted apart.
 * Internal helper function.
 */
static void check_drift(float t1, float h1, float t2, float h2, health_status_code_t *status) {
    float temp_diff = t1 - t2;
    float hum_diff = h1 - h2;

    // Absolute value calculation
    if (temp_diff < 0) temp_diff = -temp_diff;
    if (hum_diff < 0) hum_diff = -hum_diff;

    if (temp_diff > MAX_DRIFT_THRESHOLD || hum_diff > MAX_DRIFT_THRESHOLD) {
        LOG_WRN("Sensor Drift Detected! T_Diff: %d, H_Diff: %d", (int)temp_diff, (int)hum_diff);
        // Mark both sensors as having drift warning
        if (status[0] == HEALTH_OK) status[0] = VALUE_DRIFT;
        if (status[1] == HEALTH_OK) status[1] = VALUE_DRIFT;
    }
    LOG_DBG("Sensor Drift: T_Diff: %f, H_Diff: %f", temp_diff, hum_diff);
    printk("Sensor Drift: T_Diff: %f, H_Diff: %f", temp_diff, hum_diff);
}


/**
 * @brief Public API to check entire system health.
 * Performs individual checks and then cross-references for drift.
 */
void check_system_health(const struct device *sensor_A, const struct device *sensor_B, health_status_code_t *status) {
    const struct device *A = sensor_A;
    const struct device *B = sensor_B;

    float t1 = 0, h1 = 0;
    float t2 = 0, h2 = 0;

    status[0] = check_sensor(A, &t1, &h1);
    status[1] = check_sensor(B, &t2, &h2);
    if (status[0] == HEALTH_OK && status[1] == HEALTH_OK){
        check_drift(t1,h1,t2,h2,status);
    } 
}