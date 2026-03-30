#include "cyhal.h"
#include "cybsp.h"

// Motor 1
#define STEP_PIN    P9_0
#define DIR_PIN     P9_1
#define EN_PIN      P9_2

// Motor 2
#define STEP2_PIN   P10_0
#define DIR2_PIN    P10_1
#define EN2_PIN     P10_2

// Motor 3
#define STEP3_PIN   P10_3
#define DIR3_PIN    P10_4
#define EN3_PIN     P10_5

#define BUTTON_PIN  P0_4
#define SERVO_PIN   P10_6

#define TARGET_POS1 250
#define TARGET_POS2 800
#define TARGET_POS3 600

cyhal_pwm_t pwm;

int M1_Pos = 0;
int M2_Pos = 0;
int M3_Pos = 0;

bool vorige_button = 1;

// -------- LOW LEVEL STEP --------
void step_pin(uint8_t pin)
{
    cyhal_gpio_write(pin, 1);
    cyhal_system_delay_ms(2);
    cyhal_gpio_write(pin, 0);
    cyhal_system_delay_ms(2);
}

// -------- MOTOR 1 --------
void Motor1_forward()
{
    cyhal_gpio_write(DIR_PIN, 0);
    step_pin(STEP_PIN);
    M1_Pos++;
}

void Motor1_backward()
{
    if(M1_Pos > 0)
    {
        cyhal_gpio_write(DIR_PIN, 1);
        step_pin(STEP_PIN);
        M1_Pos--;
    }
}

// -------- MOTOR 2 (🔥 OMGEKEERD) --------
void Motor2_forward()
{
    cyhal_gpio_write(DIR2_PIN, 1); // 🔥 omgedraaid
    step_pin(STEP2_PIN);
    M2_Pos++;
}

void Motor2_backward()
{
    if(M2_Pos > 0)
    {
        cyhal_gpio_write(DIR2_PIN, 0); // 🔥 omgedraaid
        step_pin(STEP2_PIN);
        M2_Pos--;
    }
}

// -------- MOTOR 3 --------
void Motor3_forward()
{
    cyhal_gpio_write(DIR3_PIN, 0);
    step_pin(STEP3_PIN);
    M3_Pos++;
}

void Motor3_backward()
{
    if(M3_Pos > 0)
    {
        cyhal_gpio_write(DIR3_PIN, 1);
        step_pin(STEP3_PIN);
        M3_Pos--;
    }
}

// -------- SERVO --------
void Servo()
{
    cyhal_pwm_set_duty_cycle(&pwm, 5, 50);
    cyhal_system_delay_ms(500);

    cyhal_pwm_set_duty_cycle(&pwm, 7.5, 50);
    cyhal_system_delay_ms(500);

    cyhal_pwm_set_duty_cycle(&pwm, 5, 50);
    cyhal_system_delay_ms(500);
}

int main(void)
{
    cybsp_init();
    __enable_irq();

    // Init
    cyhal_gpio_init(STEP_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_init(STEP2_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR2_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN2_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_init(STEP3_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR3_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN3_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_init(BUTTON_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);

    cyhal_pwm_init(&pwm, SERVO_PIN, NULL);
    cyhal_pwm_start(&pwm);

    cyhal_gpio_write(EN_PIN, 1);
    cyhal_gpio_write(EN2_PIN, 1);
    cyhal_gpio_write(EN3_PIN, 1);

    while(1)
    {
        bool button = cyhal_gpio_read(BUTTON_PIN);

        if(button == 0 && vorige_button == 1)
        {
            // -------- NAAR POSITIE --------
            while(M1_Pos < TARGET_POS1)
                Motor1_forward();

            while(M2_Pos < TARGET_POS2)
                Motor2_forward();

            while(M3_Pos < TARGET_POS3)
                Motor3_forward();

            // -------- CUSTOM SEQUENCE --------
            int steps1[] = {80, 200, 50};
            int dir1[]   = {0, 1, 0};

            int steps2[] = {100, 50, 150};
            int dir2[]   = {1, 0, 1};

            int count = 3;

            for(int s = 0; s < count; s++)
            {
                // Motor 1
                for(int i = 0; i < steps1[s]; i++)
                {
                    if(dir1[s] == 0)
                        Motor1_forward();
                    else
                        Motor1_backward();
                }

                // Motor 2
                for(int i = 0; i < steps2[s]; i++)
                {
                    if(dir2[s] == 0)
                        Motor2_forward();
                    else
                        Motor2_backward();
                }

                Servo();
            }

            // -------- TERUG --------
            while(M1_Pos > 0 || M2_Pos > 0 || M3_Pos > 0)
            {
                Motor1_backward();
                Motor2_backward();
                Motor3_backward();
            }

            // reset zekerheid
            M1_Pos = 0;
            M2_Pos = 0;
            M3_Pos = 0;
        }

        vorige_button = button;
    }
}