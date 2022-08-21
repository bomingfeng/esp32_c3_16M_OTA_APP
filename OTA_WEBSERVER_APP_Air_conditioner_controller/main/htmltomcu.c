#include "htmltomcu.h"

extern nvs_handle_t my_handle;
extern TimerHandle_t time_sleep_timers;
extern MessageBufferHandle_t HtmlToMcuData;


void htmltomcudata_task(void * arg)
{
    char data[96];
    uint8_t i,data_ok,data_len,first_bit = 0,second_bit = 1,first_len,second_len,en_ac_off = 0;

    char ssid[32];      
    char password[64];

    uint32_t adc_data  = 168,ac_time = 0;
    int hour = 0,min = 0;
    time_t now;
    struct tm timeinfo;

    esp_err_t err;

    while(1)
    {
        data_len = xMessageBufferReceive(HtmlToMcuData,&data,96,(60000 / portTICK_PERIOD_MS)/*min*/ * 1);
        
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
            if(('a' == data[i]) && ('c' == data[i+1]) && ('_' == data[i+2]) && ('h' == data[i+3]) && \
                ('h' == data[i+4]) && ('m' == data[i+5]) && ('m' == data[i+6]) && (':' == data[i+7])){
                first_bit = i;
                data_ok = 5;
                en_ac_off = 0x55;
                break;
            }
        }
        if(data_ok == 2){
            //printf("first_bit:%d;second_bit:%d;\r\n",first_bit,second_bit);
            first_len = second_bit - (first_bit + 5);
            second_len = data_len - (second_bit + 5);
            //printf("first_len:%d;second_len:%d;\r\n",first_len,second_len);
            if((first_len > 0) && (second_len > 7)){
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
               // Open
                printf("\n");
                printf("Opening Non-Volatile Storage (NVS) handle... ");
                err = nvs_open("storage", NVS_READWRITE, &my_handle);
                if (err != ESP_OK) {
                    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                } else {
                    printf("Done\n");
                }   

                // Write
                printf("Updating restart counter in NVS ... \n");
                
                

                err=nvs_set_str(my_handle,"wifissid",ssid);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                err=nvs_set_str(my_handle,"wifipass",password);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                err=nvs_set_i8(my_handle,"wifi_mode",WIFI_MODE_STA);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                // Commit written value.
                // After setting any values, nvs_commit() must be called to ensure changes are written
                // to flash storage. Implementations may write to storage at other times,
                // but this is not guaranteed.
                printf("Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                // Close
                nvs_close(my_handle);
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
            // Open
                printf("\n");
                printf("Opening Non-Volatile Storage (NVS) handle... ");
                err = nvs_open("storage", NVS_READWRITE, &my_handle);
                if (err != ESP_OK) {
                    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                } else {
                    printf("Done\n");
                }   

                // Write
                printf("Updating restart counter in NVS ... ");
                
               
                err=nvs_set_u32(my_handle,"adc_config",adc_data);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                

                // Commit written value.
                // After setting any values, nvs_commit() must be called to ensure changes are written
                // to flash storage. Implementations may write to storage at other times,
                // but this is not guaranteed.
                printf("Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                // Close
                nvs_close(my_handle);
        }
        else if(data_ok == 4){
            first_len = data_len - (first_bit + 8);
            printf("first_len:%d;data_len:%d;first_bit:%d\r\n",first_len,data_len,first_bit);
            ac_time = 0;
            for(i = 0;i < first_len;i++){
                if((data[first_bit + i + 8] >= '0') && (data[first_bit + i + 8] <= '9')){
                    ac_time = 10 * ac_time + (data[first_bit + i + 8] - '0');
                }
            }
            if((ac_time > 0) && ((xEventGroupGetBits(APP_event_group) & APP_event_run_BIT) == APP_event_run_BIT)){
                xTimerStop(time_sleep_timers,portMAX_DELAY);
                xTimerChangePeriod(time_sleep_timers, (60000 / portTICK_PERIOD_MS)/*min*/ * ac_time,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
            }
            en_ac_off = 0x55;
            printf("ac_time ok:%d;\r\n",ac_time);
        }
        else if(data_ok == 5){
            first_len = data_len - (first_bit + 8);
            printf("first_len:%d;data_len:%d;first_bit:%d\r\n",first_len,data_len,first_bit);
            hour = 0;
            min = 0;
            if((data[first_bit + 0 + 8] >= '0') && (data[first_bit + 0 + 8] <= '9')){
                hour = 10 * hour + (data[first_bit + 0 + 8] - '0');
            }
            if((data[first_bit + 1 + 8] >= '0') && (data[first_bit + 1 + 8] <= '9')){
                hour = 10 * hour + (data[first_bit + 1 + 8] - '0');
            }
            if((data[first_bit + 3 + 8] >= '0') && (data[first_bit + 3 + 8] <= '9')){
                min = 10 * min + (data[first_bit + 3 + 8] - '0');
            }
            if((data[first_bit + 4 + 8] >= '0') && (data[first_bit + 4 + 8] <= '9')){
                min = 10 * min + (data[first_bit + 4 + 8] - '0');
            }
            en_ac_off = 0xaa;
            if((xEventGroupGetBits(APP_event_group) & APP_event_run_BIT) == APP_event_run_BIT){
                xTimerStop(time_sleep_timers,portMAX_DELAY);
                xTimerChangePeriod(time_sleep_timers, (60000 / portTICK_PERIOD_MS)/*min*/ * time_off *2,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
            }
            printf("ac_hhmm ok:hour%d:min%d;\r\n",hour,min);
        }

        if((xEventGroupGetBits(APP_event_group) & APP_event_run_BIT) == APP_event_run_BIT){
            time(&now);
            localtime_r(&now, &timeinfo);
            if((timeinfo.tm_hour == hour) && (timeinfo.tm_min == min) && (en_ac_off == 0xaa)){
                xTimerStop(time_sleep_timers,portMAX_DELAY);
                xTimerChangePeriod(time_sleep_timers, (60000 / portTICK_PERIOD_MS)/*min*/ * 1,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
                printf("时间模式关 ok:hour%d:min%d;\r\n",hour,min);
            }
        }        
    }
}
