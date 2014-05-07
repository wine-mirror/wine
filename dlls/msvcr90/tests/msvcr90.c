/*
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <time.h>
#include <locale.h>

#include <windef.h>
#include <winbase.h>
#include <errno.h>
#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { \
        expect_ ## func = TRUE; \
        errno = 0xdeadbeef; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func,error) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        ok( errno == (error), "got errno %u instead of %u\n", errno, (error) ); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(invalid_parameter_handler);

static _invalid_parameter_handler (__cdecl *p_set_invalid_parameter_handler)(_invalid_parameter_handler);
typedef int (__cdecl *_INITTERM_E_FN)(void);
static int (__cdecl *p_initterm_e)(_INITTERM_E_FN *table, _INITTERM_E_FN *end);
static void* (__cdecl *p_encode_pointer)(void *);
static void* (__cdecl *p_decode_pointer)(void *);
static void* (__cdecl *p_encoded_null)(void);
static int *p_sys_nerr;
static int* (__cdecl *p__sys_nerr)(void);
static char **p_sys_errlist;
static char** (__cdecl *p__sys_errlist)(void);
static __int64 (__cdecl *p_strtoi64)(const char *, char **, int);
static unsigned __int64 (__cdecl *p_strtoui64)(const char *, char **, int);
static errno_t (__cdecl *p_itoa_s)(int,char*,size_t,int);
static int (__cdecl *p_wcsncat_s)(wchar_t *dst, size_t elem, const wchar_t *src, size_t count);
static void (__cdecl *p_qsort_s)(void *, size_t, size_t, int (__cdecl *)(void *, const void *, const void *), void *);
static void* (__cdecl *p_bsearch_s)(const void *, const void *, size_t, size_t,
                                    int (__cdecl *compare)(void *, const void *, const void *), void *);
static int (__cdecl *p_controlfp_s)(unsigned int *, unsigned int, unsigned int);
static int (__cdecl *p_tmpfile_s)(FILE**);
static int (__cdecl *p_atoflt)(_CRT_FLOAT *, char *);
static unsigned int (__cdecl *p_set_abort_behavior)(unsigned int, unsigned int);
static int (__cdecl *p_sopen_s)(int*, const char*, int, int, int);
static int (__cdecl *p_wsopen_s)(int*, const wchar_t*, int, int, int);
static void* (__cdecl *p_realloc_crt)(void*, size_t);
static void* (__cdecl *p_malloc)(size_t);
static void (__cdecl *p_free)(void*);
static void* (__cdecl *p_getptd)(void);
static int* (__cdecl *p_errno)(void);
static __msvcrt_ulong* (__cdecl *p_doserrno)(void);
static void (__cdecl *p_srand)(unsigned int);
static char* (__cdecl *p_strtok)(char*, const char*);
static wchar_t* (__cdecl *p_wcstok)(wchar_t*, const wchar_t*);
static unsigned char* (__cdecl *p__mbstok)(unsigned char*, const unsigned char*);
static char* (__cdecl *p_strerror)(int);
static wchar_t* (__cdecl *p_wcserror)(int);
static char* (__cdecl *p_tmpnam)(char*);
static wchar_t* (__cdecl *p_wtmpnam)(wchar_t*);
static char* (__cdecl *p_asctime)(struct tm*);
static wchar_t* (__cdecl *p_wasctime)(struct tm*);
static struct tm* (__cdecl *p_localtime64)(__time64_t*);
static char* (__cdecl *p_ecvt)(double, int, int*, int*);
static int* (__cdecl *p_fpecode)(void);
static int (__cdecl *p_configthreadlocale)(int);
static void* (__cdecl *p_get_terminate)(void);
static void* (__cdecl *p_get_unexpected)(void);
static int (__cdecl *p__vswprintf_l)(wchar_t*, const wchar_t*, _locale_t, __ms_va_list);
static int (__cdecl *p_vswprintf_l)(wchar_t*, const wchar_t*, _locale_t, __ms_va_list);
static FILE* (__cdecl *p_fopen)(const char*, const char*);
static int (__cdecl *p_fclose)(FILE*);
static int (__cdecl *p_unlink)(const char*);
static int (__cdecl *p_access_s)(const char*, int);
static void (__cdecl *p_lock_file)(FILE*);
static void (__cdecl *p_unlock_file)(FILE*);
static int (__cdecl *p_fileno)(FILE*);
static int (__cdecl *p_feof)(FILE*);
static int (__cdecl *p_ferror)(FILE*);
static int (__cdecl *p_flsbuf)(int, FILE*);
static unsigned long (__cdecl *p_byteswap_ulong)(unsigned long);
static void** (__cdecl *p__pxcptinfoptrs)(void);
static void* (__cdecl *p__AdjustPointer)(void*, const void*);

/* make sure we use the correct errno */
#undef errno
#define errno (*p_errno())

/* type info */
typedef struct __type_info
{
  void *vtable;
  char *name;
  char  mangled[16];
} type_info;

struct __type_info_node
{
    void *memPtr;
    struct __type_info_node* next;
};

static char* (WINAPI *p_type_info_name_internal_method)(type_info*, struct __type_info_node *);
static void  (WINAPI *ptype_info_dtor)(type_info*);

#define CXX_FRAME_MAGIC_VC6 0x19930520
#define CXX_EXCEPTION       0xe06d7363

/* offsets for computing the this pointer */
typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

typedef void (*cxx_copy_ctor)(void);

