#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "sensor/dht20/dht20.c"

#if !DT_HAS_COMPAT_STATUS_OKAY(aosong_dht20)
#error "No aosong,dht20 compatible node found in the device tree"
#endif


const struct device *dht20_dev_a = DEVICE_DT_GET(DT_ALIAS(sensor_a));
const struct device *dht20_dev_b = DEVICE_DT_GET(DT_ALIAS(sensor_b));

int main(void)
{
        printf("Testing DHT20\n");
        if (!device_is_ready(dht20_dev_a)) {
                printk("DHT20 Sensor A not found!\n");
                return 0;
        }
        if (!device_is_ready(dht20_dev_b)) {
                printk("DHT20 Sensor B not found!\n");
                return 0;
        }
        struct sensor_value temperature;
        struct sensor_value humidity;
        int rca, rcb;

        rca = sensor_sample_fetch(dht20_dev_a);
        rcb = sensor_sample_fetch(dht20_dev_b);
        if (rca != 0 && rcb != 0) {
                printk("sensor_sample_fetch failed: %d\n", rca);
        }
        else {
                rca = sensor_channel_get(dht20_dev_a, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
                if (rca != 0) {
                        printk("sensor_channel_get failed - Sensor A: %d\n", rca);
                } else {
                        printk("\nSensor A:\nTemperature: %d.%06d C\n", temperature.val1, temperature.val2);
                }

                rca = sensor_channel_get(dht20_dev_a, SENSOR_CHAN_HUMIDITY, &humidity);
                if (rca != 0) {
                        printk("sensor_channel_get failed - Sensor A: %d\n", rca);
                } else {
                        printk("Humidity: %d.%06d %%\n", humidity.val1, humidity.val2);
                }

                rcb = sensor_channel_get(dht20_dev_b, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
                if (rcb != 0) {
                        printk("sensor_channel_get failed - Sensor B: %d\n", rcb);
                } else {
                        printk("\nSensor B:\nTemperature: %d.%06d C\n", temperature.val1, temperature.val2);
                }

                rcb = sensor_channel_get(dht20_dev_b, SENSOR_CHAN_HUMIDITY, &humidity);
                if (rcb != 0) {
                        printk("sensor_channel_get failed - Sensor B %d\n", rcb);
                } else {
                        printk("Humidity: %d.%06d %%\n", humidity.val1, humidity.val2);
                }
        }
        return 0;
}
