/*
 * @file main.c
 * @brief Main Firmware for the Sensor Node
 * * This firmware implements a multi-threaded RTOS architecture for:
 * 1. System Health Monitoring (Fault Detection)
 * 2. Telemetry Reporting (Temp/Humidity)
 * 3. VTT Mold Risk Modeling (Edge Computing)
 * * @platform Nordic nRF52840 (Zephyr RTOS)
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

// Sensor Driver
#include "sensor/dht20/dht20.c"

// Custom Modules
#include "modules/system_health.h"
#include "modules/vtt_model.h"
#include "modules/messaging_service.h"

#if !DT_HAS_COMPAT_STATUS_OKAY(aosong_dht20)
#error "No aosong,dht20 compatible node found in the device tree"
#endif


// * --- CONFIGURATION --- *
#define ROOM_NAME "Office Room"
#define ALERT_MESSAGE "ALERT"
#define DATA_MESSAGE "DATA"
#define TIME_STEP 1.0f
#define STACK_SIZE 2048
#define IS_SIMULATION_NODE true

// * --- Global Status Flags (Protected by Logic) --- *
bool sensor_a_enabled = false;
bool sensor_b_enabled = false;


// * --- SHARED RESOURCES --- * 
const struct device *dht20_dev_a = DEVICE_DT_GET(DT_ALIAS(sensor_a));
const struct device *dht20_dev_b = DEVICE_DT_GET(DT_ALIAS(sensor_b));


// * ---- Thread Priorities (Lower # = Higher Priority) --- *
#define HIGHEST_PRIORITY 1
#define MEDIUM_PRIORITY 2
#define LOWEST_PRIORITY 3

// * --- OS PRIMITIVES --- *

// * MUTEX LOCKS
K_MUTEX_DEFINE(sensors_lock); // Protects I2C Bus Access
K_MUTEX_DEFINE(coap_lock); // Protects OpenThread Radio Buffer

// * THREAD STACKS & DATA 
struct k_thread system_health_data;
struct k_thread vtt_model_data;
struct k_thread simple_data;

K_THREAD_STACK_DEFINE(system_health_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(vtt_model_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(simple_data_stack, STACK_SIZE);


bool get_simulated_weather(float *temp, float *hum) {
        // 1 Real Minute = 1 Simulated Hour
        // k_uptime_get() returns milliseconds. 
        // Divide by 60,000 to get "Real Minutes" which equal "Sim Hours".
        int64_t sim_hour = k_uptime_get() / 60000; 
        
        // Use modulo (%) to loop the 300-hour cycle automatically
        int64_t current_cycle_hour = sim_hour % 300;
    
        bool is_valid = true;
    
        // --- PHASE 1: TROPICAL STORM (Hours 0-100) ---
        if (current_cycle_hour <= 100) {
            *temp = 28.0; 
            *hum = 95.0; 
        } 
        // --- PHASE 2: DRY SPELL (Hours 101-200) ---
        else if (current_cycle_hour <= 200) {
            *temp = 25.0;
            *hum = 45.0; 
        }
        // --- PHASE 3: FREEZE (Hours 201-299) ---
        else {
            *temp = 5.0; 
            *hum = 90.0; 
        }
        
        return is_valid;
    }

/*
 * @brief Helper function to safely read active sensors.
 * Handles fetching, and averaging if redundant sensors are active.
 * * @param[out] out_temp  Pointer to store final temperature (deg C)
 * @param[out] out_hum   Pointer to store final humidity (%)
 * @return true if valid data read, false if all sensors failed
 */
