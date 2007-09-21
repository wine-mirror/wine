/*
 * Copyright 2001 Jon Griffiths
 * Copyright 2004 Dimitrie O. Paun
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *   Naming conventions
 *	- Symbols are prefixed with MSVCRT_ if they conflict
 *        with libc symbols
 *      - Internal symbols are usually prefixed by msvcrt_.
 *      - Exported symbols that are not present in the public
 *        headers are usually kept the same as the original.
 *   Other conventions
 *      - To avoid conflicts with the standard C library,
 *        no msvcrt headers are included in the implementation.
 *      - Instead, symbols are duplicated here, prefixed with 
 *        MSVCRT_, as explained above.
 *      - To avoid inconsistencies, a test for each symbol is
 *        added into tests/headers.c. Please always add a
 *        corresponding test when you add a new symbol!
 */

#ifndef __WINE_MSVCRT_H
#define __WINE_MSVCRT_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

typedef unsigned short MSVCRT_wchar_t;
typedef unsigned short MSVCRT_wint_t;
typedef unsigned short MSVCRT_wctype_t;
typedef unsigned short MSVCRT__ino_t;
typedef unsigned long  MSVCRT__fsize_t;
#ifdef _WIN64
typedef unsigned __int64 MSVCRT_size_t;
typedef __int64 MSVCRT_intptr_t;
typedef unsigned __int64 MSVCRT_uintptr_t;
#else
typedef unsigned int MSVCRT_size_t;
typedef int MSVCRT_intptr_t;
typedef unsigned int MSVCRT_uintptr_t;
#endif
typedef unsigned int   MSVCRT__dev_t;
typedef int  MSVCRT__off_t;
typedef long MSVCRT_clock_t;
typedef long MSVCRT_time_t;
typedef __int64 MSVCRT___time64_t;
typedef __int64 MSVCRT_fpos_t;

typedef void (*MSVCRT_terminate_handler)(void);
typedef void (*MSVCRT_terminate_function)(void);
typedef void (*MSVCRT_unexpected_handler)(void);
typedef void (*MSVCRT_unexpected_function)(void);
typedef void (*MSVCRT__se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);
typedef void (*MSVCRT__beginthread_start_routine_t)(void *);
typedef unsigned int (__stdcall *MSVCRT__beginthreadex_start_routine_t)(void *);
typedef int (*MSVCRT__onexit_t)(void);

typedef struct {long double x;} MSVCRT__LDOUBLE;

struct MSVCRT_tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};


/* TLS data */
extern DWORD msvcrt_tls_index;

struct __thread_data {
    int                             thread_errno;
    unsigned long                   thread_doserrno;
    unsigned int                    random_seed;        /* seed for rand() */
    char                           *strtok_next;        /* next ptr for strtok() */
    unsigned char                  *mbstok_next;        /* next ptr for mbstok() */
    MSVCRT_wchar_t                 *wcstok_next;        /* next ptr for wcstok() */
    char                           *efcvt_buffer;       /* buffer for ecvt/fcvt */
    char                           *asctime_buffer;     /* buffer for asctime */
    MSVCRT_wchar_t                 *wasctime_buffer;    /* buffer for wasctime */
    struct MSVCRT_tm                time_buffer;        /* buffer for localtime/gmtime */
    char                           *strerror_buffer;    /* buffer for strerror */
    int                             fpecode;
    MSVCRT_terminate_function       terminate_handler;
    MSVCRT_unexpected_function      unexpected_handler;
    MSVCRT__se_translator_function  se_translator;
    EXCEPTION_RECORD               *exc_record;
};

typedef struct __thread_data thread_data_t;

extern thread_data_t *msvcrt_get_thread_data(void);

extern int MSVCRT___lc_codepage;

void   msvcrt_set_errno(int);

void   _purecall(void);
void   _amsg_exit(int errnum);

extern char **_environ;
extern MSVCRT_wchar_t **_wenviron;

extern char ** msvcrt_SnapshotOfEnvironmentA(char **);
extern MSVCRT_wchar_t ** msvcrt_SnapshotOfEnvironmentW(MSVCRT_wchar_t **);