/* complete information about a C++ type */
#ifndef __x86_64__
typedef struct __cxx_type_info
{
    UINT             flags;        /* flags (see CLASS_* flags below) */
    const type_info *type_info;    /* C++ type info */
    this_ptr_offsets offsets;      /* offsets for computing the this pointer */
    unsigned int     size;         /* object size */
    cxx_copy_ctor    copy_ctor;    /* copy constructor */
} cxx_type_info;
#else
typedef struct __cxx_type_info
{
    UINT flags;
    unsigned int type_info;
    this_ptr_offsets offsets;
    unsigned int size;
    unsigned int copy_ctor;
} cxx_type_info;
#endif

/* table of C++ types that apply for a given object */
#ifndef __x86_64__
typedef struct __cxx_type_info_table
{
    UINT                 count;     /* number of types */
    const cxx_type_info *info[3];   /* variable length, we declare it large enough for static RTTI */
} cxx_type_info_table;
#else
typedef struct __cxx_type_info_table
{
    UINT count;
    unsigned int info[3];
} cxx_type_info_table;
#endif

/* type information for an exception object */
#ifndef __x86_64__
typedef struct __cxx_exception_type
{
    UINT                       flags;            /* TYPE_FLAG flags */
    void                     (*destructor)(void);/* exception object destructor */
    void                      *custom_handler;   /* custom handler for this exception */
    const cxx_type_info_table *type_info_table;  /* list of types for this exception object */
} cxx_exception_type;
#else
typedef struct
{
    UINT flags;
    unsigned int destructor;
    unsigned int custom_handler;
    unsigned int type_info_table;
} cxx_exception_type;
#endif

static int (__cdecl *p_is_exception_typeof)(const type_info*, EXCEPTION_POINTERS*);

static void* (WINAPI *pEncodePointer)(void *);

static int cb_called[4];
static int g_qsort_s_context_counter;
static int g_bsearch_s_context_counter;

static inline int almost_equal_f(float f1, float f2)
{
    return f1-f2 > -1e-30 && f1-f2 < 1e-30;
}

/* ########## */

/* thiscall emulation */
/* Emulate a __thiscall */
#ifdef __i386__
#ifdef _MSC_VER
static inline void* do_call_func1(void *func, void *_this)
{
  volatile void* retval = 0;
  __asm
  {
    push ecx
    mov ecx, _this
    call func
    mov retval, eax
    pop ecx
  }
  return (void*)retval;
}

static inline void* do_call_func2(void *func, void *_this, const void* arg)
{
  volatile void* retval = 0;
  __asm
  {
    push ecx
    push arg
    mov ecx, _this
    call func
    mov retval, eax
    pop ecx
  }
  return (void*)retval;
}
#else
static void* do_call_func1(void *func, void *_this)
{
  void *ret, *dummy;
  __asm__ __volatile__ ("call *%2"
                        : "=a" (ret), "=c" (dummy)
                        : "g" (func), "1" (_this)
                        : "edx", "memory" );
  return ret;
}

static void* do_call_func2(void *func, void *_this, const void* arg)
{
  void *ret, *dummy;
  __asm__ __volatile__ ("pushl %3\n\tcall *%2"
                        : "=a" (ret), "=c" (dummy)
                        : "r" (func), "r" (arg), "1" (_this)
                        : "edx", "memory" );
  return ret;
}
#endif

#define call_func1(func,_this)   do_call_func1(func,_this)
#define call_func2(func,_this,a) do_call_func2(func,_this,(const void*)a)

#else

#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */

static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(invalid_parameter_handler);
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %lx\n", (UINT_PTR)arg);
    ok(errno != 0xdeadbeef, "errno not set\n");
}

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hcrt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HMODULE hcrt;
    HMODULE hkernel32;

    SetLastError(0xdeadbeef);
    hcrt = LoadLibraryA("msvcr90.dll");
    if (!hcrt) {
        win_skip("msvcr90.dll not installed (got %d)\n", GetLastError());
        return FALSE;
    }

    SET(p_set_invalid_parameter_handler, "_set_invalid_parameter_handler");
    if(p_set_invalid_parameter_handler)
        ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
                "Invalid parameter handler was already set\n");

    SET(p_initterm_e, "_initterm_e");
    SET(p_encode_pointer, "_encode_pointer");
    SET(p_decode_pointer, "_decode_pointer");
    SET(p_encoded_null, "_encoded_null");
    SET(p_sys_nerr, "_sys_nerr");
    SET(p__sys_nerr, "__sys_nerr");
    SET(p_sys_errlist, "_sys_errlist");
    SET(p__sys_errlist, "__sys_errlist");
    SET(p_strtoi64, "_strtoi64");
    SET(p_strtoui64, "_strtoui64");
    SET(p_itoa_s, "_itoa_s");
    SET(p_wcsncat_s,"wcsncat_s" );
    SET(p_qsort_s, "qsort_s");
    SET(p_bsearch_s, "bsearch_s");
    SET(p_controlfp_s, "_controlfp_s");
    SET(p_tmpfile_s, "tmpfile_s");
    SET(p_atoflt, "_atoflt");
    SET(p_set_abort_behavior, "_set_abort_behavior");
    SET(p_sopen_s, "_sopen_s");
    SET(p_wsopen_s, "_wsopen_s");
    SET(p_realloc_crt, "_realloc_crt");
    SET(p_malloc, "malloc");
    SET(p_free, "free");
    SET(p_getptd, "_getptd");
    SET(p_errno, "_errno");
    SET(p_doserrno, "__doserrno");
    SET(p_srand, "srand");
    SET(p_strtok, "strtok");
    SET(p_wcstok, "wcstok");
    SET(p__mbstok, "_mbstok");
    SET(p_strerror, "strerror");
    SET(p_wcserror, "_wcserror");
    SET(p_tmpnam, "tmpnam");
    SET(p_wtmpnam, "_wtmpnam");
    SET(p_asctime, "asctime");
    SET(p_wasctime, "_wasctime");
    SET(p_localtime64, "_localtime64");
    SET(p_ecvt, "_ecvt");
    SET(p_fpecode, "__fpecode");
    SET(p_configthreadlocale, "_configthreadlocale");
    SET(p_get_terminate, "_get_terminate");
    SET(p_get_unexpected, "_get_unexpected");
    SET(p__vswprintf_l, "__vswprintf_l");
    SET(p_vswprintf_l, "_vswprintf_l");
    SET(p_fopen, "fopen");
    SET(p_fclose, "fclose");
    SET(p_unlink, "_unlink");
    SET(p_access_s, "_access_s");
    SET(p_lock_file, "_lock_file");
    SET(p_unlock_file, "_unlock_file");
    SET(p_fileno, "_fileno");
    SET(p_feof, "feof");
    SET(p_ferror, "ferror");
    SET(p_flsbuf, "_flsbuf");
    SET(p_byteswap_ulong, "_byteswap_ulong");
    SET(p__pxcptinfoptrs, "__pxcptinfoptrs");
    SET(p__AdjustPointer, "__AdjustPointer");
    if (sizeof(void *) == 8)
    {
        SET(p_type_info_name_internal_method, "?_name_internal_method@type_info@@QEBAPEBDPEAU__type_info_node@@@Z");
        SET(ptype_info_dtor, "??1type_info@@UEAA@XZ");
        SET(p_is_exception_typeof, "?_is_exception_typeof@@YAHAEBVtype_info@@PEAU_EXCEPTION_POINTERS@@@Z");
    }
    else
    {
#ifdef __arm__
        SET(p_type_info_name_internal_method, "?_name_internal_method@type_info@@QBAPBDPAU__type_info_node@@@Z");
        SET(ptype_info_dtor, "??1type_info@@UAA@XZ");
#else
        SET(p_type_info_name_internal_method, "?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z");
        SET(ptype_info_dtor, "??1type_info@@UAE@XZ");
#endif
        SET(p_is_exception_typeof, "?_is_exception_typeof@@YAHABVtype_info@@PAU_EXCEPTION_POINTERS@@@Z");
    }

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pEncodePointer = (void *) GetProcAddress(hkernel32, "EncodePointer");
    return TRUE;
}

static int __cdecl initterm_cb0(void)
{
    cb_called[0]++;
    return 0;
}

static int __cdecl initterm_cb1(void)
{
    cb_called[1]++;
    return 1;
}

static int __cdecl initterm_cb2(void)
{
    cb_called[2]++;
    return 2;
}


static void test__initterm_e(void)
{
    _INITTERM_E_FN table[4];
    int res;

    memset(table, 0, sizeof(table));

    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, table);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, NULL);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    if (0) {
        /* this crash on Windows */
        errno = 0xdeadbeef;
        res = p_initterm_e(NULL, table);
        trace("got %d with 0x%x\n", res, errno);
    }

    table[0] = initterm_cb0;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[1]);
    ok( !res && (cb_called[0] == 1) && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);


    /* init-function returning failure */
    table[1] = initterm_cb1;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[3]);
    ok( (res == 1) && (cb_called[0] == 1) && (cb_called[1] == 1) && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* init-function not called, when end < start */
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(&table[3], table);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* initialization stop after first non-zero result */
    table[2] = initterm_cb0;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[3]);
    ok( (res == 1) && (cb_called[0] == 1) && (cb_called[1] == 1) && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* NULL pointer in the array are skipped */
    table[1] = NULL;
    table[2] = initterm_cb2;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[3]);
    ok( (res == 2) && (cb_called[0] == 1) && !cb_called[1] && (cb_called[2] == 1),
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

}

/* Beware that _encode_pointer is a NOP before XP
   (the parameter is returned unchanged) */
static void test__encode_pointer(void)
{
    void *ptr, *res;

    ptr = (void*)0xdeadbeef;
    res = p_encode_pointer(ptr);
    res = p_decode_pointer(res);
    ok(res == ptr, "Pointers are different after encoding and decoding\n");

    ok(p_encoded_null() == p_encode_pointer(NULL), "Error encoding null\n");

    ptr = p_encode_pointer(p_encode_pointer);
    ok(p_decode_pointer(ptr) == p_encode_pointer, "Error decoding pointer\n");

    /* Not present before XP */
    if (!pEncodePointer) {
        win_skip("EncodePointer not found\n");
        return;
    }

    res = pEncodePointer(p_encode_pointer);
    ok(ptr == res, "_encode_pointer produced different result than EncodePointer\n");

}

static void test_error_messages(void)
{
    int *size, size_copy;

    size = p__sys_nerr();
    size_copy = *size;
    ok(*p_sys_nerr == *size, "_sys_nerr = %u, size = %u\n", *p_sys_nerr, *size);

    *size = 20;
    ok(*p_sys_nerr == *size, "_sys_nerr = %u, size = %u\n", *p_sys_nerr, *size);

    *size = size_copy;

    ok(*p_sys_errlist == *(p__sys_errlist()), "p_sys_errlist != p__sys_errlist()\n");
}

static void test__strtoi64(void)
{
    __int64 res;
    unsigned __int64 ures;

    SET_EXPECT(invalid_parameter_handler);
    res = p_strtoi64(NULL, NULL, 10);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    res = p_strtoi64("123", NULL, 1);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    res = p_strtoi64("123", NULL, 37);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ures = p_strtoui64(NULL, NULL, 10);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ures = p_strtoui64("123", NULL, 1);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ures = p_strtoui64("123", NULL, 37);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);
}