bool get_sensor_data(float *temparature, float *humidity)
        {
                struct sensor_value temparature_value, humidity_value;
                // Case 1: Redundancy Mode (Both Active)
                if (sensor_a_enabled && sensor_b_enabled) {
                        LOG_DBG("[HELPER] Reading Both Sensors...");

                        // 1. Fetch Raw Data
                        sensor_sample_fetch(dht20_dev_a);
                        sensor_sample_fetch(dht20_dev_b);

                        // 2. Process Sensor A
                        sensor_channel_get(dht20_dev_a, SENSOR_CHAN_AMBIENT_TEMP, &temparature_value);
                        sensor_channel_get(dht20_dev_a, SENSOR_CHAN_HUMIDITY, &humidity_value);
                        float sensor_a_temp = sensor_value_to_double(&temparature_value);
                        float sensor_a_humi = sensor_value_to_double(&humidity_value);

                        // 3. Process Sensor B
                        sensor_channel_get(dht20_dev_b, SENSOR_CHAN_AMBIENT_TEMP, &temparature_value);
                        sensor_channel_get(dht20_dev_b, SENSOR_CHAN_HUMIDITY, &humidity_value);
                        float sensor_b_temp = sensor_value_to_double(&temparature_value);
                        float sensor_b_humi = sensor_value_to_double(&humidity_value);

                        // 4. Average
                        *temparature = (sensor_a_temp + sensor_b_temp) / 2.0f;
                        *humidity = (sensor_a_humi + sensor_b_humi) / 2.0f;
                        return true;

                } else if (sensor_a_enabled || sensor_b_enabled) {
                        const struct device *working_sensor = sensor_a_enabled ? dht20_dev_a : dht20_dev_b;
                        LOG_WRN("[HELPER] Failover: Using Single Sensor.");

                        sensor_sample_fetch(working_sensor);
                        sensor_channel_get(working_sensor, SENSOR_CHAN_AMBIENT_TEMP, &temparature_value);
                        sensor_channel_get(working_sensor, SENSOR_CHAN_HUMIDITY, &humidity_value);
                        *temparature = sensor_value_to_double(&temparature_value);
                        *humidity = sensor_value_to_double(&humidity_value);
                        return true;
                }
                return false; // No Sensor Available
}


/*
 * @thread System Health
 * @priority HIGH (1)
 * @period 10 Seconds
 * Checks physical sensor wiring/status. Generates alerts on failure.
 */
void system_health_entry_point(void *p1, void *p2, void *p3){
        health_status_code_t status[2] = {0,0};
        while(1){
                LOG_DBG("[HEALTH] Checking Hardware...");

                // 1. Hardware Check (Protected)
                k_mutex_lock(&sensors_lock, K_FOREVER);
                check_system_health(dht20_dev_a, dht20_dev_b, status);
                
                // Update Global Flags safely
                sensor_a_enabled = (status[0] <= 1);
                sensor_b_enabled = (status[1] <= 1);
                k_mutex_unlock(&sensors_lock);

                // 2. Reporting (Protected)
                k_mutex_lock(&coap_lock, K_FOREVER);

                // Logic: Send ALERT only if Critical Error (>1)
                bool is_critical = (status[0] > 1 || status[1] > 1);
                if (is_critical){
                        LOG_ERR("[HEALTH] CRITICAL FAILURE! A:%d B:%d", status[0], status[1]);
                        msg_send_system_health_status(ALERT_MESSAGE, ROOM_NAME, status[0], status[1]);
                } else {
                        // ! IN CASE OF SENSOR DRIFT - SYSTEM HEALTH WILL BE SENT AS NORMAL
                        msg_send_system_health_status(DATA_MESSAGE, ROOM_NAME, status[0], status[1]);
                }
                k_mutex_unlock(&coap_lock);
                k_msleep(10000); // Check every 10s
        }
}


/*
 * @thread Telemetry (Simple Data)
 * @priority MEDIUM (2)
 * @period 60 Seconds
 * Sends raw Temperature & Humidity data to dashboard.
 */
