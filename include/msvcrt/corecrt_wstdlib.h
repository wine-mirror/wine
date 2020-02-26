/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WSTDLIB_DEFINED
#define _WSTDLIB_DEFINED

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

wchar_t*      __cdecl _itow(int,wchar_t*,int);
errno_t       __cdecl _itow_s(int,wchar_t*,int, int);
wchar_t*      __cdecl _i64tow(__int64,wchar_t*,int);
errno_t       __cdecl _i64tow_s(__int64, wchar_t*, size_t, int);
wchar_t*      __cdecl _ltow(__msvcrt_long,wchar_t*,int);
errno_t       __cdecl _ltow_s(__msvcrt_long,wchar_t*,int,int);
wchar_t*      __cdecl _ui64tow(unsigned __int64,wchar_t*,int);
errno_t       __cdecl _ui64tow_s(unsigned __int64, wchar_t*, size_t, int);
wchar_t*      __cdecl _ultow(__msvcrt_ulong,wchar_t*,int);
errno_t       __cdecl _ultow_s(__msvcrt_ulong, wchar_t*, size_t, int);
wchar_t*      __cdecl _wfullpath(wchar_t*,const wchar_t*,size_t);
wchar_t*      __cdecl _wgetenv(const wchar_t*);
void          __cdecl _wmakepath(wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
int           __cdecl _wmakepath_s(wchar_t*,size_t,const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
void          __cdecl _wperror(const wchar_t*);
int           __cdecl _wputenv(const wchar_t*);
void          __cdecl _wsearchenv(const wchar_t*,const wchar_t*,wchar_t*);
void          __cdecl _wsplitpath(const wchar_t*,wchar_t*,wchar_t*,wchar_t*,wchar_t*);
errno_t       __cdecl _wsplitpath_s(const wchar_t*,wchar_t*,size_t,wchar_t*,size_t,
                                       wchar_t*,size_t,wchar_t*,size_t);
int           __cdecl _wsystem(const wchar_t*);
double        __cdecl _wtof(const wchar_t*);
int           __cdecl _wtoi(const wchar_t*);
__int64       __cdecl _wtoi64(const wchar_t*);
__msvcrt_long __cdecl _wtol(const wchar_t*);

size_t        __cdecl mbstowcs(wchar_t*,const char*,size_t);
errno_t       __cdecl mbstowcs_s(size_t*,wchar_t*,size_t,const char*,size_t);
int           __cdecl mbtowc(wchar_t*,const char*,size_t);
float         __cdecl wcstof(const wchar_t*,wchar_t**);
double        __cdecl wcstod(const wchar_t*,wchar_t**);
__msvcrt_long __cdecl wcstol(const wchar_t*,wchar_t**,int);
size_t        __cdecl wcstombs(char*,const wchar_t*,size_t);
errno_t       __cdecl wcstombs_s(size_t*,char*,size_t,const wchar_t*,size_t);
__msvcrt_ulong __cdecl wcstoul(const wchar_t*,wchar_t**,int);
int           __cdecl wctomb(char*,wchar_t);
__int64       __cdecl _wcstoi64(const wchar_t*,wchar_t**,int);
__int64       __cdecl _wcstoi64_l(const wchar_t*,wchar_t**,int,_locale_t);
unsigned __int64 __cdecl _wcstoui64(const wchar_t*,wchar_t**,int);
unsigned __int64 __cdecl _wcstoui64_l(const wchar_t*,wchar_t**,int,_locale_t);

#ifdef __cplusplus
}
#endif

#endif /* _WSTDLIB_DEFINED */
