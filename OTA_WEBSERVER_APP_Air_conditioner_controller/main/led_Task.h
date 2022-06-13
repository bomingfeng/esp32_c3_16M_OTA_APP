#ifndef led_Task_H
#define led_Task_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_inclued.h"

void LED_Task_init(void);
void led_instructions(void *pvParam);

#ifdef __cplusplus
}
#endif

#endif /* led_Task_H */