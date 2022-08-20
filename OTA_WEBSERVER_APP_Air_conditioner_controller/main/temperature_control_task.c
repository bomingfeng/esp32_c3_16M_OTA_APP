#include "temperature_control_task.h"
#include "LED_Seg7Menu/LED_Seg7Menu.h"

extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
extern MessageBufferHandle_t ble_humidity;
extern MessageBufferHandle_t ble_Voltage;
extern MessageBufferHandle_t ble_degC;  //换算2831 = 28.31
extern MessageBufferHandle_t ds18b20degC;   //换算2831 = 28.31
extern MessageBufferHandle_t ir_tx_data;
extern MessageBufferHandle_t ir_rx_data;
extern MessageBufferHandle_t time_hour_min;
extern RTC_DATA_ATTR uint8_t sleep_ir_data[13];

extern int32_t BLe_battery;
extern nvs_handle_t my_handle;
extern ledc_channel_config_t ledc_channel[2];

extern uint32_t sse_data[sse_len];

MessageBufferHandle_t IRPS_temp;

TimerHandle_t xTimers0,xTimers1,xTimers2,io_sleep_timers,time_sleep_timers;

//const unsigned char SegDigCode[10];//正常1~10
const unsigned char SegDigRevCode[11] = {SEG7_CODE_0_Rev,SEG7_CODE_1_Rev,SEG7_CODE_2_Rev,SEG7_CODE_3_Rev,SEG7_CODE_4_Rev,
									  SEG7_CODE_5_Rev,SEG7_CODE_6_Rev,SEG7_CODE_7_Rev,SEG7_CODE_8_Rev,SEG7_CODE_9_Rev,SEG7_CODE_C_Rev | SEG7_CODE_DP};//第三个数码上下对调,11 = 温度符位。

const unsigned char SegDigRevRevCode[11] = {SEG7_CODE_0_Rev_Rev,SEG7_CODE_1_Rev_Rev,SEG7_CODE_2_Rev_Rev,SEG7_CODE_3_Rev_Rev,SEG7_CODE_4_Rev_Rev,
									  SEG7_CODE_5_Rev_Rev,SEG7_CODE_6_Rev_Rev,SEG7_CODE_7_Rev_Rev,SEG7_CODE_8_Rev_Rev,SEG7_CODE_9_Rev_Rev,SEG7_CODE_C_Rev_Rev | SEG7_CODE_DP_Rev_Rev};//第三个数码上下对调,11 = 温度符位。

/*
    xTimerStart( xTimers[ x ], 0 )

    void vTimerCallback( TimerHandle_t xTimer )
    {
        xTimerStop( pxTimer, 0 );
    }

    xTimerReset()
*/
void vTimer0Callback(TimerHandle_t xTimer)
{
    xTimerStop( xTimers0,portMAX_DELAY);
    xEventGroupSetBits(APP_event_group,APP_event_30min_timer_BIT);

}

void vTimer1Callback(TimerHandle_t xTimer)
{
    xTimerStop(xTimers1,portMAX_DELAY);
    

}

void vTimer2Callback(TimerHandle_t xTimer)
{
    xTimerStop(xTimers2,portMAX_DELAY);

}


void io_sleep_timersCallback(TimerHandle_t xTimer)
{
    xTimerStop(io_sleep_timers,portMAX_DELAY);
    xEventGroupClearBits(APP_event_group,APP_event_io_sleep_timer_BIT);
    for(uint8_t i = 0;i < 13;i++)
    {
        sleep_ir_data[i] =0;
    }
    xEventGroupSetBits(APP_event_group,APP_event_IO_wakeup_sleep_BIT);
}

