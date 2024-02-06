#pragma once

/*
 * Magic word to mark start of packet.  The top byte is left as 0 so that the
 * packet size can be OR-ed in.
 */
#define TRACE_MAGIC                  (0x008d78af)
#define TRACE_MAGIC_WITH_CYCLE_COUNT (TRACE_MAGIC ^ 0x10000)

/*
 * enumerations outside of CONFIG_LXK_TRACE so that they can be used in stand-alone parser.
 *
 * NOTE: Move un-used lines to end of list to avoid confusing parseSaleae
 */
typedef enum {
   TAG_ZERO = 0xfe12,

   TAG_FROM,
   TAG_HERE,
   TAG_MISC,
   TAG_SP,
   TAG_SWAP_IN,
   TAG_THREAD_NAME,
   TAG_TRIGGER,

   // optional tags
   TAG_LR1_STACK_TX_RADIO_START,
   TAG_SYS_CLOCK_START,

   TAG_RP_TASK_ENQUEUE,
   TAG_LR11XX_HAL_TIMER_WORK_HANDLER,

   TAG_SCHEDULE_TASK_IN_PAST,
   TAG_RADIO_PLANNER_LAUNCH_CURRENT,
   TAG_TASK_ABORT,

   TAG_PE4259_SELECT,

   TAG_TRACKER_RUN_ENGINE,

   TAG_TRACKER_SLEEP_TIME_MS,
   TAG_TRACKER_WAKE,

   TAG_SEMTRACKER_ACCEL,

   TAG_SMTC_MODEM_HAL_START_TIMER,
   TAG_RADIO_PLANNER_TASK_ENQUEUE,
   TAG_LR1_STACK_MAC_LORA_LAUNCH_CALLBACK,

   TAG_LR1_MAC_CORE_PROCESS,
   TAG_LR1_MAC_CORE_PROCESS_RETURN,
   TAG_LR1_MAC_CORE_RADIO_STATE,
   TAG_LR1_STACK_MAC_START_TIME,
   TAG_RP_TASK_ARBITER,
   TAG_RP_TASK_UPDATE_TIME,
   TAG_RP_RADIO_IRQ_CALLBACK,
   TAG_RP_TIMER_IRQ_CALLBACK,
   TAG_RP_CALLBACK,
   TAG_SMTC_MODEM_REQUEST_UPLINK,

} trace_tag_e;
