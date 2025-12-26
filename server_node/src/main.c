/**
 * @file main.c
 * @brief Server Node Entry Point (Orchestrator).
 *
 * This file sets up the RTOS environment. It:
 * 1. Defines the shared Message Queue used for inter-thread communication.
 * 2. Spawns the High-Priority Network Thread (Listener).
 * 3. Spawns the Low-Priority Node Manager Thread (Watchdog).
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/coap.h>

#include "network_listener.h"
#include "serial_bridge.h"
#include "node_manager.h"
#include "shared_types.h"

// --- Configuration ---
#define NETWORK_STACKSIZE 2048  
#define MANAGER_STACKSIZE 1024  

// --- Thread Priorities ---
// Lower number = Higher priority
#define PRIORITY_NETWORK 1       /**< HIGHEST: Must process radio packets immediately */
#define PRIORITY_MANAGER 7       /**< LOWEST: Background cleanup task */

LOG_MODULE_REGISTER(server_main, LOG_LEVEL_INF);

// --- Shared Resources ---
/**
 * @brief The Central Message Queue.
 * Acts as a buffer between the Network Listener (Producer) and the 
 * Serial Bridge (Consumer).
 * - Item Size: sizeof(server_message_t)
 * - Capacity: 10 messages
 * - Alignment: 4 bytes
 */
K_MSGQ_DEFINE(server_queue, sizeof(server_message_t), 10, 4);

// --- Thread Definitions ---
struct k_thread network_thread_data;
struct k_thread node_manager_data;

K_THREAD_STACK_DEFINE(network_thread_stack, NETWORK_STACKSIZE);
K_THREAD_STACK_DEFINE(node_manager_thread_stack, MANAGER_STACKSIZE);

/**
 * @brief Entry point for the Network Microservice.
 * Initializes the CoAP server and the Serial Bridge.
 */
void network_thread_entrypoint(void *p1, void *p2, void *p3){
	LOG_INF("Starting Network Listener...");

	// 1. Initialize the Network (Producer)
    network_listener_init(&server_queue);

    // 2. Initialize the Serial Bridge (Consumer)
    // This spawns its own internal thread to handle UART output.
    serial_bridge_init(&server_queue);

	// Thread yields forever (logic is handled by callbacks/interrupts)
    while (1) {
        k_msleep(10000); 
    }
}

/**
 * @brief Entry point for the Node Manager Microservice.
 * Periodically wakes up to check for dead nodes.
 */
void node_manager_thread_entrypoint(void *p1, void *p2, void *p3){
	LOG_INF("Starting Node Manager Thread...");
	
	// Allow network to settle before first check
    k_msleep(10000);

	while (1)
	{   
        // Run the Attendance Check
        node_manager_check_timeout(&server_queue);
        
        // Sleep until next check interval (e.g., 5 seconds)
        k_msleep(5000); 
	}
	
}

int main(void) {
	// 1. Spawn Network Thread (High Priority)
	k_thread_create(&network_thread_data, network_thread_stack, K_THREAD_STACK_SIZEOF(network_thread_stack), network_thread_entrypoint, NULL,NULL,NULL, 1, 0, K_NO_WAIT);

	// 2. Spawn Node Manager Thread (Low Priority)
    // Delayed start (5s) to let the network initialize first
	k_thread_create(&node_manager_data, node_manager_thread_stack, K_THREAD_STACK_SIZEOF(node_manager_thread_stack), node_manager_thread_entrypoint, NULL,NULL,NULL, 7, 0, K_SECONDS(5));
	return 0;
}