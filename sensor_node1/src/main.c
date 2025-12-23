#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "modules/system_health.h"
#include "modules/vtt_model.h"
#include "modules/messaging_service.h"
#include "sensor/dht20/dht20.c"

#if !DT_HAS_COMPAT_STATUS_OKAY(aosong_dht20)
#error "No aosong,dht20 compatible node found in the device tree"
#endif

#define ROOM_NAME "Living Room"
#define ALERT_MESSAGE "ALERT"
#define DATA_MESSAGE "DATA"
#define TIME_STEP 0.25f

bool sensor_a_enabled = false;
bool sensor_b_enabled = false;


// * SHARED RESOURCES * 
const struct device *dht20_dev_a = DEVICE_DT_GET(DT_ALIAS(sensor_a));
const struct device *dht20_dev_b = DEVICE_DT_GET(DT_ALIAS(sensor_b));

#define STACK_SIZE 2048

#define HIGHEST_PRIORITY 1
#define MEDIUM_PRIORITY 2
#define LOWEST_PRIORITY 3

// * MUTEX LOCKS
K_MUTEX_DEFINE(sensors_lock);
K_MUTEX_DEFINE(coap_lock);

// * THREADS DATA STRUCTURE
struct k_thread system_health_data;
struct k_thread vtt_model_data;
struct k_thread simple_data;

