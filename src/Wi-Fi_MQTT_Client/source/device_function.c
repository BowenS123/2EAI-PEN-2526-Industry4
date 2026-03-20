/*
 * device_function.c
 *
 *  Created on: 14 Mar 2026
 *      Author: 12000249
 */

#include "device_function.h"
#include "cyhal_gpio.h"
#include <stdio.h>

#include "publisher_task.h"  // Voor publisher_data_t


void DeviceFunction_init(DeviceFunction *dev, const char* name, cyhal_gpio_t motor_pin, int state_done_req) {
    if (dev == NULL) return;
    
    // Initializeer pin(nen)
    dev->motor_pin = motor_pin;
    dev->pin_initialized = false;
    
    // Configureer GPIO als output
    // cyhal_gpio_init(dev->motor_pin, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    /* Initialize LED as replacement */
    cyhal_gpio_init(dev->motor_pin, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP,
                    CYBSP_LED_STATE_OFF);
    dev->pin_initialized = true;
    
    // Kopieer device naam
    strncpy(dev->device_name.buffer, name, sizeof(dev->device_name.buffer) - 1);
    dev->device_name.buffer[sizeof(dev->device_name.buffer) - 1] = '\0';  // Zorg voor null-terminatie
    
    // Initializeer andere variabelen
    dev->is_running = false;
    dev->state_done = state_done_req;
}

void DeviceFunction_deinit(DeviceFunction *dev) {
    if (dev == NULL) return;
    
    // Zet motor uit
    if (dev->pin_initialized) {
        cyhal_gpio_write(dev->motor_pin, 0);
        cyhal_gpio_free(dev->motor_pin);
        dev->pin_initialized = false;
    }
    
    dev->is_running = false;
}

void DeviceFunction_execute(DeviceFunction *dev) {
    if (dev == NULL) return;
    
    cyhal_gpio_write(dev->motor_pin, 1);
    dev->is_running = true;
    publisher_data_t pub_cmd_0 = {
                   .cmd = PUBLISH_STATE,
                   .state = "running",
                   .detail = NULL
                 };
    xQueueSend(publisher_task_q, &pub_cmd_0, portMAX_DELAY);
    
    cyhal_system_delay_ms(3000);
    
    cyhal_gpio_write(dev->motor_pin, 0);
    dev->is_running = false;
    publisher_data_t pub_cmd = {
                   .cmd = PUBLISH_STATE,
                   .state = "idle",
                   .detail = NULL
                 };

    xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
}

const char* DeviceFunction_get_device_name(DeviceFunction *dev) {
    if (dev == NULL) return "";
    return dev->device_name.buffer;
}

int DeviceFunction_get_state_done(DeviceFunction *dev) {
    if (dev == NULL) return 0;
    return dev->state_done;
}

bool DeviceFunction_is_running(DeviceFunction *dev) {
    if (dev == NULL) return false;
    return dev->is_running;
}

