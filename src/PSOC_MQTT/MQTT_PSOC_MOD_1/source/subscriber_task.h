#ifndef SUBSCRIBER_TASK_H_
#define SUBSCRIBER_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cy_mqtt_api.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for Subscriber Task. */
#define SUBSCRIBER_TASK_PRIORITY           (2)
#define SUBSCRIBER_TASK_STACK_SIZE         (1024 * 1)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the Subscriber Task. */
typedef enum
{
    SUBSCRIBE_TO_TOPIC,
    UNSUBSCRIBE_FROM_TOPIC,
    UPDATE_ARM_STEP
} subscriber_cmd_t;

/* Struct to be passed via the subscriber task queue */
typedef struct{
    subscriber_cmd_t cmd;
    uint32_t step_index;
} subscriber_data_t;

/*******************************************************************************
* Extern Variables
********************************************************************************/
extern TaskHandle_t subscriber_task_handle;
extern QueueHandle_t subscriber_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void subscriber_task(void *pvParameters);
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info);

#endif /* SUBSCRIBER_TASK_H_ */

/* [] END OF FILE */
