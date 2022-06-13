#include "led_Task.h"

extern uint8_t ip_addr1,ip_addr2,ip_addr3,ip_addr4;

void LED_Task_init(void)
{
    xEventGroupSetBits(APP_event_group,APP_event_Standby_BIT);
    // Clear the bit
	xEventGroupClearBits(APP_event_group,APP_event_run_BIT);
}

void led_instructions(void *pvParam)
{
    uint8_t con = 0;
    uint8_t delay;
    portMUX_TYPE  test = portMUX_INITIALIZER_UNLOCKED;
    uint8_t pcWriteBuffer[2048];

    while(1) 
    {
        EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_Standby_BIT | APP_event_run_BIT,\
                                                pdFALSE,pdFALSE, 100 / portTICK_PERIOD_MS);
        if ((staBits & APP_event_Standby_BIT) != 0)
		{
            delay = 0x0c;
            
            portENTER_CRITICAL_SAFE(&test);
            gpio_set_level(3, 1);
            while(delay--);
            gpio_set_level(3, 0);
            portEXIT_CRITICAL_SAFE(&test);
        }                      
        if ((staBits & APP_event_run_BIT) != 0)
		{
            delay = 0x0c;
            
            portENTER_CRITICAL_SAFE(&test);
            gpio_set_level(4, 1);
            while(delay--);
            gpio_set_level(4, 0);
            portEXIT_CRITICAL_SAFE(&test);
        }      
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if ((sleep_keep & sleep_keep_Thermohygrometer_Low_battery_BIT) == sleep_keep_Thermohygrometer_Low_battery_BIT)
		{
            delay = 0x0F;
            portENTER_CRITICAL_SAFE(&test);
            gpio_set_level(19, 1);
            while(delay--);
            gpio_set_level(19, 0);
            portEXIT_CRITICAL_SAFE(&test);
        } 

        con++;
        if(con >= 100)
        {
            con = 0;
            tcp_client_send(ip_addr1);
            tcp_client_send(ip_addr2);
            tcp_client_send(ip_addr3);
            tcp_client_send(ip_addr4);
            vTaskList((char *)&pcWriteBuffer);
            printf("%s\r\n", pcWriteBuffer); 
        }
    }
}
