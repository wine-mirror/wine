/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _IO_DEFINED
#define _IO_DEFINED

#include <corecrt_wio.h>

#include <pshpack8.h>

#if defined(_USE_32BIT_TIME_T)
# define _finddata_t     _finddata32_t
# define _finddatai64_t  _finddata32i64_t
#else
# define _finddata_t     _finddata64i32_t
# define _finddatai64_t  _finddata64_t
#endif

struct _finddata32_t {
  unsigned   attrib;
  __time32_t time_create;
  __time32_t time_access;
  __time32_t time_write;
  _fsize_t   size;
  char       name[260];
};

struct _finddata32i64_t {
  unsigned   attrib;
  __time32_t time_create;
  __time32_t time_access;
  __time32_t time_write;
  __int64    DECLSPEC_ALIGN(8) size;
  char       name[260];
};

struct _finddata64i32_t {
  unsigned   attrib;
  __time64_t time_create;
  __time64_t time_access;
  __time64_t time_write;
  _fsize_t   size;
  char       name[260];
};

struct _finddata64_t {
  unsigned   attrib;
  __time64_t time_create;
  __time64_t time_access;
  __time64_t time_write;
  __int64    DECLSPEC_ALIGN(8) size;
  char       name[260];
};

#ifdef _UCRT
# ifdef _USE_32BIT_TIME_T
#  define _findfirst      _findfirst32
#  define _findfirsti64   _findfirst32i64
#  define _findnext       _findnext32
#  define _findnexti64    _findnext32i64
# else
#  define _findfirst      _findfirst64i32
#  define _findfirsti64   _findfirst64
#  define _findnext       _findnext64i32
#  define _findnexti64    _findnext64
# endif
#else /* _UCRT */
# ifdef _USE_32BIT_TIME_T
#  define _findfirst32    _findfirst
#  define _findfirst32i64 _findfirsti64
#  define _findnext32     _findnext
#  define _findnext32i64  _findnexti64
# else
#  define _findfirst64i32 _findfirst
#  define _findfirst64    _findfirsti64
#  define _findnext64i32  _findnext
#  define _findnext64     _findnexti64
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int      __cdecl _access(const char*,int);
_ACRTIMP int      __cdecl _chmod(const char*,int);
_ACRTIMP int      __cdecl _creat(const char*,int);
_ACRTIMP intptr_t __cdecl _findfirst32(const char*,struct _finddata32_t*);
_ACRTIMP intptr_t __cdecl _findfirst32i64(const char*, struct _finddata32i64_t*);
_ACRTIMP intptr_t __cdecl _findfirst64(const char*,struct _finddata64_t*);
_ACRTIMP intptr_t __cdecl _findfirst64i32(const char*, struct _finddata64i32_t*);
_ACRTIMP int      __cdecl _findnext32(intptr_t,struct _finddata32_t*);
_ACRTIMP int      __cdecl _findnext32i64(intptr_t,struct _finddata32i64_t*);
_ACRTIMP int      __cdecl _findnext64(intptr_t,struct _finddata64_t*);
_ACRTIMP int      __cdecl _findnext64i32(intptr_t,struct _finddata64i32_t*);
_ACRTIMP char*    __cdecl _mktemp(char*);
_ACRTIMP int      WINAPIV _open(const char*,int,...);
_ACRTIMP int      WINAPIV _sopen(const char*,int,int,...);
_ACRTIMP int      __cdecl _unlink(const char*);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _IO_DEFINED */
