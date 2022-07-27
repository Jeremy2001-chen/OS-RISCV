#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *staticArgv[] = {"./runtest.exe", "-w", "entry-static.exe", "", 0};
char *staticList[] = {
    "search_hsearch", "basename", "clocale_mbfuncs", "clock_gettime",
    "crypt", "dirname", "env", "fdopen", "fnmatch",
    "fscanf", "fwscanf", "iconv_open", "inet_pton", 
    "mbc", "memstream", "qsort", "random", "argv",
    "search_insque", "search_lsearch", "search_tsearch",
    "setjmp", "snprintf", "socket", "sscanf", "sscanf_long",
    "stat", "strftime", "string", "string_memcpy", "string_memmem",
    "string_memset", "string_strchr", "string_strcspn", "string_strstr",
    "strptime", "strtod", "strtod_simple", "strtof", "strtol", "strtold",
    "swprintf", "tgmath", "time", "tls_align", "udiv", "ungetc", "utime",
    "wcsstr", "wcstol", "pleval", "daemon_failure", "dn_expand_empty",
    "dn_expand_ptr_0", "fflush_exit", "fgets_eof", "fgetwc_buffering",
    "fpclassify_invalid_ld80", "ftello_unflushed_append", "getpwnam_r_crash",
    "getpwnam_r_errno", "iconv_roundtrips", "inet_ntop_v4mapped",
    "inet_pton_empty_last_field", "iswspace_null", "lrand48_signextend",
    "lseek_large", "malloc_0", "mbsrtowcs_overflow", "memmem_oob_read",
    "memmem_oob", "mkdtemp_failure", "mkstemp_failure", "printf_1e9_oob",
    "printf_fmt_g_round", "printf_fmt_g_zeros", "printf_fmt_n",
    "putenv_doublefree"};




/*
static:
pthread_cond 
pthread_tsd 
pthread_cancel_points 
pthread_robust_detach 
pthread_cancel_sem_wait
pthread_cond_smasher
pthread_condattr_setclock
pthread_exit_cancel
pthread_once_deadlock
pthread_rwlock_ebusy
*/
void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    for (int i = 0; i < sizeof(staticList) / sizeof(char*); i++) {
        int pid = fork();
        if (pid == 0) {
            staticArgv[3] = staticList[i];
            exec("./runtest.exe", staticArgv);
        } else {
            wait(0);
        }
    }
    
    exit(0);
}
