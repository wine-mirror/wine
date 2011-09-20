/*
 * msvcr90 specific functions
 *
 * Copyright 2010 Detlef Riekenberg
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
 */

#include "config.h"
#include <stdarg.h>

#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#include "malloc.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "sys/stat.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcr90);

#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define THISCALL_NAME(func) __ASM_NAME("__thiscall_" #func)
#define __thiscall __stdcall
#define DEFINE_THISCALL_WRAPPER(func,args) \
    extern void THISCALL(func)(void); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                      "popl %eax\n\t" \
                      "pushl %ecx\n\t" \
                      "pushl %eax\n\t" \
                      "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )

#else /* __i386__ */

#define THISCALL(func) func
#define THISCALL_NAME(func) __ASM_NAME(#func)
#define __thiscall __cdecl
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#endif /* __i386__ */

struct __type_info_node
{
    void *memPtr;
    struct __type_info_node* next;
};

typedef struct __type_info
{
  const void *vtable;
  char       *name;        /* Unmangled name, allocated lazily */
  char        mangled[32]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

typedef void* (__cdecl *malloc_func_t)(size_t);
typedef void  (__cdecl *free_func_t)(void*);

extern char* __cdecl __unDName(char *,const char*,int,malloc_func_t,free_func_t,unsigned short int);

/*********************************************************************
 *  msvcr90_stat64_to_stat32 [internal]
 */
static void msvcr90_stat64_to_stat32(const struct _stat64 *buf64, struct _stat32 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

/*********************************************************************
 *  DllMain (MSVCR90.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
        _set_printf_count_output(0);
    }
    return TRUE;
}

/*********************************************************************
 *  _decode_pointer (MSVCR90.@)
 *
 * cdecl version of DecodePointer
 *
 */
void * CDECL MSVCR90_decode_pointer(void * ptr)
{
    return DecodePointer(ptr);
}

/*********************************************************************
 *  _encode_pointer (MSVCR90.@)
 *
 * cdecl version of EncodePointer
 *
 */
void * CDECL MSVCR90_encode_pointer(void * ptr)
{
    return EncodePointer(ptr);
}

/*********************************************************************
 *  _encoded_null (MSVCR90.@)
 */
void * CDECL _encoded_null(void)
{
    TRACE("\n");

    return MSVCR90_encode_pointer(NULL);
}

/*********************************************************************
 * _invalid_parameter_noinfo (MSVCR90.@)
 */
void CDECL _invalid_parameter_noinfo(void)
{
    _invalid_parameter( NULL, NULL, NULL, 0, 0 );
}

/*********************************************************************
 * __sys_nerr (MSVCR90.@)
 */
int* CDECL __sys_nerr(void)
{
        return (int*)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_sys_nerr");
}

/*********************************************************************
 *  __sys_errlist (MSVCR90.@)
 */
char** CDECL __sys_errlist(void)
{
    return (char**)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_sys_errlist");
}

/*********************************************************************
 * __clean_type_info_names_internal (MSVCR90.@)
 */
void CDECL __clean_type_info_names_internal(void *p)
{
    FIXME("(%p) stub\n", p);
}

/*********************************************************************
 * _recalloc (MSVCR90.@)
 */
void* CDECL _recalloc(void* mem, size_t num, size_t size)
{
    size_t old_size;
    void *ret;

    if(!mem)
        return calloc(num, size);

    size = num*size;
    old_size = _msize(mem);

    ret = realloc(mem, size);
    if(!ret) {
        *_errno() = ENOMEM;
        return NULL;
    }

    if(size>old_size)
        memset((BYTE*)mem+old_size, 0, size-old_size);
    return ret;
}

/*********************************************************************
 *  _fstat32 (MSVCR90.@)
 */
int CDECL _fstat32(int fd, struct _stat32* buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
      msvcr90_stat64_to_stat32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *  _stat32 (MSVCR90.@)
 */
int CDECL _stat32(const char *path, struct _stat32* buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _stat64(path, &buf64);
  if (!ret)
      msvcr90_stat64_to_stat32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *  _wstat32 (MSVCR90.@)
 */
int CDECL _wstat32(const wchar_t *path, struct _stat32* buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _wstat64(path, &buf64);
  if (!ret)
      msvcr90_stat64_to_stat32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_fstat64i32 (MSVCRT.@)
 */

static void msvcrt_stat64_to_stat64i32(const struct _stat64 *buf64, struct _stat64i32 *buf)
{
    buf->st_dev   = buf64->st_dev;
    buf->st_ino   = buf64->st_ino;
    buf->st_mode  = buf64->st_mode;
    buf->st_nlink = buf64->st_nlink;
    buf->st_uid   = buf64->st_uid;
    buf->st_gid   = buf64->st_gid;
    buf->st_rdev  = buf64->st_rdev;
    buf->st_size  = buf64->st_size;
    buf->st_atime = buf64->st_atime;
    buf->st_mtime = buf64->st_mtime;
    buf->st_ctime = buf64->st_ctime;
}

int CDECL _fstat64i32(int fd, struct _stat64i32* buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _fstat64(fd, &buf64);
  if (!ret)
      msvcrt_stat64_to_stat64i32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *		_stat64i32 (MSVCRT.@)
 */
int CDECL _stat64i32(const char* path, struct _stat64i32 * buf)
{
  int ret;
  struct _stat64 buf64;

  ret = _stat64(path, &buf64);
  if (!ret)
    msvcrt_stat64_to_stat64i32(&buf64, buf);
  return ret;
}

/*********************************************************************
 *              _wstat64i32 (MSVCRT.@)
 */
int CDECL _wstat64i32(const wchar_t *path, struct _stat64i32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        msvcrt_stat64_to_stat64i32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *		_atoflt  (MSVCR90.@)
 */
int CDECL _atoflt( _CRT_FLOAT *value, char *str )
{
    return _atoflt_l( value, str, NULL );
}

/*********************************************************************
 * ?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z (MSVCR90.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_name_internal_method,8)
const char * __thiscall MSVCRT_type_info_name_internal_method(type_info * _this, struct __type_info_node *node)
{
    static int once;

    if (node && !once++) FIXME("type_info_node parameter ignored\n");

    if (!_this->name)
    {
      /* Create and set the demangled name */
      /* Note: mangled name in type_info struct always starts with a '.', while
       * it isn't valid for mangled name.
       * Is this '.' really part of the mangled name, or has it some other meaning ?
       */
      char* name = __unDName(0, _this->mangled + 1, 0, malloc, free, 0x2800);
      if (name)
      {
        unsigned int len = strlen(name);

        /* It seems _unDName may leave blanks at the end of the demangled name */
        while (len && name[--len] == ' ')
          name[len] = '\0';

        if (InterlockedCompareExchangePointer((void**)&_this->name, name, NULL))
        {
          /* Another thread set this member since we checked above - use it */
          free(name);
        }
      }
    }
    TRACE("(%p) returning %s\n", _this, _this->name);
    return _this->name;
}

/*********************************************************************
 *              _CRT_RTC_INIT (MSVCR90.@)
 */
void* CDECL _CRT_RTC_INIT(void *unk1, void *unk2, int unk3, int unk4, int unk5)
{
    TRACE("%p %p %x %x %x\n", unk1, unk2, unk3, unk4, unk5);
    return NULL;
}

/*********************************************************************
 *              _CRT_RTC_INITW (MSVCR90.@)
 */
void* CDECL _CRT_RTC_INITW(void *unk1, void *unk2, int unk3, int unk4, int unk5)
{
    TRACE("%p %p %x %x %x\n", unk1, unk2, unk3, unk4, unk5);
    return NULL;
}

/*********************************************************************
 *              _vswprintf_p (MSVCR90.@)
 */
int CDECL MSVCR90__vswprintf_p(wchar_t *buffer, size_t length, const wchar_t *format, __ms_va_list args)
{
    return _vswprintf_p_l(buffer, length, format, NULL, args);
}
