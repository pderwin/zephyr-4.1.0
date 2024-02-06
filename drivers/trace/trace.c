#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include "zephyr/drivers/trace.h"
#include "zephyr/kernel/thread.h"
#include "trace_internal.h"
#include "uart.h"
/*
 * Stub things out for slave
 */
static uint32_t _scratch_pad;
static uint32_t *scratch_pad = &_scratch_pad;

#define TRACE_INITED_MAGIC (0x1728d100)
#define TRACE_INITED_MASK  (0xffffff00)

#define TRACE_ARMED        (         1)
#define TRACE_ARMED_MAGIC  ( TRACE_INITED_MAGIC | TRACE_ARMED )

extern void stop (uint32_t v, uint32_t v2, uint32_t v3, uint32_t v4);

#define PACKET(__tag__,__data_word_count, __lineno__, __expr__ ) \
   { \
   uint32_t key; \
   key = irq_lock(); \
   packet_start(__tag__, __data_word_count, __lineno__); \
   __expr__; \
   packet_end(); \
   irq_unlock(key); \
   }


static void
   _trace32_bits(uint32_t value);

static void
   packet_end (void)
{
#if (USE_HWTRACE)
   hwtrace_packet_end();
#endif
#if (USE_GPIO)
   gpio_low(GPIO_PKT);
#endif
}

static void
    packet_start (uint32_t tag, uint32_t data_word_count, uint32_t lineno)
{
#if (USE_HWTRACE)
   hwtrace_packet_start();
#endif
#if (USE_GPIO)
   gpio_high(GPIO_PKT);
#endif

#if (INCLUDE_CYCLE_COUNT > 0)
   _trace32_bits(TRACE_MAGIC_WITH_CYCLE_COUNT);
   _trace32_bits(k_uptime_ticks());
#else
   /*
    * Don't allow invalid word count
    */
   if (data_word_count > 255) {
      data_word_count = 255;
   }

   _trace32_bits(TRACE_MAGIC | (data_word_count << 24));
#endif

   _trace32_bits((lineno << 16) | tag);
}

/*-------------------------------------------------------------------------
 *
 * name:        trace_init
 *
 * description: do initialization.  Call from SYS_INIT very early in
 *              power up.
 *
 * input:       none
 *
 * output:      none
 *
 *-------------------------------------------------------------------------*/
static int
   trace_init (void)
{

   /*
    * Nothing to do if we're already initialized.
    */
   if ((*scratch_pad & TRACE_INITED_MASK) == TRACE_INITED_MAGIC) {
      return 0;
   }

   *scratch_pad = TRACE_INITED_MAGIC;

#if (USE_HWTRACE)
   hwtrace_init();
#endif
#if (USE_GPIO)
   gpio_init();
#endif
#if (USE_RING)
   word_init();
#endif
#if (USE_UART > 0)
   uart_init();
#endif

   *scratch_pad = TRACE_INITED_MAGIC;

   /*
    * go ahead and start collecting data.
    */
   trace_arm();

   return 0;
}

static void
   _trace32_bits (uint32_t value)
{
//   printk("%s %d sp: %x %x \n", __func__, __LINE__, *scratch_pad, TRACE_ARMED_MAGIC);

   if (*scratch_pad == TRACE_ARMED_MAGIC) {

#if (USE_HWTRACE)
      hwtrace_put(value);
#endif
#if (USE_GPIO > 0)
//  printk("%s %d \n", __func__, __LINE__);
      gpio_put(value);
#endif
#if (USE_RING > 0)
      word_put(value);
#endif
#if (USE_UART > 0)
      uart_put(value);
#endif
   }
}

void trace (uint32_t tag, uint32_t lineno)
{
   PACKET(tag, 0, lineno, );
}

