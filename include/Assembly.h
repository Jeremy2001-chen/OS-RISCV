#ifndef _ASSEMBLY_H_
#define _ASSEMBLY_H_

#define BEGIN_FUNCTION(name) \
    .globl name; \
    .align 4; \
    name :

#endif