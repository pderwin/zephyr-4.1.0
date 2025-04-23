/*
 * Copyright (c) 2024 Phil Erwin
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "lr11xx_drv.h"
#include "lr11xx_gpio.h"
#include "lr11xx_hal.h"

#include "lr11xx_system.h"

#define BOARD_TCXO_WAKEUP_TIME 21

static const struct lr11xx_config dev_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
};

static struct lr11xx_data dev_data;

static op_mode_e op_mode_get( const void* context );
static void      op_mode_set( const void* context, op_mode_e op_mode );

static lr11xx_hal_status_t wait_on_busy( const void* context );


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

static int lr11xx_spi_transceive(const void* context, const uint8_t *req_tx, const uint8_t *req_rx,
				 const size_t req_len, const void *data_tx, void *data_rx,
				 const size_t data_len)
{
   int ret;

   const struct spi_buf tx_buf[] = {
      {
	 .buf = (uint8_t *)req_tx,
	 .len = req_len,
      },
      {
	 .buf = (uint8_t *)data_tx,
	 .len = data_tx ? data_len : 0
      }
   };

   const struct spi_buf rx_buf[] = {
      {
	 .buf = (uint8_t *)req_rx,
	 .len = req_rx ? req_len : 0,
      },
      {
	 .buf = data_rx,
	 .len = data_len
      }
   };

   const struct spi_buf_set tx = {
      .buffers = tx_buf,
      .count = ARRAY_SIZE(tx_buf),
   };

   const struct spi_buf_set rx = {
      .buffers = rx_buf,
      .count = ARRAY_SIZE(rx_buf)
   };

   /* Wake the device if necessary */
   wait_on_busy( context );

   if (!req_rx && !data_rx) {
      ret = spi_write_dt(&dev_config.bus, &tx);
   } else {
      ret = spi_transceive_dt(&dev_config.bus, &tx, &rx);
   }

   if (ret < 0) {
      printk("%s: SPI transaction failed: %i", __func__, ret);
   }

   return ret;
}


/*-------------------------------------------------------------------------
 *
 * name:         lr11xx_hal_abort_blocking_cmd
 *
 * description:  Send an empty command to the LR11xx just to get it out of
 *               some command that takea a long time.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
lr11xx_hal_status_t lr11xx_hal_abort_blocking_cmd( const void* context )
{
#if 0
    const lr11xx_hal_context_t* lr11xx_context = ( const lr11xx_hal_context_t* ) context;
    uint8_t                     command[4]     = { 0 };

    // Put NSS low to start spi transaction
    hal_gpio_set_value( lr11xx_context->nss, 0 );
    for( uint16_t i = 0; i < sizeof( command ); i++ )
    {
	hal_spi_in_out( lr11xx_context->spi_id, command[i] );
    }

    // Put NSS high as the spi transaction is finished
    hal_gpio_set_value( lr11xx_context->nss, 1 );

    lr11xx_hal_wait_on_busy( lr11xx_context->busy );
#endif

    return LR11XX_HAL_STATUS_OK;
}


/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_read
 *
 * description: send command to the device and then read back a response.
 *
 *              NOTE: the first response byte is thrown away.  It is stat1
 *                    in all responses, and the calling code does not expect
 *                    to get that byte back.
 *
 * input:       int data_length: number of bytes the caller truly wants returned.
 *                               the stat1 byte is NOT included in this count.
 *
 * output:
 *
 *-------------------------------------------------------------------------*/

lr11xx_hal_status_t lr11xx_hal_read( const void *context, const uint8_t* command, const uint16_t command_length,
				     uint8_t* data, const uint16_t data_length )
{
    uint8_t stat1 = 0;
    lr11xx_hal_status_t retVal = LR11XX_HAL_STATUS_ERROR;

//    TRACE3(TAG_LR11XX_HAL_READ, command[0], command[1], __builtin_return_address(0));

    if( lr11xx_hal_wakeup( context ) == LR11XX_HAL_STATUS_OK )
    {
	retVal = lr11xx_spi_transceive(context, command, NULL, command_length, NULL, NULL, 0);
    }

    if (LR11XX_HAL_STATUS_OK == retVal)
    {
       retVal = lr11xx_spi_transceive(context, &stat1, &stat1, 1, data, data, data_length);
    }
    return retVal;
}
/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_write
 *
 * description: write a command to the radio, and also check whether the
 *              command that was sent was the SET_SLEEP command.  This hal
 *              layer must track whether the radio has been put to sleep,
 *              so that upon the next command, it can toggle the nReset pin
 *              to recover the chip.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/