static void test__itoa_s(void)
{
    errno_t ret;
    char buffer[33];

    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, NULL, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, buffer, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == 'X', "Expected the output buffer to be untouched\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, buffer, sizeof(buffer), 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, buffer, sizeof(buffer), 64);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(12345678, buffer, 4, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\000765", 4),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(12345678, buffer, 8, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\0007654321", 8),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(-12345678, buffer, 9, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\00087654321", 9),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    ret = p_itoa_s(12345678, buffer, 9, 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "12345678"),
       "Expected output buffer string to be \"12345678\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(43690, buffer, sizeof(buffer), 2);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "1010101010101010"),
       "Expected output buffer string to be \"1010101010101010\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(1092009, buffer, sizeof(buffer), 36);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "nell"),
       "Expected output buffer string to be \"nell\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(5704, buffer, sizeof(buffer), 18);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "hag"),
       "Expected output buffer string to be \"hag\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(-12345678, buffer, sizeof(buffer), 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "-12345678"),
       "Expected output buffer string to be \"-12345678\", got \"%s\"\n",
       buffer);
}

static void test_wcsncat_s(void)
{
    static wchar_t abcW[] = {'a','b','c',0};
    int ret;
    wchar_t dst[4];
    wchar_t src[4];

    memcpy(src, abcW, sizeof(abcW));
    dst[0] = 0;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(NULL, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 0, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 0, src, _TRUNCATE);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    ret = p_wcsncat_s(dst, 4, NULL, 0);
    ok(ret == 0, "err = %d\n", ret);

    dst[0] = 0;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 2, src, 4);
    ok(ret == ERANGE, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    dst[0] = 0;
    ret = p_wcsncat_s(dst, 2, src, _TRUNCATE);
    ok(ret == STRUNCATE, "err = %d\n", ret);
    ok(dst[0] == 'a' && dst[1] == 0, "dst is %s\n", wine_dbgstr_w(dst));

    memcpy(dst, abcW, sizeof(abcW));
    dst[3] = 'd';
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);
}

/* Based on dlls/ntdll/tests/string.c */
static __cdecl int intcomparefunc(void *context, const void *a, const void *b)
{
    const int *p = a, *q = b;

    ok (a != b, "must never get the same pointer\n");
    ++*(int *) context;

    return *p - *q;
}

static __cdecl int charcomparefunc(void *context, const void *a, const void *b)
{
    const char *p = a, *q = b;

    ok (a != b, "must never get the same pointer\n");
    ++*(int *) context;

    return *p - *q;
}

static __cdecl int strcomparefunc(void *context, const void *a, const void *b)
{
    const char * const *p = a;
    const char * const *q = b;

    ok (a != b, "must never get the same pointer\n");
    ++*(int *) context;

    return lstrcmpA(*p, *q);
}

static void test_qsort_s(void)
{
    int arr[5] = { 23, 42, 8, 4, 16 };
    int arr2[5] = { 23, 42, 8, 4, 16 };
    char carr[5] = { 42, 23, 4, 8, 16 };
    const char *strarr[7] = {
    "Hello",
    "Wine",
    "World",
    "!",
    "Hopefully",
    "Sorted",
    "."
    };

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 0, 0, NULL, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 0, 0, intcomparefunc, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 0, sizeof(int), NULL, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 1, sizeof(int), intcomparefunc, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    p_qsort_s(NULL, 0, sizeof(int), intcomparefunc, NULL);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );

    /* overflow without side effects, other overflow values crash */
    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr2, (((size_t)1) << (8*sizeof(size_t) - 1)) + 1, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );
    ok(arr2[0] == 23, "should remain unsorted, arr2[0] is %d\n", arr2[0]);
    ok(arr2[1] == 42, "should remain unsorted, arr2[1] is %d\n", arr2[1]);
    ok(arr2[2] == 8,  "should remain unsorted, arr2[2] is %d\n", arr2[2]);
    ok(arr2[3] == 4,  "should remain unsorted, arr2[3] is %d\n", arr2[3]);

    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 0, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );
    ok(arr[0] == 23, "badly sorted, nmemb=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=0, arr[4] is %d\n", arr[4]);

    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 1, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );
    ok(arr[0] == 23, "badly sorted, nmemb=1, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=1, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=1, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=1, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=1, arr[4] is %d\n", arr[4]);

    SET_EXPECT(invalid_parameter_handler);
    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 5, 0, intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok(arr[0] == 23, "badly sorted, size=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, size=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, size=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, size=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, size=0, arr[4] is %d\n", arr[4]);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 5, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(arr[0] == 4,  "badly sorted, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 8,  "badly sorted, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 16, "badly sorted, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 23, "badly sorted, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 42, "badly sorted, arr[4] is %d\n", arr[4]);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)carr, 5, sizeof(char), charcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(carr[0] == 4,  "badly sorted, carr[0] is %d\n", carr[0]);
    ok(carr[1] == 8,  "badly sorted, carr[1] is %d\n", carr[1]);
    ok(carr[2] == 16, "badly sorted, carr[2] is %d\n", carr[2]);
    ok(carr[3] == 23, "badly sorted, carr[3] is %d\n", carr[3]);
    ok(carr[4] == 42, "badly sorted, carr[4] is %d\n", carr[4]);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)strarr, 7, sizeof(char*), strcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(!strcmp(strarr[0],"!"),  "badly sorted, strarr[0] is %s\n", strarr[0]);
    ok(!strcmp(strarr[1],"."),  "badly sorted, strarr[1] is %s\n", strarr[1]);
    ok(!strcmp(strarr[2],"Hello"),  "badly sorted, strarr[2] is %s\n", strarr[2]);
    ok(!strcmp(strarr[3],"Hopefully"),  "badly sorted, strarr[3] is %s\n", strarr[3]);
    ok(!strcmp(strarr[4],"Sorted"),  "badly sorted, strarr[4] is %s\n", strarr[4]);
    ok(!strcmp(strarr[5],"Wine"),  "badly sorted, strarr[5] is %s\n", strarr[5]);
    ok(!strcmp(strarr[6],"World"),  "badly sorted, strarr[6] is %s\n", strarr[6]);
}

