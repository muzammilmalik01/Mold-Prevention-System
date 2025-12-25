#ifndef NETWORK_LISTENER_H
#define NETWORK_LISTENER_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/coap.h>

void network_listener_init(struct k_msgq *queue_ptr);

#endif