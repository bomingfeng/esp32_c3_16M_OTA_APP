#ifndef temperature_control_task_H
#define temperature_control_task_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_inclued.h"

void tempps_task_init(void);
void tempps_task(void *arg);
void IRps_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* temperature_control_task_H */