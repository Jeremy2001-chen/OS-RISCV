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
    "regex_backref_0", "regex_bracket_icase", "regex_ere_backref", 
    "regex_escaped_high_byte", "regex_negated_range", "regexec_nosub", 
    "rewind_clear_error", "rlimit_open_files", "scanf_bytes_consumed", 
    "scanf_match_literal_eof", "scanf_nullbyte_char", "setvbuf_unget", "sigprocmask_internal",
    "sscanf_eof", "statvfs", "strverscmp", "syscall_sign_extend", "uselocale_0",
    "wcsncpy_read_overflow", "wcsstr_false_negative"};

/*
static:
pthread_cond 
pthread_cancel
pthread_tsd 
pthread_cancel_points 
pthread_robust_detach 
pthread_cancel_sem_wait
pthread_cond_smasher
pthread_condattr_setclock
pthread_exit_cancel
pthread_once_deadlock
pthread_rwlock_ebusy
putenv_doublefree
*/

char *dynamicArgv[] = {"./runtest.exe", "-w", "entry-dynamic.exe", "", 0};
char *dynamicList[] = {
    "argv", "basename", "clocale_mbfuncs", "clock_gettime", "crypt",
    "dirname", "dlopen", "env", "fdopen", "fnmatch", "fscanf",
    "fwscanf", "iconv_open", "inet_pton", "mbc", "memstream",
    "qsort", "random", "search_hsearch", "search_insque", "search_lsearch",
    "search_tsearch", "setjmp", "snprintf", "socket",
    "sscanf", "sscanf_long", "stat", "strftime", "string", "string_memcpy",
    "string_memmem", "string_memset", "string_strchr", "string_strcspn",
    "string_strstr", "strptime", "strtod", "strtod_simple", "strtof",
    "strtol", "strtold", "swprintf", "tgmath", "time", "udiv", 
    "ungetc", "utime", "wcsstr", "wcstol", "daemon_failure", "dn_expand_empty",
    "dn_expand_ptr_0", "fflush_exit", "fgets_eof", "fgetwc_buffering", "fpclassify_invalid_ld80",
    "ftello_unflushed_append", "getpwnam_r_crash", "getpwnam_r_errno", "iconv_roundtrips",
    "inet_ntop_v4mapped", "inet_pton_empty_last_field", "iswspace_null", "lrand48_signextend",
    "lseek_large", "malloc_0", "mbsrtowcs_overflow", "memmem_oob_read", "memmem_oob",
    "mkdtemp_failure", "mkstemp_failure", "printf_1e9_oob", "printf_fmt_g_round",
    "printf_fmt_g_zeros", "printf_fmt_n", "regex_backref_0", "regex_bracket_icase",
    "regex_ere_backref", "regex_escaped_high_byte", "regex_negated_range",
    "regexec_nosub", "rewind_clear_error", "rlimit_open_files", "scanf_bytes_consumed",
    "scanf_match_literal_eof", "scanf_nullbyte_char", "setvbuf_unget", "sigprocmask_internal",
    "sscanf_eof", "statvfs", "strverscmp", "syscall_sign_extend", "uselocale_0",
    "wcsncpy_read_overflow", "wcsstr_false_negative"
};

/*
dynamic:
pthread_cancel_points
pthread_cancel
pthread_cond
pthread_tsd
pthread_robust_detach
pthread_cond_smasher
pthread_condattr_setclock
pthread_exit_cancel
pthread_once_deadlock
pthread_rwlock_ebusy
putenv_doublefree
sem_init
tls_init
tls_local_exec
tls_get_new_dtv
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
    
    for (int i = 0; i < sizeof(dynamicList) / sizeof(char*); i++) {
        int pid = fork();
        if (pid == 0) {
            dynamicArgv[3] = dynamicList[i];
            exec("./runtest.exe", dynamicArgv);
        } else {
            wait(0);
        }
    }
    exit(0);
}