K_THREAD_STACK_DEFINE(system_health_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(vtt_model_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(simple_data_stack, STACK_SIZE);


void system_health_entry_point(void *p1, void *p2, void *p3){
        health_status_code_t status[2] = {0,0};
        while(1){
                LOG_INF("[SYSTEM_HEALTH_THREAD]: System Health Thread Running.");
                k_mutex_lock(&sensors_lock, K_FOREVER);
                LOG_INF("[SYSTEM_HEALTH_THREAD]: Acquired SENSOR_LOCK.");
                check_system_health(dht20_dev_a, dht20_dev_b, status);
                sensor_a_enabled = (status[0] <= 1);
                sensor_b_enabled = (status[1] <= 1);
                LOG_INF("[SYSTEM_HEALTH_THREAD]: Released SENSOR_LOCK.");
                k_mutex_unlock(&sensors_lock);


                k_mutex_lock(&coap_lock, K_FOREVER);
                LOG_INF("[SYSTEM_HEALTH_THREAD]: Acquired COAP_LOCK.");

                bool is_critical = (status[0] > 1 || status[1] > 1);
                if (is_critical){
                        LOG_WRN("[SYSTEM_HEALTH_THREAD]: Sending Health Report - SENSOR ISSUE DETECTED");
                        LOG_WRN("[SYSTEM_HEALTH_THREAD]: Sensor A: %d | Sensor B: %d", status[0], status[1]);
                        msg_send_system_health_status(ALERT_MESSAGE, ROOM_NAME, status[0], status[1]);
                } else {
                        // ! IN CASE OF SENSOR DRIFT - SYSTEM HEALTH WILL BE SENT AS NORMAL
                        LOG_INF("[SYSTEM_HEALTH_THREAD]: Both Sensors Enabled");
                        LOG_INF("[SYSTEM_HEALTH_THREAD]: Sending Health Report.");
                        msg_send_system_health_status(DATA_MESSAGE, ROOM_NAME, status[0], status[1]);
                }
                k_mutex_unlock(&coap_lock);
                LOG_INF("[SYSTEM_HEALTH_THREAD]: Released CoAP_LOCK.");
                k_msleep(10000);
        }
}

void simple_data_entry_point(void *p1, void *p2, void *p3){
        struct sensor_value sensor_a_temparature, sensor_a_humidity;
        struct sensor_value sensor_b_temparature, sensor_b_humidity;
        while(1){
                float temparature = 0.0f;
                float humidity = 0.0f;  
                bool valid_read = false;
                k_mutex_lock(&sensors_lock, K_FOREVER);
                LOG_INF("[SIMPLE_DATA_THREAD]: ACQUIRED SENSORS LOCK.");
                if (sensor_a_enabled && sensor_b_enabled) {
                        LOG_INF("[SIMPLE_DATA_THREAD]: Using both Sensors.");
                        sensor_sample_fetch(dht20_dev_a);
                        sensor_sample_fetch(dht20_dev_b);
                        sensor_channel_get(dht20_dev_a, SENSOR_CHAN_AMBIENT_TEMP, &sensor_a_temparature);
                        sensor_channel_get(dht20_dev_a, SENSOR_CHAN_HUMIDITY, &sensor_a_humidity);
                        sensor_channel_get(dht20_dev_b, SENSOR_CHAN_AMBIENT_TEMP, &sensor_b_temparature);
                        sensor_channel_get(dht20_dev_b, SENSOR_CHAN_HUMIDITY, &sensor_b_humidity);
                        float sensor_a_temp = sensor_value_to_double(&sensor_a_temparature);
                        float sensor_a_humi = sensor_value_to_double(&sensor_a_humidity);
                        float sensor_b_temp = sensor_value_to_double(&sensor_b_temparature);
                        float sensor_b_humi = sensor_value_to_double(&sensor_b_humidity);
                        temparature = (sensor_a_temp + sensor_b_temp) / 2.0f;
                        humidity = (sensor_a_humi + sensor_b_humi) / 2.0f;
                        valid_read = true;

                } else if (sensor_a_enabled || sensor_b_enabled) {
                        struct sensor_value working_sensor_temparature;
                        struct sensor_value working_sensor_humidity;
                        const struct device *working_sensor = sensor_a_enabled ? dht20_dev_a : dht20_dev_b;
                        LOG_WRN("[SIMPLE_DATA_THREAD]: Using One Sensor.");

                        //* 2. Get Sensor Data
                        sensor_sample_fetch(working_sensor);
                        sensor_channel_get(working_sensor, SENSOR_CHAN_AMBIENT_TEMP, &working_sensor_temparature);
                        sensor_channel_get(working_sensor, SENSOR_CHAN_HUMIDITY, &working_sensor_humidity);
                        temparature = sensor_value_to_double(&working_sensor_temparature);
                        humidity = sensor_value_to_double(&working_sensor_humidity);
                        valid_read = true;
                } 
                LOG_INF("[SIMPLE_DATA_THREAD]: RELEASED SENSORS LOCK.");
                k_mutex_unlock(&sensors_lock);
                if (valid_read){
                        // * 4. Acquire CoAP Lock
                        k_mutex_lock(&coap_lock, K_FOREVER);
                        LOG_INF("[SIMPLE_DATA_THREAD]: ACQUIRED CoAP LOCK.");

                        // * 5. Send Data
                        msg_send_simple_data(DATA_MESSAGE, ROOM_NAME, temparature, humidity);

                        // * 6. Release CoAP Lock
                        k_mutex_unlock(&coap_lock);
                        LOG_INF("[SIMPLE_DATA_THREAD]: RELEASED CoAP LOCK.");
                } else {
                        LOG_WRN("[SIMPLE_DATA_THREAD] Skipping sending telemetry: No sensors enabled.");
                }
                k_msleep(60000);
        }
}

void vtt_model_entry_point(void *p1, void *p2, void *p3){
        struct sensor_value sensor_a_temparature, sensor_a_humidity;
        struct sensor_value sensor_b_temparature, sensor_b_humidity;
        vtt_state_t room_state;
        vtt_init(&room_state, VTT_MAT_SENSITIVE);

        while(1){
                bool valid_read = false;
                float temparature = 0.0f;
                float humidity = 0.0f;  
                // * ACQUIRE SENSOR LOCK
                k_mutex_lock(&sensors_lock, K_FOREVER);
                LOG_INF("[VTT_MODEL_THREAD]: ACQUIRED SENSORS LOCK.");
                if (sensor_a_enabled && sensor_b_enabled) {
                        // * Get data from both the sensors
                        sensor_sample_fetch(dht20_dev_a);
                        sensor_sample_fetch(dht20_dev_b);
                        sensor_channel_get(dht20_dev_a, SENSOR_CHAN_AMBIENT_TEMP, &sensor_a_temparature);
                        sensor_channel_get(dht20_dev_a, SENSOR_CHAN_HUMIDITY, &sensor_a_humidity);
                        sensor_channel_get(dht20_dev_b, SENSOR_CHAN_AMBIENT_TEMP, &sensor_b_temparature);
                        sensor_channel_get(dht20_dev_b, SENSOR_CHAN_HUMIDITY, &sensor_b_humidity);
                        float sensor_a_temp = sensor_value_to_double(&sensor_a_temparature);
                        float sensor_a_humi = sensor_value_to_double(&sensor_a_humidity);
                        float sensor_b_temp = sensor_value_to_double(&sensor_b_temparature);
                        float sensor_b_humi = sensor_value_to_double(&sensor_b_humidity);
                        temparature = (sensor_a_temp + sensor_b_temp) / 2.0f;
                        humidity = (sensor_a_humi + sensor_b_humi) / 2.0f;
                        valid_read = true;

                } else if (sensor_a_enabled || sensor_b_enabled) {
                        struct sensor_value working_sensor_temparature;
                        struct sensor_value working_sensor_humidity;
                        const struct device *working_sensor = sensor_a_enabled ? dht20_dev_a : dht20_dev_b;

                        sensor_sample_fetch(working_sensor);
                        sensor_channel_get(working_sensor, SENSOR_CHAN_AMBIENT_TEMP, &working_sensor_temparature);
                        sensor_channel_get(working_sensor, SENSOR_CHAN_HUMIDITY, &working_sensor_humidity);
                        temparature = sensor_value_to_double(&working_sensor_temparature);
                        humidity = sensor_value_to_double(&working_sensor_humidity);
                        valid_read = true;
                } 

                // * RELEASE SENSOR LOCK
                k_mutex_unlock(&sensors_lock);
                LOG_INF("[VTT_MODEL_THREAD]: RELEASED SENSORS LOCK.");
                if (valid_read){
                        vtt_update(&room_state, temparature, humidity, TIME_STEP);
                        vtt_risk_level_t mold_risk_level = vtt_get_risk_level(&room_state); 
                        k_mutex_lock(&coap_lock, K_FOREVER);
                        LOG_INF("[VTT_MODEL_THREAD]: ACQUIRED CoAP LOCK.");

                        char *msg_type = (mold_risk_level == MOLD_RISK_CLEAN && !room_state.growing_condition) 
                             ? DATA_MESSAGE : ALERT_MESSAGE;

                        msg_send_mold_status(msg_type, ROOM_NAME, temparature, humidity, room_state.mold_index, mold_risk_level, room_state.growing_condition);
                        k_mutex_unlock(&coap_lock);
                        LOG_INF("[VTT_MODEL_THREAD]: RELEASED CoAP LOCK.");

                } else {
                        LOG_WRN("[VTT_MODEL_THREAD] Skipping update: No sensors enabled.");
                }
                k_msleep(900000);
        }
}

int main(void)
{
        msg_init();
        LOG_INF("[MAIN]: Message Init Finished - Sleeping for 10 Seconds.");
        k_sleep(K_SECONDS(10));
        k_thread_create(&system_health_data, system_health_stack, K_THREAD_STACK_SIZEOF(system_health_stack), system_health_entry_point, NULL,NULL,NULL, HIGHEST_PRIORITY, 0, K_NO_WAIT);
        k_thread_create(&simple_data, simple_data_stack, K_THREAD_STACK_SIZEOF(simple_data_stack), simple_data_entry_point, NULL,NULL,NULL, MEDIUM_PRIORITY, 0, K_SECONDS(4));
        k_thread_create(&vtt_model_data, vtt_model_stack, K_THREAD_STACK_SIZEOF(vtt_model_stack), vtt_model_entry_point, NULL,NULL,NULL, LOWEST_PRIORITY, 0, K_SECONDS(4));
        return 0;
}
