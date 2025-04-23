/*
 * Copyright (c) 2024 Phil Erwin
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include "lr11xx_hal.h"

/*
 * generic status values.
 */
typedef enum {
   LR11XX_DRV_STATUS_OK,
   LR11XX_DRV_STATUS_ERROR
} lr11xx_drv_status_e;

/*
 * Radio operating modes.  Basically the driver layer needs to track
 * whether the radio is in sleep state or not.
 */
typedef enum {
   OP_MODE_SLEEP,
   OP_MODE_STDBY_RC,
} op_mode_e;


#if DT_HAS_COMPAT_STATUS_OKAY(semtech_lr1110)
#define DT_DRV_COMPAT semtech_lr1110
#define LR1110_DEVICE_ID LR1110
#else
#error No LR1110 instance in device tree.
#endif

struct lr11xx_config {
   struct spi_dt_spec bus;
};

struct lr11xx_data {
   struct gpio_callback dio9_irq_callback;

   void (*radio_irq_callback)(void *context);
   void *radio_irq_context;

   op_mode_e op_mode;
};

void
    lr11xx_drv_lna_disable (void),
    lr11xx_drv_lna_enable  (void);

const void
    *lr11xx_drv_radio_context_get (void);
void
    lr11xx_drv_radio_irq_config  ( void ( *callback )( void* context ), void* context ),
    lr11xx_drv_radio_irq_disable ( void ),
    lr11xx_drv_radio_irq_enable  ( void );

uint32_t
    lr11xx_drv_read_busy_pin         (const void *context);

lr11xx_drv_status_e
    lr11xx_drv_reset                 ( const void* context ),
    lr11xx_drv_reset_programming_mode( const void* context );

void
    lr11xx_drv_timer_start( const uint32_t milliseconds, void (*callback)(void *context), void *context),
    lr11xx_drv_timer_stop (void);
