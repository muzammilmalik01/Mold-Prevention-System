#ifndef MESSAGING_SERVICE_H
#define MESSAGING_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize the CoAP Messaging Service.
 * Sets up the OpenThread instance and CoAP resources.
 */
void msg_init(void);
void msg_send_telemetry(float temp_c, float rh_percent, float mold_index, int status_code);
void msg_send_alert(int error_code, const char* message);

#endif