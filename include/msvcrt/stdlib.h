/*
 * Standard library definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDLIB_H
#define __WINE_STDLIB_H

#include <corecrt_wstdlib.h>

#include <pshpack8.h>

typedef struct
{
    float f;
} _CRT_FLOAT;

typedef struct
{
    double x;
} _CRT_DOUBLE;

typedef struct
{
    unsigned char ld[10];
} _LDOUBLE;

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        -1
#define RAND_MAX            0x7FFF

#ifndef _MAX_PATH
#define _MAX_DRIVE          3
#define _MAX_FNAME          256
#define _MAX_DIR            _MAX_FNAME
#define _MAX_EXT            _MAX_FNAME
#define _MAX_PATH           260
#endif

/* Make the secure string functions (names end in "_s") truncate their output */
#define _TRUNCATE  ((size_t)-1)

typedef struct _div_t {
    int quot;
    int rem;
} div_t;

typedef struct _ldiv_t {
    __msvcrt_long quot;
    __msvcrt_long rem;
} ldiv_t;

typedef struct _lldiv_t {
    __int64 quot;
    __int64 rem;
} lldiv_t;


#define _countof(x) (sizeof(x)/sizeof((x)[0]))

#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#ifndef __cplusplus
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

/* _set_error_mode() constants */
#define _OUT_TO_DEFAULT      0
#define _OUT_TO_STDERR       1
#define _OUT_TO_MSGBOX       2
#define _REPORT_ERRMODE      3

/* _set_abort_behavior codes */
#define _WRITE_ABORT_MSG     1
#define _CALL_REPORTFAULT    2

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__i386__) || defined(_UCRT)

extern unsigned int* __cdecl __p__osver(void);
#define _osver             (*__p__osver())
extern unsigned int* __cdecl __p__winver(void);
#define _winver            (*__p__winver())
extern unsigned int* __cdecl __p__winmajor(void);
#define _winmajor          (*__p__winmajor())
extern unsigned int* __cdecl __p__winminor(void);
#define _winminor          (*__p__winminor())
extern int*          __cdecl __p___argc(void);
#define __argc             (*__p___argc())
extern char***       __cdecl __p___argv(void);
#define __argv             (*__p___argv())
extern wchar_t***    __cdecl __p___wargv(void);
#define __wargv            (*__p___wargv())
extern char***       __cdecl __p__environ(void);
#define _environ           (*__p__environ())
extern wchar_t***    __cdecl __p__wenviron(void);
#define _wenviron          (*__p__wenviron())
extern int*          __cdecl __p__fmode(void);
#define _fmode             (*__p__fmode())

#else  /* __i386__ */

extern unsigned int _osver;
extern unsigned int _winver;
extern unsigned int _winmajor;
extern unsigned int _winminor;
extern int __argc;
extern char **__argv;
extern wchar_t **__wargv;
extern char **_environ;
extern wchar_t **_wenviron;
extern unsigned int _fmode;

#endif  /* __i386__ */

extern int            __cdecl ___mb_cur_max_func(void);
#define __mb_cur_max        ___mb_cur_max_func()
extern __msvcrt_ulong* __cdecl __doserrno(void);
#define _doserrno           (*__doserrno())
extern int*           __cdecl _errno(void);
#define errno               (*_errno())

/* FIXME: We need functions to access these:
 * int _sys_nerr;
 * char** _sys_errlist;
 */

errno_t       __cdecl _get_doserrno(int*);
errno_t       __cdecl _get_errno(int*);
errno_t       __cdecl _set_doserrno(int);
errno_t       __cdecl _set_errno(int);

#ifndef _CRT_ONEXIT_T_DEFINED
#define _CRT_ONEXIT_T_DEFINED
typedef int (__cdecl *_onexit_t)(void);
#endif


