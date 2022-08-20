
#include "timer_test.h"

/**
 * @brief Timer user data, will be pass to timer alarm callback
 */
typedef struct {
    timer_group_t timer_group;
    timer_idx_t timer_idx;
    uint64_t alarm_value;
    timer_autoreload_t auto_reload;
    uint64_t New_alarm_value;
} example_timer_user_data_t;

void cpu_timer_init(timer_group_t group_num, timer_idx_t timer_num,uint32_t divider,uint64_t alarm_value,timer_isr_t isr_handler,timer_alarm_t alarm_en,timer_autoreload_t auto_reload)
{
    timer_config_t config = {
        .clk_src = TIMER_SRC_CLK_APB,//clock source is APB
        .divider = divider,
        .counter_dir = TIMER_COUNT_UP,//向上计数
        .counter_en = TIMER_PAUSE,//暂停
        .alarm_en = alarm_en,//使能报警
        .auto_reload = auto_reload,
        //.intr_type = 
    };
    ESP_ERROR_CHECK(timer_init(group_num, timer_num, &config));

    // For the timer counter to a initial value
    ESP_ERROR_CHECK(timer_set_counter_value(group_num, timer_num, 0));

    if(alarm_en == TIMER_ALARM_EN){
        example_timer_user_data_t  *user_data = calloc(1, sizeof(example_timer_user_data_t));
        assert(user_data);
        user_data->timer_group = group_num;
        user_data->timer_idx = timer_num;
        user_data->alarm_value = alarm_value;
        user_data->auto_reload = auto_reload;
        user_data->New_alarm_value = 500000;/////////////////
        // Set alarm value and enable alarm interrupt
        ESP_ERROR_CHECK(timer_set_alarm_value(group_num, timer_num, alarm_value));
        ESP_ERROR_CHECK(timer_enable_intr(group_num, timer_num));
        // Hook interrupt callback
        ESP_ERROR_CHECK(timer_isr_callback_add(group_num, timer_num, isr_handler, user_data, 0));
    }

    // Start timer
    ESP_ERROR_CHECK(timer_start(group_num, timer_num));
}


//test 
static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    example_timer_user_data_t *user_data = (example_timer_user_data_t *) args;

    // set new alarm value if necessary
    if (!user_data->auto_reload) {
        user_data->alarm_value += user_data->New_alarm_value;
        timer_group_set_alarm_value_in_isr(user_data->timer_group, user_data->timer_idx, user_data->alarm_value);
    }

    return pdTRUE; // return whether a task switch is needed
}

void test_group1_timer1(void)
{
    cpu_timer_init(TIMER_GROUP_1, TIMER_1, 80000000, 500000, timer_group_isr_callback, TIMER_ALARM_EN, TIMER_AUTORELOAD_EN);
}