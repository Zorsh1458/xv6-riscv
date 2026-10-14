#ifndef _PROCINFO_H_
#define _PROCINFO_H_

enum procinfo_state {
    PROCINFO_UNUSED   = 0,
    PROCINFO_USED     = 1,
    PROCINFO_SLEEPING = 2,
    PROCINFO_RUNNABLE = 3,
    PROCINFO_RUNNING  = 4,
    PROCINFO_ZOMBIE   = 5
};

struct procinfo {
    int pid;
    char name[16];
    int state;
    int ppid;
    char pname[16];
};

#endif