void simple_data_entry_point(void *p1, void *p2, void *p3){
        while(1){
                float temparature = 0.0f, humidity = 0.0f;  
                bool valid_read = false;

                // 1. Get Data
                k_mutex_lock(&sensors_lock, K_FOREVER);

                if (IS_SIMULATION_NODE) {
                        valid_read = get_simulated_weather(&temparature, &humidity);
                } else {
                        valid_read = get_sensor_data(&temparature, &humidity);
                }
                k_mutex_unlock(&sensors_lock);

                // 2. Send Data
                if (valid_read){
                        k_mutex_lock(&coap_lock, K_FOREVER);
                        
                        LOG_INF("[TELEMETRY] Sending Sensor Data....");
                        msg_send_simple_data(DATA_MESSAGE, ROOM_NAME, temparature, humidity);
                        k_mutex_unlock(&coap_lock);
                } else {
                        LOG_WRN("[TELEMETRY] Skipped: Sensors unavailable");
                }
                k_msleep(50000);
        }
}

/*
 * @thread VTT Model
 * @priority LOW (3)
 * @period 15 Minutes (900s)
 * Calculates Mold Risk Index using VTT equation.
 */
void vtt_model_entry_point(void *p1, void *p2, void *p3){
        vtt_state_t room_state;

        // Initialize Model (Material Class: Sensitive)
        vtt_init(&room_state, VTT_MAT_SENSITIVE);

        while(1){
                bool valid_read = false;
                float temparature = 0.0f, humidity = 0.0f;  

                // 1. Get Data
                k_mutex_lock(&sensors_lock, K_FOREVER);
                if (IS_SIMULATION_NODE) {
                        valid_read = get_simulated_weather(&temparature, &humidity);
                } else {
                        valid_read = get_sensor_data(&temparature, &humidity);
                }
                k_mutex_unlock(&sensors_lock);

                // 2. Process & Send
                if (valid_read){
                        LOG_INF("[VTT] Running Model...");

                        vtt_update(&room_state, temparature, humidity, TIME_STEP);
                        vtt_risk_level_t mold_risk_level = vtt_get_risk_level(&room_state); 
                        k_mutex_lock(&coap_lock, K_FOREVER);

                        // Determine Message Type (Alert if Risk High OR actively growing)
                        char *msg_type = (mold_risk_level == MOLD_RISK_CLEAN && !room_state.growing_condition) 
                             ? DATA_MESSAGE : ALERT_MESSAGE;

                        msg_send_mold_status(msg_type, ROOM_NAME, temparature, humidity, room_state.mold_index, mold_risk_level, room_state.growing_condition);
                        k_mutex_unlock(&coap_lock);

                } else {
                        LOG_WRN("[VTT] Skipped: Sensors unavailable");
                }
                k_msleep(60000); 
        }
}

int main(void)
{
        LOG_INF("--- Sensor Node Booting ---");

        // * 1. Initialize Network Stack
        msg_init();

        // * 2. Wait for Network Attachment
        LOG_INF("[MAIN] Waiting for OpenThread Attachment (10s)...");
        k_sleep(K_SECONDS(10));

        // * 3. Spawn Threads

        // System Health (Starts NOW)
        k_thread_create(&system_health_data, system_health_stack, K_THREAD_STACK_SIZEOF(system_health_stack), system_health_entry_point, NULL,NULL,NULL, HIGHEST_PRIORITY, 0, K_NO_WAIT);

        // Telemetry (Starts +4s)
        k_thread_create(&simple_data, simple_data_stack, K_THREAD_STACK_SIZEOF(simple_data_stack), simple_data_entry_point, NULL,NULL,NULL, MEDIUM_PRIORITY, 0, K_SECONDS(4));
        
        // VTT Model (Starts +4s)
        k_thread_create(&vtt_model_data, vtt_model_stack, K_THREAD_STACK_SIZEOF(vtt_model_stack), vtt_model_entry_point, NULL,NULL,NULL, LOWEST_PRIORITY, 0, K_SECONDS(4));

        LOG_INF("[MAIN] All threads spawned. Entering Idle.");
        return 0;
}
