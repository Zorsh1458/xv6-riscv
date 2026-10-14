#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "proc.h"
#include "defs.h"
#include "file.h"

#define SYSDEV_ID 2

#define SYSDEV_NULL     0
#define SYSDEV_ZERO     1
#define SYSDEV_URANDOM  2
#define SYSDEV_NULLSTAT 3

static uint64 nullstat_bytes = 0;

static uint64 urandom_seed = 0x123456789abcdef0ULL;
static struct spinlock urandom_lock;

void
sysdev_init(void)
{
	initlock(&urandom_lock, "sysdev_urandom");

    devsw[SYSDEV_ID].read = sysdev_read;
    devsw[SYSDEV_ID].write = sysdev_write;
}

static uint64
lcg_next(void)
{
	urandom_seed = 6364136223846793005ULL * urandom_seed + 1442695040888963407ULL;
	return urandom_seed;
}

static int
put_byte(int user_src, uint64 dst, int idx, char b)
{
	if (user_src) {
		if (copyout(myproc()->pagetable, myproc()->sz, dst + idx, &b, 1) < 0) {
			return -1;
        }
	} else {
		*((char *)(dst + idx)) = b;
	}
	return 0;
}

static int
null_read(int user_src, uint64 dst, int n)
{
	return 0;
}

static int
null_write(int user_src, uint64 src, int n)
{
	return n;
}

static int
zero_read(int user_src, uint64 dst, int n)
{
	if (n <= 0) {
		return 0;
    }
	for (int i = 0; i < n; i++) {
		if (put_byte(user_src, dst, i, 0) < 0) {
			return -1;
        }
	}
	return n;
}

static int
zero_write(int user_src, uint64 src, int n)
{
	return -1;
}

static int
urandom_read(int user_src, uint64 dst, int n)
{
	if (n <= 0) {
		return 0;
    }

	acquire(&urandom_lock);
	for (int i = 0; i < n; i++) {
		char b = (char)(lcg_next() & 0xff);

		if (put_byte(user_src, dst, i, b) < 0) {
			release(&urandom_lock);
			return -1;
		}
	}
	release(&urandom_lock);

	return n;
}

static int
urandom_write(int user_src, uint64 src, int n)
{
	if (n != sizeof(uint64)) {
		return -1;
    }

	uint64 newseed = 0;
	if (user_src) {
		if (copyin(myproc()->pagetable, myproc()->sz, (char *)&newseed, src, sizeof(uint64)) < 0) {
			return -1;
        }
	} else {
		memmove(&newseed, (void *)src, sizeof(uint64));
	}

	acquire(&urandom_lock);
	urandom_seed = newseed;
	release(&urandom_lock);

	return n;
}

static int
nullstat_read(int user_src, uint64 dst, int n)
{
	if (n != sizeof(uint64)) {
		return -1;
    }

	uint64 val = nullstat_bytes;

	if (user_src) {
		if (copyout(myproc()->pagetable, myproc()->sz, dst, (char *)&val, sizeof(uint64)) < 0) {
            return -1;
        }
	} else {
		memmove((void *)dst, &val, sizeof(uint64));
	}
	return sizeof(uint64);
}

static int
nullstat_write(int user_src, uint64 src, int n)
{
	if (n < 0) {
		return -1;
    }

	nullstat_bytes += (uint64)n;
	return n;
}

int
sysdev_read(int user_src, uint64 dst, int n, int minor)
{
	switch(minor) {
	    case SYSDEV_NULL:     return null_read(user_src, dst, n);
	    case SYSDEV_ZERO:     return zero_read(user_src, dst, n);
	    case SYSDEV_URANDOM:  return urandom_read(user_src, dst, n);
	    case SYSDEV_NULLSTAT: return nullstat_read(user_src, dst, n);
	}
	return -1;
}

int
sysdev_write(int user_src, uint64 src, int n, int minor)
{
	switch(minor) {
	    case SYSDEV_NULL:     return null_write(user_src, src, n);
	    case SYSDEV_ZERO:     return zero_write(user_src, src, n);
	    case SYSDEV_URANDOM:  return urandom_write(user_src, src, n);
	    case SYSDEV_NULLSTAT: return nullstat_write(user_src, src, n);
	}
	return -1;
}