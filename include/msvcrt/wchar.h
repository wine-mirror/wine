/*
 * Unicode definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_WCHAR_H
#define __WINE_WCHAR_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#define WCHAR_MIN 0
#define WCHAR_MAX ((wchar_t)-1)

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

#ifndef DECLSPEC_ALIGN
# if defined(_MSC_VER) && (_MSC_VER >= 1300) && !defined(MIDL_PASS)
#  define DECLSPEC_ALIGN(x) __declspec(align(x))
# elif defined(__GNUC__)
#  define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
# else
#  define DECLSPEC_ALIGN(x)
# endif
#endif

typedef int mbstate_t;

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short  wint_t;
typedef unsigned short  wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

#ifndef _FSIZE_T_DEFINED
typedef unsigned long _fsize_t;
#define _FSIZE_T_DEFINED
#endif

#ifndef _DEV_T_DEFINED
typedef unsigned int   _dev_t;
#define _DEV_T_DEFINED
#endif

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;
#define _INO_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED
typedef int _off_t;
#define _OFF_T_DEFINED
#endif

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef _TIME64_T_DEFINED
#define _TIME64_T_DEFINED
typedef __int64 __time64_t;
#endif

#ifndef _TM_DEFINED
#define _TM_DEFINED
struct tm {
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
#endif /* _TM_DEFINED */

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
typedef struct _iobuf
{
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file;
  int   _charbuf;
  int   _bufsiz;
  char* _tmpfname;
} FILE;
#endif  /* _FILE_DEFINED */

#ifndef _WFINDDATA_T_DEFINED
#define _WFINDDATA_T_DEFINED

struct _wfinddata_t {
  unsigned attrib;
  time_t time_create;
  time_t time_access;
  time_t time_write;
  _fsize_t size;
  wchar_t name[260];
};

struct _wfinddatai64_t {
  unsigned attrib;
  time_t time_create;
  time_t time_access;
  time_t time_write;
  __int64        size;
  wchar_t name[260];
};

#endif /* _WFINDDATA_T_DEFINED */

#ifndef _STAT_DEFINED
#define _STAT_DEFINED

