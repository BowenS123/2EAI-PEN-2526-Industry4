#include "FreeRTOS.h"
#include "queue.h"

/* Task header files */
#include "publisher_task.h"
#include "mqtt_task.h"

#include "mqtt_client_config.h"
#include "string.h"
#include <stdio.h>

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"

/******************************************************************************
* Macros
******************************************************************************/
/* Queue length of a message queue that is used to communicate with the 
 * publisher task.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH     (4u)

/******************************************************************************
* Function Prototypes
******************************************************************************/
static void publish_json(cy_mqtt_publish_info_t *publish_info,
                         const char *payload,
                         const char *label);

/******************************************************************************
* Global Variables
******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Handle of the queue holding the commands for the publisher task */
QueueHandle_t publisher_task_q;

/* Structure to store publish message information. */
static cy_mqtt_publish_info_t publish_state_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_STATE_QOS,
    .topic = MQTT_STATE_TOPIC,
    .topic_len = (sizeof(MQTT_STATE_TOPIC) - 1),
    .retain = false,
    .dup = false
};

static cy_mqtt_publish_info_t publish_error_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_ERROR_QOS,
    .topic = MQTT_ERROR_TOPIC,
    .topic_len = (sizeof(MQTT_ERROR_TOPIC) - 1),
    .retain = false,
    .dup = false
};

/* Static buffer reused for outgoing payloads. */
static char json_payload[128];

/******************************************************************************
 * Function Name: publisher_task
 ****************************************************************************
 * Summary:
 *  Task that publishes MQTT messages to the broker based on commands sent by
 *  other tasks and callbacks over a message queue. It formats the payloads as
 *  compact JSON strings for state and error topics.
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
    publisher_data_t publisher_q_data;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Create a message queue to communicate with other tasks and callbacks. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data.cmd)
            {
                case PUBLISHER_INIT:
                {
                    /* Nothing to initialize now, kept for symmetry. */
                    break;
                }

                case PUBLISHER_DEINIT:
                {
                    /* Nothing to deinit because we don't own hardware. */
                    break;
                }

                case PUBLISH_STATE:
                {
                    const char *state = (publisher_q_data.state != NULL) ? publisher_q_data.state : "unknown";

                    snprintf(json_payload, sizeof(json_payload), "{\"state\":\"%s\"}", state);

                    publish_json(&publish_state_info, json_payload, "state");
                    break;
                }

                case PUBLISH_ERROR:
                {
                    const char *state = (publisher_q_data.state != NULL) ? publisher_q_data.state : "error";
                    const char *detail = (publisher_q_data.detail != NULL) ? publisher_q_data.detail : "";
                    const char *error_flag = (strcmp(state, "ok") == 0) ? "false" : "true";
                    snprintf(json_payload, sizeof(json_payload),
                             "{\"state\":\"%s\",\"error\":%s,\"message\":\"%s\"}",
                             state,
                             error_flag,
                             detail);
                    publish_json(&publish_error_info, json_payload, "error");
                    break;
                }
            }
        }
    }
}

/******************************************************************************
 * Function Name: publish_json
 ****************************************************************************
 * Summary:
 *  Helper to publish a prepared JSON payload and report errors back to the
 *  MQTT task so reconnection logic can kick in when needed.
 *
 * Parameters:
 *  cy_mqtt_publish_info_t *publish_info : MQTT publish structure to populate
 *  const char *payload                    : JSON string to send
 *  const char *label                      : Log label for readability
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publish_json(cy_mqtt_publish_info_t *publish_info,
                         const char *payload,
                         const char *label)
{
    cy_rslt_t result;
    mqtt_task_cmd_t mqtt_task_cmd;

    publish_info->payload = payload;
    publish_info->payload_len = strlen(payload);

    printf("\nPublisher: Publishing %s on '%.*s' -> %s\n",
           label,
           publish_info->topic_len,
           publish_info->topic,
           payload);

    result = cy_mqtt_publish(mqtt_connection, publish_info);

    if (result != CY_RSLT_SUCCESS)
    {
        printf("  Publisher: MQTT %s publish failed with error 0x%0X.\n\n", label, (int)result);

        /* Communicate the publish failure with the MQTT client task. */
        mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
    }
}

/* [] END OF FILE */
