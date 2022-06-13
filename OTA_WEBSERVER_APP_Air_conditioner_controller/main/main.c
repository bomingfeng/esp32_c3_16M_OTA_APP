#include "ir_rx_task.h"
#include "app_init.h"
#include "led_Task.h"
#include "app_inclued.h"
#include "tcp_client.h"
#include "ir_tx_Task.h"
#include "temperature_control_task.h"
#include "BLE_Client.h"

EventGroupHandle_t APP_event_group;

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
/*
断电保持位
*/
RTC_DATA_ATTR uint32_t sleep_keep;
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
extern MessageBufferHandle_t esp32degC; //换算2831 = 28.31
extern rmt_channel_t example_rx_channel;
extern rmt_channel_t example_tx_channel;
extern MessageBufferHandle_t IRPS_temp;

void test_test(void * arg)
{
    while(1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
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

    printf("welcome ！！！Compiled at:");printf(__TIME__);printf(" ");printf(__DATE__);printf("\r\n");

    printf("Create the event group,Message......\r\n");
    // Init the event group
	APP_event_group = xEventGroupCreate();

    ble_degC = xMessageBufferCreate(8);
    ble_humidity = xMessageBufferCreate(8);
    ble_Voltage = xMessageBufferCreate(8);
    ds18b20degC = xMessageBufferCreate(8);
    esp32degC = xMessageBufferCreate(8);
    tcp_send_data  = xMessageBufferCreate(132);
    ir_rx_data  = xMessageBufferCreate(17);
    ir_tx_data =  xMessageBufferCreate(17);
    IRPS_temp = xMessageBufferCreate(8);

    printf("Init GPIO & nvs_flash.....\r\n");
    app_init();

    printf("Create Task.....\r\n");
    OTA_Task_init();
    // Need this task to spin up, see why in task			
	xTaskCreate(systemRebootTask, "rebootTask", 2048, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

    LED_Task_init();
    xTaskCreate(led_instructions, "led_instructions", 4596, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

	MyWiFi_init();
    xTaskCreate(wifi_ap_sta, "wifi_ap_sta", 2048, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
   
    esp32_c3_soc_temp_init();
    /* 用户配置任务的优先级数值越小，那么此任务的优先级越低，空闲任务的优先级是 0。configMAX_PRIORITIES */
    xTaskCreate(tempsensor_example,"temp",        3072, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
   
    xTaskCreate(ds18x20_test,      "ds18x20",3072, NULL, ESP_TASK_PRIO_MIN + 1, NULL);

    ir_rx_task_init();
    xTaskCreate(example_ir_rx_task,"ir_rx",  3072, NULL, ESP_TASK_PRIO_MIN + 2, NULL);

    ir_tx_task_init();
    xTaskCreate(example_ir_tx_task,"ir_tx", 3072, NULL, ESP_TASK_PRIO_MIN + 1, NULL);//????

    tempps_task_init();
    xTaskCreate(IRps_task,"IRps_task",  3072, NULL, ESP_TASK_PRIO_MIN + 2,NULL);
    xTaskCreate(tempps_task,"tempps",  3072, NULL, ESP_TASK_PRIO_MIN + 1,NULL);
//    xTaskCreate(test_test, "test_test", 4096, NULL, ESP_TASK_PRIO_MIN + 1, NULL);//????

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
    
    staBits = xEventGroupWaitBits(APP_event_group,APP_event_run_BIT | APP_event_30min_timer_BIT,\
                                                pdFALSE,pdTRUE,portMAX_DELAY);
    if((staBits & (APP_event_run_BIT | APP_event_30min_timer_BIT)) == (APP_event_run_BIT | APP_event_30min_timer_BIT))
    {
        xTaskCreate(ble_init, "ble_init", 6144, NULL, ESP_TASK_PRIO_MIN + 1, NULL); 
    }
    printf("Create ble_init Task.....\r\n");
}
