/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "lr11xx_drv.h"

#include <zephyr/drivers/trace.h>

static const struct gpio_dt_spec lr11xx_gpio_reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios);
static const struct gpio_dt_spec lr11xx_gpio_busy  = GPIO_DT_SPEC_INST_GET(0, busy_gpios);
static const struct gpio_dt_spec lr11xx_gpio_dio1  = GPIO_DT_SPEC_INST_GET(0, dio1_gpios);
static const struct gpio_dt_spec lr11xx_gpio_nss   = GPIO_DT_SPEC_INST_GET(0, nss_gpios);

int __lr11xx_configure_pin(const struct gpio_dt_spec *gpio, gpio_flags_t flags)
{
	int err;

	if (!device_is_ready(gpio->port)) {
		printk("GPIO device not ready %s", gpio->port->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(gpio, flags);
	if (err) {
		printk("Cannot configure gpio %s %d: %d", gpio->port->name,
			gpio->pin, err);
		return err;
	}

	return 0;
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_reset
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
void lr11xx_reset(struct lr11xx_data *dev_data)
{
   gpio_pin_set_dt(&lr11xx_gpio_reset, 1);
   k_sleep(K_MSEC(1));
   gpio_pin_set_dt(&lr11xx_gpio_reset, 0);
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
void lr11xx_reset_programming_mode(struct lr11xx_data *dev_data)
{
   /*
    * Make BUSY pin an output and drive low.
    */
   gpio_pin_configure_dt(&lr11xx_gpio_busy, GPIO_OUTPUT_ACTIVE);
   gpio_pin_set_dt(&lr11xx_gpio_busy, 0);

   gpio_pin_set_dt(&lr11xx_gpio_reset, 1);
   k_sleep(K_MSEC(1));
   gpio_pin_set_dt(&lr11xx_gpio_reset, 0);

/*
 * Wait 500 mSecs and then make the BUSY pin an input again.
 */
   k_sleep(K_MSEC(500));

/*
 * Make BUSY pin an input again.
 */
   gpio_pin_configure_dt(&lr11xx_gpio_busy, GPIO_INPUT);

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
   gpio_pin_configure_dt(&lr11xx_gpio_nss, GPIO_OUTPUT_ACTIVE);

   gpio_pin_set_dt(&lr11xx_gpio_nss, 1);

   /*
    * Make a 20 uSec low pulse on the chip select to make the device exit sleep
    * mode.
    */
   k_busy_wait(20);

   gpio_pin_set_dt(&lr11xx_gpio_nss, 0);

   /*
    * No need for extra delay here.  The device will drop BUSY when it is ready
    * to accept commands.
    */
}

bool lr11xx_is_busy(struct lr11xx_data *dev_data)
{
   uint32_t rc;

   rc = gpio_pin_get_dt(&lr11xx_gpio_busy);

   return rc;
}

uint32_t lr11xx_get_dio1_pin_state(struct lr11xx_data *dev_data)
{
	return gpio_pin_get_dt(&lr11xx_gpio_dio1) > 0 ? 1U : 0U;
}

void lr11xx_dio1_irq_enable(struct lr11xx_data *dev_data)
{
	gpio_pin_interrupt_configure_dt(&lr11xx_gpio_dio1, GPIO_INT_EDGE_TO_ACTIVE);
}

void lr11xx_dio1_irq_disable(struct lr11xx_data *dev_data)
{
     gpio_pin_interrupt_configure_dt(&lr11xx_gpio_dio1, GPIO_INT_DISABLE);
}

static const struct device *radio_device;

static void lr11xx_dio1_irq_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
   TRACE(TAG_DIO1_IRQ_CALLBACK);

//	struct lr1110_data *dev_data = dev->data;   // dev is GPIO device
   struct lr11xx_data *data = radio_device->data;  // captured radio device

//	printk("%s %d !!!!!!!!!!!!!!!!! dev: %p (%s) &dev_data: %p &work: %p \n", __func__, __LINE__,
//	       dev,
//	       dev->name,
//	       &dev_data,
//	       &dev_data->dio1_irq_work);

   if (pins & BIT(lr11xx_gpio_dio1.pin)) {
      data->radio_irq_callback (data->radio_irq_context);
   }
}

int lr11xx_variant_init(const struct device *dev)
{
   struct lr11xx_data *dev_data = dev->data;

   radio_device = dev;

   if (gpio_pin_configure_dt(&lr11xx_gpio_reset, GPIO_OUTPUT_ACTIVE) ||
       gpio_pin_configure_dt(&lr11xx_gpio_busy, GPIO_INPUT) ||
       gpio_pin_configure_dt(&lr11xx_gpio_dio1, GPIO_INPUT)) {
      printk("%s: GPIO configuration failed.", __func__);
      return -EIO;
   }

   gpio_init_callback(&dev_data->dio1_irq_callback,
		      lr11xx_dio1_irq_callback, BIT(lr11xx_gpio_dio1.pin));

   if (gpio_add_callback(lr11xx_gpio_dio1.port,
			 &dev_data->dio1_irq_callback) < 0) {
      printk("%s: Could not set GPIO callback for DIO1 interrupt.", __func__);
      return -EIO;
   }

   return 0;
}
