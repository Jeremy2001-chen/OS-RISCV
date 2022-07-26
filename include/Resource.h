#ifndef _RESOURCE_H_
#define _RESOURCE_H_

struct ResourceLimit {
    u64 soft;  /* Soft limit */
    u64 hard;  /* Hard limit (ceiling for rlim_cur) */
};

#define RLIMIT_NOFILE 7

#endif
