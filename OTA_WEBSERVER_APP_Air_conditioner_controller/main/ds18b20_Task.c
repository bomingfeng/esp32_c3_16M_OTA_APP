#include "ds18b20_task.h"

const gpio_num_t SENSOR_GPIO = CONFIG_DS18B20_SENSOR_GPIO;
const int MAX_SENSORS = CONFIG_DS18B20_SENSOR_MAX;
MessageBufferHandle_t ds18b20degC;   //换算2831 = 28.31

extern MessageBufferHandle_t tcp_send_data;
extern char * tcprx_buffer;

void ds18x20_test(void *arg)
{
    int sensor_count;
    float temp_c = 0;
    ds18x20_addr_t addrs[MAX_SENSORS];
    float temps[MAX_SENSORS];
    while(1)
    {
        // Every RESCAN_INTERVAL samples, check to see if the sensors connected
        // to our bus have changed.
        sensor_count = ds18x20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS);
        if (sensor_count < 1)
        {
            printf("No sensors detected!\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        else
        {
            // If there were more sensors found than we have space to handle,
            // just report the first MAX_SENSORS..
            if (sensor_count > MAX_SENSORS)
                sensor_count = MAX_SENSORS;
            int degC = 0;  
            for(int a = 0; a < 18;a++)    
            {
                ds18x20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);
                for (int j = 0; j < sensor_count; j++)
                {
                    // The ds18x20 address is a 64-bit integer, but newlib-nano
                    // printf does not support printing 64-bit values, so we
                    // split it up into two 32-bit integers and print them
                    // back-to-back to make it look like one big hex number.
                    temp_c = temps[j] * 0.97;
                }
                degC = degC + ((int)((temp_c + 0.005) * 100));
                ESP_LOGI("ds18b20", "DS18B20 Sensor reports++ %d deg C\n",degC);
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
            degC = degC/18;
            printf("DS18B20 Sensor reportsAVG %d deg C\n",degC);
            xMessageBufferSend(ds18b20degC,&degC,4,portMAX_DELAY);
        }
    }
}
