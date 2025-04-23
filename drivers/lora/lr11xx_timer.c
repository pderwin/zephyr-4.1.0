/*
 * Copyright (c) 2024 Phil Erwin
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/trace.h>
#include "lr11xx_drv.h"

static void
    (*tmr_callback)(void *context);
static void
    *tmr_context;

/*-------------------------------------------------------------------------
 *
 * name:        timer_callback
 *
 * description: This is the interrupt handler callback for the timer.  Could
 *              have set the callback in the k_timer itself, but doing this
 *              routine gives the ability to inject traces if enabled.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void timer_callback (struct k_timer *tmr)
{
   tmr_callback(tmr_context);
}

static K_TIMER_DEFINE(my_timer, timer_callback, NULL);

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
void lr11xx_drv_timer_start( const uint32_t milliseconds, void (*callback)(void *context), void *context)
{
   tmr_callback = callback;
   tmr_context  = context;

   k_timer_start(&my_timer, K_MSEC(milliseconds), K_MSEC(0) );
}

void lr11xx_drv_timer_stop(void)
{
   k_timer_stop(&my_timer);
}
