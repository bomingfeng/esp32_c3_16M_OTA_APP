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
    char data[96];
    uint8_t i,data_ok,data_len,first_bit = 0,second_bit = 1,first_len,second_len;

    uint8_t ssid[32];      
    uint8_t password[64];
    uint32_t adc_data,ac_time;

    //vTaskDelete(NULL);
    while(1)
    {
        data_len = xMessageBufferReceive(HtmlToMcuData,&data,96,portMAX_DELAY);
        
        printf("data_len:%d;data:%s;\r\n",data_len,data);
        /*
        for(i = 0;i < data_len;i++){
            printf("data[%d] = 0x%x;",i,data[i]);
        }
        printf("\r\n");
        */
        data_ok = 0;
        for(i = 0;i < 80;i++){
            if(('s' == data[i]) && ('s' == data[i+1]) && ('i' == data[i+2]) && ('d' == data[i+3]) && (':' == data[i+4])){
                first_bit = i;
                data_ok = 1;
            }
            if((data_ok == 1) && ('p' == data[i]) && ('a' == data[i+1]) && ('s' == data[i+2]) && ('s' == data[i+3]) && (':' == data[i+4])){
                second_bit = i;
                data_ok = 2;
                break;
            }
            if(('a' == data[i]) && ('d' == data[i+1]) && ('c' == data[i+2]) && (':' == data[i+3])){
                first_bit = i;
                data_ok = 3;
                break;
            }            
            if(('a' == data[i]) && ('c' == data[i+1]) && ('_' == data[i+2]) && ('t' == data[i+3]) && \
                ('i' == data[i+4]) && ('m' == data[i+5]) && ('e' == data[i+6]) && (':' == data[i+7])){
                first_bit = i;
                data_ok = 4;
                break;
            }
        }
        if(data_ok == 2){
            //printf("first_bit:%d;second_bit:%d;\r\n",first_bit,second_bit);
            first_len = second_bit - (first_bit + 5);
            second_len = data_len - (second_bit + 5);
            //printf("first_len:%d;second_len:%d;\r\n",first_len,second_len);
            if(first_len > 0){
                memset(ssid,'\0',sizeof(ssid));
                for(i = 0;i < first_len;i++){
                    ssid[i] = data[first_bit + i + 5];
                }

                memset(password,'\0',sizeof(password));
                for(i = 0;i < second_len;i++){
                    password[i] = data[second_bit + i + 5];
                }
                printf("ssid:%s;password:%s;\r\n",ssid,password);
                /*
                if(strcmp(&ssid,CONFIG_STATION_SSID) == 0){
                    printf("strcmp ssid OK\r\n");
                }
                if(strcmp(&password,CONFIG_STATION_PASSPHRASE) == 0){
                    printf("strcmp password OK\r\n");
                }
                */
            }   
        }
        else if(data_ok == 3){
            first_len = data_len - (first_bit + 4);
            //printf("first_len:%d;data_len:%d;first_bit:%d\r\n",first_len,data_len,first_bit);
            adc_data = 0;
            for(i = 0;i < first_len;i++){
                if((data[first_bit + i + 4] >= '0') && (data[first_bit + i + 4] <= '9')){
                    adc_data = 10 * adc_data + (data[first_bit + i + 4] - '0');
                }
            }
            printf("adc_data ok:%d;\r\n",adc_data);
        }
        else if(data_ok == 4){
            first_len = data_len - (first_bit + 4);
            //printf("first_len:%d;data_len:%d;first_bit:%d\r\n",first_len,data_len,first_bit);
            ac_time = 0;
            for(i = 0;i < first_len;i++){
                if((data[first_bit + i + 4] >= '0') && (data[first_bit + i + 4] <= '9')){
                    ac_time = 10 * ac_time + (data[first_bit + i + 4] - '0');
                }
            }
            xTimerStop(time_sleep_timers,portMAX_DELAY);
            xTimerChangePeriod(time_sleep_timers, (60000 / portTICK_PERIOD_MS)/*min*/ * ac_time,portMAX_DELAY);
            if((xEventGroupGetBits(APP_event_group) & APP_event_run_BIT) == APP_event_run_BIT)
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
            else
                xTimerStop(time_sleep_timers,portMAX_DELAY);
            printf("ac_time ok:%d;\r\n",ac_time);
        }
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

    //xTaskCreate(test_test, "test_test", 4096, NULL, ESP_TASK_PRIO_MIN + 1, NULL);
    
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
