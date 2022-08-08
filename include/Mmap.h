#ifndef __MMAP_H
#define __MMAP_H

#define MAP_SHARED 0x01          /* Share changes */
#define MAP_PRIVATE 0x02         /* Changes are private */
#define MAP_SHARED_VALIDATE 0x03 /* share + validate extension flags */

/* 0x01 - 0x03 are defined in linux/mman.h */
#define MAP_TYPE	0x0f		/* Mask for type of mapping */
#define MAP_FIXED	0x10		/* Interpret addr exactly */
#define MAP_ANONYMOUS	0x20		/* don't use a file */

/* 0x0100 - 0x4000 flags are defined in asm-generic/mman.h */
#define MAP_POPULATE		0x008000	/* populate (prefault) pagetables */
#define MAP_NONBLOCK		0x010000	/* do not block on IO */
#define MAP_STACK		0x020000	/* give out an address that is best suited for process/thread stacks */
#define MAP_HUGETLB		0x040000	/* create a huge page mapping */
#define MAP_SYNC		0x080000 /* perform synchronous page faults for the mapping */
#define MAP_FIXED_NOREPLACE	0x100000	/* MAP_FIXED which doesn't unmap underlying mapping */

#define MAP_UNINITIALIZED 0x4000000	/* For anonymous mmap, memory could be
					 * uninitialized */

#define	PROT_READ	 0x04	/* Pages can be read.  */
#define	PROT_WRITE	 0x02	/* Pages can be written.  */
#define	PROT_EXEC	 0x01	/* Pages can be executed.  */

#endif