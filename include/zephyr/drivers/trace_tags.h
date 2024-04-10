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

   // semi-permanent
   TAG_SYS_CLOCK_START,

   //
   // temporary
   //
   TAG_RP_TASK_UPDATE_TIME,
   TAG_RP_RADIO_IRQ_CALLBACK,
   TAG_RP_TIMER_IRQ_CALLBACK,
   TAG_RP_CALLBACK,
   TAG_SMTC_MODEM_REQUEST_UPLINK,
   TAG_APPS_MODEM_EVENT_PROCESS,
   TAG_LR1_STACK_TX_RADIO_START,
   TAG_RP_TASK_ENQUEUE,
   TAG_MODEM_SUPERVISOR_ENGINE,
   TAG_MODEM_SUPERVISOR_ENGINE_LAUNCH_FUNC,
   TAG_LORAWAN_SEND_MGMT_ON_LAUNCH,
   TAG_SMTC_REAL_IS_PAYLOAD_SIZE_VALID,
   TAG_TIMER_CALLBACK,
   TAG_SEMTRACKER_GNSS_SCAN_AGGREGATE,
   TAG_SEMTRACKER_GNSS_SCAN,
   TAG_SEMTRACKER_GNSS_SCAN_DONE,
   TAG_LR11XX_DRV_LNA_DISABLE,
   TAG_LR11XX_DRV_LNA_ENABLE,
   TAG_SEMTRACKER_WIFI_SCAN,

} trace_tag_e;
