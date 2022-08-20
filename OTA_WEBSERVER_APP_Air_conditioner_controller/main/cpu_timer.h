#ifndef cpu_timer_H
#define cpu_timer_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/timer.h"

#define TIMER0_DIVIDER         20  //  Hardware timer clock divider
#define TIMER1_DIVIDER         80  //  Hardware timer clock divider
#define TIMER1_5ms_isr         1000

void cpu_timer0_init(void);
void cpu_timer0_delay_us(uint16_t us);
void IRAM_ATTR timer_group1_isr(void *para);
void cpu_timer1_init(void);

#ifdef __cplusplus
}
#endif

#endif /* led_Task_H */