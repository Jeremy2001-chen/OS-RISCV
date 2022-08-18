#include "Type.h"

void* memset(void* dst, int c, uint n) {
    char* cdst = (char*)dst;
    int i;
    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

int memcmp(const void* v1, const void* v2, uint n) {
    const uchar *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}

void* memmove(void* dst, const void* src, uint n) {
    const char* s;
    char* d;

    // if (n == 0)
    //     return dst;

    s = src;
    d = dst;
    // if (s < d && s + n > d) {
    //     s += n;
    //     d += n;
    //     while (n-- > 0)
    //         *--d = *--s;
    // } else
        while (n-- > 0)
            *d++ = *s++;

    return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void* memcpy(void* dst, const void* src, uint n) {
    return memmove(dst, src, n);
}

int strncmp(const char* p, const char* q, uint n) {
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uchar)*p - (uchar)*q;
}

char* strncpy(char* s, const char* t, int n) {
    char* os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char* safestrcpy(char* s, const char* t, int n) {
    char* os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}

int strlen(const char* s) {
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

char* strchr(const char* s, char c) {
    for (; *s; s++)
        if (*s == c)
            return (char*)s;
    return 0;
}

// convert wide char string into uchar string
void snstr(char* dst, wchar const* src, int len) {
    while (len-- && *src) {
        *dst++ = (uchar)(*src & 0xff);
        src++;
    }
    while (len-- > 0)
        *dst++ = 0;
}

/*
void* memmove(void* dst, const void* src, uint n) {
    if (n == 0 || (u64)src == (u64)dst)
        return dst;

    const char* s, *f;
    char* d;

    s = src;
    d = dst;
    f = src + n;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    } else {
        switch (LOWBIT(((u64)s) ^ ((u64)d))) {
            case 1:
                while (s < f)
                    *d++ = *s++;
                break;
            case 2:
                if (n < 2) {
                    while (s < f)
                        *d++ = *s++;
                    break;
                }
                while (((u64)s & 1)) {
                    *d++ = *s++;
                }
                while (s + 2 < f) {
                    *(u16*)d = *(u16*)s;
                    (u16*)d++;
                    (u16*)s++;
                }
                while (s < f) {
                    *d++ = *s++;
                }
                break;
            case 4:
                if (n < 4) {
                    while (s < f)
                        *d++ = *s++;
                    break;
                }
                while (((u64)s & 3)) {
                    *d++ = *s++;
                }
                while (s + 4 < f) {
                    *(u32*)d = *(u32*)s;
                    (u32*)d++;
                    (u32*)s++;
                }
                while (s < f) {
                    *d++ = *s++;
                }
                break;
            default:
                if (n < 8) {                
                    while (s < f)
                        *d++ = *s++;
                    break;
                }
                while (((u64)s & 7)) {
                    *d++ = *s++;
                }
                while (s + 8 < f) {
                    *(u64*)d = *(u64*)s;
                    (u64*)d++;
                    (u64*)s++;
                }
                while (s < f) {
                    *d++ = *s++;
                }
                break;
        }
    }
    return dst;
}
*/