static void test_bsearch_s(void)
{
    int arr[7] = { 1, 3, 4, 8, 16, 23, 42 };
    int *x, l, i, j = 1;

    SET_EXPECT(invalid_parameter_handler);
    x = p_bsearch_s(NULL, NULL, 0, 0, NULL, NULL);
    ok(x == NULL, "Expected bsearch_s to return NULL, got %p\n", x);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    g_bsearch_s_context_counter = 0;
    SET_EXPECT(invalid_parameter_handler);
    x = p_bsearch_s(&l, arr, j, 0, intcomparefunc, &g_bsearch_s_context_counter);
    ok(x == NULL, "Expected bsearch_s to return NULL, got %p\n", x);
    ok(g_bsearch_s_context_counter == 0, "callback shouldn't have been called\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    g_bsearch_s_context_counter = 0;
    SET_EXPECT(invalid_parameter_handler);
    x = p_bsearch_s(&l, arr, j, sizeof(arr[0]), NULL, &g_bsearch_s_context_counter);
    ok(x == NULL, "Expected bsearch_s to return NULL, got %p\n", x);
    ok(g_bsearch_s_context_counter == 0, "callback shouldn't have been called\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    /* just try all array sizes */
    for (j=1;j<sizeof(arr)/sizeof(arr[0]);j++) {
        for (i=0;i<j;i++) {
            l = arr[i];
            g_bsearch_s_context_counter = 0;
            x = p_bsearch_s(&l, arr, j, sizeof(arr[0]), intcomparefunc, &g_bsearch_s_context_counter);
            ok (x == &arr[i], "bsearch_s did not find %d entry in loopsize %d.\n", i, j);
            ok(g_bsearch_s_context_counter > 0, "callback wasn't called\n");
        }
        l = 4242;
        g_bsearch_s_context_counter = 0;
        x = p_bsearch_s(&l, arr, j, sizeof(arr[0]), intcomparefunc, &g_bsearch_s_context_counter);
        ok (x == NULL, "bsearch_s did find 4242 entry in loopsize %d.\n", j);
        ok(g_bsearch_s_context_counter > 0, "callback wasn't called\n");
    }
}

static void test_controlfp_s(void)
{
    unsigned int cur;
    int ret;

    SET_EXPECT(invalid_parameter_handler);
    ret = p_controlfp_s( NULL, ~0, ~0 );
    ok( ret == EINVAL, "wrong result %d\n", ret );
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    cur = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_controlfp_s( &cur, ~0, ~0 );
    ok( ret == EINVAL, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    cur = 0xdeadbeef;
    ret = p_controlfp_s( &cur, 0, 0 );
    ok( !ret, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );

    SET_EXPECT(invalid_parameter_handler);
    cur = 0xdeadbeef;
    ret = p_controlfp_s( &cur, 0x80000000, 0x80000000 );
    ok( ret == EINVAL, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    cur = 0xdeadbeef;
    /* mask is only checked when setting invalid bits */
    ret = p_controlfp_s( &cur, 0, 0x80000000 );
    ok( !ret, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );
}

static void test_tmpfile_s( void )
{
    int ret;

    SET_EXPECT(invalid_parameter_handler);
    ret = p_tmpfile_s(NULL);
    ok(ret == EINVAL, "Expected tmpfile_s to return EINVAL, got %i\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);
}

typedef struct
{
    const char *str;
    float flt;
    int ret;
} _atoflt_test;

static const _atoflt_test _atoflt_testdata[] = {
    { "12.1", 12.1, 0 },
    { "-13.721", -13.721, 0 },
    { "INF", 0.0, 0 },
    { ".21e12", 0.21e12, 0 },
    { "214353e-3", 214.353, 0 },
    { "1d9999999999999999999", 0.0, _OVERFLOW },
    { "  d10", 0.0, 0 },
    /* more significant digits */
    { "1.23456789", 1.23456789, 0 },
    { "1.23456789e1", 12.3456789, 0 },
    { "1e39", 0.0, _OVERFLOW },
    { "1e-39", 0.0, _UNDERFLOW },
    { NULL }
};

static void test__atoflt(void)
{
    _CRT_FLOAT flt;
    int ret, i = 0;

if (0)
{
    /* crashes on native */
    p_atoflt(NULL, NULL);
    p_atoflt(NULL, (char*)_atoflt_testdata[0].str);
    p_atoflt(&flt, NULL);
}

    while (_atoflt_testdata[i].str)
    {
        ret = p_atoflt(&flt, (char*)_atoflt_testdata[i].str);
        ok(ret == _atoflt_testdata[i].ret, "got ret %d, expected ret %d, for %s\n", ret,
            _atoflt_testdata[i].ret, _atoflt_testdata[i].str);

        if (ret == 0)
          ok(almost_equal_f(flt.f, _atoflt_testdata[i].flt), "got %f, expected %f, for %s\n", flt.f,
              _atoflt_testdata[i].flt, _atoflt_testdata[i].str);

        i++;
    }
}

static void test__set_abort_behavior(void)
{
    unsigned int res;

    /* default is _WRITE_ABORT_MSG | _CALL_REPORTFAULT */
    res = p_set_abort_behavior(0, 0);
    ok (res == (_WRITE_ABORT_MSG | _CALL_REPORTFAULT),
        "got 0x%x (expected 0x%x)\n", res, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    /* no internal mask */
    p_set_abort_behavior(0xffffffff, 0xffffffff);
    res = p_set_abort_behavior(0, 0);
    ok (res == 0xffffffff, "got 0x%x (expected 0x%x)\n", res, 0xffffffff);

    /* set to default value */
    p_set_abort_behavior(_WRITE_ABORT_MSG | _CALL_REPORTFAULT, 0xffffffff);
}

static void test__sopen_s(void)
{
    int ret, fd;

    SET_EXPECT(invalid_parameter_handler);
    ret = p_sopen_s(NULL, "test", _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == EINVAL, "got %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    fd = 0xdead;
    ret = p_sopen_s(&fd, "test", _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == ENOENT, "got %d, expected ENOENT\n", ret);
    ok(fd == -1, "got %d\n", fd);
}

static void test__wsopen_s(void)
{
    wchar_t testW[] = {'t','e','s','t',0};
    int ret, fd;

    SET_EXPECT(invalid_parameter_handler);
    ret = p_wsopen_s(NULL, testW, _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == EINVAL, "got %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    fd = 0xdead;
    ret = p_wsopen_s(&fd, testW, _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == ENOENT, "got %d, expected ENOENT\n", ret);
    ok(fd == -1, "got %d\n", fd);
}

static void test__realloc_crt(void)
{
    void *mem;

if (0)
{
    /* crashes on some systems starting Vista */
    p_realloc_crt(NULL, 10);
}

    mem = p_malloc(10);
    ok(mem != NULL, "memory not allocated\n");

    mem = p_realloc_crt(mem, 20);
    ok(mem != NULL, "memory not reallocated\n");

    mem = p_realloc_crt(mem, 0);
    ok(mem == NULL, "memory not freed\n");

    mem = p_realloc_crt(NULL, 0);
    ok(mem != NULL, "memory not (re)allocated for size 0\n");
    p_free(mem);
}

static void test_typeinfo(void)
{
    static type_info t1 = { NULL, NULL,{'.','?','A','V','t','e','s','t','1','@','@',0,0,0,0,0 } };
    struct __type_info_node node;
    char *name;

    /* name */
    t1.name = NULL;
    node.memPtr = NULL;
    node.next = NULL;
    name = call_func2(p_type_info_name_internal_method, &t1, &node);
    ok(name != NULL, "got %p\n", name);
    ok(name && t1.name && !strcmp(name, t1.name), "bad name '%s' for t1\n", name);

    ok(t1.name && !strcmp(t1.name, "class test1"), "demangled to '%s' for t1\n", t1.name);
    call_func1(ptype_info_dtor, &t1);
}

/* Keep in sync with msvcrt/msvcrt.h */
struct __thread_data {
    DWORD                           tid;
    HANDLE                          handle;
    int                             thread_errno;
    __msvcrt_ulong                  thread_doserrno;
    int                             unk1;
    unsigned int                    random_seed;
    char                            *strtok_next;
    wchar_t                         *wcstok_next;
    unsigned char                   *mbstok_next;
    char                            *strerror_buffer;
    wchar_t                         *wcserror_buffer;
    char                            *tmpnam_buffer;
    wchar_t                         *wtmpnam_buffer;
    void                            *unk2[2];
    char                            *asctime_buffer;
    wchar_t                         *wasctime_buffer;
    struct tm                       *time_buffer;
    char                            *efcvt_buffer;
    int                             unk3[2];
    void                            *unk4[3];
    EXCEPTION_POINTERS              *xcptinfo;
    int                             fpecode;
    pthreadmbcinfo                  mbcinfo;
    pthreadlocinfo                  locinfo;
    BOOL                            have_locale;
    int                             unk5[1];
    void*                           terminate_handler;
    void*                           unexpected_handler;
    void*                           se_translator;
    void                            *unk6[3];
    int                             unk7;
    EXCEPTION_RECORD                *exc_record;
};

static void test_getptd(void)
{
    struct __thread_data *ptd = p_getptd();
    DWORD tid = GetCurrentThreadId();
    wchar_t testW[] = {'t','e','s','t',0}, tW[] = {'t',0}, *wp;
    char test[] = "test", *p;
    unsigned char mbstok_test[] = "test", *up;
    struct tm time;
    __time64_t secs = 0;
    int dec, sign;
    void *mbcinfo, *locinfo;

    ok(ptd->tid == tid, "ptd->tid = %x, expected %x\n", ptd->tid, tid);
    ok(ptd->handle == INVALID_HANDLE_VALUE, "ptd->handle = %p\n", ptd->handle);
    ok(p_errno() == &ptd->thread_errno, "ptd->thread_errno is different then _errno()\n");
    ok(p_doserrno() == &ptd->thread_doserrno, "ptd->thread_doserrno is different then __doserrno()\n");
    p_srand(1234);
    ok(ptd->random_seed == 1234, "ptd->random_seed = %d\n", ptd->random_seed);
    p = p_strtok(test, "t");
    ok(ptd->strtok_next == p+3, "ptd->strtok_next is incorrect\n");
    wp = p_wcstok(testW, tW);
    ok(ptd->wcstok_next == wp+3, "ptd->wcstok_next is incorrect\n");
    up = p__mbstok(mbstok_test, (unsigned char*)"t");
    ok(ptd->mbstok_next == up+3, "ptd->mbstok_next is incorrect\n");
    ok(p_strerror(0) == ptd->strerror_buffer, "ptd->strerror_buffer is incorrect\n");
    ok(p_wcserror(0) == ptd->wcserror_buffer, "ptd->wcserror_buffer is incorrect\n");
    ok(p_tmpnam(NULL) == ptd->tmpnam_buffer, "ptd->tmpnam_buffer is incorrect\n");
    ok(p_wtmpnam(NULL) == ptd->wtmpnam_buffer, "ptd->wtmpnam_buffer is incorrect\n");
    memset(&time, 0, sizeof(time));
    time.tm_mday = 1;
    ok(p_asctime(&time) == ptd->asctime_buffer, "ptd->asctime_buffer is incorrect\n");
    ok(p_wasctime(&time) == ptd->wasctime_buffer, "ptd->wasctime_buffer is incorrect\n");
    ok(p_localtime64(&secs) == ptd->time_buffer, "ptd->time_buffer is incorrect\n");
    ok(p_ecvt(3.12, 1, &dec, &sign) == ptd->efcvt_buffer, "ptd->efcvt_buffer is incorrect\n");
    ok(p__pxcptinfoptrs() == (void**)&ptd->xcptinfo, "ptd->xcptinfo is incorrect\n");
    ok(p_fpecode() == &ptd->fpecode, "ptd->fpecode is incorrect\n");
    mbcinfo = ptd->mbcinfo;
    locinfo = ptd->locinfo;
    todo_wine ok(ptd->have_locale == 1, "ptd->have_locale = %x\n", ptd->have_locale);
    p_configthreadlocale(1);
    todo_wine ok(mbcinfo == ptd->mbcinfo, "ptd->mbcinfo != mbcinfo\n");
    todo_wine ok(locinfo == ptd->locinfo, "ptd->locinfo != locinfo\n");
    todo_wine ok(ptd->have_locale == 3, "ptd->have_locale = %x\n", ptd->have_locale);
    ok(p_get_terminate() == ptd->terminate_handler, "ptd->terminate_handler != _get_terminate()\n");
    ok(p_get_unexpected() == ptd->unexpected_handler, "ptd->unexpected_handler != _get_unexpected()\n");
}

static int __cdecl __vswprintf_l_wrapper(wchar_t *buf,
        const wchar_t *format, _locale_t locale, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    ret = p__vswprintf_l(buf, format, locale, valist);
    __ms_va_end(valist);
    return ret;
}

static int __cdecl _vswprintf_l_wrapper(wchar_t *buf,
        const wchar_t *format, _locale_t locale, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    ret = p_vswprintf_l(buf, format, locale, valist);
    __ms_va_end(valist);
    return ret;
}

static void test__vswprintf_l(void)
{
    static const wchar_t format[] = {'t','e','s','t',0};

    wchar_t buf[32];
    int ret;

    ret = __vswprintf_l_wrapper(buf, format, NULL);
    ok(ret == 4, "ret = %d\n", ret);
    ok(!memcmp(buf, format, sizeof(format)), "buf = %s, expected %s\n",
            wine_dbgstr_w(buf), wine_dbgstr_w(format));

    ret = _vswprintf_l_wrapper(buf, format, NULL);
    ok(ret == 4, "ret = %d\n", ret);
    ok(!memcmp(buf, format, sizeof(format)), "buf = %s, expected %s\n",
            wine_dbgstr_w(buf), wine_dbgstr_w(format));
}

struct block_file_arg
{
    FILE *read;
    FILE *write;
    HANDLE init;
    HANDLE finish;
};

static DWORD WINAPI block_file(void *arg)
{
    struct block_file_arg *files = arg;

    p_lock_file(files->read);
    p_lock_file(files->write);
    SetEvent(files->init);
    WaitForSingleObject(files->finish, INFINITE);
    p_unlock_file(files->read);
    p_unlock_file(files->write);
    return 0;
}

static void test_nonblocking_file_access(void)
{
    HANDLE thread;
    struct block_file_arg arg;
    FILE *filer, *filew;
    int ret;

    if(!p_lock_file || !p_unlock_file) {
        win_skip("_lock_file not available\n");
        return;
    }

    filew = p_fopen("test_file", "w");
    ok(filew != NULL, "unable to create test file\n");
    if(!filew)
        return;
    filer = p_fopen("test_file", "r");
    ok(filer != NULL, "unable to open test file\n");
    if(!filer) {
        p_fclose(filew);
        p_unlink("test_file");
        return;
    }

    arg.read = filer;
    arg.write = filew;
    arg.init = CreateEventW(NULL, FALSE, FALSE, NULL);
    arg.finish = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(arg.init != NULL, "CreateEventW failed\n");
    ok(arg.finish != NULL, "CreateEventW failed\n");
    thread = CreateThread(NULL, 0, block_file, (void*)&arg, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(arg.init, INFINITE);

    ret = p_fileno(filer);
    ok(ret, "_fileno(filer) returned %d\n", ret);
    ret = p_fileno(filew);
    ok(ret, "_fileno(filew) returned %d\n", ret);

    ret = p_feof(filer);
    ok(ret==0, "feof(filer) returned %d\n", ret);
    ret = p_feof(filew);
    ok(ret==0, "feof(filew) returned %d\n", ret);

    ret = p_ferror(filer);
    ok(ret==0, "ferror(filer) returned %d\n", ret);
    ret = p_ferror(filew);
    ok(ret==0, "ferror(filew) returned %d\n", ret);

    ret = p_flsbuf('a', filer);
    ok(ret==-1, "_flsbuf(filer) returned %d\n", ret);
    ret = p_flsbuf('a', filew);
    ok(ret=='a', "_flsbuf(filew) returned %d\n", ret);

    SetEvent(arg.finish);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(arg.init);
    CloseHandle(arg.finish);
    CloseHandle(thread);
    p_fclose(filer);
    p_fclose(filew);
    p_unlink("test_file");
}

static void test_byteswap(void)
{
    unsigned long ret;

    ret = p_byteswap_ulong(0x12345678);
    ok(ret == 0x78563412, "ret = %lx\n", ret);

    ret = p_byteswap_ulong(0);
    ok(ret == 0, "ret = %lx\n", ret);
}

static void test_access_s(void)
{
    FILE *f;
    int res;

    f = p_fopen("test_file", "w");
    ok(f != NULL, "unable to create test file\n");
    if(!f)
        return;

    p_fclose(f);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 0);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 2);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 4);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 6);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    SetFileAttributesA("test_file", FILE_ATTRIBUTE_READONLY);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 0);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 2);
    ok(res == EACCES, "got %x\n", res);
    ok(errno == EACCES, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 4);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 6);
    ok(res == EACCES, "got %x\n", res);
    ok(errno == EACCES, "got %x\n", res);

    SetFileAttributesA("test_file", FILE_ATTRIBUTE_NORMAL);

    p_unlink("test_file");

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 0);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 2);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 4);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);

    errno = 0xdeadbeef;
    res = p_access_s("test_file", 6);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);
}