/* FIXME: This should be declared in new.h but it's not an extern "C" so
 * it would not be much use anyway. Even for Winelib applications.
 */
int    MSVCRT__set_new_mode(int mode);

void* MSVCRT_operator_new(unsigned long size);
void  MSVCRT_operator_delete(void*);

typedef void* (*malloc_func_t)(MSVCRT_size_t);
typedef void  (*free_func_t)(void*);

extern char* __unDName(char *,const char*,int,malloc_func_t,free_func_t,unsigned short int);
extern char* __unDNameEx(char *,const char*,int,malloc_func_t,free_func_t,void *,unsigned short int);

/* Setup and teardown multi threaded locks */
extern void msvcrt_init_mt_locks(void);
extern void msvcrt_free_mt_locks(void);

extern void msvcrt_init_io(void);
extern void msvcrt_free_io(void);
extern void msvcrt_init_console(void);
extern void msvcrt_free_console(void);
extern void msvcrt_init_args(void);
extern void msvcrt_free_args(void);
extern void msvcrt_init_signals(void);
extern void msvcrt_free_signals(void);

extern unsigned msvcrt_create_io_inherit_block(STARTUPINFOA*);

/* run-time error codes */
#define _RT_STACK       0
#define _RT_NULLPTR     1
#define _RT_FLOAT       2
#define _RT_INTDIV      3
#define _RT_EXECMEM     5
#define _RT_EXECFORM    6
#define _RT_EXECENV     7
#define _RT_SPACEARG    8
#define _RT_SPACEENV    9
#define _RT_ABORT       10
#define _RT_NPTR        12
#define _RT_FPTR        13
#define _RT_BREAK       14
#define _RT_INT         15
#define _RT_THREAD      16
#define _RT_LOCK        17
#define _RT_HEAP        18
#define _RT_OPENCON     19
#define _RT_QWIN        20
#define _RT_NOMAIN      21
#define _RT_NONCONT     22
#define _RT_INVALDISP   23
#define _RT_ONEXIT      24
#define _RT_PUREVIRT    25
#define _RT_STDIOINIT   26
#define _RT_LOWIOINIT   27
#define _RT_HEAPINIT    28
#define _RT_DOMAIN      120
#define _RT_SING        121
#define _RT_TLOSS       122
#define _RT_CRNL        252
#define _RT_BANNER      255

struct MSVCRT__timeb {
    MSVCRT_time_t  time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
};

struct MSVCRT__iobuf {
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file;
  int   _charbuf;
  int   _bufsiz;
  char* _tmpfname;
};

typedef struct MSVCRT__iobuf MSVCRT_FILE;

struct MSVCRT_lconv {
    char* decimal_point;
    char* thousands_sep;
    char* grouping;
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
};

struct MSVCRT__exception {
  int     type;
  char*   name;
  double  arg1;
  double  arg2;
  double  retval;
};

struct MSVCRT__complex {
  double x;      /* Real part */
  double y;      /* Imaginary part */
};

typedef struct MSVCRT__div_t {
    int quot;  /* quotient */
    int rem;   /* remainder */
} MSVCRT_div_t;

typedef struct MSVCRT__ldiv_t {
    long quot;  /* quotient */
    long rem;   /* remainder */
} MSVCRT_ldiv_t;

struct MSVCRT__heapinfo {
  int*           _pentry;
  MSVCRT_size_t  _size;
  int            _useflag;
};

#ifdef __i386__
struct MSVCRT___JUMP_BUFFER {
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    /* Start of new struct members */
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
};
#endif /* __i386__ */

struct MSVCRT__diskfree_t {
  unsigned int total_clusters;
  unsigned int avail_clusters;
  unsigned int sectors_per_cluster;
  unsigned int bytes_per_sector;
};

struct MSVCRT__finddata_t {
  unsigned attrib;
  MSVCRT_time_t   time_create;
  MSVCRT_time_t   time_access;
  MSVCRT_time_t   time_write;
  MSVCRT__fsize_t size;
  char            name[260];
};

struct MSVCRT__finddatai64_t {
  unsigned attrib;
  MSVCRT_time_t  time_create;
  MSVCRT_time_t  time_access;
  MSVCRT_time_t  time_write;
  __int64        size;
  char           name[260];
};

