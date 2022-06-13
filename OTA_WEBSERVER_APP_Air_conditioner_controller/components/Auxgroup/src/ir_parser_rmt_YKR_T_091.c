// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdlib.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "ir_parser_rmt_YKR_T_091.h"
#include "driver/rmt.h"

#define YKR_T_091_LEADING_CODE_HIGH_US (9000)
#define YKR_T_091_LEADING_CODE_LOW_US (4500)
#define YKR_T_091_PAYLOAD_ONE_HIGH_US (560)
#define YKR_T_091_PAYLOAD_ONE_LOW_US (1690)
#define YKR_T_091_PAYLOAD_ZERO_HIGH_US (560)
#define YKR_T_091_PAYLOAD_ZERO_LOW_US (560)
#define YKR_T_091_ENDING_CODE_HIGH_US (560)

static const char *TAG = "YKR_T_091_parser";

#define YKR_T_091_CHECK(a, str, goto_tag, ret_value, ...)                               \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)



#define YKR_T_091_DATA_FRAME_RMT_WORDS (106)


typedef struct {
    ir_parser_t parent;
    uint32_t flags;
    uint32_t leading_code_high_ticks;
    uint32_t leading_code_low_ticks;
    uint32_t repeat_code_high_ticks;
    uint32_t repeat_code_low_ticks;
    uint32_t payload_logic0_high_ticks;
    uint32_t payload_logic0_low_ticks;
    uint32_t payload_logic1_high_ticks;
    uint32_t payload_logic1_low_ticks;
    uint32_t margin_ticks;
    rmt_item32_t *buffer;
    uint32_t cursor;
    uint32_t last_address;
    uint32_t last_command;
    bool repeat;
    bool inverse;
} YKR_T_091_parser_t;


static inline bool YKR_T_091_check_in_range(uint32_t raw_ticks, uint32_t target_ticks, uint32_t margin_ticks)
{
    return (raw_ticks < (target_ticks + margin_ticks)) && (raw_ticks > (target_ticks - margin_ticks));
}

static bool YKR_T_091_parse_head(YKR_T_091_parser_t *YKR_T_091_parser)
{
    YKR_T_091_parser->cursor = 0;
    rmt_item32_t item = YKR_T_091_parser->buffer[YKR_T_091_parser->cursor];
    bool ret = (item.level0 == YKR_T_091_parser->inverse) && (item.level1 != YKR_T_091_parser->inverse) &&
               YKR_T_091_check_in_range(item.duration0, YKR_T_091_parser->leading_code_high_ticks, YKR_T_091_parser->margin_ticks) &&
               YKR_T_091_check_in_range(item.duration1, YKR_T_091_parser->leading_code_low_ticks, YKR_T_091_parser->margin_ticks);
    YKR_T_091_parser->cursor += 1;
    return ret;
}

static bool YKR_T_091_parse_logic0(YKR_T_091_parser_t *YKR_T_091_parser)
{
    rmt_item32_t item = YKR_T_091_parser->buffer[YKR_T_091_parser->cursor];
    bool ret = (item.level0 == YKR_T_091_parser->inverse) && (item.level1 != YKR_T_091_parser->inverse) &&
               YKR_T_091_check_in_range(item.duration0, YKR_T_091_parser->payload_logic0_high_ticks, YKR_T_091_parser->margin_ticks) &&
               YKR_T_091_check_in_range(item.duration1, YKR_T_091_parser->payload_logic0_low_ticks, YKR_T_091_parser->margin_ticks);
    return ret;
}

static bool YKR_T_091_parse_logic1(YKR_T_091_parser_t *YKR_T_091_parser)
{
    rmt_item32_t item = YKR_T_091_parser->buffer[YKR_T_091_parser->cursor];
    bool ret = (item.level0 == YKR_T_091_parser->inverse) && (item.level1 != YKR_T_091_parser->inverse) &&
               YKR_T_091_check_in_range(item.duration0, YKR_T_091_parser->payload_logic1_high_ticks, YKR_T_091_parser->margin_ticks) &&
               YKR_T_091_check_in_range(item.duration1, YKR_T_091_parser->payload_logic1_low_ticks, YKR_T_091_parser->margin_ticks);
    return ret;
}

static esp_err_t YKR_T_091_parse_logic(ir_parser_t *parser, bool *logic)
{
    esp_err_t ret = ESP_FAIL;
    bool logic_value = false;
    YKR_T_091_parser_t *YKR_T_091_parser = __containerof(parser, YKR_T_091_parser_t, parent);
    if (YKR_T_091_parse_logic0(YKR_T_091_parser)) {
        logic_value = false;
        ret = ESP_OK;
    } else if (YKR_T_091_parse_logic1(YKR_T_091_parser)) {
        logic_value = true;
        ret = ESP_OK;
    }
    if (ret == ESP_OK) {
        *logic = logic_value;
    }
    YKR_T_091_parser->cursor += 1;
    return ret;
}

static esp_err_t YKR_T_091_parser_input(ir_parser_t *parser, void *raw_data, uint32_t length)
{
    esp_err_t ret = ESP_OK;
    YKR_T_091_parser_t *YKR_T_091_parser = __containerof(parser, YKR_T_091_parser_t, parent);
//    YKR_T_091_CHECK(raw_data, "input data can't be null", err, ESP_ERR_INVALID_ARG);
    YKR_T_091_parser->buffer = raw_data;
    // Data Frame costs 34 items and Repeat Frame costs 2 items
   if (length == YKR_T_091_DATA_FRAME_RMT_WORDS) {
         return ret;
    }
    else {
        return ESP_FAIL;
    }

}


