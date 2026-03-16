#include "cyhal.h"
#include "cybsp.h"

// Motor 1
#define STEP_PIN    P9_0
#define DIR_PIN     P9_1
#define EN_PIN      P9_2

// Motor 2
#define STEP2_PIN   P10_3
#define DIR2_PIN    P10_4
#define EN2_PIN     P10_5

#define BUTTON_PIN  P0_4

#define TARGET_POS 200

int positie = 0;
bool vorige_button = 1;

void step()
{
    // beide motoren tegelijk laten stappen
    cyhal_gpio_write(STEP_PIN, 1);
    cyhal_gpio_write(STEP2_PIN, 1);

    cyhal_system_delay_ms(2);

    cyhal_gpio_write(STEP_PIN, 0);
    cyhal_gpio_write(STEP2_PIN, 0);

    cyhal_system_delay_ms(2);
}

int main(void)
{
    cybsp_init();
    __enable_irq();

    // Motor 1
    cyhal_gpio_init(STEP_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    // Motor 2
    cyhal_gpio_init(STEP2_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR2_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN2_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    // Button
    cyhal_gpio_init(BUTTON_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);

    // Drivers aan
    cyhal_gpio_write(EN_PIN, 1);
    cyhal_gpio_write(EN2_PIN, 1);

    while(1)
    {
        bool button = cyhal_gpio_read(BUTTON_PIN);

        if(button == 0 && vorige_button == 1)
        {
            // beide motoren vooruit
            cyhal_gpio_write(DIR_PIN, 0);
            cyhal_gpio_write(DIR2_PIN, 0);

            while(positie < TARGET_POS)
            {
                step();
                positie++;
            }

            // 5 seconden wachten
            cyhal_system_delay_ms(5000);

            // beide motoren terug
            cyhal_gpio_write(DIR_PIN, 1);
            cyhal_gpio_write(DIR2_PIN, 1);

            while(positie > 0)
            {
                step();
                positie--;
            }
        }

        vorige_button = button;
    }
}