#pragma once

#ifndef CONFIG_LXK_TRACE

#define FROM()
#define HERE()
#define TRACE(t)
#define TRACE1(t, a1)
#define TRACE2(t, a1, a2)
#define TRACE3(t, a1, a2, a3)
#define TRACE4(t, a1, a2, a3, a4)
#define TRACE5(t, a1, a2, a3, a4, a5)
#define TRACE6(t, a1, a2, a3, a4, a5, a6)
#define TRACE7(t, a1, a2, a3, a4, a5, a6, a7)
#define TRACE8(t, a1, a2, a3, a4, a5, a6, a7, a8)
#define TRACE_ARM()
#define TRACE_BLOCK(tag, address, count)
#define TRACE_DUMP()
#define TRACE_MISC(__val)
#define TRACE_SP()
#define TRACE_TRIGGER()
#define TRACE_TRIGGER_NO_PRINTK()
#define TRACE_GPIO(p,v)

#else

#include <stdint.h>
#include "trace_tags.h"

/*
 * he_res is '1' to signal that we want a double pulse
 */
void trace_hb(uint32_t double_pulse, uint32_t flag);


void
    trace_arm     (void),
    trace_block   (uint32_t tag, uint32_t lineno, uint32_t addr, uint32_t count),
    trace_dump    (void),
    trace_gpio    (uint32_t gpio, uint32_t val),
    trace_here    (uint32_t lineno),
    trace_sp      (uint32_t lineno),
    trace_trigger (uint32_t do_printk, uint32_t lineno),
    trace        (uint32_t tag, uint32_t lineno),
    trace1       (uint32_t tag, uint32_t lineno, uint32_t a0),
    trace2       (uint32_t tag, uint32_t lineno, uint32_t a0, uint32_t a1),
    trace3       (uint32_t tag, uint32_t lineno, uint32_t a1, uint32_t a2, uint32_t a3),
    trace4       (uint32_t tag, uint32_t lineno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4),
    trace5       (uint32_t tag, uint32_t lineno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5),
    trace6       (uint32_t tag, uint32_t lineno,
		  uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6),
    trace7       (uint32_t tag, uint32_t lineno,
		  uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7),
    trace8       (uint32_t tag, uint32_t lineno,
		  uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8),
    trace9       (uint32_t tag, uint32_t lineno,
		  uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8, uint32_t a9);


void trace_arm_shim(void);
void trace_dump_shim(void);
void trace_gpio_shim(uint32_t gpio, uint32_t val);
void trace_trigger_shim(uint32_t do_printk, uint32_t lineno);

static inline void z_impl_trace_dump_shim(void)
{
   trace_dump();
}

static inline void z_impl_trace_gpio_shim(uint32_t gpio, uint32_t val)
{
   trace_gpio(gpio, val);
}

static inline void z_impl_trace_trigger_shim(uint32_t do_printk, uint32_t lineno)
{
   trace_trigger(do_printk, lineno);
}

#define TRACE_ARM()                  trace_arm()
#define TRACE_BLOCK(tag, addr, cnt)  trace_block(tag, __LINE__, (uint32_t) addr, cnt)
#define TRACE_HEX_DUMP(addr, cnt)    trace_block(TAG_HEX_DUMP, __LINE__, addr, cnt)
#define TRACE_DUMP()                 trace_dump()
#define TRACE_GPIO(g,v)              trace_gpio_shim(g,v)

#define TRACE(t)                     trace( t, __LINE__)
#define TRACE1(t, a1)                trace1(t, __LINE__, \
					    (uint32_t) (a1) )
#define TRACE2(t, a1, a2)	     trace2(t, __LINE__, \
					    (uint32_t) (a1), \
					    (uint32_t) (a2) )
#define TRACE3(t, a1, a2, a3)	     trace3(t, __LINE__, \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3) )
#define TRACE4(t, a1,a2,a3,a4)	     trace4(t, __LINE__, \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3), \
					    (uint32_t) (a4) )
#define TRACE5(t, a1,a2,a3,a4,a5)    trace5(t, __LINE__, \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3), \
					    (uint32_t) (a4), \
					    (uint32_t) (a5))

#define TRACE6(t, a1,a2,a3,a4,a5,a6) trace6(t, __LINE__, \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3), \
					    (uint32_t) (a4), \
					    (uint32_t) (a5), \
					    (uint32_t) (a6))

#define TRACE7(t, a1,a2,a3,a4,a5,a6,a7) trace7(t, __LINE__,        \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3), \
					    (uint32_t) (a4), \
					    (uint32_t) (a5), \
					    (uint32_t) (a6), \
					    (uint32_t) (a7))

#define TRACE8(t, a1,a2,a3,a4,a5,a6,a7,a8) trace8(t, __LINE__,     \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3), \
					    (uint32_t) (a4), \
					    (uint32_t) (a5), \
					    (uint32_t) (a6), \
					    (uint32_t) (a7), \
					    (uint32_t) (a8))

#define TRACE9(t,a1,a2,a3,a4,a5,a6,a7,a8,a9) trace9(t, __LINE__,   \
					    (uint32_t) (a1), \
					    (uint32_t) (a2), \
					    (uint32_t) (a3), \
					    (uint32_t) (a4), \
					    (uint32_t) (a5), \
					    (uint32_t) (a6), \
					    (uint32_t) (a7), \
					    (uint32_t) (a8), \
					    (uint32_t) (a9))


#define TRACE_TRIGGER()           trace_trigger_shim(1, __LINE__)
#define TRACE_TRIGGER_NO_PRINTK() trace_trigger_shim(0, __LINE__)


#define TRACE_SP()                trace_sp(__LINE__)

#define TRACE_IRQ_HIGH()          trace_irq_high()
#define TRACE_IRQ_LOW()           trace_irq_low()

extern uint32_t
   get_cpsr (void);

#define HERE() trace_here(__LINE__)
#define FROM() TRACE1(TAG_FROM, __builtin_return_address(0) )

#define TRACE_MISC(__val) TRACE1(TAG_MISC, __val);




void trace_nested_0(uint32_t lineno);

#define TRACE_NESTED_0() trace_nested_0(__LINE__)

#endif
