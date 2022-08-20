/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* ADC1 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "ADC1_single_read_Task.h"
#include "LED_Seg7Menu/LED_Seg7Menu.h"



//ADC Channels
#if CONFIG_IDF_TARGET_ESP32
        #define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_7
#elif CONFIG_IDF_TARGET_ESP32C3
        #define ADC1_EXAMPLE_CHAN0          ADC1_CHANNEL_4
#endif // CONFIG_IDF_TARGET_* 


//ADC Attenuation
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11

extern nvs_handle_t my_handle;
extern TimerHandle_t Seg7Timers;
extern uint32_t sse_data[sse_len];


void ADC1_single_read_Task(void *pvParam)
{
    int adc1_data;
    uint32_t adc_config1 = 168;
    //ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN));
 
    while (1) {
        adc1_data = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
        sse_data[2] = adc1_data;
        if(adc1_data >= adc_config1)
        {
            xTimerReset(Seg7Timers,portMAX_DELAY);
        }
        else
        {        
            xTimerStop(Seg7Timers,portMAX_DELAY);
            SegDyn_Hidden();
        }
        //printf("adc1_data:%d.\r\n",adc1_data);
        vTaskDelay(pdMS_TO_TICKS(10000));

        // Open
        //printf("\n");
        //printf("Opening Non-Volatile Storage (NVS) handle... ");
        
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK) 
        {
            //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        else 
        {
            // Read
            printf("Reading restart counter from NVS ... ");
            err = nvs_get_u32(my_handle, "adc_config", &adc_config1);
            switch (err) {
                case ESP_OK:
                    printf("Done\n");
                    printf("adc_config1 = %d\n", adc_config1);
                    break;
                case ESP_ERR_NVS_NOT_FOUND:
                    printf("The value is not initialized yet!\n");
                    break;
                default :
                    printf("Error (%s) reading!\n", esp_err_to_name(err));
            }
            // Close
            nvs_close(my_handle);
        }
    }
}
