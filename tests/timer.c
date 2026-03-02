#include "cyhal.h"
#include "cybsp.h"

#define STEP_PIN    P9_0
#define DIR_PIN     P9_1
#define EN_PIN      P9_2

int main(void)
{
    cybsp_init();
    __enable_irq();

    cyhal_gpio_init(STEP_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_write(EN_PIN, 1);

    while(1)
    {
        //rechts
        cyhal_gpio_write(STEP_PIN, 1);
        cyhal_gpio_write(DIR_PIN, 0);
        cyhal_system_delay_ms(500);

        //links
        cyhal_gpio_write(STEP_PIN, 0);
        cyhal_gpio_write(DIR_PIN, 1);
        cyhal_system_delay_ms(500);
    }
}
