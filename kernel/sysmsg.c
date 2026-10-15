#include <stdarg.h>
#include <limits.h>

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MSG_BUF_SIZE (MSG_BUF_PAGE_COUNT * PGSIZE)

static char msg_buf[MSG_BUF_SIZE];

static uint msg_head = 0;
static uint msg_tail = 0;

static int need_to_skip_start = 0;

static struct spinlock msg_lock;

static volatile int log_syscalls      = 0;
static volatile int log_interruptions = 0;
static volatile int log_forks         = 0;
static volatile int log_executions    = 0;

void
msg_init(void)
{
	initlock(&msg_lock, "msg_buf");
	msg_head = 0;
	msg_tail = 0;
}

static void
msg_putc_locked(char c)
{
	uint next = (msg_head + 1) % MSG_BUF_SIZE;
	if (next == msg_tail) {
    if (msg_buf[msg_head] == '\n') {
      need_to_skip_start = 0;
    } else {
      need_to_skip_start = 1;
    }
    msg_tail = (msg_tail + 1) % MSG_BUF_SIZE;
	}
	msg_buf[msg_head] = c;
	msg_head = next;
}

static char digits[] = "0123456789abcdef";

static void
printint(long long xx, int base, int sign)
{
  char buf[20];
  int i;
  unsigned long long x;

  if (sign && (sign = (xx < 0)))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    msg_putc_locked(buf[i]);
}

static void
printptr(uint64 x)
{
  int i;
  msg_putc_locked('0');
  msg_putc_locked('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    msg_putc_locked(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

static void
pr_msg_locked(const char *fmt, va_list ap) {
  int i, cx, c0, c1, c2;
  char *s;

  for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
    if (cx != '%') {
      msg_putc_locked(cx);
      continue;
    }
    i++;
    c0 = fmt[i + 0] & 0xff;
    c1 = c2 = 0;
    if (c0)
      c1 = fmt[i + 1] & 0xff;
    if (c1)
      c2 = fmt[i + 2] & 0xff;
    if (c0 == 'd') {
      printint(va_arg(ap, int), 10, 1);
    } else if (c0 == 'l' && c1 == 'd') {
      printint(va_arg(ap, uint64), 10, 1);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') {
      printint(va_arg(ap, uint64), 10, 1);
      i += 2;
    } else if (c0 == 'u') {
      printint(va_arg(ap, uint32), 10, 0);
    } else if (c0 == 'l' && c1 == 'u') {
      printint(va_arg(ap, uint64), 10, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'u') {
      printint(va_arg(ap, uint64), 10, 0);
      i += 2;
    } else if (c0 == 'x') {
      printint(va_arg(ap, uint32), 16, 0);
    } else if (c0 == 'l' && c1 == 'x') {
      printint(va_arg(ap, uint64), 16, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'x') {
      printint(va_arg(ap, uint64), 16, 0);
      i += 2;
    } else if (c0 == 'p') {
      printptr(va_arg(ap, uint64));
    } else if (c0 == 'c') {
      msg_putc_locked(va_arg(ap, uint));
    } else if (c0 == 's') {
      if ((s = va_arg(ap, char *)) == 0)
        s = "(null)";
      for (; *s; s++)
        msg_putc_locked(*s);
    } else if (c0 == '%') {
      msg_putc_locked('%');
    } else if (c0 == 0) {
      break;
    } else {
      // Print unknown % sequence to draw attention.
      msg_putc_locked('%');
      msg_putc_locked(c0);
    }
  }
}

static void
pr_msg_format_locked(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
	pr_msg_locked(fmt, ap);
  va_end(ap);
}

void
pr_msg(const char *fmt, ...) {
  acquire(&msg_lock);

	uint t;
	acquire(&tickslock);
	t = ticks;
	release(&tickslock);

  pr_msg_format_locked("[%d] ", t);
  
  va_list ap;
  va_start(ap, fmt);
	pr_msg_locked(fmt, ap);
  va_end(ap);

  msg_putc_locked('\n');

  release(&msg_lock);
}

uint64
sys_dmesg(void) {
  uint64 out;
  int len;

  argaddr(0, &out);
  argint(1, &len);

  if (len < 1) {
    return -1;
  }

  acquire(&msg_lock);
  
  struct proc *p = myproc();
  
  uint size = MSG_BUF_SIZE - msg_tail + msg_head;

  if (msg_head >= msg_tail) {
    size = msg_head - msg_tail;
  }

  uint avail = size;
  if (avail > len - 1) {
    avail = len - 1;
  }

  uint skip = size - avail;
  uint start = (msg_tail + skip) % MSG_BUF_SIZE;
  
  char tmp[256];
  uint done = 0;
  uint pos = start;

  if (need_to_skip_start) {
    uint skipping = 0;
    while(msg_buf[(pos + skipping) % MSG_BUF_SIZE] != '\n' && skipping < avail - 1) {
      skipping++;
    }
    if (skipping < avail - 1) {
      pos = (pos + skipping + 1) % MSG_BUF_SIZE;
      
      int new_size = size - skipping - 1;
      if (new_size < avail) {
        avail = new_size;
      }
    }
  }

  while (done < avail) {
    uint chunk = avail - done;

    if (chunk > sizeof(tmp)) {
      chunk = sizeof(tmp);
    }

    for (uint i = 0; i < chunk; i++) {
      tmp[i] = msg_buf[(pos + i) % MSG_BUF_SIZE];
    }

    if (copyout(p->pagetable, p->sz, out + done, tmp, chunk) < 0) {
      return -2;
    }

    done += chunk;
    pos = (pos + chunk) % MSG_BUF_SIZE;
  }

  char zero = 0;
  if (copyout(p->pagetable, p->sz, out + done, &zero, 1) < 0) {
    return -2;
  }

  release(&msg_lock);

  return done;
}

static uint
get_diag_end_time(int class) {
  switch (class) {
    case 0:
      return log_syscalls;
    case 1:
      return log_interruptions;
    case 2:
      return log_forks;
    case 3:
      return log_executions;
  }
  return 0;
}

int
get_diag_flag(int class) {
  uint end_time = get_diag_end_time(class);

	uint current_time;
	acquire(&tickslock);
	current_time = ticks;
	release(&tickslock);

  if (end_time > current_time) {
    return 1;
  }
  return 0;
}

uint64
sys_set_diag(void) {
  int class, duration;

  argint(0, &class);
  argint(1, &duration);

	uint end_time = UINT_MAX;
  if (duration > -1) {
	  uint current_time;
	  acquire(&tickslock);
	  current_time = ticks;
	  release(&tickslock);

    if (duration < UINT_MAX - current_time) {
      end_time = current_time + duration;
    }
  }

  switch (class) {
    case 0:
      log_syscalls = end_time;
      return 0;
    case 1:
      log_interruptions = end_time;
      return 0;
    case 2:
      log_forks = end_time;
      return 0;
    case 3:
      log_executions = end_time;
      return 0;
  }

  return -1;
}