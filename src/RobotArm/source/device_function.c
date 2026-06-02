/*
 * device_function.c
 *
 *  Created on: 14 Mar 2026
 *      Author: 12000249
 */

#include "device_function.h"
#include "cyhal_gpio.h"
#include <stdio.h>

#include "publisher_task.h"

/* CapSense includes */
#include "cycfg.h"

/******************************************************************************
* Global Motor Variables
******************************************************************************/
int M1_Pos = 0;
int M2_Pos = 0;
int M3_Pos = 0;

cyhal_pwm_t pwm;

static bool vorige_save_button = 1;

/******************************************************************************
* Global CapSense Variables
******************************************************************************/
volatile bool capsense_scan_complete = false;

/******************************************************************************
* Local Helpers
******************************************************************************/
static const char *pose_name(device_pose_t pose)
{
    switch (pose)
    {
        case DEVICE_POSE_CAMERA: return "camera";
        case DEVICE_POSE_INDUCTIVE: return "inductive";
        case DEVICE_POSE_SCALE: return "scale";
        case DEVICE_POSE_MIXER: return "mixer";
        case DEVICE_POSE_DISCARD: return "discard";
        default: return "unknown";
    }
}

static void capsense_callback(cy_stc_active_scan_sns_t *ptrActiveScan)
{
    (void)ptrActiveScan;
    capsense_scan_complete = true;
}

static void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}

/******************************************************************************
* CapSense Initialization
******************************************************************************/
uint32_t capsense_init(void)
{
    uint32_t status;

    status = Cy_CapSense_Init(&cy_capsense_context);
    if (status != CYRET_SUCCESS)
    {
        return status;
    }

    cyhal_system_set_isr(csd_interrupt_IRQn, csd_interrupt_IRQn,
                         CAPSENSE_INTR_PRIORITY, &capsense_isr);

    NVIC_ClearPendingIRQ(csd_interrupt_IRQn);
    NVIC_EnableIRQ(csd_interrupt_IRQn);

    status = Cy_CapSense_Enable(&cy_capsense_context);
    if (status != CYRET_SUCCESS)
    {
        return status;
    }

    status = Cy_CapSense_RegisterCallback(
        CY_CAPSENSE_END_OF_SCAN_E,
        capsense_callback,
        &cy_capsense_context);

    return status;
}

