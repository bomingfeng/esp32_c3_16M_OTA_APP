#include "ir_rx_task.h"
#include "app_init.h"
#include "led_Task.h"
#include "app_inclued.h"
#include "tcp_client.h"
#include "ir_tx_Task.h"
#include "temperature_control_task.h"
#include "BLE_Client.h"
#include "cpu_timer.h"
#include "MultiButton/MultiButton_poll_Task.h"
#include "sntp_task.h"
#include "LED_Seg7Menu/LED_Seg7Menu.h"
#include "ADC1_single_read_Task.h"
#include "htmltomcu.h"

EventGroupHandle_t APP_event_group;

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
/*
断电保持位
*/
RTC_DATA_ATTR uint32_t sleep_keep,test;
RTC_DATA_ATTR uint8_t sleep_ir_data[13];
 
uint8_t ip_addr1 = 0,ip_addr2 = 0,ip_addr3 = 0,ip_addr4 = 0;

extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
extern MessageBufferHandle_t ir_rx_data;
extern MessageBufferHandle_t ir_tx_data;
extern MessageBufferHandle_t ble_degC;  //换算2831 = 28.31
extern MessageBufferHandle_t ble_humidity;
extern MessageBufferHandle_t ble_Voltage;
extern MessageBufferHandle_t ds18b20degC;   //换算2831 = 28.31
extern rmt_channel_t example_rx_channel;
extern rmt_channel_t example_tx_channel;
extern MessageBufferHandle_t IRPS_temp;
extern MessageBufferHandle_t time_hour_min;
extern MessageBufferHandle_t HtmlToMcuData;
extern TimerHandle_t time_sleep_timers;


nvs_handle_t my_handle;

void test_test(void * arg)
{
    vTaskDelay(60000 / portTICK_PERIOD_MS);
    //vTaskDelete(NULL);
    while(1)
    {
        xEventGroupSetBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT);
		xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
        vTaskDelay(60000 / portTICK_PERIOD_MS);
        WIFI_Mode_Save(WIFI_MODE_AP);
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
} 
       
/*
 *
 * void app_main()
 *
 **/
void app_main()
{
    EventBits_t staBits;

    ESP_LOGI("app_main", "welcome !!! Compiled at:");
    ESP_LOGI("app_main", __TIME__);ESP_LOGI("app_main", " ");ESP_LOGI("app_main", __DATE__);ESP_LOGI("app_main", "\r\n");

    ESP_LOGI("app_main","Create the event group,Message......\r\n");
    // Init the event group
	APP_event_group = xEventGroupCreate();
    ble_degC = xMessageBufferCreate(8);
    ble_humidity = xMessageBufferCreate(8);
    ble_Voltage = xMessageBufferCreate(8);
    ds18b20degC = xMessageBufferCreate(8);
    time_hour_min = xMessageBufferCreate(6);
    tcp_send_data  = xMessageBufferCreate(132);
    ir_rx_data  = xMessageBufferCreate(17);
    ir_tx_data =  xMessageBufferCreate(17);
    IRPS_temp = xMessageBufferCreate(8);
    HtmlToMcuData = xMessageBufferCreate(100);

    ESP_LOGI("app_main","Init GPIO & nvs_flash.....\r\n");
    app_init();

    ESP_LOGI("app_main","Create Task.....\r\n");
    OTA_Task_init();
    // Need this task to spin up, see why in task			
    xTaskCreate(systemRebootTask, "rebootTask", 2048, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

    LED_Task_init();
    xTaskCreate(led_instructions, "led_instructions", 4096, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

	//MyWiFi_init();
    //xTaskCreate(wifi_ap_sta, "wifi_ap_sta", 2048, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
    xTaskCreate(vTaskWifiHandler, "vTaskWifiHandler", 6144, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

    xTaskCreate(ds18x20_task,      "ds18x20",3072, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

    ir_rx_task_init();
    xTaskCreate(example_ir_rx_task,"ir_rx",  3072, NULL, ESP_TASK_PRIO_MIN + 2, NULL);

    ir_tx_task_init();
    xTaskCreate(example_ir_tx_task,"ir_tx", 3072, NULL, ESP_TASK_PRIO_MIN + 1, NULL);//????

    tempps_task_init();
    xTaskCreate(IRps_task,"IRps_task",  3072, NULL, ESP_TASK_PRIO_MIN + 2,NULL);
    xTaskCreate(tempps_task,"tempps",  3072, NULL, ESP_TASK_PRIO_MIN + 1,NULL);

    xTaskCreate(test_test, "test_test", 4096, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
    
    xTaskCreate(htmltomcudata_task, "htmltomcudata", 4096, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
    
    //xTaskCreate(LED_Seg7Menu_Task, "LED_Seg7Menu", 4096, NULL, ESP_TASK_PRIO_MIN + 1, NULL);//????
    
/*   释放BT mode模式，释放内存   */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
    {
        vTaskDelay(8000 / portTICK_PERIOD_MS);
        xMessageBufferSend(ir_rx_data,sleep_ir_data,13,portMAX_DELAY);//
		vTaskDelay(8000 / portTICK_PERIOD_MS);
        staBits = xEventGroupWaitBits(APP_event_group,APP_event_run_BIT,\
                                        pdFALSE,pdFALSE,10000 / portTICK_PERIOD_MS);
        if((staBits & APP_event_run_BIT ) == APP_event_run_BIT)
        {                                        
            xMessageBufferSend(ir_tx_data,sleep_ir_data,13,portMAX_DELAY);//
        }
    }


    

    //xTaskCreate(MultiButton_poll_Task, "Button_poll_Task", 2048, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
    xTaskCreate(sntp_task, "sntp_task", 3072, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
    //xTaskCreate(ADC1_single_read_Task, "ADC1", 2048, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

#ifdef  XL0801
    extern void ble_adv_scan_Task(void * arg);
    xTaskCreate(ble_adv_scan_Task, "adv_scan", 6144, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

#endif

#ifdef  LYWSD03MMC
    staBits = xEventGroupWaitBits(APP_event_group,APP_event_run_BIT | APP_event_30min_timer_BIT,\
                                                pdFALSE,pdTRUE,portMAX_DELAY);
    if((staBits & (APP_event_run_BIT | APP_event_30min_timer_BIT)) == (APP_event_run_BIT | APP_event_30min_timer_BIT))
    {
        xTaskCreate(ble_init, "ble_init", 6144, NULL, ESP_TASK_PRIO_MIN + 1, NULL); 
    }
    //printf("Create ble_init Task.....\r\n");
#endif

}
