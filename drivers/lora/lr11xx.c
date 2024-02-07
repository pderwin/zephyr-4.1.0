/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "lr11xx_drv.h"

#include "lr11xx_radio.h"
#include "lr11xx_system.h"
// #include "sx12xx_common.h"

LOG_MODULE_REGISTER(lr1110, CONFIG_LORA_LOG_LEVEL);

#define BOARD_TCXO_WAKEUP_TIME 21

static const struct lr11xx_config dev_config = {
        .bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
#if HAVE_GPIO_ANTENNA_ENABLE
        .antenna_enable = GPIO_DT_SPEC_INST_GET(0, antenna_enable_gpios),
#endif
#if HAVE_GPIO_TX_ENABLE
        .tx_enable = GPIO_DT_SPEC_INST_GET(0, tx_enable_gpios),
#endif
#if HAVE_GPIO_RX_ENABLE
        .rx_enable = GPIO_DT_SPEC_INST_GET(0, rx_enable_gpios),
#endif
#if	HAVE_GPIO_RADIO_ENABLE
        .radio_enable = GPIO_DT_SPEC_INST_GET(0, radio_enable_gpios),
#endif
};

static struct lr11xx_data dev_data;

lr11xx_hal_operating_mode_t
    lr11xx_hal_get_operating_mode( const void* context );
void
    lr11xx_hal_set_operating_mode( const void* context, const lr11xx_hal_operating_mode_t op_mode );

static lr11xx_hal_status_t
    lr11xx_hal_wait_on_busy( const void* context );

static void lr11xx_dio1_irq_work_handler(struct k_work *work)
{
   LOG_DBG("Processing DIO1 interrupt");

   if (!dev_data.radio_dio_irq_callback) {
      LOG_WRN("DIO1 interrupt without valid HAL IRQ callback.");
      return;
   }

   dev_data.radio_dio_irq_callback( dev_data.radio_dio_irq_context );

#if PHIL
   if (Radio.IrqProcess) {
      Radio.IrqProcess();
   }

   lr1110_dio1_irq_enable(&dev_data);
#endif
}

static int lr11xx_lora_init(const struct device *device)
{
   int ret;

   LOG_DBG("Initializing %s", "inst 0 label");

   if (lr11xx_configure_pin(antenna_enable, GPIO_OUTPUT_INACTIVE) ||
       lr11xx_configure_pin(rx_enable, GPIO_OUTPUT_INACTIVE) ||
       lr11xx_configure_pin(tx_enable, GPIO_OUTPUT_INACTIVE) ||
       lr11xx_configure_pin(radio_enable, GPIO_OUTPUT_INACTIVE)) {
      return -EIO;
   }

   k_work_init(&dev_data.dio1_irq_work, lr11xx_dio1_irq_work_handler);

   ret = lr11xx_variant_init(device);
   if (ret) {
      LOG_ERR("Variant initialization failed");
      return ret;
   }

#if HAVE_GPIO_RADIO_ENABLE
   gpio_pin_set_dt(&dev_config.radio_enable, 0);
   k_sleep(K_MSEC(100));
   gpio_pin_set_dt(&dev_config.radio_enable, 1);
   k_sleep(K_MSEC(100));
#endif
// PHIL        ret = sx12xx_init(dev);

   if (ret < 0) {
      LOG_ERR("Failed to initialize SX12xx common");
      return ret;
   }
// enable LR_IRQ interrupt after power is turned on radio is initialized
   lr11xx_dio1_irq_enable(&dev_data);

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
   lr11xx_hal_wait_on_busy( context );

   if (!req_rx && !data_rx) {
      ret = spi_write_dt(&dev_config.bus, &tx);
   } else {
      ret = spi_transceive_dt(&dev_config.bus, &tx, &rx);
   }

   if (ret < 0) {
      LOG_ERR("SPI transaction failed: %i", ret);
   }

   return ret;
}

lr11xx_hal_status_t lr11xx_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
   lr11xx_hal_status_t status;

   if( lr11xx_hal_wakeup( context ) == LR11XX_HAL_STATUS_OK ) {
      status =  lr11xx_spi_transceive(context, command, NULL, command_length, data, NULL, data_length);
      return status;
   }

   return LR11XX_HAL_STATUS_ERROR;
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

lr11xx_hal_status_t lr11xx_hal_direct_read( const void *context, uint8_t* data, const uint16_t data_length )
{
   if( lr11xx_hal_wakeup( context ) == LR11XX_HAL_STATUS_OK ) {
      lr11xx_hal_wait_on_busy( context );
   }

    return lr11xx_spi_transceive(context, data, data, data_length, NULL, NULL, 0);
}



lr11xx_hal_status_t lr11xx_hal_write_read( const void* context, const uint8_t* command, uint8_t* data,
                                           const uint16_t data_length )
{
    if( lr11xx_hal_wakeup( context ) == LR11XX_HAL_STATUS_OK )
    {
                return lr11xx_spi_transceive(context, command, data, data_length, NULL, NULL, 0);
    }
    return LR11XX_HAL_STATUS_ERROR;
}