void time_sleep_timersCallback(TimerHandle_t xTimer)
{
    uint8_t ir_ps_data[13];
    xTimerStop(time_sleep_timers,portMAX_DELAY);
#ifdef  Gree 
/*  ir_ps_data[0] = 0x50;
    ir_ps_data[1] = 0x30;
    ir_ps_data[2] = 0x0c;
    ir_ps_data[3] = 0x51;
    ir_ps_data[4] = 0x80;
    ir_ps_data[5] = 0x00;
    ir_ps_data[6] = 0x00;
    ir_ps_data[7] = 0x11;
    ir_ps_data[8] = 0x00;
    ir_ps_data[9] = 0x00;
    ir_ps_data[10] = 0x00;
    ir_ps_data[11] = 0x00;
    ir_ps_data[12] = 0x00;
    格力空调关机开灯*/
    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x30;ir_ps_data[2] = 0x0c;ir_ps_data[3] = 0x51;
    ir_ps_data[4] = 0x80;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x00;ir_ps_data[7] = 0x11;
#endif

#ifdef  Auxgroup     
    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
    if((uxBits & APP_event_lighting_BIT) != 0)
    {
/*
ir_ps_data[0] = 0xc3;
ir_ps_data[1] = 0x03;
ir_ps_data[2] = 0x00;
ir_ps_data[3] = 0x00;
ir_ps_data[4] = 0x05;
ir_ps_data[5] = 0x00;
ir_ps_data[6] = 0x04;
ir_ps_data[7] = 0x00;
ir_ps_data[8] = 0x00;
ir_ps_data[9] = 0x00;
ir_ps_data[10] = 0x00;
ir_ps_data[11] = 0xa2;//BIT3显示不置位
ir_ps_data[12] = 0x11;
总关机 
*/
        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
        ir_ps_data[4] = 0x05;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x00;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xa2;
        ir_ps_data[12] = 0x11;
    }
    else
    {
/*
ir_ps_data[0] = 0xc3;
ir_ps_data[1] = 0x03;
ir_ps_data[2] = 0x00;
ir_ps_data[3] = 0x00;
ir_ps_data[4] = 0x05;
ir_ps_data[5] = 0x00;
ir_ps_data[6] = 0x04;
ir_ps_data[7] = 0x00;
ir_ps_data[8] = 0x00;
ir_ps_data[9] = 0x00;
ir_ps_data[10] = 0x00;
ir_ps_data[11] = 0xaa;//BIT3显示置位
ir_ps_data[12] = 0x19;
总关机 
*/
        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
        ir_ps_data[4] = 0x05;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x00;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
        ir_ps_data[12] = 0x19;
    }
#endif           
    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);

    for(uint8_t i = 0;i < 13;i++)
    {
        sleep_ir_data[i] =0;
    }
    xEventGroupSetBits(APP_event_group,APP_event_REBOOT_BIT);

}


