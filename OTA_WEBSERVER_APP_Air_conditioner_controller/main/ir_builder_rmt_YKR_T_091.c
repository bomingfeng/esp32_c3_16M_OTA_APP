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
// limitations under the License.#include <stdlib.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "ir_tools.h"
#include "driver/rmt.h"



/**
 * @brief Timings for YB0F2 protocol
 *
 */
#define YKR_T_091_LEADING_CODE_HIGH_US (9000)
#define YKR_T_091_LEADING_CODE_LOW_US (4500)
#define YKR_T_091_PAYLOAD_ONE_HIGH_US (560)
#define YKR_T_091_PAYLOAD_ONE_LOW_US (1690)
#define YKR_T_091_PAYLOAD_ZERO_HIGH_US (560)
#define YKR_T_091_PAYLOAD_ZERO_LOW_US (560)
#define YKR_T_091_ENDING_CODE_HIGH_US (560)


//static const char *TAG = "YKR_T_091_builder";

typedef struct {
    ir_builder_t parent;
    uint32_t buffer_size;
    uint32_t cursor;
    uint32_t flags;
    uint32_t leading_code_high_ticks;
    uint32_t leading_code_low_ticks;
    uint32_t repeat_code_high_ticks;
    uint32_t repeat_code_low_ticks;
    uint32_t payload_logic0_high_ticks;
    uint32_t payload_logic0_low_ticks;
    uint32_t payload_logic1_high_ticks;
    uint32_t payload_logic1_low_ticks;
    uint32_t ending_code_high_ticks;
    uint32_t ending_code_low_ticks;
    bool inverse;
    rmt_item32_t buffer[0];
} YKR_T_091_builder_t;


static esp_err_t YKR_T_091_builder_make_head(ir_builder_t *builder)
{
    YKR_T_091_builder_t *YKR_T_091_builder = __containerof(builder, YKR_T_091_builder_t, parent);
    YKR_T_091_builder->cursor = 0;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level0 = !YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration0 = YKR_T_091_builder->leading_code_high_ticks;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level1 = YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration1 = YKR_T_091_builder->leading_code_low_ticks;
    YKR_T_091_builder->cursor += 1;
    return ESP_OK;
}


 
static esp_err_t YKR_T_091_builder_make_logic0(ir_builder_t *builder)
{
    YKR_T_091_builder_t *YKR_T_091_builder = __containerof(builder, YKR_T_091_builder_t, parent);
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level0 = !YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration0 = YKR_T_091_builder->payload_logic0_high_ticks;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level1 = YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration1 = YKR_T_091_builder->payload_logic0_low_ticks;
    YKR_T_091_builder->cursor += 1;
    return ESP_OK;
}


static esp_err_t YKR_T_091_builder_make_logic1(ir_builder_t *builder)
{
    YKR_T_091_builder_t *YKR_T_091_builder = __containerof(builder, YKR_T_091_builder_t, parent);
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level0 = !YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration0 = YKR_T_091_builder->payload_logic1_high_ticks;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level1 = YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration1 = YKR_T_091_builder->payload_logic1_low_ticks;
    YKR_T_091_builder->cursor += 1;
    return ESP_OK;
}


static esp_err_t YKR_T_091_builder_make_end(ir_builder_t *builder)
{
    YKR_T_091_builder_t *YKR_T_091_builder = __containerof(builder, YKR_T_091_builder_t, parent);
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level0 = !YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration0 = YKR_T_091_builder->ending_code_high_ticks;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].level1 = YKR_T_091_builder->inverse;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].duration1 = YKR_T_091_builder->ending_code_low_ticks;
    YKR_T_091_builder->cursor += 1;
    YKR_T_091_builder->buffer[YKR_T_091_builder->cursor].val = 0;
    YKR_T_091_builder->cursor += 1;
    return ESP_OK;
}

