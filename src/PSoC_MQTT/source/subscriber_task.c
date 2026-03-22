#include "cyhal.h"
#include "cybsp.h"
#include "string.h"
#include "FreeRTOS.h"
#include "queue.h"

/* Task header files */
#include "subscriber_task.h"
#include "mqtt_task.h"
#include "publisher_task.h"
#include "device_function.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"
#include <stdlib.h>
#include <stdbool.h>

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
#define SUBSCRIBER_TASK_QUEUE_LENGTH            (2u)

/* Poll interval for queue processing. */
#define SUBSCRIBER_LOOP_POLL_MS                 (50u)

/* GPIO abstraction output pin used by DeviceFunction. */
#define DEVICE_EXEC_PIN                         (CYBSP_USER_LED)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t subscriber_task_q;

/* Track current robot state. */
static robot_state_t current_state = ROBOT_STATE_IDLE;

/* Device abstraction instance for robot action output. */
static DeviceFunction robot_arm;
static bool robot_arm_initialized = false;

/* Pending action until transportband reports done. */
static bool pending_robot_action = false;
static robot_cmd_t pending_robot_cmd = ROBOT_CMD_UNKNOWN;

/* Configure the subscription information structures. */
static cy_mqtt_subscribe_info_t subscribe_info[SUBSCRIPTION_COUNT] =
{
    {
        .qos = (cy_mqtt_qos_t) MQTT_CMD_QOS,
        .topic = MQTT_CMD_TOPIC,
        .topic_len = (sizeof(MQTT_CMD_TOPIC) - 1)
    },
    {
        .qos = (cy_mqtt_qos_t) MQTT_STATE_QOS,
        .topic = MQTT_TRANSPORT_STATE_TOPIC,
        .topic_len = (sizeof(MQTT_TRANSPORT_STATE_TOPIC) - 1)
    }
};

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void subscribe_to_topic(void);
static void unsubscribe_from_topic(void);
static robot_cmd_t parse_robot_cmd(const char *payload, int payload_len);
static bool is_transport_done_msg(const char *payload, int payload_len);
static void handle_robot_cmd(robot_cmd_t cmd, bool retain);
static void handle_transport_done(void);
static void handle_subscriber_cmd(const subscriber_data_t *subscriber_q_data);
static void publish_state(const char *state);
static void publish_error(const char *detail);

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
    bool startup_published = false;

    /* To avoid compiler warnings */
    (void) pvParameters;

    DeviceFunction_init(&robot_arm, "robot_arm", DEVICE_EXEC_PIN, 1);
    robot_arm_initialized = true;

    /* Create the queue before subscribing, so callback can enqueue safely. */
    subscriber_task_q = xQueueCreate(SUBSCRIBER_TASK_QUEUE_LENGTH, sizeof(subscriber_data_t));

    /* Subscribe to the specified MQTT topics. */
    subscribe_to_topic();

    while (true)
    {
        /* Publish initial idle state once the publisher queue becomes available. */
        if (!startup_published && (publisher_task_q != NULL))
        {
            startup_published = true;
            publish_state("idle");
            printf("Subscriber: startup -> published idle\n");
        }
		
        if (pdTRUE == xQueueReceive(subscriber_task_q, &subscriber_q_data, pdMS_TO_TICKS(SUBSCRIBER_LOOP_POLL_MS)))
        {
            handle_subscriber_cmd(&subscriber_q_data);
        }
    }
}

