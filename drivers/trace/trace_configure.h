#ifndef trace_configure_h
#define trace_configure_h 1

/*--------------- configuration flags ------------*/

#define USE_UART 1

/*
 * clock data out GPIOs manually.
 */
// #define USE_GPIO 1

/*
 * Store data into ring buffer
 */
// #define USE_RING 1

/*
 * Use the hardware block support to implement trace.
 */
// #define USE_HWTRACE 1

#if (USE_RING)
#define RING_BUFFER_SIZE_BYTES (48*1024)
#endif

/*------------- end of configuration flags ---------------*/

#endif
