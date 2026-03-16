#ifndef DEVICE_FUNCTION_H
#define DEVICE_FUNCTION_H

#include "cyhal.h"
#include "cybsp.h"
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

// Forward declaration voor queue handle
extern QueueHandle_t publisher_task_q;

typedef struct {
    char buffer[32];  // device name
} DeviceString;

typedef struct {
    // I/O pins
    cyhal_gpio_t motor_pin;
    
    // Device name
    DeviceString device_name;
    
    // Status
    bool is_running;
    int state_done;
    
    // Pin initialisatie status
    bool pin_initialized;
} DeviceFunction;

// Constructor equivalent
void DeviceFunction_init(DeviceFunction *dev, const char* name, cyhal_gpio_t motor_pin, int state_done_req);

// Destructor equivalent
void DeviceFunction_deinit(DeviceFunction *dev);

// Execute functie
void DeviceFunction_execute(DeviceFunction *dev);

// Getters
const char* DeviceFunction_get_device_name(DeviceFunction *dev);
int DeviceFunction_get_state_done(DeviceFunction *dev);
bool DeviceFunction_is_running(DeviceFunction *dev);

#endif // DEVICE_FUNCTION_H