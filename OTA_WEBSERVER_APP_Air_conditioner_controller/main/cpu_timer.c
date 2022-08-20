#include "cpu_timer.h"
#include "MultiButton/multi_button.h"

//set 80m hz /20 ,4m hz tick 
void cpu_timer0_init(void)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER0_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = TIMER_AUTORELOAD_DIS,
    }; // default clock source is APB
#if CONFIG_IDF_TARGET_ESP32
    timer_init(TIMER_GROUP_0, TIMER_1, &config);
#elif CONFIG_IDF_TARGET_ESP32C3
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
#endif // CONFIG_IDF_TARGET_*    
    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
#if CONFIG_IDF_TARGET_ESP32
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_start(TIMER_GROUP_0, TIMER_1);
#elif CONFIG_IDF_TARGET_ESP32C3
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
#endif // CONFIG_IDF_TARGET_*        
    

}
 
void cpu_timer0_delay_us(uint16_t us) 
{
    uint64_t timer_counter_value = 0;
    uint64_t timer_counter_update = 0;
    //uint32_t delay_ccount = 200 * us;
#if CONFIG_IDF_TARGET_ESP32
    timer_get_counter_value(TIMER_GROUP_0, TIMER_1,&timer_counter_value);
#elif CONFIG_IDF_TARGET_ESP32C3
    timer_get_counter_value(TIMER_GROUP_0, TIMER_0,&timer_counter_value);
#endif // CONFIG_IDF_TARGET_*      
    
    timer_counter_update = timer_counter_value + (us << 2);
    do {
#if CONFIG_IDF_TARGET_ESP32
        timer_get_counter_value(TIMER_GROUP_0, TIMER_1,&timer_counter_value);
#elif CONFIG_IDF_TARGET_ESP32C3
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0,&timer_counter_value);
#endif // CONFIG_IDF_TARGET_*           
    } while (timer_counter_value < timer_counter_update);
}

/**
 * 定时器中断函数
 */
void IRAM_ATTR timer_group1_isr(void *para)
{
	//获取定时器分组0中的哪一个定时器产生了中断
    uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_1); //获取中断状态
    if (timer_intr & TIMER_INTR_T0) {//定时器0分组的0号定时器产生中断
        /*清除中断状态*/
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_1, TIMER_0);
        /*重新使能定时器中断*/
        timer_group_enable_alarm_in_isr(TIMER_GROUP_1, TIMER_0);
    }
#if CONFIG_IDF_TARGET_ESP32    
    else if(timer_intr & TIMER_INTR_T1) {

        /*清除中断状态*/
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_1, TIMER_1);
        /*重新使能定时器中断*/
        timer_group_enable_alarm_in_isr(TIMER_GROUP_1, TIMER_1);
#elif CONFIG_IDF_TARGET_ESP32C3
    else if(timer_intr & TIMER_INTR_T0) {
        /*清除中断状态*/
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_1, TIMER_0);
        /*重新使能定时器中断*/
        timer_group_enable_alarm_in_isr(TIMER_GROUP_1, TIMER_0);
#endif // CONFIG_IDF_TARGET_* 

    }
    else {
        //printf("timer_group0_isr!\r\n");
    }
}

void cpu_timer1_init(void)
{
	/**
	 * 设置定时器初始化参数
	 */
	timer_config_t config ={
        .divider = TIMER1_DIVIDER, //分频系数
		.counter_dir = TIMER_COUNT_UP, //计数方式为向上计数
		.counter_en = TIMER_PAUSE, //调用timer_init函数以后不启动计数,调用timer_start时才开始计数
		.alarm_en = TIMER_ALARM_EN, //到达计数值启动报警（计数值溢出，进入中断）
        .auto_reload = TIMER_AUTORELOAD_EN, //自动重新装载预装值
	};
    /**
     * 初始化定时器
     *    TIMER_GROUP_0(定时器分组0)
     *    TIMER_0(0号定时器)
     */
	timer_init(TIMER_GROUP_1,TIMER_0,&config);
 
	/*设置定时器预装值*/
	timer_set_counter_value(TIMER_GROUP_1,TIMER_0,0x00000000ULL);
 
	/**
	 * 设置报警阈值
	 *  
	 */
	timer_set_alarm_value(TIMER_GROUP_1,TIMER_0,TIMER1_5ms_isr * ((TIMER_BASE_CLK / TIMER1_DIVIDER) * 1000)); //TIMER_BASE_CLK 为80M
	//定时器中断使能
	timer_enable_intr(TIMER_GROUP_1,TIMER_0);
	/**
	 * 注册定时器中断函数
	 */
	timer_isr_register(TIMER_GROUP_1,TIMER_0,
			timer_group1_isr,  //定时器中断回调函数
			(void*)TIMER_0,    //传递给定时器回调函数的参数
			ESP_INTR_FLAG_IRAM, //把中断放到 IRAM 中
			NULL //调用成功以后返回中断函数的地址,一般用不到
			);
	/*启动定时器*/
	timer_start(TIMER_GROUP_1,TIMER_0); //开始计数
}
