#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "modules/system_health.h"
#include "modules/vtt_model.h"
#include "modules/messaging_service.h"
#include "sensor/dht20/dht20.c"

#if !DT_HAS_COMPAT_STATUS_OKAY(aosong_dht20)
#error "No aosong,dht20 compatible node found in the device tree"
#endif


const struct device *dht20_dev_a = DEVICE_DT_GET(DT_ALIAS(sensor_a));
const struct device *dht20_dev_b = DEVICE_DT_GET(DT_ALIAS(sensor_b));

int main(void)
{
        struct sensor_value temperature;
        struct sensor_value humidity;
        health_status_code_t status[2];
        vtt_state_t room_state;
        vtt_init(&room_state, VTT_MAT_SENSITIVE);
        msg_init();
        while (1) {
                
                // Sending a normal update
                msg_send_mold_status("DATA", "LivingRoom", 24.5, 60.2, 1.5, 1, true);

                // Sending an Alert (High Mold)
                msg_send_mold_status("ALERT", "LivingRoom", 26.0, 95.0, 3.5, 3, true);

                k_sleep(K_SECONDS(20));
        }
        // check_system_health(dht20_dev_a, dht20_dev_b, status);
        // while (1)
        // {
        //         if (device_is_ready(dht20_dev_a)){
        //                 sensor_sample_fetch(dht20_dev_a);
        //                 sensor_channel_get(dht20_dev_a, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
        //                 sensor_channel_get(dht20_dev_a, SENSOR_CHAN_HUMIDITY, &humidity);
        //                 printk("Temperature: %d.%06d C\n", temperature.val1, temperature.val2);
        //                 printk("Humidity: %d.%06d %%\n", humidity.val1, humidity.val2);
        //                 printk("Ready to run the VTT Model :D");
        //                 vtt_update(&room_state, sensor_value_to_double(&temperature), sensor_value_to_double(&humidity), 1.00f);
        //                 int risk = vtt_get_risk_level(&room_state);
        //                 switch (risk)
        //                 {
        //                         case MOLD_RISK_CLEAN:
        //                                 printk("\nAll good :D");
        //                                 break;
        //                         case MOLD_RISK_ALERT:
        //                                 printk("\nMold slightly growing");
        //                                 break;
                                        
        //                         case MOLD_RISK_WARNING:
        //                                 printk("\nMold definitely growing :p");
        //                                 break;
        //                         case MOLD_RISK_CRITICAL:
        //                                 printk("\nMOLD IS OUT THERE!");
        //                                 break;
        //                         default:
        //                                 printk("\nOut of Syllabus XD");
        //                                 break;
        //                 }
        //                 printk("\nPrinting Raw Data:");
        //                 printk("\nCurrent Mold Index: %.2f\n", room_state.mold_index);
        //                 printk("Max Possible Mold: %.2f\n", room_state.max_possible_index);
        //                 printk("Growing Conditions: %d\n", room_state.condition);
        //                 printk("Sleeping for 60 mins....\n");
        //                 k_sleep(K_MINUTES(60));
        //         }
        // }
        return 0;
}
