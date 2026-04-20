#include "cyhal.h"
#include "cybsp.h"

#define STEP_PIN    P9_0
#define DIR_PIN     P9_1
#define EN_PIN      P9_2
#define BUTTON_PIN  P0_4
#define SERVO_PIN   P10_0

#define TARGET_POS 200

cyhal_pwm_t pwm;

int positie = 0;
bool vorige_button = 1;

void step()
{
    cyhal_gpio_write(STEP_PIN, 1);
    cyhal_system_delay_ms(2);
    cyhal_gpio_write(STEP_PIN, 0);
    cyhal_system_delay_ms(2);
}

void Servo()
{
    cyhal_pwm_set_duty_cycle(&pwm, 5, 50);   // 0°
    cyhal_system_delay_ms(2000);
    
    cyhal_pwm_set_duty_cycle(&pwm, 7.5, 50); // 90°
  	cyhal_system_delay_ms(1000);
}

int main(void)
{
    cybsp_init();
    __enable_irq();

    // Stepper pins
    cyhal_gpio_init(STEP_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    // Button
    cyhal_gpio_init(BUTTON_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);

    // Servo PWM
    cyhal_pwm_init(&pwm, SERVO_PIN, NULL);
    cyhal_pwm_start(&pwm);

    cyhal_gpio_write(EN_PIN, 1);

    while(1)
    {
        bool button = cyhal_gpio_read(BUTTON_PIN);

        if(button == 0 && vorige_button == 1)
        {
            // Stepper naar positie
            cyhal_gpio_write(DIR_PIN, 0);

            while(positie < TARGET_POS)
            {
                step();
                positie++;
            }

            // 5 seconden servo bewegen
            cyhal_system_delay_ms(1000);
            
			Servo();
			
            // Stepper terug
            cyhal_gpio_write(DIR_PIN, 1);

            while(positie > 0)
            {
                step();
                positie--;
            }
        }

        vorige_button = button;
    }
}