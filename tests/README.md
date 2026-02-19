# First Motor Test

---

## Projectbeschrijving

**First Motor Test** is een project dat focust op het correct testen van een stappenmotor die wordt aangestuurd via een PSoC 6 microcontroller.

Het doel is om duidelijke, onderhoudbare en betrouwbare unit tests te schrijven volgens goede softwareprincipes.

---

## Testrichtlijnen
---

### Test op motor

De test wordt gedaan met de motor te besturen met de Psoc6 btn. Als de btn wordt in geduwd zouw de motor normaal gezien in gang moeten gaan.

## Test code (C)

```C
#include "cyhal.h"
#include "cybsp.h"
#include <stdbool.h>

#define STEP_PIN    P9_0
#define DIR_PIN     P9_1
#define EN_PIN      P9_2
#define BUTTON_PIN  P0_4

volatile bool motor_run = false;

// Interrupt handler voor button
void button_isr(void *callback_arg, cyhal_gpio_event_t event)
{
    motor_run = !motor_run;   // Toggle motor aan/uit
}

void single_step(void)
{
    cyhal_gpio_write(STEP_PIN, 0);
    cyhal_system_delay_us(1000);
    cyhal_gpio_write(STEP_PIN, 1);
    cyhal_system_delay_us(1000);
}

int main(void)
{
    cybsp_init();
    __enable_irq();

    // Motor pins
    cyhal_gpio_init(STEP_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    cyhal_gpio_init(DIR_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    cyhal_gpio_init(EN_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    // Enable driver (LOW actief)
    cyhal_gpio_write(EN_PIN, 0);

    // Richting vastzetten (omhoog)
    cyhal_gpio_write(DIR_PIN, 0);

    // Button instellen
    cyhal_gpio_init(BUTTON_PIN,
                    CYHAL_GPIO_DIR_INPUT,
                    CYHAL_GPIO_DRIVE_PULLUP,
                    1);

    cyhal_gpio_enable_event(BUTTON_PIN,
                            CYHAL_GPIO_IRQ_FALL,
                            3,
                            true);

    cyhal_gpio_register_callback(BUTTON_PIN, button_isr);

    while(1)
    {
        if(motor_run)
        {
            single_step();
        }
    }
}

```
## Doel van dit project
---
De motor wordt geactiveerd met de btn op de Psoc6

## Technologie

- PSoC 6
- Mod
