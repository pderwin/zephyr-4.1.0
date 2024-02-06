#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include "trace_internal.h"

#if (USE_HWTRACE)

#define CONFIG      ( 0x0)
#define    CONFIG_CLOCK_WAIT_STATES_SHIFT ( 0 )

#define CONTROL     ( 0x4)
#define    CONTROL_ENABLE (1 << 0)

#define STATUS      ( 0x8)
#define    STATUS_BUSY (1 << 0)

#define XMIT_FIFO   ( 0xc)
#define SCRATCH_PAD (0x10)

#define REGS (0x60023000)

extern uint32_t zzz;

/*-------------------------------------------------------------------------
 *
 * name:        hwtrace_init
 *
 * description: initialize GPIOs for use as trace outputs
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void hwtrace_init (void)
{
   unsigned int
      gpio,
      i;
   static uint32_t
      my_gpio_list[] = { GPIO_LBT_PKT, GPIO_LBT_TRG, GPIO_LBT_DTA, GPIO_LBT_CLK };
   struct device
      *pinmux_device;

   pinmux_device = device_get_binding(DT_INST_0_LEXMARK_JAKE_PINMUX_LABEL);

   for (i=0; i< ARRAY_SIZE(my_gpio_list); i++) {

      gpio = my_gpio_list[i];

      /*
       * Set the pin mux for the pin
       */
      pinmux_pin_set(pinmux_device, gpio, 3);
   }

   /*
    * Configure some extra clock time.
    */
   sys_write32( (8 << CONFIG_CLOCK_WAIT_STATES_SHIFT) , REGS + CONFIG);

   /*
    * test scratch pad
    */
   sys_write32(0x12345678, REGS + SCRATCH_PAD);

   /*
    * Queue up a few words in the transmit FIFO
    */
   hwtrace_packet_start();
   hwtrace_put(0x8d3478af);
   hwtrace_put(0x199fe2a);
   hwtrace_put(0x70024000);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_put(0x55555555);
   hwtrace_packet_end();

   k_busy_wait(100);

   hwtrace_packet_start();
   hwtrace_put(0x8d3478af);
   hwtrace_put(0x3bfe26);
   hwtrace_put(0x80);
   hwtrace_put(0x20);

   hwtrace_packet_end();

   k_busy_wait(100);

   hwtrace_packet_start();
   hwtrace_put(0x8d3478af);
   hwtrace_put(0x49fe25);
   hwtrace_put(0x90001230);
   hwtrace_put(0x40);
   hwtrace_put(0x20);

   hwtrace_packet_end();

   k_busy_wait(100);

   hwtrace_packet_start();
   hwtrace_put(0x8d3478af);
   hwtrace_put(0x46fe1f);
   hwtrace_put(0x60020000);

   hwtrace_packet_end();
}

/*-------------------------------------------------------------------------
 *
 * name:        hwtrace_packet_end
 *
 * description: tell the block to go.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void hwtrace_packet_end (void)
{
   if (zzz) {
      printk("%s %d \n", __func__, __LINE__);
   }

   sys_write32(CONTROL_ENABLE, REGS + CONTROL);
}

/*-------------------------------------------------------------------------
 *
 * name:        hwtrace_packet_start
 *
 * description: Wait for block to be idle.  It should only take uSecs to
 *              strobe out a prior packet.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void hwtrace_packet_start (void)
{
   if (zzz) {
      printk("%s %d \n", __func__, __LINE__);
   }
   while(sys_read32(REGS + STATUS) & STATUS_BUSY) {
      k_busy_wait(5);
   }
}

/*-------------------------------------------------------------------------
 *
 * name:        hwtrace_put
 *
 * description: put a 32-bit value into the FIFO
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void hwtrace_put (uint32_t val)
{
   if (zzz) {
      printk("%s %d val: %x \n", __func__, __LINE__, val);
   }
   sys_write32(val, REGS + XMIT_FIFO);
}

#endif
