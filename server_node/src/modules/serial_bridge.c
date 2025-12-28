/**
 * @file serial_bridge.c
 * @brief Implementation of the Serial Bridge logic.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>             
#include <zephyr/sys/printk.h>  
#include <zephyr/logging/log.h>
#include "serial_bridge.h"
#include "shared_types.h"

LOG_MODULE_REGISTER(serial_brg, LOG_LEVEL_INF);

// --- Configuration ---
#define SERIAL_PRIORITY 5
#define STACKSIZE 2048

// --- Globals ---
static struct k_msgq *outgoing_queue;

// --- Thread Data ---
struct k_thread serial_thread_data;
K_THREAD_STACK_DEFINE(serial_thread_stack, STACKSIZE);


/**
 * @brief The worker thread loop.
 *
 * Waits for data and prints it to the console with a [DATA] tag.
 * The tag helps external scripts filter out system logs.
 */
void serial_thread_entry(void *p1, void *p2, void *p3){
    server_message_t msg;

    // Use printk for raw output (bypasses log formatting timestamps)
    LOG_INF("--- Serial Bridge Started ---\n");

    while (1) {
        // 1. Wait Block: Sleeps until data arrives (Efficient)
        // K_FOREVER ensures this thread consumes 0 cycles when idle.
        if (k_msgq_get(outgoing_queue, &msg, K_FOREVER) == 0) {
            
            // 2. Output: Print the payload for Python/Dashboard
            // We use the "[DATA]:" prefix to make parsing robust against log noise.
            printk("[DATA]: %s | %s\n", msg.source_ip, msg.json_payload);
        }
    }
}

void serial_bridge_init(struct k_msgq *queue_ptr){
    outgoing_queue = queue_ptr;
    // Spawn the thread immediately
    k_thread_create(&serial_thread_data, 
        serial_thread_stack,
        K_THREAD_STACK_SIZEOF(serial_thread_stack),
        serial_thread_entry, 
        NULL, NULL, NULL,
        SERIAL_PRIORITY, 
        0, 
        K_NO_WAIT);
}