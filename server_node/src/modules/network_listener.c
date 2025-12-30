/**
 * @file network_listener.c
 * @brief Implementation of the CoAP Server logic.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/coap.h>
#include <string.h>             
#include <zephyr/sys/printk.h>  
#include "network_listener.h"
#include "node_manager.h"
#include "shared_types.h"

LOG_MODULE_REGISTER(network_lst, LOG_LEVEL_INF);

// --- Configuration ---
#define COAP_PORT 5683         
#define URI_PATH "storedata"   

// --- Globals ---
static struct k_msgq *outgoing_queue; 

// --- Forward Declarations ---
static void storedata_request_handler(void *context, otMessage *message, const otMessageInfo *message_info);

// --- CoAP Resource Definition ---
static otCoapResource m_storedata_resource = {
    .mUriPath = URI_PATH,
    .mHandler = storedata_request_handler,
    .mContext = NULL,
    .mNext = NULL
};

/**
 * @brief Helper function to parse "room_name" from a flat JSON string.
 * It searches for the key "room_name" and extracts the string value.
 * * @param json_input The raw JSON string (e.g., {"room_name":"Kitchen", "temp":24})
 * @param out_buffer The buffer to store the result.
 * @param buffer_size The size of the output buffer.
 */
static void parse_room_name(const char *json_input, char *out_buffer, size_t buffer_size) {
    const char *key = "\"room_name\"";
    const char *found = strstr(json_input, key);
    
    // Default fallback
    strncpy(out_buffer, "Unknown", buffer_size - 1);
    out_buffer[buffer_size - 1] = '\0';

    if (found) {
        // Move past the key
        found += strlen(key);

        // Find the start of the value (first quote after the colon)
        const char *start_quote = strchr(found, '\"');
        if (start_quote) {
            start_quote++; // Move past the opening quote
            
            // Find the end of the value
            const char *end_quote = strchr(start_quote, '\"');
            if (end_quote) {
                size_t len = end_quote - start_quote;
                
                // Safety clamp
                if (len >= buffer_size) {
                    len = buffer_size - 1;
                }
                
                memcpy(out_buffer, start_quote, len);
                out_buffer[len] = '\0'; // Null-terminate
            }
        }
    }
}


/**
 * @brief Assigns a static IPv6 address (Mesh-Local Prefix + ::1).
 * This ensures the server always has a predictable IP for sensors to target.
 */
static void setup_static_ipv6(void) {
    otInstance *instance = openthread_get_default_instance();
    otNetifAddress aAddress;
    otError error;

    // Get the Mesh-Local Prefix (usually fdde:ad00:beef:0::/64)
    const otMeshLocalPrefix *ml_prefix = otThreadGetMeshLocalPrefix(instance);
    
    // Define the Interface ID (last 64 bits): ::0000:0000:0000:0001
    uint8_t interfaceID[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    // Construct the full 128-bit IPv6 Address
    memcpy(&aAddress.mAddress.mFields.m8[0], ml_prefix, 8);
    memcpy(&aAddress.mAddress.mFields.m8[8], interfaceID, 8);

    aAddress.mPrefixLength = 64;
    aAddress.mPreferred = true;
    aAddress.mValid = true;

    // Apply the address to the Thread Interface
    error = otIp6AddUnicastAddress(instance, &aAddress);
    
    if (error == OT_ERROR_NONE) {
        // We use printk here so it is visible immediately on boot
        LOG_INF("Static IPv6 Assigned: ...::1");
    } else {
        LOG_ERR("Failed to assign Static IP: %d", error);
    }
}

/**
 * @brief Sends a CoAP ACK (2.04 Changed) back to the sensor.
 * This confirms we received the data so the sensor stops retrying.
 */
static void send_ack_response(otMessage *request_message, const otMessageInfo *message_info) {    
    otError error = OT_ERROR_NONE;
    otMessage *response;
    otInstance *instance = openthread_get_default_instance();

    // Create a new empty response message
    response = otCoapNewMessage(instance, NULL);
    if (response == NULL) {
        LOG_ERR("Failed to allocate ACK message");
        return;
    }

    // Initialize as ACK with Code "Changed" (2.04)
    otCoapMessageInitResponse(response, request_message, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);

    // Send it
    error = otCoapSendResponse(instance, response, message_info);
    if (error != OT_ERROR_NONE) {
        LOG_ERR("Failed to send ACK: %d", error);
        otMessageFree(response);
    }
}

/**
 * @brief Main Handler: Called when a sensor sends data to "/storedata".
 */
static void storedata_request_handler(void *context, otMessage *message, const otMessageInfo *message_info) {
    server_message_t msg;
    char room_name_buffer[20];

    // 1. Extract Sender IP
    otIp6AddressToString(&message_info->mPeerAddr, msg.source_ip, sizeof(msg.source_ip));

    // 2. Read Payload (JSON)
    uint16_t payload_offset = otMessageGetOffset(message);
    uint16_t length = otMessageRead(message, payload_offset, msg.json_payload, sizeof(msg.json_payload) - 1);
    msg.json_payload[length] = '\0';
    
    // 3. Push to Main Queue (for Serial Bridge to print)
    if (k_msgq_put(outgoing_queue, &msg, K_NO_WAIT) != 0) {
        LOG_WRN("Queue full! Dropping packet from %s", msg.source_ip);
    } else {
        // 4. Update Node Registry (Heartbeat)
        parse_room_name(msg.json_payload, room_name_buffer, sizeof(room_name_buffer));
        node_manager_update(msg.source_ip, room_name_buffer, outgoing_queue);
    }

    // 5. Send ACK if the sensor asked for confirmation
    if (otCoapMessageGetType(message) == OT_COAP_TYPE_CONFIRMABLE) {
        send_ack_response(message, message_info);
    }
}


void network_listener_init(struct k_msgq *queue_ptr){
    outgoing_queue = queue_ptr;

    // 1. Setup IP
    setup_static_ipv6();

    // 2. Start CoAP Service
    otInstance *instance = openthread_get_default_instance();
    m_storedata_resource.mContext = instance;

    otError error = otCoapStart(instance, COAP_PORT);
   if (error != OT_ERROR_NONE) {
        LOG_ERR("Failed to start CoAP Server: %d", error);
        return;
    }
    // 3. Register Resource
    otCoapAddResource(instance, &m_storedata_resource);
    LOG_INF("CoAP Server listening on: %s", URI_PATH);
}