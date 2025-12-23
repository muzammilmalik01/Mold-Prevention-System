#ifndef MESSAGING_SERVICE_H
#define MESSAGING_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize the CoAP Messaging Service.
 * Sets up the OpenThread instance and CoAP resources.
 */
void msg_init(void);
void msg_send_mold_status(char* message_type, char* room_name, float temp_c, float rh_percent, float mold_index, int mold_risk_status, bool growth_status);
void msg_send_system_health_status(char *message_type, char* room_name, int sensor_1, int sensor_2);
void msg_send_simple_data(char *message_type, char* room_name, float temp_c, float rh_percent);

#endif