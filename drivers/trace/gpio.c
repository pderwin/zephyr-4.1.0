#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include "trace_internal.h"

#ifdef USE_GPIO

static unsigned int
   my_gpio_list [] = {
      GPIO_TRG,  // TRIGGER
      GPIO_DTA,       // D0 - D12
      GPIO_CLK,      // CLK - E12
      GPIO_PKT,      // debug packet
#ifdef GPIO_IRQ
      GPIO_IRQ,      // IRQ
#endif
};

#define NUMBER_GPIOS (sizeof(my_gpio_list) / sizeof(unsigned int))

static unsigned int
   func_3_list [] = {
      GPIO_LBT_CLK,
      GPIO_LBT_DTA,
      GPIO_LBT_PKT,
      GPIO_LBT_TRG,
};



typedef struct {
   uint32_t level;
   uint32_t dir;
   uint32_t set;
   uint32_t clr;
   uint32_t rise_enb;
   uint32_t fall_enb;
   uint32_t irq_status;
   uint32_t reserved0 [11];
   uint32_t irq_level;
   uint32_t irq_pol;
   uint32_t irq_enb;
   uint32_t reserved1 [11];
} GPIO_REGS_t;

/*-------------------------------------------------------------------------
 *
 * name:        gpio_out
 *
 * description: Emit a given value on a GPIO pin.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void
   gpio_out(unsigned int gpio, unsigned int value)
{
   unsigned int
      bank,
      mask;
   volatile GPIO_REGS_t
      *regs = (GPIO_REGS_t *) DT_INST_0_LEXMARK_JAKE_GPIO_BASE_ADDRESS;

   bank = gpio / 32;
   gpio = gpio % 32;

   mask = (1 << gpio);

   if (value & 1) {
      regs[bank].set = mask;
   }
   else {
      regs[bank].clr = mask;
   }
}

/*-------------------------------------------------------------------------
 *
 * name:        gpio_output
 *
 * description: Make a GPIO pin an ouput
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void
   gpio_output(unsigned int gpio)
{
   unsigned int
      bank,
      mask;
   volatile GPIO_REGS_t
      *regs = (GPIO_REGS_t *) DT_INST_0_LEXMARK_JAKE_GPIO_BASE_ADDRESS;

   bank = gpio / 32;
   gpio = gpio % 32;

   mask = (1 << gpio);
   regs[bank].dir |= mask;
}

/*-------------------------------------------------------------------------
 *
 * name:        gpio_init
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void
   gpio_init (void)
{
   unsigned int
      gpio,
      i;
   struct device
      *pinmux_device;

   pinmux_device = device_get_binding(DT_INST_0_LEXMARK_JAKE_PINMUX_LABEL);

   for (i=0; i< NUMBER_GPIOS; i++) {

      gpio = my_gpio_list[i];

      /*
       * Set the pin mux for the pin
       */
      pinmux_pin_set(pinmux_device, gpio, 0);

      /*
       * Set initial value.
       */
      gpio_out(gpio, 0);

      /*
       * Make into an output
       */
      gpio_output(gpio);
   }

#if 1
   for (i=0; i< ARRAY_SIZE(func_3_list); i++) {

      gpio = func_3_list[i];

      /*
       * Set the pin mux for the pin
       */
      pinmux_pin_set(pinmux_device, gpio, 3);
   }
#endif

}

static void
   _trace_1_bit (unsigned int value)
{
   /*
    * Setup data lines
    */
   gpio_out(GPIO_DTA, value & 1);

   /*
    * strobe the clock line.
    */
   gpio_high(GPIO_CLK);
   gpio_low(GPIO_CLK);
}

void
   gpio_put(unsigned int value)
{
   unsigned int
      i;

//   printk("GPIO_PUT: %x\n", value);
   for (i=0; i<32; i++) {
      _trace_1_bit(value >> (31 - i));
   }

   /*
    * Leave data line low between words.
    */
   gpio_out(GPIO_DTA, value & 1);

}

void
   gpio_high (unsigned int gpio)
{
   gpio_out(gpio, 1);
}

void
   gpio_low (unsigned int gpio)
{
   gpio_out(gpio, 0);
}

void gpio_trigger_low (void)
{
   gpio_high(GPIO_TRG);
}
void gpio_trigger_high (void)
{
   gpio_high(GPIO_TRG);
}

#endif
