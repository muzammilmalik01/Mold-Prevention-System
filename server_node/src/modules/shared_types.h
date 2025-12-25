// shared_types.h
#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

typedef struct {
    char json_payload[256]; // The raw data
    char source_ip[64];     // Who sent it (for your Node Registry)
} server_message_t;

#endif