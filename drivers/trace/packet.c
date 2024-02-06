#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/trace.h>
#include "trace_internal.h"

#if (USE_RING > 0)

#ifdef CONFIG_LXK_TRACE

static int trigger_time_stamp;

static void
    print_time_stamp(int time_stamp);

static void
    hdr (uint32_t tag, char *str, int time_stamp, unsigned int line_number)
{
   int
      time_stamp_relative_to_trigger;
   static int
      last_time_stamp = 0;

   /*
    * convert time_stamp to usecs.
    */
   time_stamp = time_stamp * 30;

   /*
    * Conver the absolute time to something relative to the trigger time.
    */
   time_stamp_relative_to_trigger = time_stamp - trigger_time_stamp;


   if (!last_time_stamp) {
      last_time_stamp = time_stamp_relative_to_trigger;
   }

   printk("%-25s | ", str);
   printk("%5d | ", line_number);

//   print_time_stamp(time_stamp);
   print_time_stamp(time_stamp_relative_to_trigger);
   print_time_stamp(time_stamp_relative_to_trigger - last_time_stamp);

   last_time_stamp = time_stamp_relative_to_trigger;
}

static unsigned int
   next (void)
{
   unsigned int
      val;

   word_get(&val);

   return(val);
}

#if 0
static void from(void)
{
   printk("from: 0x%x", next());
}
#endif

void
   packet_parse (unsigned int word)
{
   uint32_t
      i,
      lineno,
      lineno_tag,
      tag,
      val;
   int
      time_stamp;

   if (word != TRACE_MAGIC && word != TRACE_MAGIC_WITH_CYCLE_COUNT) {
//      printk("%s %d Lost sync: 0x%x\n", __func__,__LINE__, word);
      return;
   }

   if (word == TRACE_MAGIC_WITH_CYCLE_COUNT) {
      /*
       * Get time stamp
       */
      word_get(&time_stamp);
   }
   else {
      time_stamp = 0;
   }

   /*
    * We have the magic word.  Now get the packet length and lineno
    */
   word_get(&lineno_tag);

   tag    = (lineno_tag >>  0) & 0xffff;
   lineno = (lineno_tag >> 16) & 0xffff;

#define HDR(str) hdr(tag, str, time_stamp, lineno);

   switch (tag) {

      case TAG_TRIGGER:
         HDR("TRIGGER");
         printk("from: 0x%x", next());
         break;

      case TAG_FROM:
         HDR("FROM");
         printk("addr: 0x%x ", next());
         break;

      case TAG_HERE:
         HDR("HERE");
         printk("addr: 0x%x ", next());
         break;

      case TAG_MISC:
         HDR("MISC");
         val = next();
         printk("val: 0x%x (%d) ", val, val);
         break;

         /* ---------- optional -------- */

       case TAG_TASK_ABORT:
          HDR("TASK_ABORT");

          printk("hook_id: %d ", next());
          printk("now: %d ", next());
          printk("task->start_time_ms: %d ", next());
          break;

       case TAG_TASK_ENQUEUE:
          HDR("TASK_ENQUEUE");

          printk("hook_id: %d ", next());
          printk("payload: %x ", next());
          printk("payload_size: %d ", next());
          printk("now: %d ", next());
          printk("task->start_time_ms: %d ", next());
          break;

       case TAG_PAYLOAD_SEND:
          HDR("PAYLOAD_SEND");
          for (i=0; i<8; i++) {
             printk("%02x ", next());
          }

          break;

       case TAG_CLOCK_SYNC:
          HDR("CLOCK_SYNC");
          printk("payload_count: %d", next());
          break;

      default:
         printk("Unparsed: %x\n", tag);
         break;
   }

   printk("\n");
}

void
   packet_dump (int trig_time_stamp)
{
   unsigned int word;

   printk("packet_dump\n");

   trigger_time_stamp = trig_time_stamp;

   /*
    * Load up all words into the list to process.
    */

   word_dump_init();

   printk("\n\nTrace dump begin (%d words) \n", word_entries());

   while(word_get(&word)) {
      packet_parse(word);
   }

   printk("--- Trace dump End ----\n");

   /*
    * Blow away the current trace so that we don't display same data multiple times.
    */
   word_init();
}


static void
   print_time_stamp(int time_stamp)
{
   printk("%8d | ", time_stamp);
}

#endif
#endif
