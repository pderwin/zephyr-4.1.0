/*
 * Copyright (c) 2024 Phil Erwin
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/trace.h>
#include "lr11xx_drv.h"
#include "lr11xx_gpio.h"

static const struct gpio_dt_spec gpio_reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios);
static const struct gpio_dt_spec gpio_busy  = GPIO_DT_SPEC_INST_GET(0, busy_gpios);
static const struct gpio_dt_spec gpio_dio9  = GPIO_DT_SPEC_INST_GET(0, dio9_gpios);
static const struct gpio_dt_spec gpio_nss   = GPIO_DT_SPEC_INST_GET(0, nss_gpios);

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_gpio_reset
 *
 * description: reset the chip.  The state of the BUSY pin dictates whether
 *              we are going to be in PROGRAMMING mode vs. normal operation.
 *              For programming mode, we want to drive the pin low.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_gpio_reset(struct lr11xx_data *dev_data)
{
   gpio_pin_set_dt(&gpio_reset, 1);
   k_sleep(K_MSEC(1));
   gpio_pin_set_dt(&gpio_reset, 0);
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_reset_programming_mode
 *
 * description: reset the chip.  The state of the BUSY pin dictates whether
 *              we are going to be in PROGRAMMING mode vs. normal operation.
 *              For programming mode, we want to drive the pin low.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_gpio_reset_programming_mode(struct lr11xx_data *dev_data)
{
   /*
    * Make BUSY pin an output and drive low.
    */
   gpio_pin_configure_dt(&gpio_busy, GPIO_OUTPUT_ACTIVE);
   gpio_pin_set_dt(&gpio_busy, 0);

   gpio_pin_set_dt(&gpio_reset, 1);
   k_sleep(K_MSEC(1));
   gpio_pin_set_dt(&gpio_reset, 0);

/*
 * Wait 500 mSecs and then make the BUSY pin an input again.
 */
   k_sleep(K_MSEC(500));

/*
 * Make BUSY pin an input again.
 */
   gpio_pin_configure_dt(&gpio_busy, GPIO_INPUT);

   /*
    * wait another 100 mSecs as the code did.
    */
   k_sleep(K_MSEC(100));
   }

/*-------------------------------------------------------------------------
 *
 * name:        lr1110_CS_wakeup
 *
 * description: toggle NSS pin which will bring the LR1110 out of sleep
 *              mode.  The chip needs a low pulse to simulate a chip
 *              select operation
 *
 *              NOTE: The output pin is inverted somewhere along the way,
 *                    so the value is inverted below.
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_CS_wakeup(struct lr11xx_data *dev_data)
{
   gpio_pin_configure_dt(&gpio_nss, GPIO_OUTPUT_ACTIVE);

   gpio_pin_set_dt(&gpio_nss, 1);

   /*
    * Make a 20 uSec low pulse on the chip select to make the device exit sleep
    * mode.
    */
   k_busy_wait(20);

   gpio_pin_set_dt(&gpio_nss, 0);

   /*
    * No need for extra delay here.  The device will drop BUSY when it is ready
    * to accept commands.
    */
}

bool lr11xx_is_busy(struct lr11xx_data *dev_data)
{
   uint32_t rc;

   rc = gpio_pin_get_dt(&gpio_busy);

   return rc;
}

uint32_t lr11xx_get_dio9_pin_state(struct lr11xx_data *dev_data)
{
   return gpio_pin_get_dt(&gpio_dio9) > 0 ? 1U : 0U;
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_gpio_radio_irq_enable
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_gpio_radio_irq_enable(struct lr11xx_data *dev_data)
{
   TRACE1(TAG_GPIO_DIO9_IRQ_ENABLE, __builtin_return_address(0) );

   gpio_pin_interrupt_configure_dt(&gpio_dio9, GPIO_INT_EDGE_TO_ACTIVE);
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_gpio_radio_irq_disable
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_gpio_radio_irq_disable(struct lr11xx_data *dev_data)
{
   TRACE1(TAG_GPIO_DIO9_IRQ_DISABLE, __builtin_return_address(0) );
   gpio_pin_interrupt_configure_dt(&gpio_dio9, GPIO_INT_DISABLE);
}

static const struct device *radio_device;

/*-------------------------------------------------------------------------
 *
 * name:        dio9_irq_callback
 *
 * description:
 *
 * input:       dev: GPIO device structure
 *
 * output:      none
 *
 *-------------------------------------------------------------------------*/
static void dio9_irq_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
   TRACE(TAG_DIO9_IRQ_CALLBACK);

   struct lr11xx_data *data = radio_device->data;  // captured radio device

   if (pins & BIT(gpio_dio9.pin)) {
      data->radio_irq_callback (data->radio_irq_context);
   }
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_gpio_init
 *
 * description: Initialize all the GPIOs connected to the LR1110, and
 *              interrupt handler for the IRQ (aka DIO9) pin
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
int lr11xx_gpio_init(const struct device *dev)
{
   struct lr11xx_data *dev_data = dev->data;

   /*
    * TODO: figure out how to give a GPIO interrupt handler the context
    *       for the radio device.  Save it locally for time being.
    */
   radio_device = dev;

   if (gpio_pin_configure_dt(&gpio_reset, GPIO_OUTPUT_ACTIVE) ||
       gpio_pin_configure_dt(&gpio_busy, GPIO_INPUT) ||
       gpio_pin_configure_dt(&gpio_dio9, GPIO_INPUT)) {
      printk("%s: GPIO configuration failed.", __func__);
      return -EIO;
   }

   gpio_init_callback(&dev_data->dio9_irq_callback, dio9_irq_callback, BIT(gpio_dio9.pin));

   if (gpio_add_callback_dt(&gpio_dio9, &dev_data->dio9_irq_callback) < 0) {
      printk("%s: Could not set GPIO callback for DIO1 interrupt.", __func__);
      return -EIO;
   }

   return 0;
}