/*-------------------------------------------------------------------------
 *
 * name:        trace_block
 *
 * description: transmit out a block of data
 *
 * input:       _addr: (uint32_t) address of data
 *              count: number of bytes to send
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void trace_block (uint32_t tag, uint32_t lineno, uint32_t _addr, uint32_t count)
{
   uint32_t
      *addr = (uint32_t *) _addr,
      i,
      packet_size,
      words;

   /*
    * The hardware support can only handle 32 words at a time, so limit
    * each block to 16 words
    */
   while(count) {

      /*
       * TODO: if I set this to 13 or larger, the packet is corrupted.  Simulation
       *       doesn't reproduce it.
       */
      packet_size = min (count, 32);

      /*
       * round up to word count.
       */
      words = (packet_size +3) / 4;

      PACKET(tag, words, lineno,

	     _trace32_bits(packet_size);

	     for (i=0; i < words; i++) {

		_trace32_bits(*addr);

		addr++;
	     }
	 );

      count -= packet_size;
   }

}


void trace1 (uint32_t tag, uint32_t lineno, uint32_t a0)
{
   PACKET(tag, 1, lineno,
	  _trace32_bits(a0);
      );
}

void trace2 (uint32_t tag, uint32_t lineno, uint32_t a0, uint32_t a1)
{
   PACKET(tag, 2, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
      );
}

void trace3 (uint32_t tag, uint32_t lineno, uint32_t a0, uint32_t a1, uint32_t a2)
{
   PACKET(tag, 3, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
      );
}

void trace4 (uint32_t tag, uint32_t lineno, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3 )
{
   PACKET(tag, 4, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
	  _trace32_bits(a3);
      );
}

void trace5 (uint32_t tag, uint32_t lineno, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4 )
{
    PACKET(tag, 5, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
	  _trace32_bits(a3);
	  _trace32_bits(a4);
      );
}

void trace6 (uint32_t tag, uint32_t lineno,
	     uint32_t a0,
	     uint32_t a1,
	     uint32_t a2,
	     uint32_t a3,
	     uint32_t a4,
	     uint32_t a5 )
{
   PACKET(tag, 6, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
	  _trace32_bits(a3);
	  _trace32_bits(a4);
	  _trace32_bits(a5);
      );
}

void trace7 (uint32_t tag,
	      uint32_t lineno,
	      uint32_t a0,
	      uint32_t a1,
	      uint32_t a2,
	      uint32_t a3,
	      uint32_t a4,
	      uint32_t a5,
	      uint32_t a6 )
{
   PACKET(tag, 7, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
	  _trace32_bits(a3);
	  _trace32_bits(a4);
	  _trace32_bits(a5);
	  _trace32_bits(a6);
      );
}

void trace8 (uint32_t tag,
	      uint32_t lineno,
	      uint32_t a0,
	      uint32_t a1,
	      uint32_t a2,
	      uint32_t a3,
	      uint32_t a4,
	      uint32_t a5,
	      uint32_t a6,
	      uint32_t a7 )
{
   PACKET(tag, 8, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
	  _trace32_bits(a3);
	  _trace32_bits(a4);
	  _trace32_bits(a5);
	  _trace32_bits(a6);
	  _trace32_bits(a7);
      );
}

void trace9 (uint32_t tag,
	      uint32_t lineno,
	      uint32_t a0,
	      uint32_t a1,
	      uint32_t a2,
	      uint32_t a3,
	      uint32_t a4,
	      uint32_t a5,
	      uint32_t a6,
	     uint32_t a7,
	      uint32_t a8 )
{
   PACKET(tag, 9, lineno,
	  _trace32_bits(a0);
	  _trace32_bits(a1);
	  _trace32_bits(a2);
	  _trace32_bits(a3);
	  _trace32_bits(a4);
	  _trace32_bits(a5);
	  _trace32_bits(a6);
	  _trace32_bits(a7);
	  _trace32_bits(a8);
      );
}

void
   trace_here (uint32_t lineno)
{
   /*
    * Don't use TRACEn macro here since it will inject the line number of this subroutine.  We really want the caller's line number
    * and the return address of the caller.
    */
   trace1(TAG_HERE, lineno, (uintptr_t) __builtin_return_address(0));
}

static inline uint32_t get_sp(void)
{
	uint32_t my_sp = 0;

//        __asm__ volatile("mov %0,sp" : "=r" (my_sp) ::);

	return my_sp;
}

