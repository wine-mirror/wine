/*
 * Standard library definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDLIB_H
#define __WINE_STDLIB_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void*)0)
#endif
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif


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

#if defined(__x86_64__) && !defined(_WIN64)
#define _WIN64
#endif

#if !defined(_MSC_VER) && !defined(__int64)
# ifdef _WIN64
#   define __int64 long
# else
#   define __int64 long long
# endif
#endif

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


typedef struct _div_t {
    int quot;
    int rem;
} div_t;

typedef struct _ldiv_t {
    long quot;
    long rem;
} ldiv_t;

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

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


#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int*         __p__osver(void);
extern unsigned int*         __p__winver(void);
extern unsigned int*         __p__winmajor(void);
extern unsigned int*         __p__winminor(void);
#define _osver             (*__p__osver())
#define _winver            (*__p__winver())
#define _winmajor          (*__p__winmajor())
#define _winminor          (*__p__winminor())

extern int*                  __p___argc(void);
extern char***               __p___argv(void);
extern wchar_t***    __p___wargv(void);
extern char***               __p__environ(void);
extern wchar_t***    __p__wenviron(void);
extern int*                  __p___mb_cur_max(void);
extern unsigned long*        __doserrno(void);
extern unsigned int*         __p__fmode(void);
/* FIXME: We need functions to access these:
 * int _sys_nerr;
 * char** _sys_errlist;
 */
#define __argc             (*__p___argc())
#define __argv             (*__p___argv())
#define __wargv            (*__p___wargv())
#define _environ           (*__p__environ())
#define _wenviron          (*__p__wenviron())
#define __mb_cur_max       (*__p___mb_cur_max())
#define _doserrno          (*__doserrno())
#define _fmode             (*_fmode)


extern int*           _errno(void);
#define errno        (*_errno())


typedef int (*_onexit_t)(void);


int         _atodbl(_CRT_DOUBLE*,char*);
int         _atoflt(_CRT_FLOAT*,char*);
__int64     _atoi64(const char*);
long double _atold(const char*);
int         _atoldbl(_LDOUBLE*,char*);
void        _beep(unsigned int,unsigned int);
char*       _ecvt(double,int,int*,int*);
char*       _fcvt(double,int,int*,int*);
char*       _fullpath(char*,const char*,size_t);
char*       _gcvt(double,int,char*);
char*       _i64toa(__int64,char*,int);
char*       _itoa(int,char*,int);
char*       _ltoa(long,char*,int);
unsigned long _lrotl(unsigned long,int);
unsigned long _lrotr(unsigned long,int);
void        _makepath(char*,const char*,const char*,const char*,const char*);
size_t _mbstrlen(const char*);
_onexit_t _onexit(_onexit_t);
int         _putenv(const char*);
unsigned int _rotl(unsigned int,int);
unsigned int _rotr(unsigned int,int);
void        _searchenv(const char*,const char*,char*);
int         _set_error_mode(int);
void        _seterrormode(int);
void        _sleep(unsigned long);
void        _splitpath(const char*,char*,char*,char*,char*);
long double _strtold(const char*,char**);
void        _swab(char*,char*,int);
char*       _ui64toa(unsigned __int64,char*,int);
char*       _ultoa(unsigned long,char*,int);

void        _exit(int);
void        abort(void);
int         abs(int);
int         atexit(void (*)(void));
double      atof(const char*);
int         atoi(const char*);
long        atol(const char*);
void*       calloc(size_t,size_t);
#ifndef __i386__
div_t div(int,int);
ldiv_t ldiv(long,long);
#endif
void        exit(int);
void        free(void*);
char*       getenv(const char*);
long        labs(long);
void*       malloc(size_t);
int         mblen(const char*,size_t);
void        perror(const char*);
int         rand(void);
void*       realloc(void*,size_t);
void        srand(unsigned int);
double      strtod(const char*,char**);
long        strtol(const char*,char**,int);
unsigned long strtoul(const char*,char**,int);
int         system(const char*);
void*       bsearch(const void*,const void*,size_t,size_t,
                            int (*)(const void*,const void*));
void        qsort(void*,size_t,size_t,
                          int (*)(const void*,const void*));

#ifndef _WSTDLIB_DEFINED
#define _WSTDLIB_DEFINED
wchar_t*_itow(int,wchar_t*,int);
wchar_t*_i64tow(__int64,wchar_t*,int);
wchar_t*_ltow(long,wchar_t*,int);
wchar_t*_ui64tow(unsigned __int64,wchar_t*,int);
wchar_t*_ultow(unsigned long,wchar_t*,int);
wchar_t*_wfullpath(wchar_t*,const wchar_t*,size_t);
wchar_t*_wgetenv(const wchar_t*);
void            _wmakepath(wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
void            _wperror(const wchar_t*);
int             _wputenv(const wchar_t*);
void            _wsearchenv(const wchar_t*,const wchar_t*,wchar_t*);
void            _wsplitpath(const wchar_t*,wchar_t*,wchar_t*,wchar_t*,wchar_t*);
int             _wsystem(const wchar_t*);
int             _wtoi(const wchar_t*);
__int64         _wtoi64(const wchar_t*);
long            _wtol(const wchar_t*);

size_t mbstowcs(wchar_t*,const char*,size_t);
int            mbtowc(wchar_t*,const char*,size_t);
double         wcstod(const wchar_t*,wchar_t**);
long           wcstol(const wchar_t*,wchar_t**,int);
size_t wcstombs(char*,const wchar_t*,size_t);
unsigned long  wcstoul(const wchar_t*,wchar_t**,int);
int            wctomb(char*,wchar_t);
#endif /* _WSTDLIB_DEFINED */

#ifdef __cplusplus
}
#endif


#define environ _environ
#define onexit_t _onexit_t

static inline char* ecvt(double value, int ndigit, int* decpt, int* sign) { return _ecvt(value, ndigit, decpt, sign); }
static inline char* fcvt(double value, int ndigit, int* decpt, int* sign) { return _fcvt(value, ndigit, decpt, sign); }
static inline char* gcvt(double value, int ndigit, char* buf) { return _gcvt(value, ndigit, buf); }
static inline char* itoa(int value, char* str, int radix) { return _itoa(value, str, radix); }
static inline char* ltoa(long value, char* str, int radix) { return _ltoa(value, str, radix); }
static inline _onexit_t onexit(_onexit_t func) { return _onexit(func); }
static inline int putenv(const char* str) { return _putenv(str); }
static inline void swab(char* src, char* dst, int len) { _swab(src, dst, len); }
static inline char* ultoa(unsigned long value, char* str, int radix) { return _ultoa(value, str, radix); }

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
static inline ldiv_t __wine_msvcrt_ldiv(long num, long denom)
{
    extern unsigned __int64 ldiv(long,long);
    ldiv_t ret;
    unsigned __int64 res = ldiv(num,denom);
    ret.quot = (long)res;
    ret.rem  = (long)(res >> 32);
    return ret;
}
#define div(num,denom) __wine_msvcrt_div(num,denom)
#define ldiv(num,denom) __wine_msvcrt_ldiv(num,denom)
#endif

#include <poppack.h>

#endif /* __WINE_STDLIB_H */
