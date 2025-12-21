#include "messaging_service.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <openthread/coap.h>
#include <openthread/thread.h>
#include <stdio.h> 

LOG_MODULE_REGISTER(messaging, LOG_LEVEL_INF);

#define COAP_PORT 5683
// FIX 1: Use the FULL Server Address (ending in ::1)
#define SERVER_ADDR "fdde:ad00:beef:0:0:0:0:1" 
#define URI_PATH "storedata"

static char json_buffer[128];

// --- Callback for Delivery Confirmation ---
// FIX 2: This function catches the ACK (Success) or Timeout (Fail)
static void _delivery_report_cb(void *p_context, otMessage *p_message,
                                const otMessageInfo *p_message_info, otError result) 
{
    if (result == OT_ERROR_NONE) {
        LOG_INF("✅ Delivery Confirmed by Server!");
    } else {
        LOG_ERR("❌ Delivery Failed! Error: %d", result);
    }
}

static void _send_coap_payload(const char* payload_string) {
    otError error = OT_ERROR_NONE;
    otMessage *myMessage = NULL;
    otMessageInfo myMessageInfo;
    otInstance *myInstance = openthread_get_default_instance();

    do {
        myMessage = otCoapNewMessage(myInstance, NULL);
        if (myMessage == NULL) {
            LOG_ERR("Failed to allocate CoAP message");
            return;
        }

        otCoapMessageInit(myMessage, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_PUT);
        otCoapMessageAppendUriPathOptions(myMessage, URI_PATH);
        otCoapMessageAppendContentFormatOption(myMessage, OT_COAP_OPTION_CONTENT_FORMAT_JSON);
        otCoapMessageSetPayloadMarker(myMessage);

        error = otMessageAppend(myMessage, payload_string, (uint16_t)strlen(payload_string));
        if (error != OT_ERROR_NONE) break;

        memset(&myMessageInfo, 0, sizeof(myMessageInfo));
        otIp6AddressFromString(SERVER_ADDR, &myMessageInfo.mPeerAddr);
        myMessageInfo.mPeerPort = COAP_PORT;

        // FIX 3: Pass the callback function here!
        error = otCoapSendRequest(myInstance, myMessage, &myMessageInfo, _delivery_report_cb, NULL);

    } while (false);

    if (error != OT_ERROR_NONE) {
        LOG_ERR("CoAP Send Error: %d", error);
        if (myMessage) otMessageFree(myMessage);
    } else {
        LOG_INF("Sent: %s", payload_string);
    }
}

// ... rest of your public API functions remain the same ...
void msg_init(void) {
    otInstance *p_instance = openthread_get_default_instance();
    otCoapStart(p_instance, OT_DEFAULT_COAP_PORT); 
}

void msg_send_telemetry(float temp_c, float rh_percent, float mold_index, int status_code) {
    snprintf(json_buffer, sizeof(json_buffer), 
             "{\"type\":\"data\",\"T\":%.1f,\"H\":%.1f,\"M\":%.2f,\"S\":%d}", 
             (double)temp_c, (double)rh_percent, (double)mold_index, status_code);
    _send_coap_payload(json_buffer);
}