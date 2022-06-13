#include "temperature_control_task.h"

extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
extern MessageBufferHandle_t ble_humidity;
extern MessageBufferHandle_t ble_Voltage;
extern MessageBufferHandle_t ble_degC;  //换算2831 = 28.31
extern MessageBufferHandle_t ds18b20degC;   //换算2831 = 28.31
extern MessageBufferHandle_t esp32degC; //换算2831 = 28.31
extern MessageBufferHandle_t ir_tx_data;
extern MessageBufferHandle_t ir_rx_data;
extern RTC_DATA_ATTR uint8_t sleep_ir_data[13];
MessageBufferHandle_t IRPS_temp;

TimerHandle_t xTimers0,xTimers1,xTimers2,io_sleep_timers,time_sleep_timers;

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
    xTimers0 = xTimerCreate("Timer0",6000 * load_time,pdFALSE,( void * ) 0,vTimer0Callback);//30min
    xTimers1 = xTimerCreate("Timer1",100 * 3600,pdFALSE,( void * ) 0,vTimer1Callback);//60min
    xTimers2 = xTimerCreate("Timer2",100 * 5400,pdFALSE,( void * ) 0,vTimer2Callback);//90min
    io_sleep_timers = xTimerCreate("io_sleep_timers",6000  * sleep_time,pdFALSE,( void * ) 0,io_sleep_timersCallback);//min
    time_sleep_timers = xTimerCreate("time_sleep_timers",6000  * time_off,pdFALSE,( void * ) 0,time_sleep_timersCallback);
    xEventGroupClearBits(APP_event_group,APP_event_30min_timer_BIT | APP_event_SP_flags_BIT |APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
    xEventGroupSetBits(APP_event_group,APP_event_lighting_BIT);
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
            gpio_set_level(18, 0);
            if((ir_ps_data[3] & 0x08) == 0x08)    //判断是否开空调，开
            {
                xTimerStop(io_sleep_timers,portMAX_DELAY);  //关掉待机休眠定时器
                xEventGroupClearBits(APP_event_group,APP_event_io_sleep_timer_BIT | APP_event_SP_flags_BIT |APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
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
                    ir_time_off = 6000 * (((ir_ps_data[1] & 0x0F) * 60) + (((ir_ps_data[2] & 0x10) >> 4) * 30) + (((ir_ps_data[2] & 0x60) >> 5) * 600));
                }
                else
                {
                    ir_time_off = 6000 * time_off;
                }
                xTimerChangePeriod(time_sleep_timers,ir_time_off,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
                
                IR_temp = ir_ps_data[2] & 0x0f;
                switch (IR_temp)
                {
                    case 0:
                        IR_temp = 1750; //1600
                        break;
                    case 1:
                        IR_temp = 1840; //1700
                        break;
                    case 2:
                        IR_temp = 1930; //1800
                        break;
                    case 3:
                        IR_temp = 2020; //1900
                        break;
                    case 4:
                        IR_temp = 2110; //2000
                        break;
                    case 5:
                        IR_temp = 2200; //2100
                        break;
                    case 6:
                        IR_temp = 2290; //2200
                        break;
                    case 7:
                        IR_temp = 2380;
                        break;
                    case 8:
                        IR_temp = 2470;
                        break;
                    case 9:
                        IR_temp = 2560;
                        break;
                    case 10:
                        IR_temp = 2650;
                        break;
                    case 11:
                        IR_temp = 2730;
                        break;
                    case 12:
                        IR_temp = 2800;
                        break;
                    case 13:
                        IR_temp = 2900;
                        break;
                    case 14:
                        IR_temp = 3000;
                        break;
                    case 15:
                        IR_temp = 2800;
                        break;    
                    default:	
                        IR_temp = 2800; 
                        break;
                }
                xMessageBufferSend(IRPS_temp,&IR_temp,4,portMAX_DELAY);
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
            gpio_set_level(18, 0);
            if((ir_ps_data[9] & 0x04) == 0x04)    //判断是否开空调，开
            {
                xTimerStop(io_sleep_timers,portMAX_DELAY);  //关掉待机休眠定时器
                xEventGroupClearBits(APP_event_group,APP_event_io_sleep_timer_BIT | APP_event_SP_flags_BIT |APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
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
                    ir_time_off = 6000 * (new_num + ir_time_off);
                }
                else
                {
                    ir_time_off = 6000 * time_off;
                }
                xTimerChangePeriod(time_sleep_timers,ir_time_off,portMAX_DELAY);
                xTimerReset(time_sleep_timers,portMAX_DELAY);   //IR接收到的定时关空调时间并启动
                
                IR_temp = ir_ps_data[1] & 0x1f;
                switch (IR_temp)
                {
                    case 0x02:IR_temp = 1750;
                        break;
                    case 0x12:IR_temp = 1840;
                        break;
                    case 0x0a:IR_temp = 1930;
                        break;
                    case 0x1a:IR_temp = 2020;
                        break;
                    case 0x06:IR_temp = 2110;
                        break;
                    case 0x16:IR_temp = 2200;
                        break;
                    case 0x0e:IR_temp = 2290;
                        break;
                    case 0x1e:IR_temp = 2380;
                        break;
                    case 0x01:IR_temp = 2470;
                        break;
                    case 0x11:IR_temp = 2560;
                        break;
                    case 0x09:IR_temp = 2650;
                        break;
                    case 0x19:IR_temp = 2725;
                        break;
                    case 0x05:IR_temp = 2780;
                        break;
                    case 0x15:IR_temp = 2900;
                        break;
                    case 0x0d:IR_temp = 3000;
                        break;
                    case 0x1d:IR_temp = 3100;
                        break;
                    case 0x03:IR_temp = 3200; 
                        break;
                    default:  IR_temp = 2800;
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
        printf("onoff:%d;ir_time_off:%dmin;IR_temp:%d C\r\n",(uxBits & APP_event_run_BIT) ? 1:0,ir_time_off/6000,IR_temp);

    }
     
}

void tempps_task(void *arg)
{
    uint8_t ir_ps_data[13];
    uint32_t bleC = 0;  //换算2831 = 28.31
    uint32_t humidity_ble = 0;
    uint32_t Voltage_ble = 0;
    uint32_t ds18b20C;   //换算2831 = 28.31
    uint32_t esp32C = 0; //换算2831 = 28.31
    uint32_t IR_temp = 2800;
    uint8_t send_flags = 0x55;
    uint8_t i = 0;
    while(1)
    {
        xMessageBufferReceive(IRPS_temp,&IR_temp,4,100/portTICK_PERIOD_MS);
        if(bleC == 0)
        {
            bleC = IR_temp;
        }
        if(esp32C == 0)
        {
            esp32C = IR_temp;
        }
        if(ds18b20C == 0)
        {
            ds18b20C = IR_temp;
        }

        xMessageBufferReceive(ds18b20degC,&ds18b20C,4,100/portTICK_PERIOD_MS);
        
        xMessageBufferReceive(ble_humidity,&humidity_ble,4,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(ble_Voltage,&Voltage_ble,4,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(ble_degC,&bleC,4,100/portTICK_PERIOD_MS);
        xMessageBufferReceive(esp32degC,&esp32C,4,100/portTICK_PERIOD_MS);
        
        EventBits_t staBits = xEventGroupWaitBits(APP_event_group,APP_event_run_BIT | APP_event_30min_timer_BIT,\
                                                pdFALSE,pdTRUE,100/portTICK_PERIOD_MS);
        if((staBits & (APP_event_run_BIT | APP_event_30min_timer_BIT)) == (APP_event_run_BIT | APP_event_30min_timer_BIT))
        {
            if ((sleep_keep & sleep_keep_Thermohygrometer_Low_battery_BIT) == sleep_keep_Thermohygrometer_Low_battery_BIT)
            {
                if((ds18b20C <= esp32C) && (ds18b20C >= (IR_temp + Sp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_SP_flags_BIT) == 0))
                {
                    //开
#ifdef  Gree 
                    /*  ir_ps_data[0] = 0x50;
                        ir_ps_data[1] = 0x00;
                        ir_ps_data[2] = 0x0c;
                        ir_ps_data[3] = 0x49;
                        ir_ps_data[4] = 0x00;
                        ir_ps_data[5] = 0x00;
                        ir_ps_data[6] = 0x00;
                        ir_ps_data[7] = 0x10;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x00;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x00;
                        ir_ps_data[12] = 0x00;
                        28开机，关灯,风速自动
                    */
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x00;ir_ps_data[2] = 0x0c;ir_ps_data[3] = 0x49;
                    ir_ps_data[4] = 0x00;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x00;ir_ps_data[7] = 0x10;
#endif                   

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x05;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0xaa;//BIT3显示置位
                    ir_ps_data[12] = 0x1a;
                    28降温 开（开机状态）1风速
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x05;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x1a;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x05;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x82;  //BIT3显示不置位
                    ir_ps_data[12] = 0x22;
                    28降温 开（开机状态）1风速
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x05;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0x82;
                        ir_ps_data[12] = 0x22;
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_SP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
                    send_flags = 0x55;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
                if(((ds18b20C <= (IR_temp - Lp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) == 0)) || ((esp32C <= (IR_temp - Lp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) == 0)))
                {
                    //关
#ifdef  Gree 
                    /*  
                        ir_ps_data[0] = 0x50;
                        ir_ps_data[1] = 0x00;
                        ir_ps_data[2] = 0x0e;
                        ir_ps_data[3] = 0x49;
                        ir_ps_data[4] = 0x20;
                        ir_ps_data[5] = 0x00;
                        ir_ps_data[6] = 0x00;
                        ir_ps_data[7] = 0x11;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x00;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x00;
                        ir_ps_data[12] = 0x00;
                        30开机，关灯,风速自动
                    */
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x00;ir_ps_data[2] = 0x0e;ir_ps_data[3] = 0x49;
                    ir_ps_data[4] = 0x20;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x00;ir_ps_data[7] = 0x11;
#endif  

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x1d;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0xaa; //BIT3显示置位
                    ir_ps_data[12] = 0x0f;
                    31升温。关（开机状态）静音
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x1d;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x0f;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x1d;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x12;//BIT3显示不置位
                    ir_ps_data[12] = 0xc7;
                    31升温。关（开机状态）静音
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x1d;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0x12;
                        ir_ps_data[12] = 0xc7;
                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                     send_flags = 0xa5;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
                if(((ds18b20C <= (IR_temp - LLp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LLP_flags_BIT) == 0)) || ((esp32C <= (IR_temp - LLp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LLP_flags_BIT) == 0)))
                {
                    //全关
#ifdef  Gree 
                    /*  
                        ir_ps_data[0] = 0x50;
                        ir_ps_data[1] = 0x00;
                        ir_ps_data[2] = 0x0c;
                        ir_ps_data[3] = 0x41;
                        ir_ps_data[4] = 0x80;
                        ir_ps_data[5] = 0x00;
                        ir_ps_data[6] = 0x00;
                        ir_ps_data[7] = 0x11;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x00;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x00;
                        ir_ps_data[12] = 0x00;
                        30开机，关灯,风速自动
                    */
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x00;ir_ps_data[2] = 0x0c;ir_ps_data[3] = 0x41;
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
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0xaa;//BIT3显示置位
                    ir_ps_data[12] = 0x1f;
                    32升温。关（开机状态）静音  
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x1f;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x03;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x12;//BIT3显示不置位
                    ir_ps_data[12] = 0xd7;
                    32升温。关（开机状态）静音
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0x12;
                        ir_ps_data[12] = 0xd7;


                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LLP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                    send_flags = 0xaa;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
            }
            else
            {
                if((bleC <= esp32C) && (bleC >= (IR_temp + Sp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_SP_flags_BIT) == 0))
                {
                    //开
#ifdef  Gree 
                    /*  ir_ps_data[0] = 0x50;
                        ir_ps_data[1] = 0x00;
                        ir_ps_data[2] = 0x0c;
                        ir_ps_data[3] = 0x49;
                        ir_ps_data[4] = 0x00;
                        ir_ps_data[5] = 0x00;
                        ir_ps_data[6] = 0x00;
                        ir_ps_data[7] = 0x10;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x00;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x00;
                        ir_ps_data[12] = 0x00;
                        28开机，关灯,风速自动
                    */
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x00;ir_ps_data[2] = 0x0c;ir_ps_data[3] = 0x49;
                    ir_ps_data[4] = 0x00;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x00;ir_ps_data[7] = 0x10;
#endif  

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x05;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0xaa;//BIT3显示置位
                    ir_ps_data[12] = 0x1a;
                    28降温 开（开机状态）1风速
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x05;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x1a;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x05;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x00;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x82;  //BIT3显示不置位
                    ir_ps_data[12] = 0x22;
                    28降温 开（开机状态）1风速
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x05;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0x82;
                        ir_ps_data[12] = 0x22;



                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_SP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_LP_flags_BIT | APP_event_LLP_flags_BIT);
                    send_flags = 0x55;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
                if(((esp32C <= (IR_temp - Lp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) == 0)) || ((bleC <= (IR_temp - Lp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LP_flags_BIT) == 0)))
                {
                    //关
#ifdef  Gree 
                    /*  
                        ir_ps_data[0] = 0x50;
                        ir_ps_data[1] = 0x00;
                        ir_ps_data[2] = 0x0e;
                        ir_ps_data[3] = 0x49;
                        ir_ps_data[4] = 0x20;
                        ir_ps_data[5] = 0x00;
                        ir_ps_data[6] = 0x00;
                        ir_ps_data[7] = 0x11;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x00;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x00;
                        ir_ps_data[12] = 0x00;
                        30开机，关灯,风速自动
                    */
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x00;ir_ps_data[2] = 0x0e;ir_ps_data[3] = 0x49;
                    ir_ps_data[4] = 0x20;ir_ps_data[5] = 0x00;ir_ps_data[6] = 0x00;ir_ps_data[7] = 0x11;
#endif 

#ifdef  Auxgroup     
                    EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
                    if((uxBits & APP_event_lighting_BIT) != 0)
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x1d;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0xaa; //BIT3显示置位
                    ir_ps_data[12] = 0x0f;
                    31升温。关（开机状态）静音
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x1d;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x0f;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x1d;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x12;//BIT3显示不置位
                    ir_ps_data[12] = 0xc7;
                    31升温。关（开机状态）静音
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x1d;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0x12;
                        ir_ps_data[12] = 0xc7;


                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                     send_flags = 0xa5;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
                if(((esp32C <= (IR_temp - LLp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LLP_flags_BIT) == 0)) || ((bleC <= (IR_temp - LLp)) && ((xEventGroupGetBits(APP_event_group)  & APP_event_LLP_flags_BIT) == 0)))
                {
                    //全关
#ifdef  Gree 
                    /*  
                        ir_ps_data[0] = 0x50;
                        ir_ps_data[1] = 0x00;
                        ir_ps_data[2] = 0x0c;
                        ir_ps_data[3] = 0x41;
                        ir_ps_data[4] = 0x80;
                        ir_ps_data[5] = 0x00;
                        ir_ps_data[6] = 0x00;
                        ir_ps_data[7] = 0x11;
                        ir_ps_data[8] = 0x00;
                        ir_ps_data[9] = 0x00;
                        ir_ps_data[10] = 0x00;
                        ir_ps_data[11] = 0x00;
                        ir_ps_data[12] = 0x00;
                        30开机，关灯,风速自动
                    */
                    ir_ps_data[0] = 0x50;ir_ps_data[1] = 0x00;ir_ps_data[2] = 0x0c;ir_ps_data[3] = 0x41;
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
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0xaa;//BIT3显示置位
                    ir_ps_data[12] = 0x1f;
                    32升温。关（开机状态）静音  
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0xaa;
                        ir_ps_data[12] = 0x1f;
                        xEventGroupClearBits(APP_event_group,APP_event_lighting_BIT);
                    }
                    else
                    {
                /*
                    ir_ps_data[0] = 0xc3;
                    ir_ps_data[1] = 0x03;
                    ir_ps_data[2] = 0x00;
                    ir_ps_data[3] = 0x00;
                    ir_ps_data[4] = 0x06;
                    ir_ps_data[5] = 0x01;
                    ir_ps_data[6] = 0x04;
                    ir_ps_data[7] = 0x00;
                    ir_ps_data[8] = 0x00;
                    ir_ps_data[9] = 0x04;
                    ir_ps_data[10] = 0x00;
                    ir_ps_data[11] = 0x12;//BIT3显示不置位
                    ir_ps_data[12] = 0xd7;
                    32升温。关（开机状态）静音
                */
                        ir_ps_data[0] = 0xc3;ir_ps_data[1] = 0x03;ir_ps_data[2] = 0x00;ir_ps_data[3] = 0x00;
                        ir_ps_data[4] = 0x06;ir_ps_data[5] = 0x01;ir_ps_data[6] = 0x04;ir_ps_data[7] = 0x00;
                        ir_ps_data[8] = 0x00;ir_ps_data[9] = 0x04;ir_ps_data[10] = 0x00;ir_ps_data[11] = 0x12;
                        ir_ps_data[12] = 0xd7;                        

                    }
#endif 
                    xEventGroupSetBits(APP_event_group,APP_event_LLP_flags_BIT);
                    xEventGroupClearBits(APP_event_group,APP_event_SP_flags_BIT);
                    send_flags = 0xaa;
                    xMessageBufferSend(ir_tx_data,ir_ps_data,13,portMAX_DELAY);
                }
            }
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
        if(i >= 100)
        {
            i = 0;
            TickType_t xRemainingTime;
            /* 计算xTimer引用的定时器之前剩余的时间
            到期。TickType_t 是无符号类型，因此减法将导致
            即使计时器直到滴答声之后才会过期，也是正确的答案
            计数已溢出。*/ 
            xRemainingTime = xTimerGetExpiryTime(time_sleep_timers) - xTaskGetTickCount();
            EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
            printf("onoff(1=开，0=关):%d;55=开 & aa=全关 & a5=关:%x;time_off:%d;IR_temp:%dC;bleC::%dC;humidity_ble:%d%%;Voltage_ble:%dmV;esp32C:%dC;ds18b20C:%dC\r\n",   \
            (uxBits & APP_event_run_BIT) ? 1:0,send_flags,(xRemainingTime/6000),IR_temp,bleC,humidity_ble,Voltage_ble,esp32C,ds18b20C);

            tcprx_buffer = "Automatic control status 55=on & aa=Quanguan & a5=shut hex";
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
            tcp_client_send((xRemainingTime/6000));

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

            tcprx_buffer = "esp32C temperature /100 C hex";
            uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										1000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
            tcp_client_send(esp32C);

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
            tcp_client_send(ds18b20C);
        }
        i++;
    }
}