#ifndef __x86_64__
#define EXCEPTION_REF(instance, name) &instance.name
#else
#define EXCEPTION_REF(instance, name) FIELD_OFFSET(struct _exception_data, name)
#endif
static void test_is_exception_typeof(void)
{
    const type_info ti1 =  {NULL, NULL, {'.','?','A','V','t','e','s','t','1','@','@',0}};
    const type_info ti2 =  {NULL, NULL, {'.','?','A','V','t','e','s','t','2','@','@',0}};
    const type_info ti3 =  {NULL, NULL, {'.','?','A','V','t','e','s','t','3','@','@',0}};

    const struct _exception_data {
        type_info ti1;
        type_info ti2;
        cxx_type_info cti1;
        cxx_type_info cti2;
        cxx_type_info_table tit;
        cxx_exception_type et;
    } exception_data = {
        {NULL, NULL, {'.','?','A','V','t','e','s','t','1','@','@',0}},
        {NULL, NULL, {'.','?','A','V','t','e','s','t','2','@','@',0}},
        {0, EXCEPTION_REF(exception_data, ti1)},
        {0, EXCEPTION_REF(exception_data, ti2)},
        {2, {EXCEPTION_REF(exception_data, cti1), EXCEPTION_REF(exception_data, cti2)}},
        {0, 0, 0, EXCEPTION_REF(exception_data, tit)}
    };

    EXCEPTION_RECORD rec = {CXX_EXCEPTION, 0, NULL, NULL, 3, {CXX_FRAME_MAGIC_VC6, 0, (ULONG_PTR)&exception_data.et}};
    EXCEPTION_POINTERS except_ptrs = {&rec, NULL};

    int ret;

#ifdef __x86_64__
    rec.NumberParameters = 4;
    rec.ExceptionInformation[3] = (ULONG_PTR)&exception_data;
#endif

    ret = p_is_exception_typeof(&ti1, &except_ptrs);
    ok(ret == 1, "_is_exception_typeof returned %d\n", ret);
    ret = p_is_exception_typeof(&ti2, &except_ptrs);
    ok(ret == 1, "_is_exception_typeof returned %d\n", ret);
    ret = p_is_exception_typeof(&ti3, &except_ptrs);
    ok(ret == 0, "_is_exception_typeof returned %d\n", ret);
}