/******************************************************************************
 * Function Name: subscribe_to_topic
 ******************************************************************************
 * Summary:
 *  Function that subscribes to the MQTT topic specified by the macro 
 *  'MQTT_CMD_TOPIC'. This operation is retried a maximum of 
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
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* Subscribe with the configured parameters. */
    for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++)
    {
        result = cy_mqtt_subscribe(mqtt_connection, subscribe_info, SUBSCRIPTION_COUNT);
        if (result == CY_RSLT_SUCCESS)
        {
            printf("\nMQTT client subscribed to the topic '%.*s' successfully (QoS %d).\n",
                   subscribe_info[0].topic_len, subscribe_info[0].topic, (int)subscribe_info[0].qos);
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

    /* Data to be sent to the subscriber task queue. */
    subscriber_data_t subscriber_q_data;

    printf("  \nSubscriber: Incoming MQTT message received:\n"
           "    Publish topic name: %.*s\n"
           "    Publish QoS: %d\n"
           "    Publish payload: %.*s\n",
           received_msg_info->topic_len, received_msg_info->topic,
           (int) received_msg_info->qos,
           (int) received_msg_info->payload_len, (const char *)received_msg_info->payload);

    if ((received_msg_info->topic_len == (sizeof(MQTT_CMD_TOPIC) - 1))
        && (strncmp(received_msg_info->topic, MQTT_CMD_TOPIC, received_msg_info->topic_len) == 0))
    {
        robot_cmd_t cmd = parse_robot_cmd(received_msg, received_msg_len);
        subscriber_q_data.cmd = HANDLE_ROBOT_CMD;
        subscriber_q_data.robot_cmd = cmd;
        subscriber_q_data.retain = received_msg_info->retain;
    }
    else if ((received_msg_info->topic_len == (sizeof(MQTT_TRANSPORT_STATE_TOPIC) - 1))
             && (strncmp(received_msg_info->topic, MQTT_TRANSPORT_STATE_TOPIC, received_msg_info->topic_len) == 0)
             && is_transport_done_msg(received_msg, received_msg_len))
    {
        subscriber_q_data.cmd = HANDLE_TRANSPORT_DONE;
        subscriber_q_data.robot_cmd = ROBOT_CMD_UNKNOWN;
        subscriber_q_data.retain = received_msg_info->retain;
    }
    else
    {
        return;
    }

    xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);
}

/******************************************************************************
 * Function Name: unsubscribe_from_topic
 ******************************************************************************
 * Summary:
 *  Function that unsubscribes from the topic specified by the macro 
 *  'MQTT_CMD_TOPIC'.
 *
 * Parameters:
 *  void 
 *
 * Return:
 *  void 
 *
 ******************************************************************************/
static void unsubscribe_from_topic(void)
{
    cy_rslt_t result = cy_mqtt_unsubscribe(mqtt_connection, 
                                           (cy_mqtt_unsubscribe_info_t *) subscribe_info, 
                                           SUBSCRIPTION_COUNT);

    if (result != CY_RSLT_SUCCESS)
    {
        printf("MQTT Unsubscribe operation failed with error 0x%0X!\n", (int)result);
    }
}

/******************************************************************************
 * Function Name: parse_robot_cmd
 ****************************************************************************
 * Summary:
 *  Very small JSON helper that detects {"cmd":"pick"|"place"|"home"|"stop"}.
 *  The payload is first copied into a bounded local buffer and then searched.
 *
 * Parameters:
 *  const char *payload    : Raw MQTT payload pointer
 *  int payload_len        : Payload length
 *
 * Return:
 *  robot_cmd_t : Parsed command enum (ROBOT_CMD_UNKNOWN if not found)
 *
 ******************************************************************************/
static robot_cmd_t parse_robot_cmd(const char *payload, int payload_len)
{
    char buf[160];
    int copy_len = (payload_len < (int)(sizeof(buf) - 1)) ? payload_len : (int)(sizeof(buf) - 1);

    memcpy(buf, payload, copy_len);
    buf[copy_len] = '\0';

    if (strstr(buf, "\"cmd\":\"start\"") != NULL)
    {
        return ROBOT_CMD_START;
    }
    if (strstr(buf, "\"cmd\":\"stop\"") != NULL)
    {
        return ROBOT_CMD_STOP;
    }

    return ROBOT_CMD_UNKNOWN;
}

static bool is_transport_done_msg(const char *payload, int payload_len)
{
    /* Check if payload contains "done" as a state value */
    if (payload_len >= 4)  /* at least "done" length */
    {
        /* Simple check for "state":"done" pattern */
        if (strstr(payload, "\"state\":\"done\"") != NULL)
        {
            return true;
        }
        
        /* Alternative if payload might have whitespace */
        if (strstr(payload, "\"state\": \"done\"") != NULL)
        {
            return true;
        }
    }
    
    return false;
}

