/**
 * @file messaging_service.c
 * @brief Implementation of CoAP Messaging over OpenThread
 * * Uses the Zephyr OpenThread API to construct CoAP Confirmable (CON) PUT requests
 * containing JSON payloads.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#include "messaging_service.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <openthread/coap.h>
#include <openthread/thread.h>
#include <math.h>
#include <stdio.h> 

LOG_MODULE_REGISTER(messaging, LOG_LEVEL_INF);

// Standard CoAP Port
#define COAP_PORT 5683

// Target Border Router IPv6 Address (Mesh Local or Global)
// Note: In production, this might be discovered via DNS-SD or hardcoded
#define SERVER_ADDR "fdde:ad00:beef:0:0:0:0:1" 

// The CoAP Resource Path on the server (e.g., coap://[addr]/storedata)
#define URI_PATH "storedata"

// Shared buffer for constructing JSON strings.
// PROTECTED BY: coap_lock (defined in main.c)
static char json_buffer[256];

/**
 * @brief CoAP Delivery Callback
 * * Triggered when an ACK is received from the server (Success) 
 * or when the transaction times out (Failure).
 */
static void _delivery_report_cb(void *p_context, otMessage *p_message,
                                const otMessageInfo *p_message_info, otError result) 
{
    if (result == OT_ERROR_NONE) {
        LOG_INF("✅ Delivery Confirmed by Server!");
    } else {
        LOG_ERR("❌ Delivery Failed! Error: %d", result);
    }
}

/**
 * @brief Internal helper to build and transmit a CoAP packet.
 * * 1. Allocates a new OpenThread Message buffer.
 * 2. Sets CoAP Type (Confirmable) and Code (PUT).
 * 3. Appends the URI Path and Content-Format (JSON).
 * 4. Appends the payload string.
 * 5. Sends the request via the Thread Interface.
 * * @param payload_string Null-terminated JSON string to send.
 */
static void _send_coap_payload(const char* payload_string) {
    otError error = OT_ERROR_NONE;
    otMessage *myMessage = NULL;
    otMessageInfo myMessageInfo;
    otInstance *myInstance = openthread_get_default_instance();

    do {
        // 1. New Message Allocation
        myMessage = otCoapNewMessage(myInstance, NULL);
        if (myMessage == NULL) {
            LOG_ERR("Failed to allocate CoAP message");
            return;
        }

        // 2. Header Setup (CON = Confirmable, PUT = Update Resource)
        otCoapMessageInit(myMessage, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_PUT);
        otCoapMessageAppendUriPathOptions(myMessage, URI_PATH);
        otCoapMessageAppendContentFormatOption(myMessage, OT_COAP_OPTION_CONTENT_FORMAT_JSON);
        otCoapMessageSetPayloadMarker(myMessage);

        // 3. Payload Append
        error = otMessageAppend(myMessage, payload_string, (uint16_t)strlen(payload_string));
        if (error != OT_ERROR_NONE) break;

        // 4. Destination Setup
        memset(&myMessageInfo, 0, sizeof(myMessageInfo));
        otIp6AddressFromString(SERVER_ADDR, &myMessageInfo.mPeerAddr);
        myMessageInfo.mPeerPort = COAP_PORT;

        // 5. Transmit (with Callback for ACK)
        error = otCoapSendRequest(myInstance, myMessage, &myMessageInfo, _delivery_report_cb, NULL);

    } while (false);

    // Error Handling
    if (error != OT_ERROR_NONE) {
        LOG_ERR("CoAP Send Error: %d", error);
        // If sending failed, we must free the message manually.
        // If sending succeeded, OpenThread stack owns the message now.
        if (myMessage) otMessageFree(myMessage);
    } else {
        LOG_INF("Sent: %s", payload_string);
    }
}

// --- Public API Implementation ---
void msg_init(void) {
    otInstance *p_instance = openthread_get_default_instance();
    otCoapStart(p_instance, OT_DEFAULT_COAP_PORT); 
}


void msg_send_mold_status(char* message_type, char* room_name, float temp_c, float rh_percent, float mold_index, int mold_risk_status, bool growth_status, bool is_simulation_node) {
    // Note: We cast floats to (double) because standard snprintf implementation 
    // in some embedded C libraries (like Newlib) expects doubles for %f.
    snprintf(json_buffer, sizeof(json_buffer), 
             "{\"message_type\":\"%s\",\"room_name\":\"%s\",\"temparature\":%.2f,\"humidity\":%.2f,\"mold_index\":%.2f,\"mold_risk_status\":%d,\"growth_status\":%d, \"is_simulation_node\":%d}", 
             message_type, 
             room_name, 
             (double)temp_c, 
             (double)rh_percent, 
             (double)mold_index, 
             mold_risk_status, 
             (int)growth_status,
             (int)is_simulation_node);
             
    _send_coap_payload(json_buffer);
}

void msg_send_system_health_status(char *message_type, char* room_name, int sensor_1, int sensor_2) {
    snprintf(json_buffer, sizeof(json_buffer), 
             "{\"message_type\":\"%s\",\"room_name\":\"%s\",\"sensor_1_status\":%d,\"sensor_2_status\":%d}", 
             message_type, 
             room_name, 
             sensor_1, 
             sensor_2);
             
    _send_coap_payload(json_buffer);
}

void msg_send_simple_data(char *message_type, char* room_name, float temp_c, float rh_percent, bool is_simulation_node){
    snprintf(json_buffer, sizeof(json_buffer), 
             "{\"message_type\":\"%s\",\"room_name\":\"%s\",\"temparature\":%.2f,\"humidity\":%.2f, \"is_simulation_node\":%d}", 
             message_type, 
             room_name, 
             temp_c, 
             rh_percent,
            (int)is_simulation_node);
             
    _send_coap_payload(json_buffer);
}