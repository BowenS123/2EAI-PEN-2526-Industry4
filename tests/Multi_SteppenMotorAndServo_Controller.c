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

#define TARGET_POS1 800
#define TARGET_POS2 800
#define TARGET_POS3 600

cyhal_pwm_t pwm;

int M1_Pos = 0;
int M2_Pos = 0;
int M3_Pos = 0;

bool vorige_button = 1;

// STEPPERS
void Motor1()
{
    cyhal_gpio_write(STEP_PIN, 1);
    cyhal_system_delay_ms(2);
    cyhal_gpio_write(STEP_PIN, 0);
    cyhal_system_delay_ms(2);
}

void Motor2()
{
    cyhal_gpio_write(STEP2_PIN, 1);
    cyhal_system_delay_ms(2);
    cyhal_gpio_write(STEP2_PIN, 0);
    cyhal_system_delay_ms(2);
}

void Motor3()
{
    cyhal_gpio_write(STEP3_PIN, 1);
    cyhal_system_delay_ms(2);
    cyhal_gpio_write(STEP3_PIN, 0);
    cyhal_system_delay_ms(2);
}

// SERVO
void Servo()
{
    cyhal_pwm_set_duty_cycle(&pwm, 5, 50);   // 0°
    cyhal_system_delay_ms(1000);

    cyhal_pwm_set_duty_cycle(&pwm, 7.5, 50); // 90°
    cyhal_system_delay_ms(1000);

    cyhal_pwm_set_duty_cycle(&pwm, 5, 50);   // 🔥 0°
    cyhal_system_delay_ms(1000);
}

int main(void)
{
    cybsp_init();
    __enable_irq();

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
            // vooruit
            cyhal_gpio_write(DIR_PIN, 0);
            cyhal_gpio_write(DIR2_PIN, 1);
            cyhal_gpio_write(DIR3_PIN, 0);

            // Motor 1
            while(M1_Pos < TARGET_POS1)
            {
                Motor1();
                M1_Pos++;
            }

            // Motor 2
            while(M2_Pos < TARGET_POS2)
            {
                Motor2();
                M2_Pos++;
            }

            // Motor 3
            while(M3_Pos < TARGET_POS3)
            {
                Motor3();
                M3_Pos++;
            }

            
            cyhal_system_delay_ms(1000);
            Servo();

            // terug
            cyhal_gpio_write(DIR_PIN, 1);
            cyhal_gpio_write(DIR2_PIN, 0);
            cyhal_gpio_write(DIR3_PIN, 1);

            while(M1_Pos > 0 || M2_Pos > 0 || M3_Pos > 0)
            {
                if(M1_Pos > 0)
                {
                    Motor1();
                    M1_Pos--;
                }

                if(M2_Pos > 0)
                {
                    Motor2();
                    M2_Pos--;
                }

                if(M3_Pos > 0)
                {
                    Motor3();
                    M3_Pos--;
                }
            }
        }

        vorige_button = button;
    }
}
