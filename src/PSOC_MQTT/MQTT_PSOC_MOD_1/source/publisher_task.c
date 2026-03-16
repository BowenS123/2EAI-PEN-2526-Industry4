#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"

#include "publisher_task.h"
#include "mqtt_task.h"
#include "subscriber_task.h"

#include "mqtt_client_config.h"
#include "string.h"
#include <stdio.h>

#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"

/******************************************************************************
* Macros
******************************************************************************/
/* Interrupt priority for User Button Input. */
#define USER_BTN_INTR_PRIORITY          (3)

/* The maximum number of times each PUBLISH in this example will be retried. */
#define PUBLISH_RETRY_LIMIT             (10)

/* A PUBLISH message is retried if no response is received within this 
 * time (in milliseconds).
 */
#define PUBLISH_RETRY_MS                (1000)

/* Queue length of a message queue that is used to communicate with the 
 * publisher task.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH     (3u)

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void publisher_init(void);
static void publisher_deinit(void);
static void isr_button_press(void *callback_arg, cyhal_gpio_event_t event);
void print_heap_usage(char *msg);

/******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Handle of the queue holding the commands for the publisher task */
QueueHandle_t publisher_task_q;

/* Structure to store publish message information. */
cy_mqtt_publish_info_t publish_state_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_PUB_STATE7_TOPIC,
    .topic_len = (sizeof(MQTT_PUB_STATE7_TOPIC) - 1),
    .retain = true,
    .dup = false
};

cy_mqtt_publish_info_t publish_desc_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_PUB_DESC_TOPIC,
    .topic_len = (sizeof(MQTT_PUB_DESC_TOPIC) - 1),
    .retain = true,
    .dup = false
};

/* Static buffer reused for outgoing step payloads. */
static char step_payload[64];
static char desc_payload[64];

/* Structure that stores the callback data for the GPIO interrupt event. */
cyhal_gpio_callback_data_t cb_data =
{
    .callback = isr_button_press,
    .callback_arg = NULL
};

/******************************************************************************
 * Function Name: publisher_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the user button GPIO for the publisher and publishes 
 *  MQTT messages to the broker. The user button init and deinit operations,
 *  and the MQTT publish operation is performed based on commands sent by other
 *  tasks and callbacks over a message queue.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void publisher_task(void *pvParameters)
{
    /* Status variable */
    cy_rslt_t result;

    publisher_data_t publisher_q_data;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Initialize and set-up the user button GPIO. */
    publisher_init();

    /* Create a message queue to communicate with other tasks and callbacks. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

    /* Publish initial waiting state for robot arm: step 7 = 0 with waiting description. */
    publisher_data_t initial_state =
    {
        .cmd = PUBLISH_MQTT_MSG,
        .step_index = 7u,
        .state_value = 0u,
        .description = "robot_waiting"
    };
    xQueueSend(publisher_task_q, &initial_state, 0);

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data.cmd)
            {
                case PUBLISHER_INIT:
                {
                    /* Initialize and set-up the user button GPIO. */
                    publisher_init();
                    break;
                }

                case PUBLISHER_DEINIT:
                {
                    /* Deinit the user button GPIO and corresponding interrupt. */
                    publisher_deinit();
                    break;
                }

                case PUBLISH_MQTT_MSG:
                {
                    const char *desc = (publisher_q_data.description != NULL) ? publisher_q_data.description : "Step";
                    uint32_t state_value = (publisher_q_data.state_value <= 1u) ? publisher_q_data.state_value : 1u;

                    snprintf(step_payload, sizeof(step_payload), "%lu", (unsigned long)state_value);

                    publish_state_info.payload = step_payload;
                    publish_state_info.payload_len = strlen(step_payload);

                    printf("\nPublisher: Publishing state on '%s' -> %s\n",
                        publish_state_info.topic, step_payload);

                    result = cy_mqtt_publish(mqtt_connection, &publish_state_info);

                    if (result != CY_RSLT_SUCCESS)
                    {
                        printf("  Publisher: MQTT Publish failed with error 0x%0X.\n\n", (int)result);

                        /* Communicate the publish failure with the the MQTT 
                         * client task.
                         */
                        mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
                        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
                    }

                    /* Publish description if provided. */
                    snprintf(desc_payload, sizeof(desc_payload), "%s", desc);
                    publish_desc_info.payload = desc_payload;
                    publish_desc_info.payload_len = strlen(desc_payload);
					printf("  Publisher: MQTT description %s publish %s.\n\n",publish_desc_info.topic,publish_desc_info.payload);
                    result = cy_mqtt_publish(mqtt_connection, &publish_desc_info);

                    if (result != CY_RSLT_SUCCESS)
                    {
                        printf("  Publisher: MQTT description publish failed with error 0x%0X.\n\n", (int)result);
                        mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
                        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
                    }

                    print_heap_usage("publisher_task: After publishing state/description");
                    break;
                }
            }
        }
    }
}

/******************************************************************************
 * Function Name: publisher_init
 ******************************************************************************
 * Summary:
 *  Function that initializes and sets-up the user button GPIO pin along with  
 *  its interrupt.
 * 
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publisher_init(void)
{
    /* Initialize the user button GPIO and register interrupt on falling edge. */
    cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT,
                    CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    cyhal_gpio_register_callback(CYBSP_USER_BTN, &cb_data);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL,
                            USER_BTN_INTR_PRIORITY, true);
    
        printf("\nPress the user button (SW2) to publish the next robot arm step on topic '%s'...\n", 
            publish_state_info.topic);
}

/******************************************************************************
 * Function Name: publisher_deinit
 ******************************************************************************
 * Summary:
 *  Cleanup function for the publisher task that disables the user button  
 *  interrupt and deinits the user button GPIO pin.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publisher_deinit(void)
{
    /* Deregister the ISR and disable the interrupt on the user button. */
    cyhal_gpio_register_callback(CYBSP_USER_BTN, &cb_data);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL,
                            USER_BTN_INTR_PRIORITY, false);
    cyhal_gpio_free(CYBSP_USER_BTN);
}

/******************************************************************************
 * Function Name: isr_button_press
 ******************************************************************************
 * Summary:
 *  GPIO interrupt service routine. This function detects button presses and
 *  sends the publish command along with the next step index to the publisher
 *  task over a message queue. During bench testing, each press simulates the
 *  robot arm advancing to a new step.
 *
 * Parameters:
 *  void *callback_arg : pointer to variable passed to the ISR (unused)
 *  cyhal_gpio_event_t event : GPIO event type (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void isr_button_press(void *callback_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    publisher_data_t publisher_q_data;

    /* To avoid compiler warnings */
    (void) callback_arg;
    (void) event;

    /* Assign the publish command to be sent to the publisher task. */
    publisher_q_data.cmd = PUBLISH_MQTT_MSG;

    /* When user presses the button we mark robot arm step 7 as done. */
    publisher_q_data.step_index = 7u;
    publisher_q_data.state_value = 1u;
    publisher_q_data.description = "robotarm_done";

    /* Send the command and data to publisher task over the queue */
    xQueueSendFromISR(publisher_task_q, &publisher_q_data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* [] END OF FILE */