uint32_t reverse_32bit(uint32_t num)
{
	uint8_t i;
	uint8_t bit;
	uint32_t new_num = 0;
	for (i = 0; i < 32; i++)
	{
		bit = num & 1;            //取出最后一位
		new_num <<= 1;            //新数左移
		new_num = new_num | bit;   //把刚取出的一位加到新数
		num >>= 1;                //原数右移，准备取第二位
	}
	return new_num;
}

uint8_t reverse_8bit(uint8_t num)
{
	uint8_t i;
	uint8_t bit;
	uint8_t new_num = 0;
	for (i = 0; i < 8; i++)
	{
		bit = num & 1;            //取出最后一位
		new_num <<= 1;            //新数左移
		new_num = new_num | bit;   //把刚取出的一位加到新数
		num >>= 1;                //原数右移，准备取第二位
	}
	return new_num;
}

static esp_err_t YKR_T_091_parser_get_scan_code(ir_parser_t *parser, uint32_t *data1, uint32_t *data2, uint32_t *data3,uint32_t *data4)
{
    esp_err_t ret = ESP_FAIL;
    uint32_t addr = 0;
    bool logic_value = false;
    YKR_T_091_parser_t *YKR_T_091_parser = __containerof(parser, YKR_T_091_parser_t, parent);
        if (YKR_T_091_parse_head(YKR_T_091_parser))
        {
            //MSB -> LSB
            addr = 0;
            for (int i = 0; i < 32; i++) 
            {
                addr <<= 1;
                if (YKR_T_091_parse_logic(parser, &logic_value) == ESP_OK) 
                {
                    addr |= logic_value;
                }
            }
            *data1 = addr;
            
            addr = 0;
            for (int i = 0; i < 32; i++) 
            {
                addr <<= 1;
                if (YKR_T_091_parse_logic(parser, &logic_value) == ESP_OK) 
                {
                    addr |= logic_value;
                }
            }
            *data2 = addr;

            addr = 0;
            for (int i = 0; i < 32; i++) 
            {
                addr <<= 1;
                if (YKR_T_091_parse_logic(parser, &logic_value) == ESP_OK) 
                {
                    addr |= logic_value;
                }
            }
            *data3 = addr;

            addr = 0;
            for (int i = 0; i < 32; i++) 
            {
                addr <<= 1;
                if (YKR_T_091_parse_logic(parser, &logic_value) == ESP_OK) 
                {
                    addr |= logic_value;
                }
            }
            *data4 = addr;
            
            YKR_T_091_parser->cursor += 1;
            ret = ESP_OK;
        }
    return ret;
}

static esp_err_t YKR_T_091_parser_del(ir_parser_t *parser)
{
    YKR_T_091_parser_t *YKR_T_091_parser = __containerof(parser, YKR_T_091_parser_t, parent);
    free(YKR_T_091_parser);
    return ESP_OK;
}

ir_parser_t *ir_parser_rmt_new_YKR_T_091(const ir_parser_config_t *config)
{
    ir_parser_t *ret = NULL;
    YKR_T_091_CHECK(config, "nec configuration can't be null", err, NULL);

    YKR_T_091_parser_t *YKR_T_091_parser = calloc(1, sizeof(YKR_T_091_parser_t));
    YKR_T_091_CHECK(YKR_T_091_parser, "request memory for YKR_T_091_parser failed", err, NULL);

    YKR_T_091_parser->flags = config->flags;
    if (config->flags & IR_TOOLS_FLAGS_INVERSE) {
        YKR_T_091_parser->inverse = true;
    }

    uint32_t counter_clk_hz = 0;
    YKR_T_091_CHECK(rmt_get_counter_clock((rmt_channel_t)config->dev_hdl, &counter_clk_hz) == ESP_OK,   \
              "get rmt counter clock failed", err, NULL);
    float ratio = (float)counter_clk_hz / 1e6;
    YKR_T_091_parser->leading_code_high_ticks = (uint32_t)(ratio * YKR_T_091_LEADING_CODE_HIGH_US);
    YKR_T_091_parser->leading_code_low_ticks = (uint32_t)(ratio * YKR_T_091_LEADING_CODE_LOW_US);
    YKR_T_091_parser->payload_logic0_high_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ZERO_HIGH_US);
    YKR_T_091_parser->payload_logic0_low_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ZERO_LOW_US);
    YKR_T_091_parser->payload_logic1_high_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ONE_HIGH_US);
    YKR_T_091_parser->payload_logic1_low_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ONE_LOW_US);
    YKR_T_091_parser->margin_ticks = (uint32_t)(ratio * config->margin_us);
    YKR_T_091_parser->parent.input = YKR_T_091_parser_input;
    YKR_T_091_parser->parent.get_scan_code = YKR_T_091_parser_get_scan_code;//????
    YKR_T_091_parser->parent.del = YKR_T_091_parser_del;
    return &YKR_T_091_parser->parent;
err:
    return ret;
}
