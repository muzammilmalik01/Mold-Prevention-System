#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/coap.h>
#include <string.h>             
#include <zephyr/sys/printk.h>  
#include "network_listener.h"
#include "shared_types.h"

LOG_MODULE_REGISTER(server_net, LOG_LEVEL_INF);

#define COAP_PORT 5683
#define URI_PATH "storedata"

static struct k_msgq *outgoing_queue;

static void storedata_request_handler(void *context, otMessage *message, const otMessageInfo *message_info);

static otCoapResource m_storedata_resource = {
    .mUriPath = URI_PATH,
    .mHandler = storedata_request_handler,
    .mContext = NULL,
    .mNext = NULL
};


static void setup_static_ipv6(void) {
    otInstance *instance = openthread_get_default_instance();
    otNetifAddress aAddress;
    otError error;

    // Get the Mesh-Local Prefix (usually fdde:ad00:beef:0::/64)
    const otMeshLocalPrefix *ml_prefix = otThreadGetMeshLocalPrefix(instance);
    
    // We want the address to end in ::1 (Interface ID = 1)
    uint8_t interfaceID[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    // Construct the full IPv6 Address
    memcpy(&aAddress.mAddress.mFields.m8[0], ml_prefix, 8);
    memcpy(&aAddress.mAddress.mFields.m8[8], interfaceID, 8);

    aAddress.mPrefixLength = 64;
    aAddress.mPreferred = true;
    aAddress.mValid = true;

    // Add it to the interface
    error = otIp6AddUnicastAddress(instance, &aAddress);
    
    if (error == OT_ERROR_NONE) {
        printk("Static IPv6 Assigned: ...::1");
    } else {
        printk("Failed to assign Static IP: %d", error);
    }
}

static void send_ack_response(otMessage *request_message, const otMessageInfo *message_info) {    
    otError error = OT_ERROR_NONE;
    otMessage *response;
    otInstance *instance = openthread_get_default_instance();

    response = otCoapNewMessage(instance, NULL);
    if (response == NULL) {
        return;
    }

    // Initialize as an ACK (Acknowledgment) with Code "Changed" (2.04) or "Valid"
    otCoapMessageInitResponse(response, request_message, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);

    error = otCoapSendResponse(instance, response, message_info);
    if (error != OT_ERROR_NONE) {
        otMessageFree(response);
    }
}

static void storedata_request_handler(void *context, otMessage *message, const otMessageInfo *message_info) {
    server_message_t msg;

    otIp6AddressToString(&message_info->mPeerAddr, msg.source_ip, sizeof(msg.source_ip));

    uint16_t payload_offset = otMessageGetOffset(message);
    uint16_t length = otMessageRead(message, payload_offset, msg.json_payload, sizeof(msg.json_payload) - 1);
    msg.json_payload[length] = '\0';
    
    if (k_msgq_put(outgoing_queue, &msg, K_NO_WAIT) != 0) {
        printk("Queue full! Dropping packet from %s", msg.source_ip);
    } else {
        // printk("\n\nReceived from %s: %s", msg.source_ip, msg.json_payload);
    }

    if (otCoapMessageGetType(message) == OT_COAP_TYPE_CONFIRMABLE) {
        send_ack_response(message, message_info);
    }
}

void network_listener_init(struct k_msgq *queue_ptr){
    outgoing_queue = queue_ptr;
    setup_static_ipv6();
    otInstance *instance = openthread_get_default_instance();
    m_storedata_resource.mContext = instance;
    otError error = otCoapStart(instance, COAP_PORT);
    if (error != OT_ERROR_NONE) {
        printk("Failed to start CoAP: %d", error);
        return;
    }

    otCoapAddResource(instance, &m_storedata_resource);
    LOG_INF("CoAP Server listening on: %s", URI_PATH);
}