/******************************************************************************
* Motor Initialization
******************************************************************************/
void motor_init(void)
{
    cyhal_gpio_init(STEP1_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR1_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN1_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_init(STEP2_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR2_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN2_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_init(STEP3_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR3_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN3_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_init(SAVE_BUTTON_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);

    cyhal_pwm_init(&pwm, SERVO_PIN, NULL);
    cyhal_pwm_start(&pwm);
    Servo_startpositie();

    cyhal_gpio_write(EN1_PIN, 1);
    cyhal_gpio_write(EN2_PIN, 1);
    cyhal_gpio_write(EN3_PIN, 1);
}

/******************************************************************************
* Step Control
******************************************************************************/
void step_pin(uint8_t pin)
{
    cyhal_gpio_write(pin, 1);
    cyhal_system_delay_ms(2);
    cyhal_gpio_write(pin, 0);
    cyhal_system_delay_ms(2);
}

void Motor1_left(void)
{
    cyhal_gpio_write(DIR1_PIN, 1);
    step_pin(STEP1_PIN);
    if (M1_Pos > 0) M1_Pos--;
}

void Motor1_right(void)
{
    cyhal_gpio_write(DIR1_PIN, 0);
    step_pin(STEP1_PIN);
    M1_Pos++;
}

void Motor2_left(void)
{
    cyhal_gpio_write(DIR2_PIN, 0);
    step_pin(STEP2_PIN);
    if (M2_Pos > 0) M2_Pos--;
}

void Motor2_right(void)
{
    cyhal_gpio_write(DIR2_PIN, 1);
    step_pin(STEP2_PIN);
    M2_Pos++;
}

void Motor3_left(void)
{
    cyhal_gpio_write(DIR3_PIN, 1);
    step_pin(STEP3_PIN);
    if (M3_Pos > 0) M3_Pos--;
}

void Motor3_right(void)
{
    cyhal_gpio_write(DIR3_PIN, 0);
    step_pin(STEP3_PIN);
    M3_Pos++;
}

/******************************************************************************
* Servo Control
******************************************************************************/
void Servo_startpositie(void)
{
    cyhal_pwm_set_duty_cycle(&pwm, 5.0, 50);
    cyhal_system_delay_ms(1000);
}

void Servo_kiep(void)
{
    cyhal_system_delay_ms(1000);
    cyhal_pwm_set_duty_cycle(&pwm, 9.0, 50);
    cyhal_system_delay_ms(3000);

    cyhal_pwm_set_duty_cycle(&pwm, 5.0, 50);
    cyhal_system_delay_ms(3000);
}

/******************************************************************************
* Index-based Motor Control
******************************************************************************/
void motor_left_by_index(int idx)
{
    if (idx == 0) Motor3_left();
    else if (idx == 1) Motor2_left();
    else if (idx == 2) Motor1_left();
}

void motor_right_by_index(int idx)
{
    if (idx == 0) Motor3_right();
    else if (idx == 1) Motor2_right();
    else if (idx == 2) Motor1_right();
}

int get_motor_pos(int idx)
{
    if (idx == 0) return M3_Pos;
    if (idx == 1) return M2_Pos;
    return M1_Pos;
}

/******************************************************************************
* Pose Control
******************************************************************************/
void set_pose_value(int *pose, int idx, int value)
{
    if (idx == 0) pose[2] = value;
    else if (idx == 1) pose[1] = value;
    else if (idx == 2) pose[0] = value;
}

void print_pose(const char *name, int pose[3])
{
    printf("%s: M1=%d  M2=%d  M3=%d\r\n", name, pose[0], pose[1], pose[2]);
}

void move_to_pose(int target[3])
{
    while ((M1_Pos != target[0]) ||
           (M2_Pos != target[1]) ||
           (M3_Pos != target[2]))
    {
        if (M1_Pos < target[0]) Motor1_right();
        else if (M1_Pos > target[0]) Motor1_left();

        if (M2_Pos < target[1]) Motor2_right();
        else if (M2_Pos > target[1]) Motor2_left();

        if (M3_Pos < target[2]) Motor3_right();
        else if (M3_Pos > target[2]) Motor3_left();
    }
}

bool save_pressed(void)
{
    bool b = cyhal_gpio_read(SAVE_BUTTON_PIN);

    if ((b == 0) && (vorige_save_button == 1))
    {
        cyhal_system_delay_ms(200);
        vorige_save_button = b;
        return true;
    }

    vorige_save_button = b;
    return false;
}

/******************************************************************************
* DeviceFunction Implementation
******************************************************************************/
void DeviceFunction_init(DeviceFunction *dev, const char* name, cyhal_gpio_t motor_pin, int state_done_req)
{
    if (dev == NULL)
    {
        return;
    }

    dev->motor_pin = motor_pin;
    dev->pin_initialized = false;

    cyhal_gpio_init(dev->motor_pin, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP,
                    CYBSP_LED_STATE_OFF);
    dev->pin_initialized = true;

    strncpy(dev->device_name.buffer, name, sizeof(dev->device_name.buffer) - 1);
    dev->device_name.buffer[sizeof(dev->device_name.buffer) - 1] = '\0';

    dev->is_running = false;
    dev->state_done = false;
    dev->teach_mode = DEVICE_TEACH_IDLE;
    dev->teach_pose = DEVICE_POSE_CAMERA;
    dev->teach_motor_index = 0;

    for (int pose_index = 0; pose_index < DEVICE_POSE_COUNT; pose_index++)
    {
        dev->pose_values[pose_index][0] = 0;
        dev->pose_values[pose_index][1] = 0;
        dev->pose_values[pose_index][2] = 0;
    }

    (void)state_done_req;

    motor_init();

    if (capsense_init() != CYRET_SUCCESS)
    {
        printf("CapSense init failed\r\n");
    }
    else
    {
        printf("CapSense initialized successfully\r\n");
        Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
    }
}

void DeviceFunction_deinit(DeviceFunction *dev)
{
    if (dev == NULL)
    {
        return;
    }

    if (dev->pin_initialized)
    {
        cyhal_gpio_write(dev->motor_pin, 0);
        cyhal_gpio_free(dev->motor_pin);
        dev->pin_initialized = false;
    }

    dev->is_running = false;
    dev->state_done = false;
    dev->teach_mode = DEVICE_TEACH_IDLE;
    dev->teach_motor_index = 0;
}

void DeviceFunction_start_teach(DeviceFunction *dev, device_pose_t pose)
{
    if (dev == NULL)
    {
        return;
    }

    dev->teach_mode = DEVICE_TEACH_ACTIVE;
    dev->teach_pose = pose;
    dev->teach_motor_index = 0;

    printf("Teach %s positie\r\n", pose_name(pose));
    printf("Button0 = rechts, Button1 = links, P0_4 = opslaan/volgende\r\n");
}

void DeviceFunction_process_capsense(DeviceFunction *dev)
{
    uint32_t button0_status;
    uint32_t button1_status;

    if ((dev == NULL) || (dev->teach_mode == DEVICE_TEACH_IDLE))
    {
        return;
    }

    button0_status = Cy_CapSense_IsSensorActive(
        CY_CAPSENSE_BUTTON0_WDGT_ID,
        CY_CAPSENSE_BUTTON0_SNS0_ID,
        &cy_capsense_context);

    button1_status = Cy_CapSense_IsSensorActive(
        CY_CAPSENSE_BUTTON1_WDGT_ID,
        CY_CAPSENSE_BUTTON1_SNS0_ID,
        &cy_capsense_context);

    if ((button0_status != 0u) && (button1_status == 0u))
    {
        motor_right_by_index(dev->teach_motor_index);
    }
    else if ((button1_status != 0u) && (button0_status == 0u))
    {
        motor_left_by_index(dev->teach_motor_index);
    }
}

bool DeviceFunction_handle_save(DeviceFunction *dev)
{
    if ((dev == NULL) || (dev->teach_mode == DEVICE_TEACH_IDLE))
    {
        return false;
    }

    set_pose_value(dev->pose_values[dev->teach_pose],
                   dev->teach_motor_index,
                   get_motor_pos(dev->teach_motor_index));

    if (dev->teach_motor_index == 0)
    {
        printf("Motor 3 opgeslagen op %d\r\n", M3_Pos);
    }
    else if (dev->teach_motor_index == 1)
    {
        printf("Motor 2 opgeslagen op %d\r\n", M2_Pos);
    }
    else if (dev->teach_motor_index == 2)
    {
        printf("Motor 1 opgeslagen op %d\r\n", M1_Pos);
    }

    dev->teach_motor_index++;

    if (dev->teach_motor_index == 1)
    {
        printf("Nu motor 2 instellen...\r\n");
    }
    else if (dev->teach_motor_index == 2)
    {
        printf("Nu motor 1 instellen...\r\n");
    }
    else if (dev->teach_motor_index >= 3)
    {
        print_pose(pose_name(dev->teach_pose), dev->pose_values[dev->teach_pose]);
        dev->teach_mode = DEVICE_TEACH_IDLE;
        dev->teach_motor_index = 0;
        printf("\r\nTeach klaar.\r\n");
        return true;
    }

    return false;
}

void DeviceFunction_move_to_pose(DeviceFunction *dev, device_pose_t pose)
{
    if (dev == NULL)
    {
        return;
    }

    printf("Ga naar %s positie...\r\n", pose_name(pose));
    move_to_pose(dev->pose_values[pose]);

    if (pose == DEVICE_POSE_CAMERA)
    {
        Servo_startpositie();
    }
    else if (pose == DEVICE_POSE_DISCARD)
    {
        Servo_kiep();
    }

    printf("%s bereikt.\r\n", pose_name(pose));
}

void DeviceFunction_execute(DeviceFunction *dev)
{
    if (dev == NULL)
    {
        return;
    }

    printf("Device is running\n");
    if (!dev->state_done)
    {
        dev->is_running = true;
        publisher_data_t pub_cmd = {0};
        pub_cmd.cmd = PUBLISH_STATE;
        strncpy(pub_cmd.state, "running", sizeof(pub_cmd.state) - 1);
        pub_cmd.state[sizeof(pub_cmd.state) - 1] = '\0';
        xQueueSend(publisher_task_q, &pub_cmd, portMAX_DELAY);

        cyhal_gpio_write(dev->motor_pin, 1);
        cyhal_system_delay_ms(3000);

        cyhal_gpio_write(dev->motor_pin, 0);
        dev->is_running = false;
        dev->state_done = true;

        publisher_data_t pub_cmd_done = {0};
        pub_cmd_done.cmd = PUBLISH_STATE;
        strncpy(pub_cmd_done.state, "done", sizeof(pub_cmd_done.state) - 1);
        pub_cmd_done.state[sizeof(pub_cmd_done.state) - 1] = '\0';
        printf("Device is done\n");
        xQueueSend(publisher_task_q, &pub_cmd_done, portMAX_DELAY);
    }
}

const char* DeviceFunction_get_device_name(DeviceFunction *dev)
{
    if (dev == NULL)
    {
        return "";
    }

    return dev->device_name.buffer;
}

int DeviceFunction_get_state_done(DeviceFunction *dev)
{
    if (dev == NULL)
    {
        return 0;
    }

    return dev->state_done;
}

bool DeviceFunction_is_running(DeviceFunction *dev)
{
    if (dev == NULL)
    {
        return false;
    }

    return dev->is_running;
}

const char* DeviceFunction_get_pose_name(device_pose_t pose)
{
    return pose_name(pose);
}
