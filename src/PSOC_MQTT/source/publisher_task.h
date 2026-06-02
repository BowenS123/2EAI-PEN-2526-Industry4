#ifndef PUBLISHER_TASK_H_
#define PUBLISHER_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for Button Task. */
#define PUBLISHER_TASK_PRIORITY               (2)
#define PUBLISHER_TASK_STACK_SIZE             (1024 * 1)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the Publisher Task. */
typedef enum
{
    PUBLISHER_INIT,
    PUBLISHER_DEINIT,
    PUBLISH_STATE,
    PUBLISH_ERROR
} publisher_cmd_t;

/* Struct to be passed via the publisher task queue */
typedef struct{
    publisher_cmd_t cmd;
    const char *state;
    const char *detail;
} publisher_data_t;

/*******************************************************************************
* Extern Variables
********************************************************************************/
extern TaskHandle_t publisher_task_handle;
extern QueueHandle_t publisher_task_q;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void publisher_task(void *pvParameters);

#endif /* PUBLISHER_TASK_H_ */

/* [] END OF FILE */
