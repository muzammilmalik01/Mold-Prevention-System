/**
 * @file network_listener.h
 * @brief CoAP Server and Network Interface.
 *
 * This module acts as the "Entry Point" for all radio traffic.
 * It initializes the OpenThread stack, assigns a static IPv6 address,
 * and sets up a CoAP resource (/storedata) to listen for incoming 
 * sensor measurements.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#ifndef NETWORK_LISTENER_H
#define NETWORK_LISTENER_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/coap.h>

/**
 * @brief Initializes the Network Listener.
 * * 1. Sets a Static IPv6 address (Mesh-Local + ::1).
 * 2. Starts the OpenThread CoAP Service.
 * 3. Registers the "storedata" resource handler.
 * 4. Connects the module to the central message queue.
 * * @param queue_ptr Pointer to the global server_queue for passing data to other threads.
 */
void network_listener_init(struct k_msgq *queue_ptr);

#endif