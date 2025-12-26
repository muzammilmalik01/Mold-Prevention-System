/**
 * @file serial_bridge.h
 * @brief Serial Output Bridge.
 *
 * This module acts as the "Consumer" of the data pipeline.
 * It runs in a dedicated thread that waits for messages to appear in the 
 * global queue. When a message arrives, it formats and prints it to the 
 * USB Serial Console (UART) for external processing (e.g., by a Python script).
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#ifndef SERIAL_BRIDGE_H
#define SERIAL_BRIDGE_H

#include <zephyr/kernel.h>

/**
 * @brief Initializes and starts the Serial Bridge Thread.
 *
 * This spawns a background thread that blocks (sleeps) until data 
 * is available in the provided queue. It uses 0% CPU while waiting.
 *
 * @param queue_ptr Pointer to the global server_queue containing incoming data.
 */
void serial_bridge_init(struct k_msgq *queue_ptr);
#endif