struct MSVCRT__wfinddata_t {
  unsigned attrib;
  MSVCRT_time_t   time_create;
  MSVCRT_time_t   time_access;
  MSVCRT_time_t   time_write;
  MSVCRT__fsize_t size;
  MSVCRT_wchar_t  name[260];
};

struct MSVCRT__wfinddatai64_t {
  unsigned attrib;
  MSVCRT_time_t   time_create;
  MSVCRT_time_t   time_access;
  MSVCRT_time_t   time_write;
  __int64         size;
  MSVCRT_wchar_t  name[260];
};

struct MSVCRT__utimbuf
{
    MSVCRT_time_t actime;
    MSVCRT_time_t modtime;
};

/* for FreeBSD */
#undef st_atime
#undef st_ctime
#undef st_mtime

struct MSVCRT__stat {
  MSVCRT__dev_t  st_dev;
  MSVCRT__ino_t  st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  MSVCRT__dev_t  st_rdev;
  MSVCRT__off_t  st_size;
  MSVCRT_time_t  st_atime;
  MSVCRT_time_t  st_mtime;
  MSVCRT_time_t  st_ctime;
};

struct MSVCRT_stat {
  MSVCRT__dev_t  st_dev;
  MSVCRT__ino_t  st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  MSVCRT__dev_t  st_rdev;
  MSVCRT__off_t  st_size;
  MSVCRT_time_t  st_atime;
  MSVCRT_time_t  st_mtime;
  MSVCRT_time_t  st_ctime;
};

struct MSVCRT__stati64 {
  MSVCRT__dev_t  st_dev;
  MSVCRT__ino_t  st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  MSVCRT__dev_t  st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  MSVCRT_time_t  st_atime;
  MSVCRT_time_t  st_mtime;
  MSVCRT_time_t  st_ctime;
};

struct MSVCRT__stat64 {
  MSVCRT__dev_t     st_dev;
  MSVCRT__ino_t     st_ino;
  unsigned short    st_mode;
  short             st_nlink;
  short             st_uid;
  short             st_gid;
  MSVCRT__dev_t     st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  MSVCRT___time64_t st_atime;
  MSVCRT___time64_t st_mtime;
  MSVCRT___time64_t st_ctime;
};

#define MSVCRT_WEOF (MSVCRT_wint_t)(0xFFFF)
#define MSVCRT_EOF       (-1)
#define MSVCRT_TMP_MAX   0x7fff
#define MSVCRT_RAND_MAX  0x7fff
#define MSVCRT_BUFSIZ    512

#define MSVCRT_STDIN_FILENO  0
#define MSVCRT_STDOUT_FILENO 1
#define MSVCRT_STDERR_FILENO 2

/* more file._flag flags, but these conflict with Unix */
#define MSVCRT__IOFBF    0x0000
#define MSVCRT__IONBF    0x0004
#define MSVCRT__IOLBF    0x0040

#define MSVCRT_FILENAME_MAX 260
#define MSVCRT_stdin       (MSVCRT__iob+MSVCRT_STDIN_FILENO)
#define MSVCRT_stdout      (MSVCRT__iob+MSVCRT_STDOUT_FILENO)
#define MSVCRT_stderr      (MSVCRT__iob+MSVCRT_STDERR_FILENO)

#define MSVCRT__P_WAIT    0
#define MSVCRT__P_NOWAIT  1
#define MSVCRT__P_OVERLAY 2
#define MSVCRT__P_NOWAITO 3
#define MSVCRT__P_DETACH  4

