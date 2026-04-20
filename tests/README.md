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

int main(void)
{
    cybsp_init();
    __enable_irq();

    cyhal_gpio_init(STEP_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(DIR_PIN,  CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    cyhal_gpio_init(EN_PIN,   CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    cyhal_gpio_write(EN_PIN, 1);


    cyhal_gpio_init(BUTTON_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);

    bool direction = false;
    bool last_state = 1;


    cyhal_gpio_write(STEP_PIN, 1);
    cyhal_gpio_write(DIR_PIN, 0);

    while(1)
    {
        bool now = cyhal_gpio_read(BUTTON_PIN);


        if(last_state == 1 && now == 0)
        {

            cyhal_system_delay_ms(50);
            now = cyhal_gpio_read(BUTTON_PIN);
            if(now == 0)
            {
                direction = !direction;

                if(!direction)
                {
                    //links
                    cyhal_gpio_write(STEP_PIN, 1);
                    cyhal_gpio_write(DIR_PIN, 0);
                }
                else
                {
                	//rechts
                    cyhal_gpio_write(STEP_PIN, 0);
                    cyhal_gpio_write(DIR_PIN, 1);
                }


                while(cyhal_gpio_read(BUTTON_PIN) == 0)
                {
                    cyhal_system_delay_ms(10);
                }
            }
        }

        last_state = now;
        cyhal_system_delay_ms(5);
    }
}


```
## Doel van dit project
---
De motor wordt geactiveerd met de btn op de Psoc6

## Technologie

- PSoC 6
- Mod
