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
#include "ir_parser_rmt_YAPOF3.h"
#include "driver/rmt.h"

static const char *TAG = "YAPOF3_parser";
#define YAPOF3_CHECK(a, str, goto_tag, ret_value, ...)                               \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

#define YAPOF3_DATA_FRAME_RMT_WORDS (70)


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
} YAPOF3_parser_t;


static inline bool YAPOF3_check_in_range(uint32_t raw_ticks, uint32_t target_ticks, uint32_t margin_ticks)
{
    return (raw_ticks < (target_ticks + margin_ticks)) && (raw_ticks > (target_ticks - margin_ticks));
}

static bool YAPOF3_parse_head(YAPOF3_parser_t *YAPOF3_parser)
{
    YAPOF3_parser->cursor = 0;
    rmt_item32_t item = YAPOF3_parser->buffer[YAPOF3_parser->cursor];
    bool ret = (item.level0 == YAPOF3_parser->inverse) && (item.level1 != YAPOF3_parser->inverse) &&
               YAPOF3_check_in_range(item.duration0, YAPOF3_parser->leading_code_high_ticks, YAPOF3_parser->margin_ticks) &&
               YAPOF3_check_in_range(item.duration1, YAPOF3_parser->leading_code_low_ticks, YAPOF3_parser->margin_ticks);
    YAPOF3_parser->cursor += 1;
    return ret;
}

static bool YAPOF3_parse_logic0(YAPOF3_parser_t *YAPOF3_parser)
{
    rmt_item32_t item = YAPOF3_parser->buffer[YAPOF3_parser->cursor];
    bool ret = (item.level0 == YAPOF3_parser->inverse) && (item.level1 != YAPOF3_parser->inverse) &&
               YAPOF3_check_in_range(item.duration0, YAPOF3_parser->payload_logic0_high_ticks, YAPOF3_parser->margin_ticks) &&
               YAPOF3_check_in_range(item.duration1, YAPOF3_parser->payload_logic0_low_ticks, YAPOF3_parser->margin_ticks);
    return ret;
}

static bool YAPOF3_parse_logic1(YAPOF3_parser_t *YAPOF3_parser)
{
    rmt_item32_t item = YAPOF3_parser->buffer[YAPOF3_parser->cursor];
    bool ret = (item.level0 == YAPOF3_parser->inverse) && (item.level1 != YAPOF3_parser->inverse) &&
               YAPOF3_check_in_range(item.duration0, YAPOF3_parser->payload_logic1_high_ticks, YAPOF3_parser->margin_ticks) &&
               YAPOF3_check_in_range(item.duration1, YAPOF3_parser->payload_logic1_low_ticks, YAPOF3_parser->margin_ticks);
    return ret;
}

static esp_err_t YAPOF3_parse_logic(ir_parser_t *parser, bool *logic)
{
    esp_err_t ret = ESP_FAIL;
    bool logic_value = false;
    YAPOF3_parser_t *YAPOF3_parser = __containerof(parser, YAPOF3_parser_t, parent);
    if (YAPOF3_parse_logic0(YAPOF3_parser)) {
        logic_value = false;
        ret = ESP_OK;
    } else if (YAPOF3_parse_logic1(YAPOF3_parser)) {
        logic_value = true;
        ret = ESP_OK;
    }
    if (ret == ESP_OK) {
        *logic = logic_value;
    }
    YAPOF3_parser->cursor += 1;
    return ret;
}

static bool YAPOF3_parse_repeat_frame(YAPOF3_parser_t *YAPOF3_parser)
{
 //   YAPOF3_parser->cursor = 0;
    rmt_item32_t item = YAPOF3_parser->buffer[YAPOF3_parser->cursor];
    bool ret = (item.level0 == YAPOF3_parser->inverse) && (item.level1 != YAPOF3_parser->inverse) &&
               YAPOF3_check_in_range(item.duration0, YAPOF3_parser->repeat_code_high_ticks, YAPOF3_parser->margin_ticks) &&
               YAPOF3_check_in_range(item.duration1, YAPOF3_parser->repeat_code_low_ticks, YAPOF3_parser->margin_ticks);
    YAPOF3_parser->cursor += 1;
    return ret;
}

