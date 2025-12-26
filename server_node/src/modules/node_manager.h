/**
 * @file node_manager.h
 * @brief Node Registry and Watchdog Module.
 *
 * This module maintains a "live list" of all connected sensor nodes.
 * It tracks the last time a heartbeat was received from each node and 
 * automatically generates alerts if a node goes silent for longer 
 * than the configured timeout period.
 *
 * @note This module is thread-safe.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/**
 * @brief Structure representing a single Sensor Node in the registry.
 */
typedef struct {
    char source_ip[64];    /**< Unique IPv6 Address (Primary Key) */
    char room_name[20];    /**< Friendly Name (e.g., "Living Room") */
    int64_t last_seen;     /**< System uptime (ms) when last packet arrived */
    bool is_online;        /**< Current connection status flag */
} node_info_t;

/**
 * @brief Updates the registry when a valid packet is received.
 * * Call this function from the Network Thread. It resets the watchdog 
 * timer for the specific node. If the node is new, it is added to the 
 * registry. If the node was marked offline, it is marked online.
 *
 * @param ip_addr   The IPv6 string of the sender.
 * @param room_name The friendly room name extracted from the JSON payload.
 */
void node_manager_update(const char *ip_addr, const char *room_name);

/**
 * @brief Periodically checks for dead nodes.
 * * This function should be called periodically (e.g., every 5 seconds) 
 * from a low-priority background thread. It iterates through the registry 
 * and checks if the time since 'last_seen' exceeds the threshold.
 *
 * If a timeout is detected, a JSON alert is pushed to the server_queue.
 *
 * @param queue_ptr Pointer to the main outgoing message queue.
 */
void node_manager_check_timeout(struct k_msgq *queue_ptr);

#endif