#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/coap.h>

#include "network_listener.h"
#include "serial_bridge.h"
#include "shared_types.h"

#define STACKSIZE 2048

LOG_MODULE_REGISTER(server_main, LOG_LEVEL_INF);

K_MSGQ_DEFINE(server_queue, sizeof(server_message_t), 10, 4);

struct k_thread network_thread_data;

K_THREAD_STACK_DEFINE(network_thread_stack, STACKSIZE);

void network_thread_entrypoint(void *p1, void *p2, void *p3){
	LOG_INF("Starting Network Listener...");
	network_listener_init(&server_queue);
	serial_bridge_init(&server_queue);

	while(1){
		k_msleep(10000); 
	}
}

int main(void) {
	k_thread_create(&network_thread_data, network_thread_stack, K_THREAD_STACK_SIZEOF(network_thread_stack), network_thread_entrypoint, NULL,NULL,NULL, 1, 0, K_NO_WAIT);
	return 0;
}