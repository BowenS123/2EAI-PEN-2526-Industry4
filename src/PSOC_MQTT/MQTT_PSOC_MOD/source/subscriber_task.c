#include "cyhal.h"
#include "cybsp.h"
#include "string.h"
#include "FreeRTOS.h"

/* Task header files */
#include "subscriber_task.h"
#include "mqtt_task.h"
#include "publisher_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"
#include <stdlib.h>
#include <stdbool.h>

/* Device Function for mixer */
#include "device_function.h"

/******************************************************************************
* Macros
******************************************************************************/
/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3u)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000)

/* The number of MQTT topics to be subscribed to. */
#define SUBSCRIPTION_COUNT                      (2)

/* Queue length of a message queue that is used to communicate with the 
 * subscriber task.
 */
#define SUBSCRIBER_TASK_QUEUE_LENGTH            (1u)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t subscriber_task_q;

/* Track last step index received from the broker. */
static uint32_t current_step_index = 0u;
/* Track last mixer state to suppress stale retained values. -1 = unknown, 0/1 = last observed. */
static int last_mixer_state = -1;
static DeviceFunction robot_device;


/* Configure the subscription information structures (state + description). */
static cy_mqtt_subscribe_info_t subscribe_info[SUBSCRIPTION_COUNT] =
{
    {
        .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
        .topic = MQTT_PUB_STATE6_TOPIC,
        .topic_len = (sizeof(MQTT_PUB_STATE6_TOPIC) - 1)
    },
    {
        .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
        .topic = MQTT_SUB_RESET_TOPIC,
        .topic_len = (sizeof(MQTT_SUB_RESET_TOPIC) - 1)
    },
};

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void subscribe_to_topic(void);
static void unsubscribe_from_topic(void);
void print_heap_usage(char *msg);

/******************************************************************************
 * Function Name: subscriber_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the user LED GPIO, subscribes to the specified MQTT topic,
 *  and controls the user LED based on the received commands over the message 
 *  queue. The task can also unsubscribe from the topic based on the commands
 *  via the message queue.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void subscriber_task(void *pvParameters)
{
    subscriber_data_t subscriber_q_data;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Initialize the Device Robot */
    DeviceFunction_init(&robot_device, "Robot", CYBSP_USER_LED, 6);  // 6 = required state_done (mixer klaar)

    /* Subscribe to the specified MQTT topic. */
    subscribe_to_topic();

    /* Create a message queue to communicate with other tasks and callbacks. */
    subscriber_task_q = xQueueCreate(SUBSCRIBER_TASK_QUEUE_LENGTH, sizeof(subscriber_data_t));

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(subscriber_task_q, &subscriber_q_data, portMAX_DELAY))
        {
            switch(subscriber_q_data.cmd)
            {
                case SUBSCRIBE_TO_TOPIC:
                {
                    subscribe_to_topic();
                    break;
                }

                case UNSUBSCRIBE_FROM_TOPIC:
                {
                    unsubscribe_from_topic();
                    break;
                }

                case UPDATE_ARM_STEP:
                {
                    current_step_index = subscriber_q_data.step_index;

                    printf("Subscriber: robot arm reported step %lu\n", (unsigned long)current_step_index);
                    print_heap_usage("subscriber_task: After updating step index");
                    break;
                }
            }
        }
    }
}

