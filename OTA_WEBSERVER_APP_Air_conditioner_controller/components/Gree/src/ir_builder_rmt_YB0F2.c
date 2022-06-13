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
#include "ir_parser_rmt_YAPOF3.h"
#include "driver/rmt.h"

//static const char *TAG = "YB0F2_builder";

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
} YB0F2_builder_t;


static esp_err_t YB0F2_builder_make_head(ir_builder_t *builder)
{
    
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
    YB0F2_builder->cursor = 0;
    YB0F2_builder->buffer[YB0F2_builder->cursor].level0 = !YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration0 = YB0F2_builder->leading_code_high_ticks;
    YB0F2_builder->buffer[YB0F2_builder->cursor].level1 = YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration1 = YB0F2_builder->leading_code_low_ticks;
    YB0F2_builder->cursor += 1;
    return ESP_OK;
}
static esp_err_t YB0F2_build_repeat_frame(ir_builder_t *builder)
{
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
    YB0F2_builder->buffer[YB0F2_builder->cursor].level0 = !YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration0 = YB0F2_builder->repeat_code_high_ticks;
    YB0F2_builder->buffer[YB0F2_builder->cursor].level1 = YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration1 = YB0F2_builder->repeat_code_low_ticks;
    YB0F2_builder->cursor += 1;
    return ESP_OK;
}
static esp_err_t YB0F2_builder_make_logic0(ir_builder_t *builder)
{
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
    YB0F2_builder->buffer[YB0F2_builder->cursor].level0 = !YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration0 = YB0F2_builder->payload_logic0_high_ticks;
    YB0F2_builder->buffer[YB0F2_builder->cursor].level1 = YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration1 = YB0F2_builder->payload_logic0_low_ticks;
    YB0F2_builder->cursor += 1;
    return ESP_OK;
}

static esp_err_t YB0F2_builder_make_logic1(ir_builder_t *builder)
{
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
    YB0F2_builder->buffer[YB0F2_builder->cursor].level0 = !YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration0 = YB0F2_builder->payload_logic1_high_ticks;
    YB0F2_builder->buffer[YB0F2_builder->cursor].level1 = YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration1 = YB0F2_builder->payload_logic1_low_ticks;
    YB0F2_builder->cursor += 1;
    return ESP_OK;
}


static esp_err_t YB0F2_builder_make_end(ir_builder_t *builder)
{
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
    YB0F2_builder->buffer[YB0F2_builder->cursor].level0 = !YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration0 = YB0F2_builder->ending_code_high_ticks;
    YB0F2_builder->buffer[YB0F2_builder->cursor].level1 = YB0F2_builder->inverse;
    YB0F2_builder->buffer[YB0F2_builder->cursor].duration1 = YB0F2_builder->ending_code_low_ticks;
    YB0F2_builder->cursor += 1;
    YB0F2_builder->buffer[YB0F2_builder->cursor].val = 0;
    YB0F2_builder->cursor += 1;
    return ESP_OK;
}

static esp_err_t YB0F2_build_frame(ir_builder_t *builder, uint8_t * ir_tx_data)
{
    int i;
    uint32_t address1 = (ir_tx_data[0] << 24) | (ir_tx_data[1] << 16) | (ir_tx_data[2] << 8) | ir_tx_data[3];
    uint32_t command1 = (ir_tx_data[4] << 24) | (ir_tx_data[5] << 16) | (ir_tx_data[6] << 8) | ir_tx_data[7];
    builder->make_head(builder);
    // LSB -> MSB
    for (i = 0; i < 32; i++) {
        if (address1 & (1 << i)) {
            builder->make_logic1(builder);
        } else {
            builder->make_logic0(builder);
        }
    }

    builder->make_logic0(builder);
    builder->make_logic1(builder);
    builder->make_logic0(builder);

    builder->build_repeat_frame(builder);

    for (i = 0; i < 32; i++) {
        if (command1 & (1 << i)) {
            builder->make_logic1(builder);
        } else {
            builder->make_logic0(builder);
        }
    }
    builder->make_end(builder);
    return ESP_OK;
}


static esp_err_t YB0F2_builder_get_result(ir_builder_t *builder, void *result, size_t *length)
{
    esp_err_t ret = ESP_OK;
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
//    YAPOF3_CHECK(result && length, "result and length can't be null", err, ESP_ERR_INVALID_ARG);
    *(rmt_item32_t **)result = YB0F2_builder->buffer;
    *length = YB0F2_builder->cursor;
    return ret;
//err:
//  return ret;
}

static esp_err_t YB0F2_builder_del(ir_builder_t *builder)
{
    YB0F2_builder_t *YB0F2_builder = __containerof(builder, YB0F2_builder_t, parent);
    free(YB0F2_builder);
    return ESP_OK;
}

ir_builder_t *ir_builder_rmt_new_YB0F2(const ir_builder_config_t *config)
{
//    ir_builder_t *ret = NULL;


    uint32_t builder_size = sizeof(YB0F2_builder_t) + config->buffer_size * sizeof(rmt_item32_t);
    YB0F2_builder_t *YB0F2_builder = calloc(1, builder_size);


    YB0F2_builder->buffer_size = config->buffer_size;
    YB0F2_builder->flags = config->flags;
    if (config->flags & IR_TOOLS_FLAGS_INVERSE) {
        YB0F2_builder->inverse = true;
    }

    uint32_t counter_clk_hz = 0;

    rmt_get_counter_clock((rmt_channel_t)config->dev_hdl, &counter_clk_hz);          
    float ratio = (float)counter_clk_hz / 1e6;
    YB0F2_builder->leading_code_high_ticks = (uint32_t)(ratio * YAPOF3_LEADING_CODE_HIGH_US);
    YB0F2_builder->leading_code_low_ticks = (uint32_t)(ratio * YAPOF3_LEADING_CODE_LOW_US);



    YB0F2_builder->payload_logic0_high_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ZERO_HIGH_US);
    YB0F2_builder->payload_logic0_low_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ZERO_LOW_US);

    YB0F2_builder->payload_logic1_high_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ONE_HIGH_US);
    YB0F2_builder->payload_logic1_low_ticks = (uint32_t)(ratio * YAPOF3_PAYLOAD_ONE_LOW_US);

    YB0F2_builder->repeat_code_high_ticks = (uint32_t)(ratio * YAPOF3_REPEAT_CODE_HIGH_US);
    YB0F2_builder->repeat_code_low_ticks = (uint32_t)(ratio * YAPOF3_REPEAT_CODE_LOW_US);

    YB0F2_builder->ending_code_high_ticks = (uint32_t)(ratio * YAPOF3_ENDING_CODE_HIGH_US);
    YB0F2_builder->ending_code_low_ticks = 0x7FFF;

    YB0F2_builder->parent.make_head = YB0F2_builder_make_head;

    YB0F2_builder->parent.make_logic0 = YB0F2_builder_make_logic0;

    YB0F2_builder->parent.make_logic1 = YB0F2_builder_make_logic1;

 YB0F2_builder->parent.build_repeat_frame = YB0F2_build_repeat_frame;
 
    YB0F2_builder->parent.make_end = YB0F2_builder_make_end;

    YB0F2_builder->parent.build_frame = YB0F2_build_frame;

   

    YB0F2_builder->parent.get_result = YB0F2_builder_get_result;
    YB0F2_builder->parent.del = YB0F2_builder_del;

    return &YB0F2_builder->parent;
}
