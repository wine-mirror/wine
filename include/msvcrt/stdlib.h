/*
 * Standard library definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDLIB_H
#define __WINE_STDLIB_H
#define __WINE_USE_MSVCRT

#include "winnt.h"
#include "msvcrt/malloc.h"                /* For size_t, malloc & co */
#include "msvcrt/search.h"                /* For bsearch and qsort */


#ifndef USE_MSVCRT_PREFIX
#define EXIT_SUCCESS        0
#define EXIT_FAILURE        -1
#define RAND_MAX            0x7FFF
#else
#define MSVCRT_RAND_MAX     0x7FFF
#endif /* USE_MSVCRT_PREFIX */

#ifndef _MAX_PATH
#define _MAX_DRIVE          3
#define _MAX_FNAME          256
#define _MAX_DIR            _MAX_FNAME
#define _MAX_EXT            _MAX_FNAME
#define _MAX_PATH           260
#endif


typedef struct MSVCRT(_div_t) {
    int quot;
    int rem;
} MSVCRT(div_t);

typedef struct MSVCRT(_ldiv_t) {
    long quot;
    long rem;
} MSVCRT(ldiv_t);


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

extern unsigned int*         __p__osver();
extern unsigned int*         __p__winver();
extern unsigned int*         __p__winmajor();
extern unsigned int*         __p__winminor();
#define _osver             (*__p__osver())
#define _winver            (*__p__winver())
#define _winmajor          (*__p__winmajor())
#define _winminor          (*__p__winminor())

extern int*                  __p___argc(void);
extern char***               __p___argv(void);
extern WCHAR***              __p___wargv(void);
extern char***               __p__environ(void);
extern WCHAR***              __p__wenviron(void);
extern int*                  __p___mb_cur_max(void);
extern unsigned long*        __doserrno(void);
extern unsigned int*         __p__fmode(void);
/* FIXME: We need functions to access these:
 * int _sys_nerr;
 * char** _sys_errlist;
 */
#ifndef USE_MSVCRT_PREFIX
#define __argc             (*__p___argc())
#define __argv             (*__p___argv())
#define __wargv            (*__p___wargv())
#define _environ           (*__p__environ())
#define _wenviron          (*__p__wenviron())
#define __mb_cur_max       (*__p___mb_cur_max())
#define _doserrno          (*__doserrno())
#define _fmode             (*_fmode)
#elif !defined(__WINE__)
#define MSVCRT___argc      (*__p___argc())
#define MSVCRT___argv      (*__p___argv())
#define MSVCRT___wargv     (*__p___wargv())
#define MSVCRT__environ    (*__p__environ())
#define MSVCRT__wenviron   (*__p__wenviron())
#define MSVCRT___mb_cur_max (*__p___mb_cur_max())
#define MSVCRT__doserrno   (*__doserrno())
#define MSVCRT__fmode      (*_fmode())
#endif /* USE_MSVCRT_PREFIX, __WINE__ */


extern int*           MSVCRT(_errno)(void);
#ifndef USE_MSVCRT_PREFIX
#define errno              (*_errno())
#elif !defined(__WINE__)
#define MSVCRT_errno       (*MSVCRT__errno())
#endif /* USE_MSVCRT_PREFIX, __WINE__ */


typedef int (*_onexit_t)(void);


__int64     _atoi64(const char*);
long double _atold(const char*);
void        _beep(unsigned int,unsigned int);
char*       _ecvt(double,int,int*,int*);
char*       _fcvt(double,int,int*,int*);
char*       _fullpath(char*,const char*,MSVCRT(size_t));
char*       _gcvt(double,int,char*);
char*       _i64toa(__int64,char*,int);
char*       _itoa(int,char*,int);
char*       _ltoa(long,char*,int);
unsigned long _lrotl(unsigned long,int);
unsigned long _lrotr(unsigned long,int);
void        _makepath(char*,const char*,const char*,const char*,const char*);
MSVCRT(size_t) _mbstrlen(const char*);
_onexit_t   _onexit(_onexit_t);
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

void        MSVCRT(_exit)(int);
void        MSVCRT(abort)();
int         MSVCRT(abs)(int);
int         MSVCRT(atexit)(void (*)(void));
double      MSVCRT(atof)(const char*);
int         MSVCRT(atoi)(const char*);
long        MSVCRT(atol)(const char*);
#ifdef __i386__
long long    MSVCRT(div)(int,int);
unsigned long long MSVCRT(ldiv)(long,long);
#else
MSVCRT(div_t) MSVCRT(div)(int,int);
MSVCRT(ldiv_t) MSVCRT(ldiv)(long,long);
#endif
void        MSVCRT(exit)(int);
char*       MSVCRT(getenv)(const char*);
long        MSVCRT(labs)(long);
int         MSVCRT(mblen)(const char*,MSVCRT(size_t));
void        MSVCRT(perror)(const char*);
int         MSVCRT(rand)(void);
void        MSVCRT(srand)(unsigned int);
double      MSVCRT(strtod)(const char*,char**);
long        MSVCRT(strtol)(const char*,char**,int);
unsigned long MSVCRT(strtoul)(const char*,char**,int);
int         MSVCRT(system)(const char*);

WCHAR*      _itow(int,WCHAR*,int);
WCHAR*      _i64tow(__int64,WCHAR*,int);
WCHAR*      _ltow(long,WCHAR*,int);
WCHAR*      _ui64tow(unsigned __int64,WCHAR*,int);
WCHAR*      _ultow(unsigned long,WCHAR*,int);
WCHAR*      _wfullpath(WCHAR*,const WCHAR*,size_t);
WCHAR*      _wgetenv(const WCHAR*);
void        _wmakepath(WCHAR*,const WCHAR*,const WCHAR*,const WCHAR*,const WCHAR*);
void        _wperror(const WCHAR*);
int         _wputenv(const WCHAR*);
void        _wsearchenv(const WCHAR*,const WCHAR*,WCHAR*);
void        _wsplitpath(const WCHAR*,WCHAR*,WCHAR*,WCHAR*,WCHAR*);
int         _wsystem(const WCHAR*);
int         _wtoi(const WCHAR*);
__int64     _wtoi64(const WCHAR*);
long        _wtol(const WCHAR*);

MSVCRT(size_t) MSVCRT(mbstowcs)(WCHAR*,const char*,MSVCRT(size_t));
int         MSVCRT(mbtowc)(WCHAR*,const char*,MSVCRT(size_t));
double      MSVCRT(wcstod)(const WCHAR*,WCHAR**);
long        MSVCRT(wcstol)(const WCHAR*,WCHAR**,int);
MSVCRT(size_t) MSVCRT(wcstombs)(char*,const WCHAR*,MSVCRT(size_t));
unsigned long MSVCRT(wcstoul)(const WCHAR*,WCHAR**,int);
int         MSVCRT(wctomb)(char*,WCHAR);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define environ _environ
#define onexit_t _onexit_t

#define ecvt _ecvt
#define fcvt _fcvt
#define gcvt _gcvt
#define itoa _itoa
#define ltoa _ltoa
#define onexit _onexit
#define putenv _putenv
#define swab _swab
#define ultoa _ultoa
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_STDLIB_H */