struct _stat {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  _off_t st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

struct stat {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  _off_t st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

struct _stati64 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

struct _stat64 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  __time64_t     st_atime;
  __time64_t     st_mtime;
  __time64_t     st_ctime;
};
#endif /* _STAT_DEFINED */

/* ASCII char classification table - binary compatible */
#define _UPPER        0x0001  /* C1_UPPER */
#define _LOWER        0x0002  /* C1_LOWER */
#define _DIGIT        0x0004  /* C1_DIGIT */
#define _SPACE        0x0008  /* C1_SPACE */
#define _PUNCT        0x0010  /* C1_PUNCT */
#define _CONTROL      0x0020  /* C1_CNTRL */
#define _BLANK        0x0040  /* C1_BLANK */
#define _HEX          0x0080  /* C1_XDIGIT */
#define _LEADBYTE     0x8000
#define _ALPHA       (0x0100|_UPPER|_LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED
int is_wctype(wint_t,wctype_t);
int isleadbyte(int);
int iswalnum(wint_t);
int iswalpha(wint_t);
int iswascii(wint_t);
int iswcntrl(wint_t);
int iswctype(wint_t,wctype_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
wchar_t towlower(wchar_t);
wchar_t towupper(wchar_t);
#endif /* _WCTYPE_DEFINED */

#ifndef _WDIRECT_DEFINED
#define _WDIRECT_DEFINED
int              _wchdir(const wchar_t*);
wchar_t* _wgetcwd(wchar_t*,int);
wchar_t* _wgetdcwd(int,wchar_t*,int);
int              _wmkdir(const wchar_t*);
int              _wrmdir(const wchar_t*);
#endif /* _WDIRECT_DEFINED */

#ifndef _WIO_DEFINED
#define _WIO_DEFINED
int         _waccess(const wchar_t*,int);
int         _wchmod(const wchar_t*,int);
int         _wcreat(const wchar_t*,int);
long        _wfindfirst(const wchar_t*,struct _wfinddata_t*);
long        _wfindfirsti64(const wchar_t*, struct _wfinddatai64_t*);
int         _wfindnext(long,struct _wfinddata_t*);
int         _wfindnexti64(long, struct _wfinddatai64_t*);
wchar_t*_wmktemp(wchar_t*);
int         _wopen(const wchar_t*,int,...);
int         _wrename(const wchar_t*,const wchar_t*);
int         _wsopen(const wchar_t*,int,int,...);
int         _wunlink(const wchar_t*);
#endif /* _WIO_DEFINED */

#ifndef _WLOCALE_DEFINED
#define _WLOCALE_DEFINED
wchar_t* _wsetlocale(int,const wchar_t*);
#endif /* _WLOCALE_DEFINED */

#ifndef _WPROCESS_DEFINED
#define _WPROCESS_DEFINED
int         _wexecl(const wchar_t*,const wchar_t*,...);
int         _wexecle(const wchar_t*,const wchar_t*,...);
int         _wexeclp(const wchar_t*,const wchar_t*,...);
int         _wexeclpe(const wchar_t*,const wchar_t*,...);
int         _wexecv(const wchar_t*,const wchar_t* const *);
int         _wexecve(const wchar_t*,const wchar_t* const *,const wchar_t* const *);
int         _wexecvp(const wchar_t*,const wchar_t* const *);
int         _wexecvpe(const wchar_t*,const wchar_t* const *,const wchar_t* const *);
int         _wspawnl(int,const wchar_t*,const wchar_t*,...);
int         _wspawnle(int,const wchar_t*,const wchar_t*,...);
int         _wspawnlp(int,const wchar_t*,const wchar_t*,...);
int         _wspawnlpe(int,const wchar_t*,const wchar_t*,...);
int         _wspawnv(int,const wchar_t*,const wchar_t* const *);
int         _wspawnve(int,const wchar_t*,const wchar_t* const *,const wchar_t* const *);
int         _wspawnvp(int,const wchar_t*,const wchar_t* const *);
int         _wspawnvpe(int,const wchar_t*,const wchar_t* const *,const wchar_t* const *);
int         _wsystem(const wchar_t*);
#endif /* _WPROCESS_DEFINED */

#ifndef _WSTAT_DEFINED
#define _WSTAT_DEFINED
int _wstat(const wchar_t*,struct _stat*);
int _wstati64(const wchar_t*,struct _stati64*);
int _wstat64(const wchar_t*,struct _stat64*);
#endif /* _WSTAT_DEFINED */

#ifndef _WSTDIO_DEFINED
#define _WSTDIO_DEFINED
wint_t  _fgetwchar(void);
wint_t  _fputwchar(wint_t);
wchar_t*_getws(wchar_t*);
int             _putws(const wchar_t*);
int             _snwprintf(wchar_t*,size_t,const wchar_t*,...);
int             _vsnwprintf(wchar_t*,size_t,const wchar_t*,va_list);
FILE*   _wfdopen(int,const wchar_t*);
FILE*   _wfopen(const wchar_t*,const wchar_t*);
FILE*   _wfreopen(const wchar_t*,const wchar_t*,FILE*);
FILE*   _wfsopen(const wchar_t*,const wchar_t*,int);
void            _wperror(const wchar_t*);
FILE*   _wpopen(const wchar_t*,const wchar_t*);
int             _wremove(const wchar_t*);
wchar_t*_wtempnam(const wchar_t*,const wchar_t*);
wchar_t*_wtmpnam(wchar_t*);

wint_t  fgetwc(FILE*);
wchar_t*fgetws(wchar_t*,int,FILE*);
wint_t  fputwc(wint_t,FILE*);
int             fputws(const wchar_t*,FILE*);
int             fwprintf(FILE*,const wchar_t*,...);
int             fputws(const wchar_t*,FILE*);
int             fwscanf(FILE*,const wchar_t*,...);
wint_t  getwc(FILE*);
wint_t  getwchar(void);
wchar_t*getws(wchar_t*);
wint_t  putwc(wint_t,FILE*);
wint_t  putwchar(wint_t);
int             putws(const wchar_t*);
int             swprintf(wchar_t*,const wchar_t*,...);
int             swscanf(const wchar_t*,const wchar_t*,...);
wint_t  ungetwc(wint_t,FILE*);
int             vfwprintf(FILE*,const wchar_t*,va_list);
int             vswprintf(wchar_t*,const wchar_t*,va_list);
int             vwprintf(const wchar_t*,va_list);
int             wprintf(const wchar_t*,...);
int             wscanf(const wchar_t*,...);
#endif /* _WSTDIO_DEFINED */

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

#ifndef _WSTRING_DEFINED
#define _WSTRING_DEFINED
wchar_t*_wcsdup(const wchar_t*);
int             _wcsicmp(const wchar_t*,const wchar_t*);
int             _wcsicoll(const wchar_t*,const wchar_t*);
wchar_t*_wcslwr(wchar_t*);
int             _wcsnicmp(const wchar_t*,const wchar_t*,size_t);
wchar_t*_wcsnset(wchar_t*,wchar_t,size_t);
wchar_t*_wcsrev(wchar_t*);
wchar_t*_wcsset(wchar_t*,wchar_t);
wchar_t*_wcsupr(wchar_t*);

wchar_t*wcscat(wchar_t*,const wchar_t*);
wchar_t*wcschr(const wchar_t*,wchar_t);
int             wcscmp(const wchar_t*,const wchar_t*);
int             wcscoll(const wchar_t*,const wchar_t*);
wchar_t*wcscpy(wchar_t*,const wchar_t*);
size_t  wcscspn(const wchar_t*,const wchar_t*);
size_t  wcslen(const wchar_t*);
wchar_t*wcsncat(wchar_t*,const wchar_t*,size_t);
int             wcsncmp(const wchar_t*,const wchar_t*,size_t);
wchar_t*wcsncpy(wchar_t*,const wchar_t*,size_t);
wchar_t*wcspbrk(const wchar_t*,const wchar_t*);
wchar_t*wcsrchr(const wchar_t*,wchar_t wcFor);
size_t  wcsspn(const wchar_t*,const wchar_t*);
wchar_t*wcsstr(const wchar_t*,const wchar_t*);
wchar_t*wcstok(wchar_t*,const wchar_t*);
size_t  wcsxfrm(wchar_t*,const wchar_t*,size_t);
#endif /* _WSTRING_DEFINED */

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED
wchar_t*_wasctime(const struct tm*);
size_t  wcsftime(wchar_t*,size_t,const wchar_t*,const struct tm*);
wchar_t*_wctime(const time_t*);
wchar_t*_wstrdate(wchar_t*);
wchar_t*_wstrtime(wchar_t*);
#endif /* _WTIME_DEFINED */

wchar_t btowc(int);
size_t  mbrlen(const char *,size_t,mbstate_t*);
size_t  mbrtowc(wchar_t*,const char*,size_t,mbstate_t*);
size_t  mbsrtowcs(wchar_t*,const char**,size_t,mbstate_t*);
size_t  wcrtomb(char*,wchar_t,mbstate_t*);
size_t  wcsrtombs(char*,const wchar_t**,size_t,mbstate_t*);
int             wctob(wint_t);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_WCHAR_H */