void lr11xx_hal_dio_irq_register( void (*callback)(void *context), void *context)
{
   struct lr11xx_data *data = &dev_data;

   data->radio_dio_irq_callback = callback;
   data->radio_dio_irq_context  = context;
}

lr11xx_hal_status_t lr11xx_hal_reset( const void* context )
{
   LOG_DBG("Resetting radio");

   lr11xx_reset(&dev_data);

   /* Device transitions to standby on reset */
   lr11xx_hal_set_operating_mode(context, LR11XX_HAL_OP_MODE_STDBY_RC);

   return LR11XX_STATUS_OK;

}

lr11xx_hal_status_t lr11xx_hal_reset_programming_mode( const void* context )
{
   LOG_DBG("Resetting radio");

   lr11xx_reset_programming_mode(&dev_data);

   return LR11XX_STATUS_OK;

}

lr11xx_hal_status_t lr11xx_hal_wakeup( const void *device )
{
   lr11xx_hal_status_t status;

   if( ( lr11xx_hal_get_operating_mode( device ) == LR11XX_HAL_OP_MODE_SLEEP ) ||
       ( lr11xx_hal_get_operating_mode( device ) == LR11XX_HAL_OP_MODE_RX_DC ) )
   {

      lr11xx_CS_wakeup(&dev_data);

      // Radio is awake in STDBY_RC mode
      lr11xx_hal_set_operating_mode(device, LR11XX_HAL_OP_MODE_STDBY_RC);

      /*
       * Now that we're no longer in sleep mode, clear any interrupts.  Seeing a CMD_ERROR
       * interrupt pending after the CS is toggled.
       */
      lr11xx_system_clear_irq_status(device, LR11XX_SYSTEM_IRQ_ALL_MASK);

      lr11xx_dio1_irq_enable(&dev_data);  // only reenable IRQ if actually waking up
   }

   status =  lr11xx_hal_wait_on_busy(device);

   return status;
}

void lr11xx_hal_dio1_irq( const void* context, bool enable )
{
    if (enable)
    {
        lr11xx_dio1_irq_enable(&dev_data);
    }
    else
    {
        lr11xx_dio1_irq_disable(&dev_data);
    }
}

#define	DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE,	okay),
                 "No default LoRa radio	specified in DT");

#define	DEFAULT_RADIO DEVICE_DT_NAME(DEFAULT_RADIO_NODE)

const void *lr11xx_hal_get_radio_context( void )
{
   const struct device *device;

   device = device_get_binding(DEFAULT_RADIO);

   return device;
}

lr11xx_hal_operating_mode_t lr11xx_hal_get_operating_mode( const void *_device )
{
   const struct device *device = _device;
   struct lr11xx_data *data = device->data;

   return data->op_mode;
}

void lr11xx_hal_set_operating_mode( const void *_device, const lr11xx_hal_operating_mode_t op_mode )
{
   const struct device *device = _device;
   struct lr11xx_data *data = device->data;

   data->op_mode = op_mode;

#if defined( USE_RADIO_DEBUG )
    switch( op_mode )
    {
    case LR1110_HAL_OP_MODE_TX:
                lr1110_set_rx_enable(0);
                lr1110_set_tx_enable(1);
        break;
    case LR1110_HAL_OP_MODE_RX:
    case LR1110_HAL_OP_MODE_RX_C:
    case LR1110_HAL_OP_MODE_RX_DC:
                lr1110_set_tx_enable(0);
                lr1110_set_rx_enable(1);
        break;
    default:
                lr1110_set_rx_enable(0);
                lr1110_set_tx_enable(0);
        break;
    }
#endif
}

#define WAIT_USECS (10)

static lr11xx_hal_status_t lr11xx_hal_wait_on_busy( const void* context )
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
            printk("%s: busy for %d seconds... ( op_mode: %d ) (from %p) \n", __func__, secs, lr11xx_hal_get_operating_mode( context ),  __builtin_return_address(0) );
         }
      }
   }

   return LR11XX_HAL_STATUS_OK;
}

void lr1110_board_set_rf_tx_power( const void* context, int8_t power )
{
    printk("%s %d (from %p) \n", __func__, __LINE__, __builtin_return_address(0) );

    // TODO: Add PA Config check
    if( power > 0 )
    {
        if( power > 22 )
        {
            power = 22;
        }
    }
    else
    {
        if( power < -9 )
        {
            power = -9;
        }
    }
    lr11xx_radio_set_tx_params( context, power, LR11XX_RADIO_RAMP_48_US );
}

uint32_t lr11xx_board_get_tcxo_wakeup_time( const void* context )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

uint32_t lr11xx_get_dio_1_pin_state( const void* context )
{
    //return GpioRead( &( ( lr1110_t* ) context )->dio_1 );
        return lr11xx_get_dio1_pin_state(&dev_data);
}

DEVICE_DT_INST_DEFINE(0, &lr11xx_lora_init, NULL, &dev_data,
                      &dev_config, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
                      &lr11xx_lora_api);
