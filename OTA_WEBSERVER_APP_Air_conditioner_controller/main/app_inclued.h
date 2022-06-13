#ifndef main_H
#define main_H

#include <stdio.h>
#include <stdlib.h>
#include <nvs_flash.h>
#include "OTAServer.h"
#include "MyWiFi.h"
#include "esp_task.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "nvs.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "ds18x20.h"
#include "driver/temp_sensor.h"
#include "esp_sntp.h"
#include "esp_sleep.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/event_groups.h"
#include "ESP32_C3_SOC_Temp_Task.h"
#include "tcp_client.h"
#include "ds18b20_task.h"
#include "ir_rx_task.h"

//#define Gree    
#define Auxgroup 

#ifdef  Gree
#include "ir_parser_rmt_YAPOF3.h"
#define ir_parser_rmt_new(ir_parser_config) ir_parser_rmt_new_YAPOF3(ir_parser_config)
#define ir_builder_rmt_new(ir_builder_config) ir_builder_rmt_new_YB0F2(ir_builder_config)
#define RMT_CONFIG_TX(gpio, channel_id) RMT_YB0F2_CONFIG_TX(gpio, channel_id)
#define IR_BUILDER_CONFIG(dev) IR_BUILDER_YB0F2_CONFIG(dev)
#define RMT_CONFIG_RX(gpio, channel_id) RMT_YB0F2_CONFIG_RX(gpio, channel_id)
#define IR_PARSER_CONFIG(dev) IR_PARSER_YB0F2_CONFIG(dev)
#endif

#ifdef  Auxgroup
#include "ir_parser_rmt_YKR_T_091.h"
#define ir_parser_rmt_new(ir_parser_config) ir_parser_rmt_new_YKR_T_091(ir_parser_config)
#define ir_builder_rmt_new(ir_builder_config) ir_builder_rmt_new_YKR_T_091(ir_builder_config)
#define RMT_CONFIG_TX(gpio, channel_id) RMT_YKR_T_091_CONFIG_TX(gpio, channel_id)
#define IR_BUILDER_CONFIG(dev) IR_BUILDER_YKR_T_091_CONFIG(dev)
#define RMT_CONFIG_RX(gpio, channel_id) RMT_YKR_T_091_CONFIG_RX(gpio, channel_id)
#define IR_PARSER_CONFIG(dev) IR_PARSER_YKR_T_091_CONFIG(dev)
#endif

#define GATTC_TAG "LYWSD03MMC"
#define Sp 18 //温度上、下限
#define Lp 15
#define LLp 25
#define load_time 30 //多久控制空调
#define sleep_time 5  //休眠时间min
#define time_off 500 //默认定时关空调时间MIN

extern EventGroupHandle_t APP_event_group;
#define APP_event_REBOOT_BIT BIT0   // =1 重起
#define APP_event_deepsleep_BIT BIT1    // =1 休眠5s使能
#define APP_event_WIFI_STA_CONNECTED_BIT  BIT2  // =1 wifi连接
#define APP_event_WIFI_AP_CONNECTED_BIT  BIT3   // =1 有设备连接本热点
#define APP_event_Standby_BIT  BIT4 // =1 空调待机
#define APP_event_run_BIT  BIT5 // =1 空调运行
#define APP_event_tcp_client_send_BIT  BIT6 // =1 TCP_Client允许发送数据
#define APP_event_lighting_BIT   BIT7    // =1 空调灯光显示
#define APP_event_30min_timer_BIT BIT8   // =1 
#define APP_event_IO_wakeup_sleep_BIT BIT9  // =1 io 休眠使能
#define APP_event_io_sleep_timer_BIT BIT10 // =1 启动io_sleep_timers定时器

#define APP_event_SP_flags_BIT BIT11 // =1 
#define APP_event_LP_flags_BIT BIT12 // =1
#define APP_event_LLP_flags_BIT BIT13 // =1

extern RTC_DATA_ATTR uint32_t sleep_keep;
#define sleep_keep_WIFI_AP_OR_STA_BIT BIT0
#define sleep_keep_Thermohygrometer_Low_battery_BIT  BIT1


#endif /* main_H */