static void test__AdjustPointer(void)
{
    int off = 0xf0;
    void *obj1 = &off;
    void *obj2 = (char*)&off - 2;
    struct test_data {
        void *ptr;
        void *ret;
        struct {
            int this_offset;
            int vbase_descr;
            int vbase_offset;
        } this_ptr_offsets;
    } data[] = {
        {NULL, NULL, {0, -1, 0}},
        {(void*)0xbeef, (void*)0xbef0, {1, -1, 1}},
        {(void*)0xbeef, (void*)0xbeee, {-1, -1, 0}},
        {&obj1, (char*)&obj1 + off, {0, 0, 0}},
        {(char*)&obj1 - 5, (char*)&obj1 + off, {0, 5, 0}},
        {(char*)&obj1 - 3, (char*)&obj1 + off + 24, {24, 3, 0}},
        {(char*)&obj2 - 17, (char*)&obj2 + off + 4, {4, 17, 2}}
    };
    void *ret;
    int i;

    for(i=0; i<sizeof(data)/sizeof(data[0]); i++) {
        ret = p__AdjustPointer(data[i].ptr, &data[i].this_ptr_offsets);
        ok(ret == data[i].ret, "%d) __AdjustPointer returned %p, expected %p\n", i, ret, data[i].ret);
    }
}

START_TEST(msvcr90)
{
    if(!init())
        return;

    test__initterm_e();
    test__encode_pointer();
    test_error_messages();
    test__strtoi64();
    test__itoa_s();
    test_wcsncat_s();
    test_qsort_s();
    test_bsearch_s();
    test_controlfp_s();
    test_tmpfile_s();
    test__atoflt();
    test__set_abort_behavior();
    test__sopen_s();
    test__wsopen_s();
    test__realloc_crt();
    test_typeinfo();
    test_getptd();
    test__vswprintf_l();
    test_nonblocking_file_access();
    test_byteswap();
    test_access_s();
    test_is_exception_typeof();
    test__AdjustPointer();
}
