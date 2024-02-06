#ifndef trace_internal_h
#define trace_internal_h 1

#include <zephyr/drivers/trace.h>
#include "trace_configure.h"

#ifndef min
#define min(a,b) (a < b ? a : b)
#endif

/* M/M card.  Jtag1 and Jtag2 connectors */
#define GPIO_CLK     (69)   // P2.A1
#define GPIO_PKT     (70)   // P2.B1
#define GPIO_DTA     (19)   // P2.E12
#define GPIO_TRG     (47)   // P2.F12
// #define GPIO_IRQ     (5)   // jtag1-5
#define GPIO_LBT_PKT (71)
#define GPIO_LBT_TRG (72)
#define GPIO_LBT_DTA (73)
#define GPIO_LBT_CLK (74)

#if (USE_GPIO > 0)
void     gpio_high           (uint32_t gpio);
void     gpio_init           (void);
void     gpio_low            (uint32_t gpio);
void     gpio_out            (uint32_t gpio, uint32_t value);
void     gpio_put            (uint32_t value);
void     gpio_trigger_low    (void);
void     gpio_trigger_high   (void);
#endif

#if (USE_HWTRACE > 0)
void     hwtrace_init         (void);
void     hwtrace_packet_end   (void);
void     hwtrace_packet_start (void);
void     hwtrace_put          (uint32_t value);
#endif

char    *irq_name            (uint32_t irq);
char    *notify_str          (uint32_t which);

void     packet_dump         (int trigger_time_stamp);
void     packet_parse        (uint32_t word);
char    *polygon_request_str (uint32_t code);

void     word_arm            (void);
void     word_dump_init      (void);
int      word_entries        (void);
int      word_get            (uint32_t *ip);
void     word_init           (void);
void     word_magic_set      (void);
void     word_put            (uint32_t word);
uint32_t words_valid         (void);

#endif
