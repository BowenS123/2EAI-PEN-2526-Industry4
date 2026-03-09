/******************************************************************************
* File Name:   subscriber_task.c
*
* Description: This file contains the task that subscribes to 'MQTT_SUB_TOPIC'
*              to receive robot-arm step updates. Each received step toggles a
*              heartbeat LED and logs the current step index.
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

/******************************************************************************
* Macros
******************************************************************************/
/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3u)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000)

/* The number of MQTT topics to be subscribed to. */
#define SUBSCRIPTION_COUNT                      (3)

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

/* Configure the subscription information structures (state + description). */
static cy_mqtt_subscribe_info_t subscribe_info[SUBSCRIPTION_COUNT] =
{
    {
        .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
        .topic = MQTT_PUB_STATE_TOPIC,
        .topic_len = (sizeof(MQTT_PUB_STATE_TOPIC) - 1)
    },
    {
        .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
        .topic = MQTT_PUB_STATE6_TOPIC,
        .topic_len = (sizeof(MQTT_PUB_STATE6_TOPIC) - 1)
    },
    {
        .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
        .topic = MQTT_PUB_STATE7_TOPIC,
        .topic_len = (sizeof(MQTT_PUB_STATE7_TOPIC) - 1)
    },
    /* Description topic intentionally not subscribed (ignored). */
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

    /* Initialize the User LED to provide a simple heartbeat on step updates. */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP,
                    CYBSP_LED_STATE_OFF);

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
                    /* Toggle LED to acknowledge and store the step index. */
                    cyhal_gpio_toggle(CYBSP_USER_LED);
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

    /* Data to be sent to the subscriber task queue. */
    subscriber_data_t subscriber_q_data;

    printf("  \nSubscriber: Incoming MQTT message received:\n"
           "    Publish topic name: %.*s\n"
           "    Publish QoS: %d\n"
           "    Publish payload: %.*s\n",
           received_msg_info->topic_len, received_msg_info->topic,
           (int) received_msg_info->qos,
           (int) received_msg_info->payload_len, (const char *)received_msg_info->payload);

    /* Null-terminate the payload into a working buffer for parsing. */
    char payload_buf[96];
    int copy_len = (received_msg_len < (int)(sizeof(payload_buf) - 1)) ? received_msg_len : (int)(sizeof(payload_buf) - 1);
    memcpy(payload_buf, received_msg, copy_len);
    payload_buf[copy_len] = '\0';

    bool is_state_topic = (received_msg_info->topic_len == strlen(MQTT_PUB_STATE_TOPIC)) &&
                          (strncmp(received_msg_info->topic, MQTT_PUB_STATE_TOPIC, received_msg_info->topic_len) == 0);
    bool is_state6_topic = (received_msg_info->topic_len == strlen(MQTT_PUB_STATE6_TOPIC)) &&
                           (strncmp(received_msg_info->topic, MQTT_PUB_STATE6_TOPIC, received_msg_info->topic_len) == 0);
    bool is_state7_topic = (received_msg_info->topic_len == strlen(MQTT_PUB_STATE7_TOPIC)) &&
                           (strncmp(received_msg_info->topic, MQTT_PUB_STATE7_TOPIC, received_msg_info->topic_len) == 0);

    /* Detect mixer completion message from state path and auto-drive robot arm state. */
    if (is_state_topic || is_state6_topic || is_state7_topic)
    {
        /* If the dedicated mixer topic (/6) carries a bare numeric value, parse it strictly. */
        if (is_state6_topic)
        {
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
                printf("Subscriber: /6 numeric payload=%lu retain=%d -> last_mixer_state=%d.\n", mixer_val, (int)received_msg_info->retain, last_mixer_state);

                if (mixer_val == 1u)
                {
                    publisher_data_t pub_cmd =
                    {
                        .cmd = PUBLISH_MQTT_MSG,
                        .step_index = 7u,
                        .state_value = 1u,
                        .description = "robotarm_done"
                    };

                    printf("Subscriber: Mixer /6=1 -> auto publish robot arm 7=1.\n");
                    xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
                    return;
                }
                else if (mixer_val == 0u)
                {
                    publisher_data_t pub_cmd =
                    {
                        .cmd = PUBLISH_MQTT_MSG,
                        .step_index = 7u,
                        .state_value = 0u,
                        .description = "robotarm_waiting"
                    };

                    printf("Subscriber: Mixer /6=0 -> auto publish robot arm 7=0.\n");
                    xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
                    return;
                }
                /* Any other numeric value on /6 is ignored. */
                printf("Subscriber: Mixer /6 numeric value %lu ignored (not 0/1).\n", mixer_val);
            }
        }

        /* If mixer reports done/not-done via combined state topic payloads like "6 = 1" / "6 = 0". */
        if (strstr(payload_buf, "6 = 1") != NULL)
        {
            publisher_data_t pub_cmd =
            {
                .cmd = PUBLISH_MQTT_MSG,
                .step_index = 7u,
                .state_value = 1u,
                .description = "robotarm_done"
            };

            printf("Subscriber: Combined topic shows 6 = 1 -> auto publish robot arm 7=1.\n");
            xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
            return;
        }

        if (strstr(payload_buf, "6 = 0") != NULL)
        {
            publisher_data_t pub_cmd =
            {
                .cmd = PUBLISH_MQTT_MSG,
                .step_index = 7u,
                .state_value = 0u,
                .description = "robotarm_waiting"
            };

            printf("Subscriber: Combined topic shows 6 = 0 -> auto publish robot arm 7=0.\n");
            xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);
            return;
        }

        /* Fall back to plain numeric step parsing for other state updates. */
        char *end_ptr = NULL;
        unsigned long parsed_step = strtoul(payload_buf, &end_ptr, 10);

        if ((end_ptr == payload_buf) || (*end_ptr != '\0'))
        {
            printf("  Subscriber: Payload is not a valid step number.\n");
            return;
        }

        subscriber_q_data.cmd = UPDATE_ARM_STEP;
        subscriber_q_data.step_index = (uint32_t) parsed_step;

        printf("Subscriber: Parsed numeric state %lu -> queue UPDATE_ARM_STEP.\n", parsed_step);
        print_heap_usage("MQTT subscription callback");

        /* Send the command and data to subscriber task queue */
        xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);
    }
}

/******************************************************************************
 * Function Name: unsubscribe_from_topic
 ******************************************************************************
 * Summary:
 *  Function that unsubscribes from the topic specified by the macro 
 *  'MQTT_SUB_TOPIC'.
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

/* [] END OF FILE */
