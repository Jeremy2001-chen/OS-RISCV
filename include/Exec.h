#ifndef _EXEC_H_
#define _EXEC_H_

#include <Fat.h>
#include <Process.h>

typedef struct Process Process;
typedef struct ProcessSegmentMap {
    Dirent *sourceFile;
    u64 va;
    u64 fileOffset;
    u32 len;
    u32 flag;
    struct ProcessSegmentMap *next;
} ProcessSegmentMap;

#define MAP_ZERO 1

#define SEGMENT_MAP_NUMBER 1024
u64 sys_exec(void);
int segmentMapAlloc(ProcessSegmentMap **psm);
void appendSegmentMap(Process *p, ProcessSegmentMap *psm);
void processMapFree(Process *p);

#endif