#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "mqtt_task.h"

#include "FreeRTOS.h"
#include "task.h"

/* Include serial flash library and QSPI memory configurations only for the
 * kits that require the Wi-Fi firmware to be loaded in external QSPI NOR flash.
 */
#if defined(CY_DEVICE_PSOC6A512K)
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#endif

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, sets up 
 *  the MQTT client task, and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main()
{
    cy_rslt_t result;
    
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    (void) result;
    __enable_irq();
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

#if defined(CY_DEVICE_PSOC6A512K)
    /* Initialize the QSPI serial NOR flash with clock frequency of 50 MHz. */
    const uint32_t bus_frequency = 50000000lu;
    cy_serial_flash_qspi_init(smifMemConfigs[0], CYBSP_QSPI_D0, CYBSP_QSPI_D1,
                                  CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC, NC, NC,
                                  CYBSP_QSPI_SCK, CYBSP_QSPI_SS, bus_frequency);

    /* Enable the XIP mode to get the Wi-Fi firmware from the external flash. */
    cy_serial_flash_qspi_enable_xip(true);
#endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
#if defined(COMPONENT_CM0P)
    printf("CE229889 - MQTT Client running on CM0+\n");
#endif

#if defined(COMPONENT_CM4)
    printf("CE229889 - MQTT Client running on CM4\n");
#endif

#if defined(COMPONENT_CM7)
    printf("CE229889 - MQTT Client running on CM7\n");
#endif
    printf("===============================================================\n\n");

    xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE,
                NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    CY_ASSERT(0);
}

/* [] END OF FILE */