/******************************************************************************
 * Function Name: handle_robot_cmd
 ****************************************************************************
 * Summary:
 *  Update internal state machine and publish state/error updates according to
 *  the command. This function keeps processing non-blocking; real motion logic
 *  can be hooked where indicated.
 *
 * Parameters:
 *  robot_cmd_t cmd : Parsed command enum
 *  bool retain     : Indicates if the payload was retained (used for logging)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void handle_robot_cmd(robot_cmd_t cmd, bool retain)
{
    switch (cmd)
    {
		case ROBOT_CMD_START:
        {
			handle_transport_done();
            break;
        }
        case ROBOT_CMD_STOP:
        {
            pending_robot_action = false;
            pending_robot_cmd = ROBOT_CMD_UNKNOWN;
            if (robot_arm_initialized)
            {
                cyhal_gpio_write(robot_arm.motor_pin, CYBSP_LED_STATE_OFF);
                robot_arm.is_running = false;
            }
            current_state = ROBOT_STATE_STOPPED;
            publish_state("idle");
            break;
        }

        case ROBOT_CMD_UNKNOWN:
        default:
        {
            pending_robot_action = false;
            pending_robot_cmd = ROBOT_CMD_UNKNOWN;
            current_state = ROBOT_STATE_ERROR;
            publish_error("unknown command");
            break;
        }
    }

    printf("Subscriber: handled cmd=%d retain=%d -> state=%d\n", (int)cmd, (int)retain, (int)current_state);
}

static void handle_transport_done(void)
{
    if (!robot_arm_initialized)
    {
        publish_error("device not initialized");
        return;
    }

    current_state = ROBOT_STATE_MOVING;
    printf("Subscriber: transport done -> executing cmd=%d on %s\n",
           (int)pending_robot_cmd,
           DeviceFunction_get_device_name(&robot_arm));

    DeviceFunction_execute(&robot_arm);

    pending_robot_action = false;
    pending_robot_cmd = ROBOT_CMD_UNKNOWN;
    current_state = ROBOT_STATE_IDLE;
    printf("Subscriber: robot cycle done -> back to idle\n");
}

static void handle_subscriber_cmd(const subscriber_data_t *subscriber_q_data)
{
    if (subscriber_q_data == NULL)
    {
        return;
    }

    switch(subscriber_q_data->cmd)
    {
        case SUBSCRIBE_TO_TOPIC:
        {
            subscribe_to_topic();
            break;
        }

        case UNSUBSCRIBE_FROM_TOPIC:
        {
            unsubscribe_from_topic();
            if (robot_arm_initialized)
            {
                DeviceFunction_deinit(&robot_arm);
                robot_arm_initialized = false;
            }
            break;
        }

        case HANDLE_ROBOT_CMD:
        {
            handle_robot_cmd(subscriber_q_data->robot_cmd, subscriber_q_data->retain);
            break;
        }

        case HANDLE_TRANSPORT_DONE:
        {
            handle_transport_done();
            break;
        }

        default:
            break;
    }
}

/******************************************************************************
 * Function Name: publish_state
 ****************************************************************************
 * Summary:
 *  Enqueue a state JSON message for publishing.
 *
 * Parameters:
 *  const char *state : State string (e.g., idle/running/base)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publish_state(const char *state)
{
    publisher_data_t pub_cmd =
    {
        .cmd = PUBLISH_STATE,
        .state = state,
        .detail = NULL
    };

    publisher_data_t pub_error_clear_cmd =
    {
        .cmd = PUBLISH_ERROR,
        .state = "ok",
        .detail = ""
    };

    if (publisher_task_q != NULL)
    {
        xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
        xQueueSend(publisher_task_q, &pub_error_clear_cmd, portMAX_DELAY);
    }
    else
    {
        printf("Subscriber: publisher_task_q is NULL, state not sent yet.\n");
    }
}

/******************************************************************************
 * Function Name: publish_error
 ****************************************************************************
 * Summary:
 *  Enqueue an error JSON message for publishing.
 *
 * Parameters:
 *  const char *detail : Error detail string
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publish_error(const char *detail)
{
    publisher_data_t pub_cmd =
    {
        .cmd = PUBLISH_ERROR,
        .state = "error",
        .detail = detail
    };

    if (publisher_task_q != NULL)
    {
        xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
    }
    else
    {
        printf("Subscriber: publisher_task_q is NULL, error not sent yet.\n");
    }
}

/* [] END OF FILE */
