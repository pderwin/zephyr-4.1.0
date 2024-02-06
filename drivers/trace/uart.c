#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "trace_internal.h"

#ifdef USE_UART

static const struct device *uart_device;

/*-------------------------------------------------------------------------
 *
 * name:        uart_init
 *
 * description: Send a 32-bit word out the uart
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void
    uart_init (void)
{

   uart_device = DEVICE_DT_GET(DT_NODELABEL(uart1));

   if ( !uart_device ) {
      printk("%s: Cannot find uart!\n", __func__);
      return;
   }
}

void
   uart_put(unsigned int value)
{
   unsigned int
      i;

   if (uart_device) {
      for (i=0; i<4; i++) {

	 /*
	  * xmit high byte of the word first.
	  */
	 uart_poll_out(uart_device, value >> 24);

	 value = (value << 8);
      }
   }
}

#endif
