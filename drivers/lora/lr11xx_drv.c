/*
 * Copyright (c) 2024 Phil Erwin
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/trace.h>
#include "lr11xx_drv.h"
#include "lr11xx_gpio.h"
#include "lr11xx_hal.h"

#include "lr11xx_system.h"

#define BOARD_TCXO_WAKEUP_TIME 21

static const struct lr11xx_config dev_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
};

static struct lr11xx_data dev_data;

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_lora_init
 *
 * description: Initialize the LORA device.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static int lr11xx_lora_init(const struct device *device)
{
   int ret;

   /*
    * Get all the GPIOs configured for the radio.
    */
   ret = lr11xx_gpio_init(device);

   if (ret) {
      printk("%s: gpio initialization failed", __func__);
      return ret;
   }

   /*
    * Enable radio interrupt
    */
   lr11xx_gpio_radio_irq_enable(&dev_data);

   return 0;
}

static const struct lora_driver_api lr11xx_lora_api = {
//        .config = sx12xx_lora_config,
//        .send = sx12xx_lora_send,
//        .recv = sx12xx_lora_recv,
//        .test_cw = sx12xx_lora_test_cw,
};

/*-------------------------------------------------------------------------
 *
 * name:
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_drv_lna_disable (void)
{
   TRACE(TAG_LR11XX_DRV_LNA_DISABLE);

   printf("%s %d \n", __func__,__LINE__);
   lr11xx_gpio_lna_ctl_disable();
}

/*-------------------------------------------------------------------------
 *
 * name:
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_drv_lna_enable (void)
{
   TRACE(TAG_LR11XX_DRV_LNA_ENABLE);

   printf("%s %d \n", __func__,__LINE__);
   lr11xx_gpio_lna_ctl_enable();
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_config_radio_irq
 *
 * description: called from  smtc_modem_hal_irq_config_radio_irq() to
 *              configure the callback routine for when a radion IRQ occurs.
 *
 * input:
 *
 * output:      none
 *
 *-------------------------------------------------------------------------*/
void lr11xx_drv_radio_irq_config( void (*callback)(void *context), void *context)
{
   struct lr11xx_data *data = &dev_data;

   data->radio_irq_callback = callback;
   data->radio_irq_context  = context;
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_drv_radio_irq_disable
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_drv_radio_irq_disable (void)
{
   lr11xx_gpio_radio_irq_disable(&dev_data);
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_drv_radio_irq_enable
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lr11xx_drv_radio_irq_enable (void)
{
   lr11xx_gpio_radio_irq_enable(&dev_data);
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_drv_reset_programming_mode
 *
 * description: Reset the LR1110 and get it into programming mode.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
lr11xx_drv_status_e lr11xx_drv_reset_programming_mode( const void* context )
{
   printk("%s: Resetting radio", __func__);

   lr11xx_gpio_reset_programming_mode(&dev_data);

   return LR11XX_DRV_STATUS_OK;

}

#define	DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE,	okay),
		 "No default LoRa radio	specified in DT");

#define	DEFAULT_RADIO DEVICE_DT_NAME(DEFAULT_RADIO_NODE)

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_get_radio_context
 *
 * description: return ptr. to device struct for our radio.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
const void *lr11xx_drv_radio_context_get ( void )
{
   return device_get_binding(DEFAULT_RADIO);
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_drv_read_busy_pin
 *
 * description: Read the BUSY pin from the LR1110
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
uint32_t lr11xx_drv_read_busy_pin (const void *context)
{
   return lr11xx_gpio_busy(&dev_data);
}

#define WAIT_USECS (10)

uint32_t lr11xx_board_get_tcxo_wakeup_time( const void* context )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

DEVICE_DT_INST_DEFINE(0, &lr11xx_lora_init, NULL, &dev_data,
		      &dev_config, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &lr11xx_lora_api);
