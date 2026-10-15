#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define RtcReg(reg) ((volatile uint32 *)(RTC_BASE + (reg)))

#define ReadRtcReg(reg) (*RtcReg(reg))

static struct spinlock rtc_lock;

void
rtc_init(void)
{
	initlock(&rtc_lock, "rtc");
}

static uint32
rtc_read_low(void)
{
	return ReadRtcReg(RTC_LOW);
}

static uint32
rtc_read_high(void)
{
	return ReadRtcReg(RTC_HIGH);
}

uint64
rtc_read_ns(void)
{
	uint64 low, high;

	acquire(&rtc_lock);
	low  = (uint64)rtc_read_low();
	high = (uint64)rtc_read_high();
	release(&rtc_lock);

	return (high << 32) | low;
}