#define MSVCRT_EPERM   1
#define MSVCRT_ENOENT  2
#define MSVCRT_ESRCH   3
#define MSVCRT_EINTR   4
#define MSVCRT_EIO     5
#define MSVCRT_ENXIO   6
#define MSVCRT_E2BIG   7
#define MSVCRT_ENOEXEC 8
#define MSVCRT_EBADF   9
#define MSVCRT_ECHILD  10
#define MSVCRT_EAGAIN  11
#define MSVCRT_ENOMEM  12
#define MSVCRT_EACCES  13
#define MSVCRT_EFAULT  14
#define MSVCRT_EBUSY   16
#define MSVCRT_EEXIST  17
#define MSVCRT_EXDEV   18
#define MSVCRT_ENODEV  19
#define MSVCRT_ENOTDIR 20
#define MSVCRT_EISDIR  21
#define MSVCRT_EINVAL  22
#define MSVCRT_ENFILE  23
#define MSVCRT_EMFILE  24
#define MSVCRT_ENOTTY  25
#define MSVCRT_EFBIG   27
#define MSVCRT_ENOSPC  28
#define MSVCRT_ESPIPE  29
#define MSVCRT_EROFS   30
#define MSVCRT_EMLINK  31
#define MSVCRT_EPIPE   32
#define MSVCRT_EDOM    33
#define MSVCRT_ERANGE  34
#define MSVCRT_EDEADLK 36
#define MSVCRT_EDEADLOCK MSVCRT_EDEADLK
#define MSVCRT_ENAMETOOLONG 38
#define MSVCRT_ENOLCK  39
#define MSVCRT_ENOSYS  40
#define MSVCRT_ENOTEMPTY 41
#define MSVCRT_EILSEQ    42

#define MSVCRT_LC_ALL          0
#define MSVCRT_LC_COLLATE      1
#define MSVCRT_LC_CTYPE        2
#define MSVCRT_LC_MONETARY     3
#define MSVCRT_LC_NUMERIC      4
#define MSVCRT_LC_TIME         5
#define MSVCRT_LC_MIN          MSVCRT_LC_ALL
#define MSVCRT_LC_MAX          MSVCRT_LC_TIME

#define MSVCRT__HEAPEMPTY      -1
#define MSVCRT__HEAPOK         -2
#define MSVCRT__HEAPBADBEGIN   -3
#define MSVCRT__HEAPBADNODE    -4
#define MSVCRT__HEAPEND        -5
#define MSVCRT__HEAPBADPTR     -6

#define MSVCRT__FREEENTRY      0
#define MSVCRT__USEDENTRY      1

#define MSVCRT__OUT_TO_DEFAULT 0
#define MSVCRT__REPORT_ERRMODE 3

/* ASCII char classification table - binary compatible */
#define MSVCRT__UPPER    0x0001  /* C1_UPPER */
#define MSVCRT__LOWER    0x0002  /* C1_LOWER */
#define MSVCRT__DIGIT    0x0004  /* C1_DIGIT */
#define MSVCRT__SPACE    0x0008  /* C1_SPACE */
#define MSVCRT__PUNCT    0x0010  /* C1_PUNCT */
#define MSVCRT__CONTROL  0x0020  /* C1_CNTRL */
#define MSVCRT__BLANK    0x0040  /* C1_BLANK */
#define MSVCRT__HEX      0x0080  /* C1_XDIGIT */
#define MSVCRT__LEADBYTE 0x8000
#define MSVCRT__ALPHA   (0x0100|MSVCRT__UPPER|MSVCRT__LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

#define MSVCRT__IOREAD   0x0001
#define MSVCRT__IOWRT    0x0002
#define MSVCRT__IOMYBUF  0x0008
#define MSVCRT__IOEOF    0x0010
#define MSVCRT__IOERR    0x0020
#define MSVCRT__IOSTRG   0x0040
#define MSVCRT__IORW     0x0080

#define MSVCRT__S_IEXEC  0x0040
#define MSVCRT__S_IWRITE 0x0080
#define MSVCRT__S_IREAD  0x0100
#define MSVCRT__S_IFIFO  0x1000
#define MSVCRT__S_IFCHR  0x2000
#define MSVCRT__S_IFDIR  0x4000
#define MSVCRT__S_IFREG  0x8000
#define MSVCRT__S_IFMT   0xF000

#define MSVCRT__LK_UNLCK  0
#define MSVCRT__LK_LOCK   1
#define MSVCRT__LK_NBLCK  2
#define MSVCRT__LK_RLCK   3
#define MSVCRT__LK_NBRLCK 4

#define	MSVCRT__SH_COMPAT	0x00	/* Compatibility */
#define	MSVCRT__SH_DENYRW	0x10	/* Deny read/write */
#define	MSVCRT__SH_DENYWR	0x20	/* Deny write */
#define	MSVCRT__SH_DENYRD	0x30	/* Deny read */
#define	MSVCRT__SH_DENYNO	0x40	/* Deny nothing */

