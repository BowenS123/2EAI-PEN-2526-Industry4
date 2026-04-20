#include "cyhal.h"
#include "cybsp.h"

#define SERVO_PIN P10_6

cyhal_pwm_t pwm;

int main(void)
{
    cybsp_init();
    __enable_irq();

    cyhal_pwm_init(&pwm, SERVO_PIN, NULL);

    cyhal_pwm_start(&pwm);

    while(1)
    {
        // 0 graden
        cyhal_pwm_set_duty_cycle(&pwm, 5, 50);
        cyhal_system_delay_ms(2000);

        // 90 graden
        cyhal_pwm_set_duty_cycle(&pwm, 7.5, 50);
        cyhal_system_delay_ms(2000);
    }
}