/******************************************************************************
 * Function Name: subscribe_to_topic
 ******************************************************************************
 * Summary:
 *  Function that subscribes to the MQTT topic specified by the macro 
 *  'MQTT_SUB_TOPIC'. This operation is retried a maximum of 
 *  'MAX_SUBSCRIBE_RETRIES' times with interval of 
 *  'MQTT_SUBSCRIBE_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void subscribe_to_topic(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    mqtt_task_cmd_t mqtt_task_cmd;

    for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++)
    {
        result = cy_mqtt_subscribe(mqtt_connection, subscribe_info, SUBSCRIPTION_COUNT);
        if (result == CY_RSLT_SUCCESS)
        {
            printf("\nMQTT client subscribed to the topics '%.*s' and '%.*s' successfully.\n",
                   subscribe_info[0].topic_len, subscribe_info[0].topic,
                   subscribe_info[1].topic_len, subscribe_info[1].topic);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
    }

    if (result != CY_RSLT_SUCCESS)
    {
        printf("\nMQTT Subscribe failed with error 0x%0X after %d retries...\n\n", 
               (int)result, MAX_SUBSCRIBE_RETRIES);

        /* Notify the MQTT client task about the subscription failure */
        mqtt_task_cmd = HANDLE_MQTT_SUBSCRIBE_FAILURE;
        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
    }
}

/******************************************************************************
 * Function Name: mqtt_subscription_callback
 ******************************************************************************
 * Summary:
 *  Callback to handle incoming MQTT messages. This callback prints the 
 *  contents of the incoming message and informs the subscriber task, via a 
 *  message queue, to turn on / turn off the device based on the received 
 *  message.
 *
 * Parameters:
 *  cy_mqtt_publish_info_t *received_msg_info : Information structure of the 
 *                                              received MQTT message
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info)
{
    /* Received MQTT message */
    const char *received_msg = received_msg_info->payload;
    int received_msg_len = received_msg_info->payload_len;

    /* Null-terminate the payload into a working buffer for parsing. */
    char payload_buf[96];
    int copy_len = (received_msg_len < (int)(sizeof(payload_buf) - 1)) ? received_msg_len : (int)(sizeof(payload_buf) - 1);
    memcpy(payload_buf, received_msg, copy_len);
    payload_buf[copy_len] = '\0';

    bool is_state6_topic = (received_msg_info->topic_len == strlen(MQTT_PUB_STATE6_TOPIC)) &&
                           (strncmp(received_msg_info->topic, MQTT_PUB_STATE6_TOPIC, received_msg_info->topic_len) == 0);
    bool is_reset_topic = (received_msg_info->topic_len == strlen(MQTT_SUB_RESET_TOPIC)) &&
                           (strncmp(received_msg_info->topic, MQTT_SUB_RESET_TOPIC, received_msg_info->topic_len) == 0);

    if (is_state6_topic){
        /* If the dedicated mixer topic (/6) carries a bare numeric value, parse it strictly. */
            char *end_ptr = NULL;
            unsigned long mixer_val = strtoul(payload_buf, &end_ptr, 10);

            if ((end_ptr != payload_buf) && (*end_ptr == '\0'))
            {
                /* Ignore the first retained message if it is stale. */
                if (received_msg_info->retain && last_mixer_state == -1)
                {
                    last_mixer_state = (int)mixer_val;
                    printf("Subscriber: Retained /6 first message ignored (value=%lu).\n", mixer_val);
                    return;
                }

                last_mixer_state = (int)mixer_val;

                if (mixer_val == 1u)
                {
					// hier device execute, dan wanneer klaar code publish robot is done (7=1)
					DeviceFunction_execute(&robot_device);
                    return;
                }
        }
    }
    
    if(is_reset_topic){
			publisher_data_t pub_cmd =
            {
                .cmd = PUBLISH_MQTT_MSG,
                .step_index = 7u,
                .state_value = 0u,
                .description = "robotarm_waiting"
            };

            xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
            return;
	}
}

/******************************************************************************
 * Function Name: unsubscribe_from_topic
 ******************************************************************************/
static void unsubscribe_from_topic(void)
{
    cy_rslt_t result = cy_mqtt_unsubscribe(mqtt_connection, (cy_mqtt_unsubscribe_info_t *) subscribe_info, SUBSCRIPTION_COUNT);
    if (result != CY_RSLT_SUCCESS){printf("MQTT Unsubscribe operation failed with error 0x%0X!\n", (int)result);}
}

/* [] END OF FILE */