#define MSVCRT__O_RDONLY        0
#define MSVCRT__O_WRONLY        1
#define MSVCRT__O_RDWR          2
#define MSVCRT__O_ACCMODE       (MSVCRT__O_RDONLY|MSVCRT__O_WRONLY|MSVCRT__O_RDWR)
#define MSVCRT__O_APPEND        0x0008
#define MSVCRT__O_RANDOM        0x0010
#define MSVCRT__O_SEQUENTIAL    0x0020
#define MSVCRT__O_TEMPORARY     0x0040
#define MSVCRT__O_NOINHERIT     0x0080
#define MSVCRT__O_CREAT         0x0100
#define MSVCRT__O_TRUNC         0x0200
#define MSVCRT__O_EXCL          0x0400
#define MSVCRT__O_SHORT_LIVED   0x1000
#define MSVCRT__O_TEXT          0x4000
#define MSVCRT__O_BINARY        0x8000
#define MSVCRT__O_RAW           MSVCRT__O_BINARY

/* _statusfp bit flags */
#define MSVCRT__SW_INEXACT      0x00000001 /* inexact (precision) */
#define MSVCRT__SW_UNDERFLOW    0x00000002 /* underflow */
#define MSVCRT__SW_OVERFLOW     0x00000004 /* overflow */
#define MSVCRT__SW_ZERODIVIDE   0x00000008 /* zero divide */
#define MSVCRT__SW_INVALID      0x00000010 /* invalid */

#define MSVCRT__SW_UNEMULATED     0x00000040  /* unemulated instruction */
#define MSVCRT__SW_SQRTNEG        0x00000080  /* square root of a neg number */
#define MSVCRT__SW_STACKOVERFLOW  0x00000200  /* FP stack overflow */
#define MSVCRT__SW_STACKUNDERFLOW 0x00000400  /* FP stack underflow */

#define MSVCRT__SW_DENORMAL     0x00080000 /* denormal status bit */

/* fpclass constants */
#define MSVCRT__FPCLASS_SNAN 0x0001  /* Signaling "Not a Number" */
#define MSVCRT__FPCLASS_QNAN 0x0002  /* Quiet "Not a Number" */
#define MSVCRT__FPCLASS_NINF 0x0004  /* Negative Infinity */
#define MSVCRT__FPCLASS_NN   0x0008  /* Negative Normal */
#define MSVCRT__FPCLASS_ND   0x0010  /* Negative Denormal */
#define MSVCRT__FPCLASS_NZ   0x0020  /* Negative Zero */
#define MSVCRT__FPCLASS_PZ   0x0040  /* Positive Zero */
#define MSVCRT__FPCLASS_PD   0x0080  /* Positive Denormal */
#define MSVCRT__FPCLASS_PN   0x0100  /* Positive Normal */
#define MSVCRT__FPCLASS_PINF 0x0200  /* Positive Infinity */

#define MSVCRT__EM_INVALID    0x00000010
#define MSVCRT__EM_DENORMAL   0x00080000
#define MSVCRT__EM_ZERODIVIDE 0x00000008
#define MSVCRT__EM_OVERFLOW   0x00000004
#define MSVCRT__EM_UNDERFLOW  0x00000002
#define MSVCRT__EM_INEXACT    0x00000001
#define MSVCRT__IC_AFFINE     0x00040000
#define MSVCRT__IC_PROJECTIVE 0x00000000
#define MSVCRT__RC_CHOP       0x00000300
#define MSVCRT__RC_UP         0x00000200
#define MSVCRT__RC_DOWN       0x00000100
#define MSVCRT__RC_NEAR       0x00000000
#define MSVCRT__PC_24         0x00020000
#define MSVCRT__PC_53         0x00010000
#define MSVCRT__PC_64         0x00000000

#define MSVCRT_CLOCKS_PER_SEC 1000

