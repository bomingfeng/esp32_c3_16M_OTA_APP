#include "led_Task.h"


#define LEDC_LS_TIMER          LEDC_TIMER_0
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH0_GPIO       (23)
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_LS_CH1_GPIO       (22)
#define LEDC_LS_CH1_CHANNEL    LEDC_CHANNEL_1

#define LEDC_TEST_CH_NUM       (2)

extern uint8_t ip_addr1,ip_addr2,ip_addr3,ip_addr4;
extern int32_t BLe_battery;

/*
    * Prepare individual configuration
    * for each channel of LED Controller
    * by selecting:
    * - controller's channel number
    * - output duty cycle, set initially to 0
    * - GPIO number where LED is connected to
    * - speed mode, either high or low
    * - timer servicing selected channel
    *   Note: if different channels use one timer,
    *         then frequency and bit_num of these channels
    *         will be the same
    */
ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
        {
        .channel    = LEDC_LS_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_LS_CH0_GPIO,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER,
#if CONFIG_IDF_TARGET_ESP32
        .flags.output_invert = 0
#elif CONFIG_IDF_TARGET_ESP32C3

#endif // CONFIG_IDF_TARGET_*   
    },
    {
        .channel    = LEDC_LS_CH1_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_LS_CH1_GPIO,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER,
#if CONFIG_IDF_TARGET_ESP32
        .flags.output_invert = 0
#elif CONFIG_IDF_TARGET_ESP32C3

#endif // CONFIG_IDF_TARGET_*   
    },
};

void LED_Task_init(void)
{
    xEventGroupSetBits(APP_event_group,APP_event_Standby_BIT);
    // Clear the bit
	xEventGroupClearBits(APP_event_group,APP_event_run_BIT | APP_event_IR_LED_flags_BIT);
}

void led_instructions(void *pvParam)
{
    uint8_t con = 0;
    uint8_t Standby,Runby;
    uint8_t pcWriteBuffer[2048];

    int ch;

    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,           // timer mode
        .timer_num = LEDC_LS_TIMER,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    Standby = 0xaa;
    Runby = 0xaa;

    // Initialize fade service.
    ledc_fade_func_install(0);

    while(1) 
    {
        EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_Standby_BIT | APP_event_run_BIT,\
                                                pdFALSE,pdFALSE, 100 / portTICK_PERIOD_MS);
        if((BLe_battery <= BLe_battery_low) || ((staBits & APP_event_BLE_CONNECTED_flags_BIT) != APP_event_BLE_CONNECTED_flags_BIT))
        {
            if((staBits & APP_event_IR_LED_flags_BIT) != 0)
            {
                xEventGroupClearBits(APP_event_group,APP_event_IR_LED_flags_BIT);
            }
            else if((((staBits & APP_event_Standby_BIT) != 0) && (Standby == 0xaa)) || (BLe_battery <= BLe_battery_low)  || ((staBits & APP_event_BLE_CONNECTED_flags_BIT) != APP_event_BLE_CONNECTED_flags_BIT))
            {
                Standby = 0x55;
                ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 300);
                ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
            }      
            else
            {
                Standby = 0xaa;
                ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 0);
                ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
            }

            if ((((staBits & APP_event_run_BIT) != 0) && (Runby == 0xaa)) || (((staBits &APP_event_30min_timer_BIT) == 0) && ((staBits & APP_event_run_BIT) != 0)))
            {
                Runby = 0x55;
                ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 4000);
                ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
            }      
            else
            {
                Runby = 0xaa;
                ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 0);
                ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
            }   
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        else
        {
            if((staBits & APP_event_IR_LED_flags_BIT) != 0)
            {
                xEventGroupClearBits(APP_event_group,APP_event_IR_LED_flags_BIT);
                Standby = 0x55;
            }
            else if(((staBits & APP_event_Standby_BIT) != 0) && (Standby == 0xaa))
            {
                Standby = 0x55;
                ledc_set_fade_with_time(ledc_channel[0].speed_mode, \
                        ledc_channel[0].channel, 300, 3000);
                ledc_fade_start(ledc_channel[0].speed_mode, \
                       ledc_channel[0].channel, LEDC_FADE_NO_WAIT);
            }      
            else
            {
                Standby = 0xaa;
                ledc_set_fade_with_time(ledc_channel[0].speed_mode, \
                    ledc_channel[0].channel, 0, 3000);
                ledc_fade_start(ledc_channel[0].speed_mode, \
                    ledc_channel[0].channel, LEDC_FADE_NO_WAIT);
            }

            if (((staBits &APP_event_30min_timer_BIT) == 0) && ((staBits & APP_event_run_BIT) != 0))
            {
                Runby = 0x55;
                ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 4000);
                ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
            }
            else if(((staBits & APP_event_run_BIT) != 0) && (Runby == 0xaa))
            {
                Runby = 0x55;
                ledc_set_fade_with_time(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, 4000, 3000);
                ledc_fade_start(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, LEDC_FADE_NO_WAIT);
            }      
            else
            {
                Runby = 0xaa;
                ledc_set_fade_with_time(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, 0, 3000);
                ledc_fade_start(ledc_channel[1].speed_mode,
                    ledc_channel[1].channel, LEDC_FADE_NO_WAIT);
            }
            vTaskDelay(3500 / portTICK_PERIOD_MS);
        }                                        
        
        /*
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
        }*/
    }
}
