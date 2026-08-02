/*
 * msvcr100 specific functions
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

#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "malloc.h"
#include "mbstring.h"
#include "limits.h"
#include "sys/stat.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#define INVALID_PMT(x,err)   (*_errno() = (err), _invalid_parameter(NULL, NULL, NULL, 0, 0))
#define CHECK_PMT_ERR(x,err) ((x) || (INVALID_PMT( 0, (err) ), FALSE))
#define CHECK_PMT(x)         CHECK_PMT_ERR((x), EINVAL)

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
 *  stat64_to_stat32 [internal]
 */
static void stat64_to_stat32(const struct _stat64 *buf64, struct _stat32 *buf)
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
 *		wmemcpy_s (MSVCR100.@)
 */
int CDECL wmemcpy_s(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, (unsigned long)numberOfElements, src, (unsigned long)count);

    if (!count)
        return 0;

    if (!CHECK_PMT(dest != NULL)) return EINVAL;

    if (!CHECK_PMT(src != NULL)) {
        memset(dest, 0, numberOfElements*sizeof(wchar_t));
        return EINVAL;
    }
    if (!CHECK_PMT_ERR(count <= numberOfElements, ERANGE)) {
        memset(dest, 0, numberOfElements*sizeof(wchar_t));
        return ERANGE;
    }

    memcpy(dest, src, sizeof(wchar_t)*count);
    return 0;
}

 /*********************************************************************
 *		wmemmove_s (MSVCR100.@)
 */
int CDECL wmemmove_s(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, (unsigned long)numberOfElements, src, (unsigned long)count);

    if (!count)
        return 0;

    /* Native does not seem to conform to 6.7.1.2.3 in
     * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1225.pdf
     * in that it does not zero the output buffer on constraint violation.
     */
    if (!CHECK_PMT(dest != NULL)) return EINVAL;
    if (!CHECK_PMT(src != NULL)) return EINVAL;
    if (!CHECK_PMT_ERR(count <= numberOfElements, ERANGE)) return ERANGE;

    memmove(dest, src, sizeof(wchar_t)*count);
    return 0;
}

/*********************************************************************
 *  _encoded_null (MSVCR100.@)
 */
void * CDECL _encoded_null(void)
{
    TRACE("\n");

    return EncodePointer(NULL);
}

/*********************************************************************
 * _invalid_parameter_noinfo (MSVCR100.@)
 */
void CDECL _invalid_parameter_noinfo(void)
{
    _invalid_parameter( NULL, NULL, NULL, 0, 0 );
}

/*********************************************************************
 * __sys_nerr (MSVCR100.@)
 */
int* CDECL __sys_nerr(void)
{
    return (int*)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_sys_nerr");
}

/*********************************************************************
 *  __sys_errlist (MSVCR100.@)
 */
char** CDECL __sys_errlist(void)
{
    return (char**)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_sys_errlist");
}

/*********************************************************************
 * __clean_type_info_names_internal (MSVCR100.@)
 */
void CDECL __clean_type_info_names_internal(void *p)
{
    FIXME("(%p) stub\n", p);
}

/*********************************************************************
 * _recalloc (MSVCR100.@)
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
        memset((BYTE*)ret+old_size, 0, size-old_size);
    return ret;
}

/*********************************************************************
 *  _stat32 (MSVCR100.@)
 */
int CDECL _stat32(const char *path, struct _stat32* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _stat64(path, &buf64);
    if (!ret)
        stat64_to_stat32(&buf64, buf);
    return ret;
}

/*********************************************************************
 *  _wstat32 (MSVCR100.@)
 */
int CDECL _wstat32(const wchar_t *path, struct _stat32* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        stat64_to_stat32(&buf64, buf);
    return ret;
}

static void stat64_to_stat32i64(const struct _stat64 *buf64, struct _stat32i64 *buf)
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
 *  _stat32i64 (MSVCR100.@)
 */
int CDECL _stat32i64(const char *path, struct _stat32i64* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _stat64(path, &buf64);
    if (!ret)
        stat64_to_stat32i64(&buf64, buf);
    return ret;
}

/*********************************************************************
 *  _wstat32i64 (MSVCR100.@)
 */
int CDECL _wstat32i64(const wchar_t *path, struct _stat32i64* buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        stat64_to_stat32i64(&buf64, buf);
    return ret;
}

static void stat64_to_stat64i32(const struct _stat64 *buf64, struct _stat64i32 *buf)
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
 * _stat64i32 (MSVCR100.@)
 */
int CDECL _stat64i32(const char* path, struct _stat64i32 * buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _stat64(path, &buf64);
    if (!ret)
        stat64_to_stat64i32(&buf64, buf);
    return ret;
}

/*********************************************************************
 * _wstat64i32 (MSVCR100.@)
 */
