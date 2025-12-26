/**
 * @file shared_types.h
 * @brief Common Data Structures.
 *
 * This file defines the shared message formats used for inter-thread 
 * communication. These structures allow the Network Listener to pass 
 * data to the Serial Bridge and Node Manager in a consistent format.
 * @authors: muzamil.py, Google Gemini 3 Pro
 * NOTE: Main Architecture designed by muzamil.py. Optimization, Code Reviews and Code Documentation by Google Gemini 3 Pro.
 */
#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

/**
 * @brief The Standard Message Envelope.
 * * This structure is used in the main 'server_queue'.
 * It acts as a container for data moving from the radio (CoAP) 
 * to the output (Serial Console).
 */
typedef struct {
    /** * @brief The raw JSON data string.
     * Contains either sensor data (e.g., {"temp": 24}) or 
     * system alerts (e.g., {"event": "node_lost"}).
     * sized to 256 bytes to accommodate moderate JSON complexity.
     */
    char json_payload[256]; 

    /** * @brief The IPv6 address of the sender.
     * stored as a string (e.g., "fdde:ad00:beef:0:0:0:0:1").
     * Used for identifying which sensor sent the data.
     */
    char source_ip[64];     
} server_message_t;

#endif