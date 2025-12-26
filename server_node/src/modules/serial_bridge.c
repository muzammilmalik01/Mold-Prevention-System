#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>             
#include <zephyr/sys/printk.h>  
#include "serial_bridge.h"
#include "shared_types.h"
#define SERIAL_PRIORITY 5

static struct k_msgq *outgoing_queue;


#define STACKSIZE 2048
struct k_thread serial_thread_data;
K_THREAD_STACK_DEFINE(serial_thread_stack, STACKSIZE);

void serial_thread_entry(void *p1, void *p2, void *p3){
    server_message_t msg;

    printk("--- Serial Bridge Started ---\n");

    while (1) {
        // Wait forever for data. This uses 0% CPU while waiting.
        if (k_msgq_get(outgoing_queue, &msg, K_FOREVER) == 0) {
            
            // Success! Print the data.
            printk("[DATA]: %s\n", msg.json_payload);
        }
    }
}

void serial_bridge_init(struct k_msgq *queue_ptr){
    outgoing_queue = queue_ptr;
    k_thread_create(&serial_thread_data, 
        serial_thread_stack,
        K_THREAD_STACK_SIZEOF(serial_thread_stack),
        serial_thread_entry, 
        NULL, NULL, NULL,
        SERIAL_PRIORITY, 
        0, 
        K_NO_WAIT);
}