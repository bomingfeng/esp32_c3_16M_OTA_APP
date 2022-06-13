#include "ESP32_C3_SOC_Temp_Task.h"

MessageBufferHandle_t esp32degC; //换算2831 = 28.31
extern MessageBufferHandle_t tcp_send_data;
extern char * tcprx_buffer;

void esp32_c3_soc_temp_init(void)
{
    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI("esp32_c3_soc_temp", "Initializing Temperature sensor");
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor_get_config(&temp_sensor);
    ESP_LOGI("esp32_c3_soc_temp", "default dac %d, clk_div %d", temp_sensor.dac_offset, temp_sensor.clk_div);
    temp_sensor.dac_offset = TSENS_DAC_DEFAULT; // DEFAULT: range:-10℃ ~  80℃, error < 1℃.
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
    ESP_LOGI("esp32_c3_soc_temp", "Temperature sensor started");
}

/******************************************************************************/
void tempsensor_example(void *arg)
{
    float tsens_out = 0;
    int degC = 0;
    while(1)
    {
        for(int a = 0; a < 18;a++)    
        {
            temp_sensor_read_celsius(&tsens_out);
            degC = degC + ((int)(((tsens_out * 0.84) + 0.005) * 100));
            ESP_LOGI("esp32_c3_soc_temp", "ESP32-C3 Sensor reports++ %d deg C\n",degC);
           vTaskDelay(pdMS_TO_TICKS(5000));
        }
        degC = degC/18;
        printf("ESP32-C3 Sensor reportsAVG %d deg C\n",degC);
        xMessageBufferSend(esp32degC,&degC,4,portMAX_DELAY);
    }
}