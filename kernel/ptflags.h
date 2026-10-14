#ifndef _PT_FLAGS_H_
#define _PT_FLAGS_H_

#define PT_CLEAR_A 0x1
#define PT_CLEAR_D 0x2

#define PT_CLEAR_MASK (PT_CLEAR_A | PT_CLEAR_D)

int clear_accessed_dirty(void *buf, int len, int flags);

#endif