/* signals */
#define MSVCRT_SIGINT   2
#define MSVCRT_SIGILL   4
#define MSVCRT_SIGFPE   8
#define MSVCRT_SIGSEGV  11
#define MSVCRT_SIGTERM  15
#define MSVCRT_SIGBREAK 21
#define MSVCRT_SIGABRT  22
#define MSVCRT_NSIG     (MSVCRT_SIGABRT + 1)

typedef void (*MSVCRT___sighandler_t)(int);

#define MSVCRT_SIG_DFL ((MSVCRT___sighandler_t)0)
#define MSVCRT_SIG_IGN ((MSVCRT___sighandler_t)1)
#define MSVCRT_SIG_ERR ((MSVCRT___sighandler_t)-1)

void           MSVCRT_free(void*);
void*          MSVCRT_malloc(MSVCRT_size_t);
void*          MSVCRT_calloc(MSVCRT_size_t,MSVCRT_size_t);
void*          MSVCRT_realloc(void*,MSVCRT_size_t);

int            MSVCRT_iswalpha(MSVCRT_wint_t);
int            MSVCRT_iswspace(MSVCRT_wint_t);
int            MSVCRT_iswdigit(MSVCRT_wint_t);
int            MSVCRT_isleadbyte(int);

int            MSVCRT_fgetc(MSVCRT_FILE*);
int            MSVCRT_ungetc(int,MSVCRT_FILE*);
MSVCRT_wint_t  MSVCRT_fgetwc(MSVCRT_FILE*);
MSVCRT_wint_t  MSVCRT_ungetwc(MSVCRT_wint_t,MSVCRT_FILE*);
void           MSVCRT__exit(int);
void           MSVCRT_abort(void);
unsigned long* MSVCRT___doserrno(void);
int*           MSVCRT__errno(void);
char*          MSVCRT_getenv(const char*);
char*          MSVCRT_setlocale(int,const char*);
int            MSVCRT_fclose(MSVCRT_FILE*);
void           MSVCRT_terminate(void);
MSVCRT_FILE*   MSVCRT__p__iob(void);
MSVCRT_time_t  MSVCRT_mktime(struct MSVCRT_tm *t);
struct MSVCRT_tm* MSVCRT_localtime(const MSVCRT_time_t* secs);
struct MSVCRT_tm* MSVCRT_gmtime(const MSVCRT_time_t* secs);
MSVCRT_clock_t MSVCRT_clock(void);
double         MSVCRT_difftime(MSVCRT_time_t time1, MSVCRT_time_t time2);
MSVCRT_time_t  MSVCRT_time(MSVCRT_time_t*);
MSVCRT_FILE*   MSVCRT__fdopen(int, const char *);
int            MSVCRT_vsnprintf(char *str, unsigned int len, const char *format, va_list valist);
int            MSVCRT_vsnwprintf(MSVCRT_wchar_t *str, unsigned int len,
                                 const MSVCRT_wchar_t *format, va_list valist );
int            MSVCRT_raise(int sig);

#ifndef __WINE_MSVCRT_TEST
int            MSVCRT__write(int,const void*,unsigned int);
int            _getch(void);
int            _ismbstrail(const unsigned char* start, const unsigned char* str);
MSVCRT_intptr_t _spawnve(int,const char*,const char* const *,const char* const *);
void           _searchenv(const char*,const char*,char*);
int            _getdrive(void);
char*          _strdup(const char*);
char*          _strnset(char*,int,MSVCRT_size_t);
char*          _strset(char*,int);
int            _ungetch(int);
int            _cputs(const char*);
int            _cprintf(const char*,...);
char***        __p__environ(void);
int*           __p___mb_cur_max(void);
unsigned int*  __p__fmode(void);
MSVCRT_wchar_t*   _wcsdup(const MSVCRT_wchar_t*);
MSVCRT_wchar_t*** __p__wenviron(void);
char*         _strdate(char* date);
char*         _strtime(char* date);
void          _ftime(struct MSVCRT__timeb *buf);
int           MSVCRT__close(int);
int           MSVCRT__dup(int);
int           MSVCRT__dup2(int, int);
int           MSVCRT__pipe(int *, unsigned int, int);
MSVCRT_wchar_t* _wgetenv(const MSVCRT_wchar_t*);
#endif

#endif /* __WINE_MSVCRT_H */