int           __cdecl _atodbl(_CRT_DOUBLE*,char*);
int           __cdecl _atodbl_l(_CRT_DOUBLE*,char*,_locale_t);
int           __cdecl _atoflt(_CRT_FLOAT*,char*);
int           __cdecl _atoflt_l(_CRT_FLOAT*,char*,_locale_t);
__int64       __cdecl _atoi64(const char*);
long double   __cdecl _atold(const char*);
int           __cdecl _atoldbl(_LDOUBLE*,char*);
void          __cdecl _beep(unsigned int,unsigned int);
char*         __cdecl _ecvt(double,int,int*,int*);
char*         __cdecl _fcvt(double,int,int*,int*);
char*         __cdecl _fullpath(char*,const char*,size_t);
char*         __cdecl _gcvt(double,int,char*);
errno_t       __cdecl _gcvt_s(char*, size_t, double, int);
char*         __cdecl _i64toa(__int64,char*,int);
errno_t       __cdecl _i64toa_s(__int64, char*, size_t, int);
char*         __cdecl _itoa(int,char*,int);
errno_t       __cdecl _itoa_s(int,char*,size_t,int);
char*         __cdecl _ltoa(__msvcrt_long,char*,int);
errno_t       __cdecl _ltoa_s(__msvcrt_long, char*, size_t, int);
__msvcrt_ulong __cdecl _lrotl(__msvcrt_ulong,int);
__msvcrt_ulong __cdecl _lrotr(__msvcrt_ulong,int);
void          __cdecl _makepath(char*,const char*,const char*,const char*,const char*);
int           __cdecl _makepath_s(char*,size_t,const char*,const char*,const char*,const char*);
size_t        __cdecl _mbstrlen(const char*);
_onexit_t     __cdecl _onexit(_onexit_t);
int           __cdecl _putenv(const char*);
unsigned int  __cdecl _rotl(unsigned int,int);
unsigned int  __cdecl _rotr(unsigned int,int);
void          __cdecl _searchenv(const char*,const char*,char*);
int           __cdecl _set_error_mode(int);
void          __cdecl _seterrormode(int);
void          __cdecl _sleep(__msvcrt_ulong);
void          __cdecl _splitpath(const char*,char*,char*,char*,char*);
void          __cdecl _swab(char*,char*,int);
char*         __cdecl _ui64toa(unsigned __int64,char*,int);
errno_t       __cdecl _ui64toa_s(unsigned __int64,char*,size_t,int);
char*         __cdecl _ultoa(__msvcrt_ulong,char*,int);
errno_t       __cdecl _ultoa_s(__msvcrt_ulong,char*,size_t,int);

_ACRTIMP DECLSPEC_NORETURN void __cdecl _Exit(int);
_ACRTIMP DECLSPEC_NORETURN void __cdecl _exit(int);
_ACRTIMP DECLSPEC_NORETURN void __cdecl abort(void);
int           __cdecl abs(int);
int           __cdecl atexit(void (__cdecl *)(void));
double        __cdecl atof(const char*);
int           __cdecl atoi(const char*);
int           __cdecl _atoi_l(const char*,_locale_t);
__msvcrt_long __cdecl atol(const char*);
__int64       __cdecl atoll(const char*);
void*         __cdecl calloc(size_t,size_t);
#ifndef __i386__
div_t  __cdecl div(int,int);
ldiv_t __cdecl ldiv(__msvcrt_long,__msvcrt_long);
#endif
lldiv_t       __cdecl lldiv(__int64,__int64);
_ACRTIMP DECLSPEC_NORETURN void __cdecl exit(int);
void          __cdecl free(void*);
char*         __cdecl getenv(const char*);
__msvcrt_long __cdecl labs(__msvcrt_long);
__int64       __cdecl llabs(__int64);
void*         __cdecl malloc(size_t);
int           __cdecl mblen(const char*,size_t);
void          __cdecl perror(const char*);
int           __cdecl rand(void);
errno_t       __cdecl rand_s(unsigned int*);
void*         __cdecl realloc(void*,size_t);
void          __cdecl srand(unsigned int);
float         __cdecl strtof(const char*,char**);
double        __cdecl strtod(const char*,char**);
__msvcrt_long __cdecl strtol(const char*,char**,int);
__msvcrt_ulong __cdecl strtoul(const char*,char**,int);
__int64       __cdecl strtoll_l(const char*,char**,int,_locale_t);
unsigned __int64 __cdecl strtoull_l(const char*,char**,int,_locale_t);
__int64       __cdecl strtoimax(const char*,char**,int);
__int64       __cdecl strtoimax_l(const char*,char**,int,_locale_t);
unsigned __int64 __cdecl strtoumax(const char*,char**,int);
unsigned __int64 __cdecl strtoumax_l(const char*,char**,int,_locale_t);
__int64       __cdecl _strtoi64(const char*,char**,int);
__int64       __cdecl _strtoi64_l(const char*,char**,int,_locale_t);
unsigned __int64 __cdecl _strtoui64(const char*,char**,int);
unsigned __int64 __cdecl _strtoui64_l(const char*,char**,int,_locale_t);
int           __cdecl system(const char*);
void*         __cdecl bsearch(const void*,const void*,size_t,size_t,int (__cdecl *)(const void*,const void*));
void          __cdecl qsort(void*,size_t,size_t,int (__cdecl *)(const void*,const void*));

typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t*, const wchar_t*, const wchar_t*, unsigned, uintptr_t);
_invalid_parameter_handler __cdecl _set_invalid_parameter_handler(_invalid_parameter_handler);
_invalid_parameter_handler __cdecl _get_invalid_parameter_handler(void);
_ACRTIMP _invalid_parameter_handler __cdecl _get_thread_local_invalid_parameter_handler(void);
_ACRTIMP _invalid_parameter_handler __cdecl _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
void __cdecl _invalid_parameter(const wchar_t *expr, const wchar_t *func, const wchar_t *file,
                                unsigned int line, uintptr_t arg);

#ifdef _UCRT
_ACRTIMP double __cdecl _strtold_l(const char*,char**,_locale_t);
static inline long double strtold(const char *string, char **endptr) { return _strtold_l(string, endptr, NULL); }
#endif /* _UCRT */

#ifdef __cplusplus
}
#endif


#define environ _environ
#define onexit_t _onexit_t

static inline char* ecvt(double value, int ndigit, int* decpt, int* sign) { return _ecvt(value, ndigit, decpt, sign); }
static inline char* fcvt(double value, int ndigit, int* decpt, int* sign) { return _fcvt(value, ndigit, decpt, sign); }
static inline char* gcvt(double value, int ndigit, char* buf) { return _gcvt(value, ndigit, buf); }
static inline char* itoa(int value, char* str, int radix) { return _itoa(value, str, radix); }
static inline char* ltoa(__msvcrt_long value, char* str, int radix) { return _ltoa(value, str, radix); }
static inline _onexit_t onexit(_onexit_t func) { return _onexit(func); }
static inline int putenv(const char* str) { return _putenv(str); }
static inline __int64 strtoll(const char *ptr, char **endptr, int base) { return _strtoi64(ptr, endptr, base); }
static inline unsigned __int64 __cdecl strtoull(const char *ptr, char **endptr, int base) { return _strtoui64(ptr, endptr, base); }
static inline void swab(char* src, char* dst, int len) { _swab(src, dst, len); }
static inline char* ultoa(__msvcrt_ulong value, char* str, int radix) { return _ultoa(value, str, radix); }

#ifdef __i386__
static inline div_t __wine_msvcrt_div(int num, int denom)
{
    extern unsigned __int64 div(int,int);
    div_t ret;
    unsigned __int64 res = div(num,denom);
    ret.quot = (int)res;
    ret.rem  = (int)(res >> 32);
    return ret;
}
static inline ldiv_t __wine_msvcrt_ldiv(__msvcrt_long num, __msvcrt_long denom)
{
    extern unsigned __int64 ldiv(__msvcrt_long,__msvcrt_long);
    ldiv_t ret;
    unsigned __int64 res = ldiv(num,denom);
    ret.quot = (__msvcrt_long)res;
    ret.rem  = (__msvcrt_long)(res >> 32);
    return ret;
}
#define div(num,denom) __wine_msvcrt_div(num,denom)
#define ldiv(num,denom) __wine_msvcrt_ldiv(num,denom)
#endif

#include <poppack.h>

#endif /* __WINE_STDLIB_H */
