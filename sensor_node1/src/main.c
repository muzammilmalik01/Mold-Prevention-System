#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "modules/system_health.h"
#include "sensor/dht20/dht20.c"

#if !DT_HAS_COMPAT_STATUS_OKAY(aosong_dht20)
#error "No aosong,dht20 compatible node found in the device tree"
#endif


const struct device *dht20_dev_a = DEVICE_DT_GET(DT_ALIAS(sensor_a));
const struct device *dht20_dev_b = DEVICE_DT_GET(DT_ALIAS(sensor_b));

int main(void)
{
        health_status_code_t status[2];
        check_system_health(dht20_dev_a, dht20_dev_b, status);
        return 0;
}