static esp_err_t YAPOF3_parser_input(ir_parser_t *parser, void *raw_data, uint32_t length)
{
    esp_err_t ret = ESP_OK;
    YAPOF3_parser_t *YAPOF3_parser = __containerof(parser, YAPOF3_parser_t, parent);
//    YAPOF3_CHECK(raw_data, "input data can't be null", err, ESP_ERR_INVALID_ARG);
    YAPOF3_parser->buffer = raw_data;
    // Data Frame costs 34 items and Repeat Frame costs 2 items
   if (length == YAPOF3_DATA_FRAME_RMT_WORDS) {
         return ret;
    }
    else {
        return ESP_FAIL;
    }

}

static esp_err_t YAPOF3_parser_get_scan_code(ir_parser_t *parser, uint32_t *parserdata1,uint32_t *parserdata2,uint32_t *parserdata3,uint32_t *parserdata4)
{
    esp_err_t ret = ESP_FAIL;
    bool logic_value = false;
    uint32_t rx_buf1 = 0,rx_buf2 = 0;
    int  i;
    YAPOF3_parser_t *YAPOF3_parser = __containerof(parser, YAPOF3_parser_t, parent);
    
    if (YAPOF3_parse_head(YAPOF3_parser))
    {
        //LSB -> MSB
        for (  i = 0; i < 32; i++) 
        {
            if (YAPOF3_parse_logic(parser, &logic_value) == ESP_OK) 
            {
                rx_buf1 |= logic_value << i;
            }
        }

        YAPOF3_parse_logic(parser, &logic_value);
        YAPOF3_parse_logic(parser, &logic_value);
        YAPOF3_parse_logic(parser, &logic_value);

        YAPOF3_parse_repeat_frame(YAPOF3_parser);

        for (  i = 0; i < 32; i++) 
        {
            if (YAPOF3_parse_logic(parser, &logic_value) == ESP_OK) 
            {
                rx_buf2 |= logic_value << i; 
            }
        }

        *parserdata1 = rx_buf1;
        *parserdata2 = rx_buf2;
        YAPOF3_parser->cursor += 1;
        ret = ESP_OK;
    }
    return ret;
}

static esp_err_t YAPOF3_parser_del(ir_parser_t *parser)
{
    YAPOF3_parser_t *YAPOF3_parser = __containerof(parser, YAPOF3_parser_t, parent);
    free(YAPOF3_parser);
    return ESP_OK;
}

ir_parser_t *ir_parser_rmt_new_YAPOF3(const ir_parser_config_t *config)
{
    ir_parser_t *ret = NULL;
    YAPOF3_CHECK(config, "nec configuration can't be null", err, NULL);

    YAPOF3_parser_t *YAPOF3_parser = calloc(1, sizeof(YAPOF3_parser_t));
    YAPOF3_CHECK(YAPOF3_parser, "request memory for YAPOF3_parser failed", err, NULL);

    YAPOF3_parser->flags = config->flags;
    if (config->flags & IR_TOOLS_FLAGS_INVERSE) {
        YAPOF3_parser->inverse = true;
    }

    uint32_t counter_clk_hz = 0;
    YAPOF3_CHECK(rmt_get_counter_clock((rmt_channel_t)config->dev_hdl, &counter_clk_hz) == ESP_OK,
              "get rmt counter clock failed", err, NULL);
    float ratio = (float)counter_clk_hz / 1e6;
    YAPOF3_parser->leading_code_high_ticks = (uint32_t)(ratio * YAPOF3_LEADING_CODE_HIGH_US);
    YAPOF3_parser->leading_code_low_ticks = (uint32_t)(ratio * YAPOF3_LEADING_CODE_LOW_US);
    YAPOF3_parser->repeat_code_high_ticks = (uint32_t)(ratio * YAPOF3_REPEAT_CODE_HIGH_US);
    YAPOF3_parser->repeat_code_low_ticks = (uint32_t)(ratio * YAPOF3_REPEAT_CODE_LOW_US);
    YAPOF3_parser->payload_logic0_high_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ZERO_HIGH_US);
    YAPOF3_parser->payload_logic0_low_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ZERO_LOW_US);
    YAPOF3_parser->payload_logic1_high_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ONE_HIGH_US);
    YAPOF3_parser->payload_logic1_low_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ONE_LOW_US);
    YAPOF3_parser->margin_ticks = (uint32_t)(ratio * config->margin_us);
    YAPOF3_parser->parent.input = YAPOF3_parser_input;
    YAPOF3_parser->parent.get_scan_code = YAPOF3_parser_get_scan_code;
    YAPOF3_parser->parent.del = YAPOF3_parser_del;
    return &YAPOF3_parser->parent;
err:
    return ret;
}