void
   trace_sp (uint32_t lineno)
{
   /*
    * Don't use TRACEn macro here since it will inject the line number of this subroutine.  We really want the caller's line number
    * and the return address of the caller.
    */
   trace2(TAG_SP, lineno, get_sp(), (uintptr_t) __builtin_return_address(0));
}




#if 0
void trace_mem (uint32_t lineno, uint32_t *start, uint32_t len_bytes)
{
   uint32_t
      i,
      words;

   /*
    * Round up to correct number of words.
    */
   words = (len_bytes + 3) / 4;

   TRACE3(TRACE_MEMORY_DUMP_START, lineno, (uint32_t) start, len_bytes);

   for (i=0; i < words; i++) {
      TRACE2(TRACE_MEMORY_DUMP_ENTRY, (uint32_t) start, *start);
      start++;
   }
}
#endif

static int trigger_time_stamp;

void
   trace_trigger (uint32_t do_printk, uint32_t lineno)
{
   uint32_t
      key;

   key = irq_lock();

#ifdef USE_GPIO
   gpio_trigger_high();
   gpio_trigger_low();
#endif

   /*
    * Only change the trigger time stamp if we are armed and in user context (for CONFIG_USERSPACE)
    */
   if (*scratch_pad == TRACE_ARMED_MAGIC) {
      trigger_time_stamp = k_cycle_get_32();
   }

   packet_start(TAG_TRIGGER, 1, lineno);

   _trace32_bits((uintptr_t) __builtin_return_address(0) );

   packet_end();

//   if (do_printk) {
//      printk("\n*** TRIGGER *** L %d  time_stamp: %d \n", lineno, trigger_time_stamp);
//   }

   if (*scratch_pad == TRACE_ARMED_MAGIC) {

      trace_dump();

//      *scratch_pad &= ~TRACE_ARMED;
   }
   else {
//      printk("%s %d NOT DUMPING TRACE DUE TO NOT ARMED\n", __func__,__LINE__);
   }

   irq_unlock(key);
}

void
   trace_irq_high (void)
{
#if (USE_GPIO)
#if (GPIO_IRQ)
   uint32_t
      key;

   key = irq_lock();

   trace_init();

   gpio_high(GPIO_IRQ);

   irq_unlock(key);
#endif
#endif
}

void
   trace_irq_low (void)
{
#if (USE_GPIO)
#if (GPIO_IRQ)
   uint32_t
      key;

   key = irq_lock();

   trace_init();

   gpio_low(GPIO_IRQ);

   irq_unlock(key);
#endif
#endif
}

void
   trace_arm (void)
{
   *scratch_pad |= TRACE_ARMED;

#if (USE_RING> 0)
   word_magic_set();
#endif
}

static void
   trace_store_disable (void)
{
   *scratch_pad &= ~TRACE_ARMED;

#if (USE_RING> 0)
   word_magic_set();
#endif
}

#if 0
uint32_t
   trace_is_armed (void)
{
   return(trace_armed);
}
#endif

extern void uart_send(char *str, uint32_t flag);

// extern void soc_restore_printk(void);
void
   trace_dump (void)
{

   /*
    * Don't pollute trace with further stores.
    */
   trace_store_disable();

   uart_send("TRACE DUMP\n", 0);

#if (USE_RING > 0)
   uint32_t
      key;

   if (! words_valid()) {
      printk("No valid trace data.\n");
      return;
   }

   key = irq_lock();

   printk("%s %d called from %p \n", __func__,__LINE__, __builtin_return_address(0) );

   /*
    * This will make sure no more data goes into ring.
    */
   packet_dump(trigger_time_stamp);

   irq_unlock(key);
#endif
}


void
   trace_gpio (uint32_t gpio, uint32_t val)
{
#ifdef USE_GPIO
   uint32_t
      key;

   key = irq_lock();

   gpio_out(gpio, val);

   irq_unlock(key);
#endif
}

#ifdef CONFIG_LXK_TRACE_SWAPS
void
   trace_swap_in (void)
{
   TRACE1(TAG_SWAP_IN, _current);
}
#endif

void
   trace_isr_exit (void)
{
//   TRACE(TRACE_ISR_EXIT);
}

SYS_INIT(trace_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
