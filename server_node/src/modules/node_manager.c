/**
 * @file node_manager.c
 * @brief Implementation of the Node Registry and Watchdog logic.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "shared_types.h"
#include "node_manager.h"

// --- Configuration ---
#define MAX_NODES 10          /**< Maximum number of sensors to track */
#define TIMEOUT_SECONDS 15    /**< Time (in sec) before a node is considered dead */


// --- Logging & Globals ---
LOG_MODULE_REGISTER(node_mng, LOG_LEVEL_INF);
K_MUTEX_DEFINE(registry_lock); 
static node_info_t registry[MAX_NODES];

void node_manager_update(const char *ip_addr, const char *room_name) {
    
    bool found = false;
    int64_t now = k_uptime_get();

    // 1. Acquire Lock
    k_mutex_lock(&registry_lock, K_FOREVER);

    // 2. Search for EXISTING node to update
    for (int node = 0; node < MAX_NODES; node++){
        // Check if slot is occupied AND IP matches
        if (registry[node].source_ip[0] != '\0' && strcmp(registry[node].source_ip, ip_addr) == 0){
            registry[node].last_seen = now;

            // Update Room Name (handle case where sensor is renamed/moved)
            strncpy(registry[node].room_name, room_name, sizeof(registry[node].room_name) - 1);
            registry[node].room_name[sizeof(registry[node].room_name) - 1] = '\0'; // Ensure null-termination

            if (!registry[node].is_online){
                registry[node].is_online = true;
                LOG_INF("Node Reconnected: %s (%s)", ip_addr, room_name);
            }
            found = true;
            break;
        }
    }
    // 3. If not found, add as NEW node
    if (!found){
        for (int node = 0; node < MAX_NODES; node++){
            // Find first empty slot (indicated by empty IP string)
            if(registry[node].source_ip[0] == '\0'){
                // Copy Data safely
                strncpy(registry[node].source_ip, ip_addr, sizeof(registry[node].source_ip) - 1);
                registry[node].source_ip[sizeof(registry[node].source_ip) - 1] = '\0';

                strncpy(registry[node].room_name, room_name, sizeof(registry[node].room_name) - 1);
                registry[node].room_name[sizeof(registry[node].room_name) - 1] = '\0';

                registry[node].last_seen = now;
                registry[node].is_online = true;
                LOG_INF("New Node Registered: %s (%s)", ip_addr, room_name);
                found = true;
                break;
            }

            if(!found){
                LOG_WRN("Registry Full! Could not track new node: %s", ip_addr);
            }
        } 
    }
    // 3. Release the damn lock.
    k_mutex_unlock(&registry_lock);
}

void node_manager_check_timeout(struct k_msgq *queue_ptr){
    server_message_t msg;
    int64_t now = k_uptime_get();

    k_mutex_lock(&registry_lock, K_FOREVER);

    for (int node = 0; node < MAX_NODES; node++){
        // Skip empty slots
        if (registry[node].source_ip[0] == '\0') {
            continue;
        }

        if (!registry[node].is_online) {
            continue;
        }
        int64_t diff = now - registry[node].last_seen;
        
        // --- TIMEOUT CONDITION ---
        if((diff > (TIMEOUT_SECONDS * 1000)) && registry[node].is_online){
            // 1. Mark as Offline locally
            registry[node].is_online = false;
            
            // 2. Prepare Alert Message structure
            strncpy(msg.source_ip, registry[node].source_ip, sizeof(msg.source_ip) - 1);
            msg.source_ip[sizeof(msg.source_ip) - 1] = '\0';

            // 3. Format JSON Payload
            snprintf(msg.json_payload, sizeof(msg.json_payload), 
                     "{\"event\":\"node_lost\", \"room\":\"%s\", \"ip\":\"%s\"}", 
                     registry[node].room_name, registry[node].source_ip);

            // 4. Push to Queue (Non-blocking)
            if (k_msgq_put(queue_ptr, &msg, K_NO_WAIT) != 0) {
                LOG_WRN("Queue full! Dropping Timeout Alert for %s", registry[node].room_name);
                } else {
                    LOG_INF("TIMEOUT ALERT SENT: %s", registry[node].room_name);
                }
        }
    }
    k_mutex_unlock(&registry_lock);
}