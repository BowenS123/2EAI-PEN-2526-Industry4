#ifndef DEVICE_FUNCTION_H_
#define DEVICE_FUNCTION_H_

#include "cyhal.h"
#include "cybsp.h"
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

/* CapSense includes */
#include "cycfg_capsense.h"

/* External reference to publisher task queue */
extern QueueHandle_t publisher_task_q;

/******************************************************************************
* Motor Pin Definitions
******************************************************************************/
/* Motor 1 */
#define STEP1_PIN   P9_0
#define DIR1_PIN    P9_1
#define EN1_PIN     P9_2

/* Motor 2 */
#define STEP2_PIN   P10_0
#define DIR2_PIN    P10_1
#define EN2_PIN     P10_2

/* Motor 3 */
#define STEP3_PIN   P10_3
#define DIR3_PIN    P10_4
#define EN3_PIN     P10_5

/* Servo */
#define SERVO_PIN   P10_6

/* Save button */
#define SAVE_BUTTON_PIN P0_4

/******************************************************************************
* CapSense Configuration
******************************************************************************/
#define CAPSENSE_INTR_PRIORITY (7u)

/******************************************************************************
* Global Motor Variables
******************************************************************************/
extern int M1_Pos;
extern int M2_Pos;
extern int M3_Pos;
extern cyhal_pwm_t pwm;

/******************************************************************************
* Global CapSense Variables
******************************************************************************/
extern volatile bool capsense_scan_complete;

typedef struct
{
    char buffer[32];
} device_string_t;

typedef enum
{
    DEVICE_POSE_CAMERA = 0,
    DEVICE_POSE_INDUCTIVE = 1,
    DEVICE_POSE_SCALE = 2,
    DEVICE_POSE_MIXER = 3,
    DEVICE_POSE_DISCARD = 4,
    DEVICE_POSE_COUNT
} device_pose_t;

typedef enum
{
    DEVICE_TEACH_IDLE = 0,
    DEVICE_TEACH_ACTIVE = 1
} device_teach_mode_t;

typedef struct
{
    cyhal_gpio_t motor_pin;
    device_string_t device_name;
    bool is_running;
    int state_done;
    bool pin_initialized;

    device_teach_mode_t teach_mode;
    device_pose_t teach_pose;
    int teach_motor_index;
    int pose_values[DEVICE_POSE_COUNT][3];
} DeviceFunction;

/******************************************************************************
* Function Prototypes - Device Initialization
******************************************************************************/
void DeviceFunction_init(DeviceFunction *dev,
                         const char *name,
                         cyhal_gpio_t motor_pin,
                         int state_done_req);
void DeviceFunction_deinit(DeviceFunction *dev);
void DeviceFunction_execute(DeviceFunction *dev);
void DeviceFunction_start_teach(DeviceFunction *dev, device_pose_t pose);
void DeviceFunction_process_capsense(DeviceFunction *dev);
bool DeviceFunction_handle_save(DeviceFunction *dev);
void DeviceFunction_move_to_pose(DeviceFunction *dev, device_pose_t pose);

/******************************************************************************
* Function Prototypes - Motor Control
******************************************************************************/
void motor_init(void);

/* Step control */
void step_pin(uint8_t pin);

/* Motor 1 control */
void Motor1_left(void);
void Motor1_right(void);

/* Motor 2 control */
void Motor2_left(void);
void Motor2_right(void);

/* Motor 3 control */
void Motor3_left(void);
void Motor3_right(void);

/* Servo control */
void Servo_startpositie(void);
void Servo_kiep(void);

/* Index-based motor control */
void motor_left_by_index(int idx);
void motor_right_by_index(int idx);
int get_motor_pos(int idx);

/* Pose control */
void set_pose_value(int *pose, int idx, int value);
void print_pose(const char *name, int pose[3]);
void move_to_pose(int target[3]);

/* Save button */
bool save_pressed(void);

/******************************************************************************
* Function Prototypes - CapSense Control
******************************************************************************/
uint32_t capsense_init(void);

/******************************************************************************
* Function Prototypes - Status Queries
******************************************************************************/
const char* DeviceFunction_get_device_name(DeviceFunction *dev);
int DeviceFunction_get_state_done(DeviceFunction *dev);
bool DeviceFunction_is_running(DeviceFunction *dev);
const char* DeviceFunction_get_pose_name(device_pose_t pose);

#endif /* DEVICE_FUNCTION_H_ */
