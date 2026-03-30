#ifndef DEVICE_FUNCTION_H_
#define DEVICE_FUNCTION_H_

#include "cyhal.h"
#include "cybsp.h"
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

/* External reference to publisher task queue */
extern QueueHandle_t publisher_task_q;

typedef struct
{
    char buffer[32];
} device_string_t;

typedef struct
{
    cyhal_gpio_t motor_pin;
    device_string_t device_name;
    bool is_running;
    int state_done;
    bool pin_initialized;
} DeviceFunction;

/* Initialization and cleanup */
void DeviceFunction_init(DeviceFunction *dev,
                         const char *name,
                         cyhal_gpio_t motor_pin,
                         int state_done_req);
void DeviceFunction_deinit(DeviceFunction *dev);

/* Execute device action */
void DeviceFunction_execute(DeviceFunction *dev);

/* Status queries */
const char* DeviceFunction_get_device_name(DeviceFunction *dev);
int DeviceFunction_get_state_done(DeviceFunction *dev);
bool DeviceFunction_is_running(DeviceFunction *dev);

#endif /* DEVICE_FUNCTION_H_ */
