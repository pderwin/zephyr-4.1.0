#include <zephyr/kernel.h>
#include <string.h>
#include "trace_internal.h"

#if (USE_RING > 0)

/*
 * Use powers of two to make indexing easy
 */
#define NUMBER_TRACE_ENTRIES ((RING_BUFFER_SIZE_BYTES / sizeof(unsigned int) ) )

#define WORD_MAGIC (0xe4f1a935)

typedef struct {
   unsigned int

   /*
    * Used only during data readback.  Will be set to NUMBER_TRACE_ENTRIES and counted down to 0 to
    * know when data is consumed.
    */
      word_count,
      word_index,
      word_size,
      word_magic;

   unsigned int
      trace_data[NUMBER_TRACE_ENTRIES];

} lxk_trace_ring_t;

/*
 * Don't want this cleared at POR
 */
lxk_trace_ring_t __noinit lxk_trace_ring;

void phil_hex(uint32_t val);
void phil_str(char *str);

uint32_t words_valid (void)
{
   uint32_t rc = 0;

   if (lxk_trace_ring.word_magic == WORD_MAGIC) {
      rc = 1;
   }

   return rc;
}

void
   word_init (void)
{
   memset(lxk_trace_ring.trace_data, 0x11, sizeof(lxk_trace_ring.trace_data));
}

void
   word_dump_init (void)
{
#if 0

   uint32_t
      i,
      *ip,
      j;

   ip = lxk_trace_ring.trace_data;

   printk("\n\n");

   for (i=0; i<8; i++) {
      for (j=0; j<8; j++) {
         printk("%08x ", *ip);
         ip++;
      }
      printk("\n");
   }
#endif

   lxk_trace_ring.word_count = NUMBER_TRACE_ENTRIES;
}

int
   word_entries (void)
{
   return (NUMBER_TRACE_ENTRIES);
}

void word_magic_set (void)
{
   lxk_trace_ring.word_index = 0;

   lxk_trace_ring.word_size  = NUMBER_TRACE_ENTRIES;
   lxk_trace_ring.word_magic = WORD_MAGIC;
}

/*-------------------------------------------------------------------------
 *
 * name:        word_put
 *
 * description:  Put a word of data into the lxk_trace_ring.  Assumes interrupts
 *               are disabled outside of this routine.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void
   word_put(unsigned int word)
{
   lxk_trace_ring.trace_data[lxk_trace_ring.word_index % NUMBER_TRACE_ENTRIES] = word;
   lxk_trace_ring.word_index++;
}

int
   word_get(unsigned int *ip)
{
   if (lxk_trace_ring.word_count) {

      *ip = lxk_trace_ring.trace_data[lxk_trace_ring.word_index % NUMBER_TRACE_ENTRIES];

//      printk("%s %d %08x wi: %d wc: %d \n", __func__,__LINE__, *ip, lxk_trace_ring.word_index % NUMBER_TRACE_ENTRIES, lxk_trace_ring.word_count);

      lxk_trace_ring.word_index++;

      lxk_trace_ring.word_count--;

      return(1);
   }

   *ip = 0x11111111;
   return(0);
}

#endif