static esp_err_t YKR_T_091_build_frame(ir_builder_t *builder, uint32_t data1, uint32_t data2, uint32_t data3,uint8_t data4)
{
    int i;
    uint32_t address1;
    builder->make_head(builder);

    address1 = data1;
    // MSB -> LSB
    for (i = 0; i < 32; i++) {
        if(address1 & 0x80000000) {
            builder->make_logic1(builder);
        } else {
            builder->make_logic0(builder);
        }
        address1 <<= 1;
    }

    address1 = data2;
    // MSB -> LSB
    for (i = 0; i < 32; i++) {
        if(address1 & 0x80000000) {
            builder->make_logic1(builder);
        } else {
            builder->make_logic0(builder);
        }
        address1 <<= 1;
    }

    address1 = data3;
    // MSB -> LSB
    for (i = 0; i < 32; i++) {
        if(address1 & 0x80000000) {
            builder->make_logic1(builder);
        } else {
            builder->make_logic0(builder);
        }
        address1 <<= 1;
    }

    address1 = data4;
    // MSB -> LSB
    for (i = 0; i < 8; i++) {
        if(address1 & 0x80) {
            builder->make_logic1(builder);
        } else {
            builder->make_logic0(builder);
        }
        address1 <<= 1;
    }

    builder->make_end(builder);
    return ESP_OK;
}


static esp_err_t YKR_T_091_builder_get_result(ir_builder_t *builder, void *result, size_t *length)
{
    YKR_T_091_builder_t *YKR_T_091_builder = __containerof(builder, YKR_T_091_builder_t, parent);
    *(rmt_item32_t **)result = YKR_T_091_builder->buffer;
    *length = YKR_T_091_builder->cursor;
    return ESP_OK;
}

static esp_err_t YKR_T_091_builder_del(ir_builder_t *builder)
{
    YKR_T_091_builder_t *YKR_T_091_builder = __containerof(builder, YKR_T_091_builder_t, parent);
    free(YKR_T_091_builder);
    return ESP_OK;
}


ir_builder_t *ir_builder_rmt_new_YKR_T_091(const ir_builder_config_t *config)
{
    uint32_t builder_size = sizeof(YKR_T_091_builder_t) + config->buffer_size * sizeof(rmt_item32_t);
    YKR_T_091_builder_t *YKR_T_091_builder = calloc(1, builder_size);

    YKR_T_091_builder->buffer_size = config->buffer_size;
    YKR_T_091_builder->flags = config->flags;
    if (config->flags & IR_TOOLS_FLAGS_INVERSE) {
        YKR_T_091_builder->inverse = true;
    }
    uint32_t counter_clk_hz = 0;
    rmt_get_counter_clock((rmt_channel_t)config->dev_hdl, &counter_clk_hz);          
    float ratio = (float)counter_clk_hz / 1e6;
    YKR_T_091_builder->leading_code_high_ticks = (uint32_t)(ratio * YKR_T_091_LEADING_CODE_HIGH_US);
    YKR_T_091_builder->leading_code_low_ticks = (uint32_t)(ratio * YKR_T_091_LEADING_CODE_LOW_US);

    YKR_T_091_builder->payload_logic0_high_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ZERO_HIGH_US);
    YKR_T_091_builder->payload_logic0_low_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ZERO_LOW_US);

    YKR_T_091_builder->payload_logic1_high_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ONE_HIGH_US);
    YKR_T_091_builder->payload_logic1_low_ticks = (uint32_t)(ratio * YKR_T_091_PAYLOAD_ONE_LOW_US);

    YKR_T_091_builder->ending_code_high_ticks = (uint32_t)(ratio * YKR_T_091_ENDING_CODE_HIGH_US);
    YKR_T_091_builder->ending_code_low_ticks = 0x7FFF;

    YKR_T_091_builder->parent.make_head = YKR_T_091_builder_make_head;

    YKR_T_091_builder->parent.make_logic0 = YKR_T_091_builder_make_logic0;

    YKR_T_091_builder->parent.make_logic1 = YKR_T_091_builder_make_logic1;
 
    YKR_T_091_builder->parent.make_end = YKR_T_091_builder_make_end;

    YKR_T_091_builder->parent.build_frame = YKR_T_091_build_frame;

    YKR_T_091_builder->parent.get_result = YKR_T_091_builder_get_result;
    YKR_T_091_builder->parent.del = YKR_T_091_builder_del;
    return &YKR_T_091_builder->parent;
}
