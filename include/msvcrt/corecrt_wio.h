/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WIO_DEFINED
#define _WIO_DEFINED

#include <corecrt.h>

#include <pshpack8.h>

typedef __msvcrt_ulong _fsize_t;

struct _wfinddata_t {
  unsigned attrib;
  time_t   time_create;
  time_t   time_access;
  time_t   time_write;
  _fsize_t size;
  wchar_t  name[260];
};

struct _wfinddatai64_t {
  unsigned attrib;
  time_t  time_create;
  time_t  time_access;
  time_t  time_write;
  __int64 DECLSPEC_ALIGN(8) size;
  wchar_t name[260];
};

#ifdef __cplusplus
extern "C" {
#endif

int      __cdecl _waccess(const wchar_t*,int);
int      __cdecl _wchmod(const wchar_t*,int);
int      __cdecl _wcreat(const wchar_t*,int);
intptr_t __cdecl _wfindfirst(const wchar_t*,struct _wfinddata_t*);
intptr_t __cdecl _wfindfirsti64(const wchar_t*, struct _wfinddatai64_t*);
int      __cdecl _wfindnext(intptr_t,struct _wfinddata_t*);
int      __cdecl _wfindnexti64(intptr_t, struct _wfinddatai64_t*);
wchar_t* __cdecl _wmktemp(wchar_t*);
int      WINAPIV _wopen(const wchar_t*,int,...);
int      __cdecl _wrename(const wchar_t*,const wchar_t*);
int      WINAPIV _wsopen(const wchar_t*,int,int,...);
int      __cdecl _wunlink(const wchar_t*);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _WIO_DEFINED */