int CDECL _wstat64i32(const wchar_t *path, struct _stat64i32 *buf)
{
    int ret;
    struct _stat64 buf64;

    ret = _wstat64(path, &buf64);
    if (!ret)
        stat64_to_stat64i32(&buf64, buf);
    return ret;
}

/*********************************************************************
 * _atoflt  (MSVCR100.@)
 */
int CDECL _atoflt( _CRT_FLOAT *value, char *str )
{
    return _atoflt_l( value, str, NULL );
}

/*********************************************************************
 * ?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z (MSVCR100.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_name_internal_method,8)
const char * __thiscall type_info_name_internal_method(type_info * _this, struct __type_info_node *node)
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
 * _CRT_RTC_INIT (MSVCR100.@)
 */
void* CDECL _CRT_RTC_INIT(void *unk1, void *unk2, int unk3, int unk4, int unk5)
{
    TRACE("%p %p %x %x %x\n", unk1, unk2, unk3, unk4, unk5);
    return NULL;
}

/*********************************************************************
 * _CRT_RTC_INITW (MSVCR100.@)
 */
void* CDECL _CRT_RTC_INITW(void *unk1, void *unk2, int unk3, int unk4, int unk5)
{
    TRACE("%p %p %x %x %x\n", unk1, unk2, unk3, unk4, unk5);
    return NULL;
}

/*********************************************************************
 * _vswprintf_p (MSVCR100.@)
 */
int CDECL _vswprintf_p(wchar_t *buffer, size_t length, const wchar_t *format, __ms_va_list args)
{
    return _vswprintf_p_l(buffer, length, format, NULL, args);
}

/*********************************************************************
 * _vscwprintf_p (MSVCR100.@)
 */
int CDECL _vscwprintf_p(const wchar_t *format, __ms_va_list args)
{
    return _vscwprintf_p_l(format, NULL, args);
}

/*********************************************************************
 * _byteswap_ushort (MSVCR100.@)
 */
unsigned short CDECL _byteswap_ushort(unsigned short s)
{
    return (s<<8) + (s>>8);
}

/*********************************************************************
 * _byteswap_ulong (MSVCR100.@)
 */
ULONG CDECL _byteswap_ulong(ULONG l)
{
    return (l<<24) + ((l<<8)&0xFF0000) + ((l>>8)&0xFF00) + (l>>24);
}

/*********************************************************************
 * _byteswap_uint64 (MSVCR100.@)
 */
unsigned __int64 CDECL _byteswap_uint64(unsigned __int64 i)
{
    return (i<<56) + ((i&0xFF00)<<40) + ((i&0xFF0000)<<24) + ((i&0xFF000000)<<8) +
        ((i>>8)&0xFF000000) + ((i>>24)&0xFF0000) + ((i>>40)&0xFF00) + (i>>56);
}

/*********************************************************************
 * _sprintf_p (MSVCR100.@)
 */
int CDECL _sprintf_p(char *buffer, size_t length, const char *format, ...)
{
    __ms_va_list valist;
    int r;

    __ms_va_start(valist, format);
    r = _vsprintf_p_l(buffer, length, format, NULL, valist);
    __ms_va_end(valist);

    return r;
}

/*********************************************************************
 * _get_timezone (MSVCR100.@)
 */
int CDECL _get_timezone(LONG *timezone)
{
    if(!CHECK_PMT(timezone != NULL)) return EINVAL;

    *timezone = *(LONG*)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_timezone");
    return 0;
}

/*********************************************************************
 * _get_daylight (MSVCR100.@)
 */
int CDECL _get_daylight(int *hours)
{
    if(!CHECK_PMT(hours != NULL)) return EINVAL;

    *hours = *(int*)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_daylight");
    return 0;
}

/* copied from dlls/msvcrt/heap.c */
#define SAVED_PTR(x) ((void *)((DWORD_PTR)((char *)x - sizeof(void *)) & \
                               ~(sizeof(void *) - 1)))

/*********************************************************************
 * _aligned_msize (MSVCR100.@)
 */
size_t CDECL _aligned_msize(void *p, size_t alignment, size_t offset)
{
    void **alloc_ptr;

    if(!CHECK_PMT(p)) return -1;

    if(alignment < sizeof(void*))
        alignment = sizeof(void*);

    alloc_ptr = SAVED_PTR(p);
    return _msize(*alloc_ptr)-alignment-sizeof(void*);
}

int CDECL MSVCR100_atoi(const char *str)
{
    return _atoi_l(str, NULL);
}

unsigned char* CDECL MSVCR100__mbstok(unsigned char *str, const unsigned char *delim)
{
    return _mbstok_l(str, delim, NULL);
}

/*********************************************************************
 *  DllMain (MSVCR100.@)
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
