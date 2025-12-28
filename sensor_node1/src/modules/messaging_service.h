/**
 * @file messaging_service.h
 * @brief CoAP Messaging Interface for the Sensor Node
 * * This module handles the formatting of JSON payloads and the transmission
 * of data over the OpenThread Mesh network using the CoAP protocol.
 * * @note This module is NOT thread-safe by itself. The caller must ensure
 * mutex locking (e.g., using coap_lock) before calling send functions
 * to prevent buffer corruption.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */

#ifndef MESSAGING_SERVICE_H
#define MESSAGING_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the OpenThread CoAP Service.
 * * Starts the CoAP engine on the default OpenThread instance. 
 * Must be called once at system startup before sending any messages.
 */
void msg_init(void);

/**
 * @brief Sends VTT Mold Model results to the Server Node.
 * * Formats the mold risk data into a JSON string and sends a CoAP PUT request.
 * * @param message_type    String identifier (e.g., "DATA" or "ALERT")
 * @param room_name       Location identifier (e.g., "Living Room")
 * @param temp_c          Current Temperature (Celsius)
 * @param rh_percent      Current Relative Humidity (%)
 * @param mold_index      Calculated Mold Index (0.0 to 6.0)
 * @param mold_risk_status Risk Level Enum (0=Clean, 1=Warning, 2=Critical)
 * @param growth_status   Boolean indicating if mold is actively growing
 */
void msg_send_mold_status(char* message_type, char* room_name, float temp_c, float rh_percent, float mold_index, int mold_risk_status, bool growth_status, bool is_simulation_node);

/**
 * @brief Sends System Health diagnostic data.
 * * Used to report hardware failures or sensor drift issues.
 * * @param message_type    "DATA" (Heartbeat) or "ALERT" (Failure)
 * @param room_name       Location identifier
 * @param sensor_1        Status code for Sensor A (0=OK, 1=Drift, 2=Fail)
 * @param sensor_2        Status code for Sensor B
 */
void msg_send_system_health_status(char *message_type, char* room_name, int sensor_1, int sensor_2);

/**
 * @brief Sends raw telemetry data (Temperature & Humidity).
 * * @param message_type    Usually "DATA"
 * @param room_name       Location identifier
 * @param temp_c          Temperature (Celsius)
 * @param rh_percent      Relative Humidity (%)
 */
void msg_send_simple_data(char *message_type, char* room_name, float temp_c, float rh_percent, bool is_simulation_nod);

#endif