/******************************************************************************
* File Name:   subscriber_task.c
*
* Description: This file contains the task that subscribes to the robot command
*              topic and drives a small state machine. Each received command
*              toggles a heartbeat LED and publishes a JSON state update.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"
#include "string.h"
#include "FreeRTOS.h"
#include "queue.h"

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
#include <stdint.h>

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

/* Poll interval to also handle local button demo flow. */
#define SUBSCRIBER_LOOP_POLL_MS                 (50u)

/* Demo runtime: machine stays in running state for 3 seconds. */
#define DEMO_RUNNING_TIME_MS                    (3000u)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t subscriber_task_q;

/* Track current robot state. */
static robot_state_t current_state = ROBOT_STATE_IDLE;

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
    bool robot_running_active = false;
    bool startup_published = false;
    TickType_t robot_running_until = 0;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Initialize the User LED to provide a simple heartbeat when commands arrive. */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP,
                    CYBSP_LED_STATE_OFF);

    /* Subscribe to the specified MQTT topic. */
    subscribe_to_topic();

    /* Create a message queue to communicate with other tasks and callbacks. */
    subscriber_task_q = xQueueCreate(SUBSCRIBER_TASK_QUEUE_LENGTH, sizeof(subscriber_data_t));

    while (true)
    {
        /* Publish initial idle state once the publisher queue becomes available. */
        if (!startup_published && (publisher_task_q != NULL))
        {
            startup_published = true;
            publish_state("idle");
            printf("Subscriber: startup -> published idle\n");
        }

        /* Poll queue with timeout so local button/timer demo logic can run. */
        if (pdTRUE == xQueueReceive(subscriber_task_q, &subscriber_q_data, pdMS_TO_TICKS(SUBSCRIBER_LOOP_POLL_MS)))
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

                case HANDLE_ROBOT_CMD:
                {
                    cyhal_gpio_toggle(CYBSP_USER_LED);
                    if (subscriber_q_data.robot_cmd == ROBOT_CMD_STOP)
                    {
                        robot_running_active = false;
                        handle_robot_cmd(subscriber_q_data.robot_cmd, subscriber_q_data.retain);
                    }
                    else if (subscriber_q_data.robot_cmd == ROBOT_CMD_UNKNOWN)
                    {
                        handle_robot_cmd(subscriber_q_data.robot_cmd, subscriber_q_data.retain);
                    }
                    else
                    {
                        robot_running_active = true;
                        robot_running_until = xTaskGetTickCount() + pdMS_TO_TICKS(DEMO_RUNNING_TIME_MS);
                        current_state = ROBOT_STATE_MOVING;
                        publish_state("running");
                        printf("Subscriber: robot cmd received -> running for %lu ms\n",
                               (unsigned long)DEMO_RUNNING_TIME_MS);
                    }
                    break;
                }

                case HANDLE_TRANSPORT_DONE:
                {
                    if (!robot_running_active)
                    {
                        robot_running_active = true;
                        robot_running_until = xTaskGetTickCount() + pdMS_TO_TICKS(DEMO_RUNNING_TIME_MS);
                        current_state = ROBOT_STATE_MOVING;
                        publish_state("running");
                        printf("Subscriber: transport done -> robot running for %lu ms\n",
                               (unsigned long)DEMO_RUNNING_TIME_MS);
                    }
                    break;
                }
            }
        }

        if (robot_running_active && ((int32_t)(xTaskGetTickCount() - robot_running_until) >= 0))
        {
            robot_running_active = false;
            current_state = ROBOT_STATE_IDLE;
            publish_state("idle");
            printf("Subscriber: robot cycle done -> back to idle\n");
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

    if (strstr(buf, "\"cmd\":\"pick\"") != NULL)
    {
        return ROBOT_CMD_PICK;
    }
    if (strstr(buf, "\"cmd\":\"place\"") != NULL)
    {
        return ROBOT_CMD_PLACE;
    }
    if (strstr(buf, "\"cmd\":\"home\"") != NULL)
    {
        return ROBOT_CMD_HOME;
    }
    if (strstr(buf, "\"cmd\":\"stop\"") != NULL)
    {
        return ROBOT_CMD_STOP;
    }

    return ROBOT_CMD_UNKNOWN;
}

static bool is_transport_done_msg(const char *payload, int payload_len)
{
    char buf[160];
    int copy_len = (payload_len < (int)(sizeof(buf) - 1)) ? payload_len : (int)(sizeof(buf) - 1);

    memcpy(buf, payload, copy_len);
    buf[copy_len] = '\0';

    return ((strstr(buf, "\"state\":\"done\"") != NULL)
            || (strstr(buf, "\"status\":\"done\"") != NULL)
            || (strstr(buf, "\"done\":true") != NULL));
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
        case ROBOT_CMD_PICK:
        case ROBOT_CMD_PLACE:
        case ROBOT_CMD_HOME:
        {
            current_state = ROBOT_STATE_IDLE;
            publish_state("idle");
            printf("Subscriber: cmd=%d accepted, waiting for transportband done\n", (int)cmd);
            break;
        }

        case ROBOT_CMD_STOP:
        {
            current_state = ROBOT_STATE_STOPPED;
            publish_state("idle");
            break;
        }

        case ROBOT_CMD_UNKNOWN:
        default:
        {
            current_state = ROBOT_STATE_ERROR;
            publish_error("unknown command");
            break;
        }
    }

    printf("Subscriber: handled cmd=%d retain=%d -> state=%d\n", (int)cmd, (int)retain, (int)current_state);
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
