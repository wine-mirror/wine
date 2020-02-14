/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _WSTDIO_DEFINED
#define _WSTDIO_DEFINED

#include <corecrt.h>
#include <corecrt_stdio_config.h>

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack8.h>

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
#include <pshpack8.h>
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
#include <poppack.h>
#endif  /* _FILE_DEFINED */

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

wint_t   __cdecl _fgetwc_nolock(FILE*);
wint_t   __cdecl _fgetwchar(void);
wint_t   __cdecl _fputwc_nolock(wint_t,FILE*);
wint_t   __cdecl _fputwchar(wint_t);
wint_t   __cdecl _getwc_nolock(FILE*);
wchar_t* __cdecl _getws(wchar_t*);
wint_t   __cdecl _putwc_nolock(wint_t,FILE*);
int      __cdecl _putws(const wchar_t*);
int      WINAPIV _snwprintf(wchar_t*,size_t,const wchar_t*,...);
int      WINAPIV _snwprintf_s(wchar_t*,size_t,size_t,const wchar_t*,...);
int      WINAPIV _scwprintf(const wchar_t*,...);
wint_t   __cdecl _ungetwc_nolock(wint_t,FILE*);
int      __cdecl _vscwprintf(const wchar_t*,__ms_va_list);
int      __cdecl _vscwprintf_p_l(const wchar_t*,_locale_t,__ms_va_list);
int      __cdecl _vsnwprintf(wchar_t*,size_t,const wchar_t*,__ms_va_list);
int      __cdecl _vsnwprintf_s(wchar_t*,size_t,size_t,const wchar_t*,__ms_va_list);
int      __cdecl _vswprintf_p_l(wchar_t*,size_t,const wchar_t*,_locale_t,__ms_va_list);
FILE*    __cdecl _wfdopen(int,const wchar_t*);
FILE*    __cdecl _wfopen(const wchar_t*,const wchar_t*);
errno_t  __cdecl _wfopen_s(FILE**,const wchar_t*,const wchar_t*);
FILE*    __cdecl _wfreopen(const wchar_t*,const wchar_t*,FILE*);
FILE*    __cdecl _wfsopen(const wchar_t*,const wchar_t*,int);
void     __cdecl _wperror(const wchar_t*);
FILE*    __cdecl _wpopen(const wchar_t*,const wchar_t*);
int      __cdecl _wremove(const wchar_t*);
wchar_t* __cdecl _wtempnam(const wchar_t*,const wchar_t*);
wchar_t* __cdecl _wtmpnam(wchar_t*);

wint_t   __cdecl fgetwc(FILE*);
wchar_t* __cdecl fgetws(wchar_t*,int,FILE*);
wint_t   __cdecl fputwc(wint_t,FILE*);
int      __cdecl fputws(const wchar_t*,FILE*);
int      WINAPIV fwprintf(FILE*,const wchar_t*,...);
int      WINAPIV fwprintf_s(FILE*,const wchar_t*,...);
int      __cdecl fputws(const wchar_t*,FILE*);
int      WINAPIV fwscanf(FILE*,const wchar_t*,...);
int      WINAPIV fwscanf_s(FILE*,const wchar_t*,...);
wint_t   __cdecl getwc(FILE*);
wint_t   __cdecl getwchar(void);
wchar_t* __cdecl getws(wchar_t*);
wint_t   __cdecl putwc(wint_t,FILE*);
wint_t   __cdecl putwchar(wint_t);
int      __cdecl putws(const wchar_t*);
int      WINAPIV swprintf_s(wchar_t*,size_t,const wchar_t*,...);
int      WINAPIV swscanf(const wchar_t*,const wchar_t*,...);
int      WINAPIV swscanf_s(const wchar_t*,const wchar_t*,...);
wint_t   __cdecl ungetwc(wint_t,FILE*);
int      __cdecl vfwprintf(FILE*,const wchar_t*,__ms_va_list);
int      __cdecl vfwprintf_s(FILE*,const wchar_t*,__ms_va_list);
int      __cdecl vswprintf_s(wchar_t*,size_t,const wchar_t*,__ms_va_list);
int      __cdecl vwprintf(const wchar_t*,__ms_va_list);
int      __cdecl vwprintf_s(const wchar_t*,__ms_va_list);
int      WINAPIV wprintf(const wchar_t*,...);
int      WINAPIV wprintf_s(const wchar_t*,...);
int      WINAPIV wscanf(const wchar_t*,...);
int      WINAPIV wscanf_s(const wchar_t*,...);

#ifdef _CRT_NON_CONFORMING_SWPRINTFS
int WINAPIV swprintf(wchar_t*,const wchar_t*,...);
int __cdecl vswprintf(wchar_t*,const wchar_t*,__ms_va_list);
#else  /*  _CRT_NON_CONFORMING_SWPRINTFS */
static inline int vswprintf(wchar_t *buffer, size_t size, const wchar_t *format, __ms_va_list args) { return _vsnwprintf(buffer,size,format,args); }
static inline int WINAPIV swprintf(wchar_t *buffer, size_t size, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list args;

    __ms_va_start(args, format);
    ret = _vsnwprintf(buffer, size, format, args);
    __ms_va_end(args);
    return ret;
}
#endif  /*  _CRT_NON_CONFORMING_SWPRINTFS */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _WSTDIO_DEFINED */