lr11xx_hal_status_t lr11xx_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
				      const uint8_t* data, const uint16_t data_length )
{
   lr11xx_hal_status_t status;

//   TRACE3(TAG_LR11XX_HAL_WRITE, command[0], command[1], __builtin_return_address(0));

   if ( lr11xx_hal_wakeup( context ) == LR11XX_HAL_STATUS_OK ) {

      status =  lr11xx_spi_transceive(context, command, NULL, command_length, data, NULL, data_length);

      // LR11XX_SYSTEM_SET_SLEEP_OC=0x011B opcode. In sleep mode the radio busy line is held at 1 => do not test it
      if ( ( command[0] == 0x01 ) && ( command[1] == 0x1B ) ) {

	 op_mode_set( context, OP_MODE_SLEEP );

	 // add a incompressible delay to prevent trying to wake the radio before it is full asleep
	 k_busy_wait( 500 );
      }

      k_busy_wait( 500 );
      return status;
   }

   return LR11XX_HAL_STATUS_ERROR;
}


lr11xx_hal_status_t lr11xx_hal_direct_read( const void *context, uint8_t* data, const uint16_t data_length )
{
   /*
    * Wake the device if we are in sleep mode.
    */
   if ( lr11xx_hal_wakeup( context ) == LR11XX_HAL_STATUS_OK ) {
      wait_on_busy( context );
   }

    return lr11xx_spi_transceive(context, data, data, data_length, NULL, NULL, 0);
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
void lr11xx_drv_disable_radio_irq (void)
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
 * name:        lr11xx_hal_reset
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
lr11xx_hal_status_t lr11xx_hal_reset( const void* context )
{
   printk("%s: Resetting radio\n", __func__);

   lr11xx_gpio_reset(&dev_data);

   /* Device transitions to standby on reset */
   op_mode_set(context, OP_MODE_STDBY_RC);

   return LR11XX_STATUS_OK;

}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_reset_programming_mode
 *
 * description: Reset the LR1110 and get it into programming mode.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
lr11xx_hal_status_t lr11xx_hal_reset_programming_mode( const void* context )
{
   printk("%s: Resetting radio", __func__);

   lr11xx_gpio_reset_programming_mode(&dev_data);

   return LR11XX_STATUS_OK;

}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_wakeup
 *
 * description: Wake the radio if in sleep mode, and wait for BUSY to
 *              de-assert.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
lr11xx_hal_status_t lr11xx_hal_wakeup( const void *context )
{
   if ( op_mode_get( context ) == OP_MODE_SLEEP ) {

      lr11xx_CS_wakeup(&dev_data);

      // Radio is awake in STDBY_RC mode
      op_mode_set (context, OP_MODE_STDBY_RC);

      /*
       * Now that we're no longer in sleep mode, clear any interrupts.  Seeing a CMD_ERROR
       * interrupt pending after the CS is toggled.
       */
      lr11xx_system_clear_irq_status(context, LR11XX_SYSTEM_IRQ_ALL_MASK);

      lr11xx_gpio_radio_irq_enable(&dev_data);  // only reenable IRQ if actually waking up
   }

   return wait_on_busy(context);
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
const void *lr11xx_hal_get_radio_context ( void )
{
   return device_get_binding(DEFAULT_RADIO);
}

/*-------------------------------------------------------------------------
 *
 * name:        op_mode_get
 *
 * description: return current operating mode
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static op_mode_e op_mode_get ( const void *context)
{
   const struct device *device = context;
   struct lr11xx_data *data = device->data;

   return data->op_mode;
}

/*-------------------------------------------------------------------------
 *
 * name:        op_mode_set
 *
 * description: Change the current operating mode.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void op_mode_set( const void *context, const op_mode_e op_mode )
{
   const struct device *device = context;
   struct lr11xx_data *data = device->data;

   data->op_mode = op_mode;
}

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_read_busy_pin
 *
 * description: Read the BUSY pin from the LR1110
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
uint32_t lr11xx_hal_read_busy_pin (const void *context)
{
   return lr11xx_is_busy(&dev_data);
}

#define WAIT_USECS (10)

/*-------------------------------------------------------------------------
 *
 * name:        lr11xx_hal_wait_on_busy
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static lr11xx_hal_status_t wait_on_busy( const void* context )
{
   uint32_t
      msecs = 0,
      secs  = 0,
      usecs = 0;

   while (lr11xx_is_busy(&dev_data)) {

      k_busy_wait( WAIT_USECS );

      usecs += WAIT_USECS;

      if (usecs >= 1000) {
	 usecs = 0;
	 msecs++;

	 if ((msecs % 1000) == 0) {
	    msecs = 0;
	    secs++;
	    printk("%s: busy for %d seconds... ( op_mode: %d ) (from %p) \n", __func__, secs, op_mode_get( context ),  __builtin_return_address(0) );
	 }
      }
   }

   return LR11XX_HAL_STATUS_OK;
}

uint32_t lr11xx_board_get_tcxo_wakeup_time( const void* context )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

DEVICE_DT_INST_DEFINE(0, &lr11xx_lora_init, NULL, &dev_data,
		      &dev_config, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &lr11xx_lora_api);
