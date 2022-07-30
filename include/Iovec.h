#ifndef _IOVEC_H_
#define _IOVEC_H_

#include "Type.h"

struct Iovec {
    void* iovBase;	/* Pointer to data. */
    u64 iovLen;	/* Length of data. */
};

#define IOVMAX 64
#endif