void tempps_task_init(void)
{
    xTimers0 = xTimerCreate("Timer0",(60000 / portTICK_PERIOD_MS)/*min*/ * load_time,pdFALSE,( void * ) 0,vTimer0Callback);//30min
    xTimers1 = xTimerCreate("Timer1",(60000 / portTICK_PERIOD_MS)/*min*/ * 60,pdFALSE,( void * ) 0,vTimer1Callback);//60min
    xTimers2 = xTimerCreate("Timer2",(60000 / portTICK_PERIOD_MS)/*min*/ * 90,pdFALSE,( void * ) 0,vTimer2Callback);//90min
    io_sleep_timers = xTimerCreate("io_sleep_timers",(60000 / portTICK_PERIOD_MS)/*min*/ * sleep_time,pdFALSE,( void * ) 0,io_sleep_timersCallback);//min
    time_sleep_timers = xTimerCreate("time_sleep_timers",(60000 / portTICK_PERIOD_MS)/*min*/ * time_off,pdFALSE,( void * ) 0,time_sleep_timersCallback);
    xEventGroupClearBits(APP_event_group,APP_event_30min_timer_BIT | APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
    xEventGroupSetBits(APP_event_group,APP_event_lighting_BIT | APP_event_SP_flags_BIT);
}

void IRps_task(void *arg)
{
    uint8_t ir_ps_data[13];
    uint32_t IR_temp = 0,ir_time_off = 0;
    uint8_t i;
    EventBits_t uxBits;
    xTimerReset(io_sleep_timers,portMAX_DELAY);
    xEventGroupSetBits(APP_event_group,APP_event_io_sleep_timer_BIT);
    while(1)
    {   /* 等待IR接收发送正确的数据 */
        xMessageBufferReceive(ir_rx_data,(void *)ir_ps_data,sizeof(ir_ps_data),portMAX_DELAY);
        for(i = 0;i < 13;i++)
        {
            printf("ir_ps_data[%d] = 0x%02x;\r\n",i,ir_ps_data[i]);
        }
        
#ifdef  Gree
        if((ir_ps_data[0] & 0xf0) == 0x50)
        {
            for(i = 0;i < 13;i++)
            {
                sleep_ir_data[i] =  ir_ps_data[i];
            }
            if((ir_ps_data[3] & 0x08) == 0x08)    //判断是否开空调，开
            {
                xTimerStop(io_sleep_timers,portMAX_DELAY);  //关掉待机休眠定时器
                xEventGroupSetBits(APP_event_group,APP_event_SP_flags_BIT);  
                xEventGroupClearBits(APP_event_group,APP_event_io_sleep_timer_BIT | APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
                uxBits = xEventGroupGetBits(APP_event_group);
                if((uxBits & APP_event_run_BIT) == 0)
                {
                    xTimerReset(xTimers0,portMAX_DELAY);
                }
                xEventGroupClearBits(APP_event_group,APP_event_REBOOT_BIT);
                xEventGroupSetBits(APP_event_group,APP_event_run_BIT);  //开空调LED亮
                xEventGroupClearBits(APP_event_group,APP_event_Standby_BIT);//关空调LED灯灭。

                xTimerStop(time_sleep_timers,portMAX_DELAY);
                if(ir_ps_data[2] & 0x80)    //
                {
                    ir_time_off = (60000 / portTICK_PERIOD_MS)/*min*/ * (((ir_ps_data[1] & 0x0F) * 60) + (((ir_ps_data[2] & 0x10) >> 4) * 30) + (((ir_ps_data[2] & 0x60) >> 5) * 600));
                }
                else
                {
                    ir_time_off = (60000 / portTICK_PERIOD_MS)/*min*/ * time_off;
                }
                xTimerChangePeriod(time_sleep_timers,ir_time_off,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
                
                IR_temp = ir_ps_data[2] & 0x0f;
                switch (IR_temp)
                {
                    case 0:
                        IR_temp = 2606; //1600
                        break;
                    case 1:
                        IR_temp = 2620; //1700
                        break;
                    case 2:
                        IR_temp = 2634; //1800
                        break;
                    case 3:
                        IR_temp = 2648; //1900
                        break;
                    case 4:
                        IR_temp = 2662; //2000
                        break;
                    case 5:
                        IR_temp = 2676; //2100
                        break;
                    case 6:
                        IR_temp = 2690; //2200
                        break;
                    case 7:
                        IR_temp = 2704;//23
                        break;
                    case 8:
                        IR_temp = 2718;//24
                        break;
                    case 9:
                        IR_temp = 2732;//25
                        break;
                    case 10:
                        IR_temp = 2746;//26
                        break;
                    case 11:
                        IR_temp = 2760;//2700
                        break;
                    case 12:
                        IR_temp = 2780;
                        break;
                    case 13:
                        IR_temp = 2780;
                        break;
                    case 14:
                        IR_temp = 2780;
                        break;
                    case 15:
                        IR_temp = 2780;
                        break;    
                    default:	
                        IR_temp = 2750; 
                        break;
                }
                xMessageBufferSend(IRPS_temp,&IR_temp,4,portMAX_DELAY);
                /*
                ir_ps_data[0] = 0x50;
                ir_ps_data[1] = 0x30;
                ir_ps_data[2] = 0x00;
                ir_ps_data[3] = 0x79;
                ir_ps_data[4] = 0x40;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x00;
                ir_ps_data[7] = 0x11;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x00;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0x00;
                ir_ps_data[12] = 0x00;
                16开机，开灯,风速强劲。左右上下扫风开

                ir_ps_data[0] = 0x50;
                ir_ps_data[1] = 0x30;
                ir_ps_data[2] = 0x08;
                ir_ps_data[3] = 0x79;
                ir_ps_data[4] = 0xc0;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x00;
                ir_ps_data[7] = 0x11;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x00;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0x00;
                ir_ps_data[12] = 0x00;
                24开机，开灯,风速强劲。左右上下扫风开
                ir ok
                ir_ps_data[0] = 0x50;
                ir_ps_data[1] = 0x30;
                ir_ps_data[2] = 0x07;
                ir_ps_data[3] = 0x79;
                ir_ps_data[4] = 0xb0;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x00;
                ir_ps_data[7] = 0x11;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x00;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0x00;
                ir_ps_data[12] = 0x00;
                23开机，开灯,风速强劲。左右上下扫风开((uxBits & APP_event_ds18b20_CONNECTED_flags_BIT) == APP_event_ds18b20_CONNECTED_flags_BIT)
                */
                /*if((((ir_ps_data[2] & 0x0f) >= 6) && ((uxBits & APP_event_ds18b20_CONNECTED_flags_BIT) == APP_event_ds18b20_CONNECTED_flags_BIT)) || \
                 (((ir_ps_data[2] & 0x0f) >= 6) && ((uxBits & APP_event_BLE_CONNECTED_flags_BIT) == APP_event_BLE_CONNECTED_flags_BIT)))
                {
                    ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x30;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0x40;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
                    vTaskDelay(800/portTICK_PERIOD_MS);
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }*/
            }
            else    //关
            {          
                for(uint8_t i = 0;i < 13;i++)
                {
                    sleep_ir_data[i] =0;
                }  
                uxBits = xEventGroupGetBits(APP_event_group);
                if((uxBits & APP_event_Standby_BIT) == 0)
                {
                /*  ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x30;
                    ir_ps_data[2] = 0x0c;
                    ir_ps_data[3] = 0x51;
                    ir_ps_data[4] = 0x80;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
                    格力空调关机开灯*/
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x30;ir_ps_data[2] = 0x0c;ir_ps_data[3] = 0x51;
                    ir_ps_data[4] = 0x80;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x00;ir_ps_data[7] = 0x11;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);

                    xEventGroupSetBits(APP_event_group,APP_event_Standby_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_run_BIT);
                    xEventGroupSetBits(APP_event_group,APP_event_REBOOT_BIT);
                }
                xTimerReset(io_sleep_timers,portMAX_DELAY);
                xEventGroupSetBits(APP_event_group,APP_event_io_sleep_timer_BIT);
            }
        }
#endif

#ifdef  Auxgroup  
        if((ir_ps_data[0] & 0xc3) == 0xc3)
        {
            for(i = 0;i < 13;i++)
            {
                sleep_ir_data[i] =  ir_ps_data[i];
            }
            if((ir_ps_data[9] & 0x04) == 0x04)    //判断是否开空调，开
            {
                xTimerStop(io_sleep_timers,portMAX_DELAY);  //关掉待机休眠定时器
                xEventGroupSetBits(APP_event_group,APP_event_SP_flags_BIT);
                xEventGroupClearBits(APP_event_group,APP_event_io_sleep_timer_BIT | APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
                uxBits = xEventGroupGetBits(APP_event_group);
                if((uxBits & APP_event_run_BIT) == 0)
                {
                    xTimerReset(xTimers0,portMAX_DELAY);
                }
                xEventGroupClearBits(APP_event_group,APP_event_REBOOT_BIT);
                xEventGroupSetBits(APP_event_group,APP_event_run_BIT);  //开空调LED亮
                xEventGroupClearBits(APP_event_group,APP_event_Standby_BIT);//关空调LED灯灭。

                xTimerStop(time_sleep_timers,portMAX_DELAY);
                if(((ir_ps_data[5] & 0x78) != 0) || ((ir_ps_data[4] & 0xf8) != 0))    //
                {
                    if(ir_ps_data[5] & 0x78)
                    {
                        ir_time_off = 30;
                    }
                    else
                    {
                        ir_time_off = 0;
                    }
                    uint8_t bit;
                    uint8_t num = (ir_ps_data[4] & 0xf8);
                    uint32_t new_num = 0;
                    for (i = 0; i < 8; i++)
                    {
                        bit = num & 1;            //取出最后一位
                        new_num <<= 1;            //新数左移
                        new_num = new_num | bit;   //把刚取出的一位加到新数
                        num >>= 1;                //原数右移，准备取第二位
                    }
                    new_num *= 60;
                    ir_time_off = (60000 / portTICK_PERIOD_MS)/*min*/ * (new_num + ir_time_off);
                }
                else
                {
                    ir_time_off = (60000 / portTICK_PERIOD_MS)/*min*/ * time_off;
                }
                xTimerChangePeriod(time_sleep_timers,ir_time_off,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
                
                IR_temp = ir_ps_data[1] & 0x1f;
                switch (IR_temp)
                {
                    case 0x02:IR_temp = 2606;//16
                        break;
                    case 0x12:IR_temp = 2620;//17
                        break;
                    case 0x0a:IR_temp = 2634;//18
                        break;
                    case 0x1a:IR_temp = 2648;//19
                        break;
                    case 0x06:IR_temp = 2662;//20
                        break;
                    case 0x16:IR_temp = 2676;//21
                        break;
                    case 0x0e:IR_temp = 2690;//22
                        break;
                    case 0x1e:IR_temp = 2704;//23
                        break;
                    case 0x01:IR_temp = 2718;//24
                        break;
                    case 0x11:IR_temp = 2732;//25
                        break;
                    case 0x09:IR_temp = 2746;//26
                        break;
                    case 0x19:IR_temp = 2760;//27
                        break;
                    case 0x05:IR_temp = 2780;//28
                        break;
                    case 0x15:IR_temp = 2780;//29
                        break;
                    case 0x0d:IR_temp = 2780;//30
                        break;
                    case 0x1d:IR_temp = 2780;
                        break;
                    case 0x03:IR_temp = 2780; 
                        break;
                    default:  IR_temp = 2780;
                        break;
                }
                if((ir_ps_data[11] & 0x08) == 0x08)
                {
                    uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                        xEventGroupSetBits(APP_event_group,APP_event_lighting_BIT);
                    }
                }
                xMessageBufferSend(IRPS_temp,&IR_temp,4,portMAX_DELAY);
            }
            else    //关
            {            
                for(uint8_t i = 0;i < 13;i++)
                {
                    sleep_ir_data[i] =0;
                }
                if((ir_ps_data[11] & 0x08) == 0x08)
                {
                    uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                        xEventGroupSetBits(APP_event_group,APP_event_lighting_BIT);
                    }
                }
                uxBits = xEventGroupGetBits(APP_event_group);
                if((uxBits & APP_event_Standby_BIT) == 0)
                {
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {


                /*
                ir_ps_data[0] = 0xc3;
                ir_ps_data[1] = 0x03;
                ir_ps_data[2] = 0x00;
                ir_ps_data[3] = 0x00;
                ir_ps_data[4] = 0x05;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x04;
                ir_ps_data[7] = 0x00;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x00;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0xa2;//BIT3显示不置位
                ir_ps_data[12] = 0x11;
                总关机 
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x05;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x00;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xa2;
                        ir_ps_data[12] = 0x11;
                    }
                    else
                    {
                /*
                ir_ps_data[0] = 0xc3;
                ir_ps_data[1] = 0x03;
                ir_ps_data[2] = 0x00;
                ir_ps_data[3] = 0x00;
                ir_ps_data[4] = 0x05;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x04;
                ir_ps_data[7] = 0x00;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x00;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0xaa;//BIT3显示置位
                ir_ps_data[12] = 0x19;
                总关机 
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x05;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x00;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x19;
                    }
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                    xEventGroupSetBits(APP_event_group,APP_event_Standby_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_run_BIT);
                    xEventGroupSetBits(APP_event_group,APP_event_REBOOT_BIT);
                }
                xTimerReset(io_sleep_timers,portMAX_DELAY);
                xEventGroupSetBits(APP_event_group,APP_event_io_sleep_timer_BIT);    
            }
        }
#endif
        
        uxBits = xEventGroupGetBits(APP_event_group);
        //printf("onoff:%d;ir_time_off:%dmin;IR_temp:%d C\r\n",(uxBits & APP_event_run_BIT) ? 1:0,ir_time_off/6000,IR_temp);
        if(((uxBits & APP_event_ds18b20_CONNECTED_flags_BIT) != APP_event_ds18b20_CONNECTED_flags_BIT) && \
                ((uxBits & APP_event_BLE_CONNECTED_flags_BIT) != APP_event_BLE_CONNECTED_flags_BIT))
        {
            vTaskDelay(200/portTICK_PERIOD_MS);
        }
        ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 0);
        ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
    }
     
}

void tempps_task(void *arg)
{
    uint8_t ir_ps_data[13];
    uint32_t bleC,i = 0;  //换算2831 = 28.31
    uint32_t humidity_ble = 0;
    uint32_t Voltage_ble;
    uint32_t ds18b20C;   //换算2831 = 28.31
    uint32_t IR_temp = 2800;
    uint16_t hour_min = 0xC000;
    uint8_t send_flags = 0x55;
    uint8_t VoltageL,VoltageH;
    uint8_t dec_time = 0;
    EventBits_t staBits;
    VoltageL = 0xaa;
    VoltageH = 0xaa;
    Voltage_ble = BLe_battery_low + 1;
    //TickType_t xLastWakeTime = xTaskGetTickCount();//获取当前系统时间

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
        //printf("Done\n");

        // Read
        //printf("Reading restart counter from NVS ... \n");
        
        err = nvs_get_i32(my_handle, "BLe_battery", &BLe_battery);
        switch (err) 
        {
            case ESP_OK:
                //printf("Done\n");
                //printf("Restart counter = %d\n", BLe_battery);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                //printf("The value is not initialized yet!\n");
                break;
            default :
                //printf("Error (%s) reading!\n", esp_err_to_name(err));
                break;
        }
        // Close
        nvs_close(my_handle);
    }

    while(1)
    {
        xMessageBufferReceive(IRPS_temp,&IR_temp,4,100/portTICK_PERIOD_MS);
        if(bleC == 0)
        {
            bleC = IR_temp;
        }

        if(ds18b20C == 0)
        {
            ds18b20C = IR_temp;
        }

        xMessageBufferReceive(ds18b20degC,&ds18b20C,4,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(time_hour_min,&hour_min,2,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(ble_humidity,&humidity_ble,4,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(ble_Voltage,&Voltage_ble,4,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(ble_degC,&bleC,4,100/portTICK_PERIOD_MS);
        
        if(((Voltage_ble != BLe_battery) && (Voltage_ble <= BLe_battery_low) && (VoltageL == 0xaa)) || ((Voltage_ble != BLe_battery) && (Voltage_ble >= BLe_battery_High) && (VoltageH == 0xaa)))
        {
            if(Voltage_ble <= BLe_battery_low)
            {
                VoltageL = 0x55;
            }
            if(Voltage_ble >= BLe_battery_High)
            {
                VoltageH = 0x55;
            }
            BLe_battery = Voltage_ble;
            
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
                //printf("Done\n");
                // Write
                //printf("Updating restart counter in NVS ... ");
                err = nvs_set_i32(my_handle, "BLe_battery", BLe_battery);
                //printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

                // Commit written value.
                // After setting any values, nvs_commit() must be called to ensure changes are written
                // to flash storage. Implementations may write to storage at other times,
                // but this is not guaranteed.
                //printf("Committing updates in NVS ... ");
                err = nvs_commit(my_handle);
                //printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
            }
            // Close
            nvs_close(my_handle);
        }


        staBits = xEventGroupGetBits(APP_event_group);//xEventGroupWaitBits(APP_event_group,APP_event_run_BIT | APP_event_30min_timer_BIT,pdFALSE,pdTRUE,100/portTICK_PERIOD_MS);
            
        if(((staBits & (APP_event_run_BIT | APP_event_30min_timer_BIT | APP_event_BLE_CONNECTED_flags_BIT)) == \
                      (APP_event_run_BIT | APP_event_30min_timer_BIT | APP_event_BLE_CONNECTED_flags_BIT)) || \
                      ((staBits & (APP_event_run_BIT | APP_event_30min_timer_BIT | APP_event_ds18b20_CONNECTED_flags_BIT)) == \
                      (APP_event_run_BIT | APP_event_30min_timer_BIT | APP_event_ds18b20_CONNECTED_flags_BIT)))
        {
            if((Voltage_ble >= BLe_battery_low) || ((staBits & APP_event_BLE_CONNECTED_flags_BIT) == APP_event_BLE_CONNECTED_flags_BIT))
            {
            
                if((bleC >= (IR_temp + Sp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_SP_flags_BIT) == 0))
                {
                    //开
#ifdef  Gree 
                     ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x00;
                    ir_ps_data[2] = 0x0a;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0xe0;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
#endif  

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                        

                        ir_ps_data[0] = 0xc3;
                        ir_ps_data[1] = 0x19;
                        ir_ps_data[2] = 0x00;
                        ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;
                        ir_ps_data[5] = 0x01;
                        ir_ps_data[6] = 0x04;
                        ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x04;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0xaa;//BIT3显示置位
                        ir_ps_data[12] = 0x0b;
                        /*皇龙湾主卧奥克斯空调。27制冷 开（开机状态）静音，左右上下都摆风。*/
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                        ir_ps_data[0] = 0xc3;
                        ir_ps_data[1] = 0x19;
                        ir_ps_data[2] = 0x00;
                        ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;
                        ir_ps_data[5] = 0x01;
                        ir_ps_data[6] = 0x04;
                        ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x04;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x82;//BIT3显示不置位
                        ir_ps_data[12] = 0x3d;
                        /*皇龙湾主卧奥克斯空调。27制冷 开（开机状态）静音，左右上下都摆风。*/
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_SP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
                    send_flags = 0x55;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                    ESP_LOGI("xEventBits", "23开机，关灯,风速最大。\r\n");
                }
                if(((bleC <= (IR_temp - Lp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) == 0)))
                {
                    //关
#ifdef  Gree 
                    ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x00;
                    ir_ps_data[2] = 0x0d;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0x10;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
#endif 

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                        ir_ps_data[0] = 0xc3;
                        ir_ps_data[1] = 0x15;
                        ir_ps_data[2] = 0x00;
                        ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;
                        ir_ps_data[5] = 0x01;
                        ir_ps_data[6] = 0x04;
                        ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x04;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0xaa;//BIT3显示置位
                        ir_ps_data[12] = 0x07;
                        /*皇龙湾主卧奥克斯空调。29制冷 开（开机状态）静音，左右上下都摆风。*/
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                        ir_ps_data[0] = 0xc3;
                        ir_ps_data[1] = 0x15;
                        ir_ps_data[2] = 0x00;
                        ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;
                        ir_ps_data[5] = 0x01;
                        ir_ps_data[6] = 0x04;
                        ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x04;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x02;//BIT3显示不置位
                        ir_ps_data[12] = 0xd3;
                        /*皇龙湾主卧奥克斯空调。29制冷 开（开机状态）静音，左右上下都摆风。*/
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                     send_flags = 0xa5;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                    ESP_LOGI("xEventBits", "关机，关灯,风速最大。\r\n");
                }
                if(((bleC <= (IR_temp - LLp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LLP_flags_BIT) == 0)))
                {
                    //全关
#ifdef  Gree 
                    ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x00;
                    ir_ps_data[2] = 0x0e;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0x20;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
#endif 

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                        ir_ps_data[0] = 0xc3;
                        ir_ps_data[1] = 0x0d;
                        ir_ps_data[2] = 0x00;
                        ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;
                        ir_ps_data[5] = 0x01;
                        ir_ps_data[6] = 0x04;
                        ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x04;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0xaa;//BIT3显示置位
                        ir_ps_data[12] = 0x17;
                         /*皇龙湾主卧奥克斯空调。30制冷 开（开机状态）静音，左右上下都摆风。*/
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                        ir_ps_data[0] = 0xc3;
                        ir_ps_data[1] = 0x0d;
                        ir_ps_data[2] = 0x00;
                        ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;
                        ir_ps_data[5] = 0x01;
                        ir_ps_data[6] = 0x04;
                        ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x04;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x02;//BIT3显示不置位
                        ir_ps_data[12] = 0xcb;
                         /*皇龙湾主卧奥克斯空调。30制冷 开（开机状态）静音，左右上下都摆风。*/
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LLP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                    send_flags = 0xaa;
                    if((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) != 0)
                    {
                        vTaskDelay(2000/portTICK_PERIOD_MS);
                        xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                        VoltageL = 0xaa;
                    }
                    ESP_LOGI("xEventBits", "全关机，关灯,风速最大。\r\n");
                }
            }
            else
            {
                    if((ds18b20C >= (IR_temp + Sp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_SP_flags_BIT) == 0))
                {
                    //开
#ifdef  Gree 
                    ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x00;
                    ir_ps_data[2] = 0x0a;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0xe0;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
#endif                   

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                /*
                ir_ps_data[0] = 0xc3;
                ir_ps_data[1] = 0x09;
                ir_ps_data[2] = 0x00;
                ir_ps_data[3] = 0x00;
                ir_ps_data[4] = 0x05;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x04;
                ir_ps_data[7] = 0x00;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x04;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0xa8;
                ir_ps_data[12] = 0x12;

                    26降温 开（开机状态）自动风速 左右上下开
                */
                        ir_ps_data[0] = 0xc3;
                ir_ps_data[1] = 0x09;
                ir_ps_data[2] = 0x00;
                ir_ps_data[3] = 0x00;
                ir_ps_data[4] = 0x05;
                ir_ps_data[5] = 0x00;
                ir_ps_data[6] = 0x04;
                ir_ps_data[7] = 0x00;
                ir_ps_data[8] = 0x00;
                ir_ps_data[9] = 0x04;
                ir_ps_data[10] = 0x00;
                ir_ps_data[11] = 0xa8;
                ir_ps_data[12] = 0x12;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x09;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x05;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x80;
                    ir_ps_data[12] = 0x2c;
                    26降温 开（开机状态）自动风速 左右上下开
                */
                        ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x09;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x05;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x80;
                    ir_ps_data[12] = 0x2c;
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_SP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
                    send_flags = 0x55;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
                if(((ds18b20C <= (IR_temp - Lp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) == 0)) )
                {
                    //关
#ifdef  Gree 
                    ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x00;
                    ir_ps_data[2] = 0x0d;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0x10;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
#endif  

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                ir_ps_data[0] = 0xc3;
ir_ps_data[1] = 0x15;
ir_ps_data[2] = 0x00;
ir_ps_data[3] = 0x00;
ir_ps_data[4] = 0x05;
ir_ps_data[5] = 0x00;
ir_ps_data[6] = 0x04;
ir_ps_data[7] = 0x00;
ir_ps_data[8] = 0x00;
ir_ps_data[9] = 0x04;
ir_ps_data[10] = 0x00;
ir_ps_data[11] = 0xa8;
ir_ps_data[12] = 0x06;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                ir_ps_data[0] = 0xc3;
ir_ps_data[1] = 0x15;
ir_ps_data[2] = 0x00;
ir_ps_data[3] = 0x00;
ir_ps_data[4] = 0x05;
ir_ps_data[5] = 0x00;
ir_ps_data[6] = 0x04;
ir_ps_data[7] = 0x00;
ir_ps_data[8] = 0x00;
ir_ps_data[9] = 0x04;
ir_ps_data[10] = 0x00;
ir_ps_data[11] = 0x80;
ir_ps_data[12] = 0x32;
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                     send_flags = 0xa5;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
                if(((ds18b20C <= (IR_temp - LLp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LLP_flags_BIT) == 0)))
                {
                    //全关
#ifdef  Gree 
                    ir_ps_data[0] = 0x50;
                    ir_ps_data[1] = 0x00;
                    ir_ps_data[2] = 0x0e;
                    ir_ps_data[3] = 0x79;
                    ir_ps_data[4] = 0x20;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x00;
                    ir_ps_data[7] = 0x11;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x00;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x00;
                    ir_ps_data[12] = 0x00;
#endif 

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                ir_ps_data[0] = 0xc3;
ir_ps_data[1] = 0x0d;
ir_ps_data[2] = 0x00;
ir_ps_data[3] = 0x00;
ir_ps_data[4] = 0x05;
ir_ps_data[5] = 0x00;
ir_ps_data[6] = 0x04;
ir_ps_data[7] = 0x00;
ir_ps_data[8] = 0x00;
ir_ps_data[9] = 0x04;
ir_ps_data[10] = 0x00;
ir_ps_data[11] = 0xa8;
ir_ps_data[12] = 0x16;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                ir_ps_data[0] = 0xc3;
ir_ps_data[1] = 0x0d;
ir_ps_data[2] = 0x00;
ir_ps_data[3] = 0x00;
ir_ps_data[4] = 0x05;
ir_ps_data[5] = 0x00;
ir_ps_data[6] = 0x04;
ir_ps_data[7] = 0x00;
ir_ps_data[8] = 0x00;
ir_ps_data[9] = 0x04;
ir_ps_data[10] = 0x00;
ir_ps_data[11] = 0x00;
ir_ps_data[12] = 0xca;


                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LLP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                    send_flags = 0xaa;
                    if((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) != 0)
                    {
                        vTaskDelay(2000/portTICK_PERIOD_MS);
                        xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                        VoltageL = 0xaa;
                    }
                }
            }
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
        if((i >= 5000) && ((staBits &APP_event_30min_timer_BIT) != 0) && (Voltage_ble == (BLe_battery_low + 1)))
        {
            Voltage_ble = BLe_battery_low;
            i = 0;
        }
        #if 0
        if((i % 100) == 0)
        {
            TickType_t xRemainingTime;
            /* 计算xTimer引用的定时器之前剩余的时间
            到期。TickType_t 是无符号类型，因此减法将导致
            即使计时器直到滴答声之后才会过期，也是正确的答案
            计数已溢出。*/ 
            xRemainingTime = xTimerGetExpiryTime(time_sleep_timers) - xTaskGetTickCount();
            EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
            printf("onoff(1=开，0=关):%d;55=开 & aa=全关 & a5=关:%x;time_off:%d;IR_temp:%dC;bleC::%dC;humidity_ble:%d%%;Voltage_ble:%dmV;ds18b20C:%dC\r\n",   
            (uxBits & APP_event_run_BIT) ? 1:0,send_flags,(xRemainingTime/6000),IR_temp,bleC,humidity_ble,Voltage_ble,ds18b20C);

            /*tcprx_buffer = "Automatic control status 55=on & aa=Quanguan & a5=shut hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
			tcp_client_send(send_flags);

            tcprx_buffer = "Off time min hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            //tcp_client_send((xRemainingTime/6000));

            tcprx_buffer = "setting temperature /100 C hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            tcp_client_send(IR_temp);

            tcprx_buffer = "Bluetooth temperature /100 C hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            tcp_client_send(bleC);

            tcprx_buffer = "Bluetooth humidity /100 % hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            tcp_client_send(humidity_ble);

            tcprx_buffer = "Bluetooth Voltage /100 mV hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            tcp_client_send(Voltage_ble);

            tcprx_buffer = "ds18b20C temperature /100 C hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            tcp_client_send(ds18b20C);*/
        }
        #endif
        i++;


        EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
        if((dec_time >= 55) || (hour_min == 0xC000)) //0.74
        {
            if(dec_time >= 64)
            {
                dec_time = 0;
            }
            if((uxBits & APP_event_BLE_CONNECTED_flags_BIT) != APP_event_BLE_CONNECTED_flags_BIT)//ds18b20
            {
                segDisBuff[0] = SegDigCode[ds18b20C / 1000];
                segDisBuff[1] = SegDigCode[(ds18b20C % 1000) / 100] | SEG7_CODE_DP;
                segDisBuff[2] = SegDigRevCode[(ds18b20C % 100) / 10];
            }
            else //蓝牙的温度
            {
                segDisBuff[0] = SegDigCode[bleC / 1000];
                segDisBuff[1] = SegDigCode[(bleC % 1000) / 100] | SEG7_CODE_DP;
                segDisBuff[2] = SegDigRevCode[(bleC % 100) / 10];
            }
            segDisBuff[3] = SegDigRevRevCode[10];
        }
        else
        {
            segDisBuff[0] = SegDigCode[(hour_min >> 8) / 10];
            segDisBuff[1] = SegDigCode[(hour_min >> 8) % 10];
            segDisBuff[2] = SegDigRevCode[(hour_min & 0xFF) / 10];
            if((dec_time % 2) == 0)
            {                
                segDisBuff[1] |= SEG7_CODE_DP;
                segDisBuff[2] |= SEG7_CODE_DP;  

            }
            segDisBuff[3] = SegDigRevRevCode[(hour_min & 0xFF) % 10];//BLe_battery <= BLe_battery_low
            if((uxBits & (APP_event_SP_flags_BIT | APP_event_run_BIT)) == (APP_event_SP_flags_BIT | APP_event_run_BIT))
            {
                segDisBuff[3] |= SEG7_CODE_DP_Rev_Rev;
            }
        }
        if(segDisBuff[0] == SEG7_CODE_0)
        {
            segDisBuff[0] = SEG7_CODE_NULL;
        }
        if((Voltage_ble <= BLe_battery_low) || (((uxBits & APP_event_BLE_CONNECTED_flags_BIT) != APP_event_BLE_CONNECTED_flags_BIT) && ((uxBits & (APP_event_run_BIT | APP_event_30min_timer_BIT)) == (APP_event_run_BIT | APP_event_30min_timer_BIT))))
        {
            segDisBuff[0] |= SEG7_CODE_DP;
        }
        dec_time++;

        if((uxBits & APP_event_run_BIT) == APP_event_run_BIT){
            sse_data[4] |= BIT0;
        }
        else{
            sse_data[4] &= ~BIT0;
        }
        if((uxBits & (APP_event_run_BIT | APP_event_SP_flags_BIT)) == (APP_event_run_BIT | APP_event_SP_flags_BIT)){
            sse_data[4] |= BIT1;
        }
        else{
            sse_data[4] &= ~BIT1;
        }
        if((uxBits & (APP_event_run_BIT | APP_event_LP_flags_BIT)) == (APP_event_run_BIT | APP_event_LP_flags_BIT)){
            sse_data[4] |= BIT2;
        }
        else{
            sse_data[4] &= ~BIT2;
        }
        if((uxBits & (APP_event_run_BIT | APP_event_LLP_flags_BIT)) == (APP_event_run_BIT | APP_event_LLP_flags_BIT)){
            sse_data[4] |= BIT3;
        }
        else{
            sse_data[4] &= ~BIT3;
        }
        sse_data[6] = IR_temp;
        sse_data[7] = (xTimerGetExpiryTime(time_sleep_timers) - xTaskGetTickCount()) / (60000 / portTICK_PERIOD_MS);
    }
}