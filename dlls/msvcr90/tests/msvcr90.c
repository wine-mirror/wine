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
#include <float.h>
#include <mbctype.h>
#include <mbstring.h>
#include <share.h>
#include <sys/stat.h>
#include <time.h>
#include <locale.h>
#include <fpieee.h>
#include <excpt.h>
#include <process.h>
#include <search.h>
#include <signal.h>

#include <windef.h>
#include <winbase.h>
#include <errno.h>
#include "wine/test.h"

#define WX_OPEN           0x01
#define WX_ATEOF          0x02
#define WX_READNL         0x04
#define WX_PIPE           0x08
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TTY            0x40
#define WX_TEXT           0x80

#define MSVCRT_FD_BLOCK_SIZE 32

typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                lookahead[3];
    int                 exflag;
    CRITICAL_SECTION    crit;
    char textmode : 7;
    char unicode : 1;
    char pipech2[2];
    __int64 startpos;
    BOOL utf8translations;
    char dbcsBuffer;
    BOOL dbcsBufferUsed;
} ioinfo;
static ioinfo **__pioinfo;
#undef _sys_nerr
static int *_sys_nerr;
#undef _sys_errlist
static char **_sys_errlist;

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

void* __cdecl _encode_pointer(void *);
void* __cdecl _decode_pointer(void *);
void* __cdecl _encoded_null(void);
void* __cdecl _realloc_crt(void*, size_t);
void* __cdecl _getptd(void);
void* __cdecl _get_terminate(void);
void* __cdecl _get_unexpected(void);
int __cdecl __vswprintf_l(wchar_t*, const wchar_t*, _locale_t, va_list);
int __cdecl _vswprintf_l(wchar_t*, const wchar_t*, _locale_t, va_list);
void* __cdecl __AdjustPointer(void*, const void*);

struct __lc_time_data {
    const char *short_wday[7];
    const char *wday[7];
    const char *short_mon[12];
    const char *mon[12];
    const char *am;
    const char *pm;
    const char *short_date;
    const char *date;
    const char *time;
    LCID lcid;
    int  unk;
    int  refcount;
};

typedef struct threadmbcinfostruct {
    int refcount;
    int mbcodepage;
    int ismbcodepage;
    int mblcid;
    unsigned short mbulinfo[6];
    unsigned char mbctype[257];
    unsigned char mbcasemap[256];
} threadmbcinfo;

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
#ifdef __i386__
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
#ifdef __i386__
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
#ifdef __i386__
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
#define call_func2(func,_this,a) do_call_func2(func,_this,(const void*)(a))

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
    ok(arg == 0, "arg = %Ix\n", arg);
    ok(errno != 0xdeadbeef, "errno not set\n");
}

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hcrt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HMODULE hcrt = GetModuleHandleA("msvcr90.dll");

    ok(_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    SET(__pioinfo, "__pioinfo");
    SET(_sys_nerr, "_sys_nerr");
    SET(_sys_errlist, "_sys_errlist");

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

    pEncodePointer = (void *) GetProcAddress(GetModuleHandleA("kernel32.dll"), "EncodePointer");
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
    _PIFV table[4];
    int res;

    memset(table, 0, sizeof(table));

    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(table, table);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(table, NULL);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    if (0) {
        /* this crash on Windows */
        errno = 0xdeadbeef;
        res = _initterm_e(NULL, table);
        trace("got %d with 0x%x\n", res, errno);
    }

    table[0] = initterm_cb0;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(table, &table[1]);
    ok( !res && (cb_called[0] == 1) && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);


    /* init-function returning failure */
    table[1] = initterm_cb1;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(table, &table[3]);
    ok( (res == 1) && (cb_called[0] == 1) && (cb_called[1] == 1) && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* init-function not called, when end < start */
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(&table[3], table);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* initialization stop after first non-zero result */
    table[2] = initterm_cb0;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(table, &table[3]);
    ok( (res == 1) && (cb_called[0] == 1) && (cb_called[1] == 1) && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* NULL pointer in the array are skipped */
    table[1] = NULL;
    table[2] = initterm_cb2;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = _initterm_e(table, &table[3]);
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
    res = _encode_pointer(ptr);
    res = _decode_pointer(res);
    ok(res == ptr, "Pointers are different after encoding and decoding\n");

    ok(_encoded_null() == _encode_pointer(NULL), "Error encoding null\n");

    ptr = _encode_pointer(_encode_pointer);
    ok(_decode_pointer(ptr) == _encode_pointer, "Error decoding pointer\n");

    /* Not present before XP */
    if (!pEncodePointer) {
        win_skip("EncodePointer not found\n");
        return;
    }

    res = pEncodePointer(_encode_pointer);
    ok(ptr == res, "_encode_pointer produced different result than EncodePointer\n");

}

static void test_error_messages(void)
{
    int *size, size_copy;

    size = __sys_nerr();
    size_copy = *size;
    ok(*_sys_nerr == *size, "_sys_nerr = %u, size = %u\n", *_sys_nerr, *size);

    *size = 20;
    ok(*_sys_nerr == *size, "_sys_nerr = %u, size = %u\n", *_sys_nerr, *size);

    *size = size_copy;

    ok(*_sys_errlist == *(__sys_errlist()), "_sys_errlist != __sys_errlist()\n");
}

static void test__strtoi64(void)
{
    __int64 res;
    unsigned __int64 ures;

    SET_EXPECT(invalid_parameter_handler);
    res = _strtoi64(NULL, NULL, 10);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    res = _strtoi64("123", NULL, 1);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    res = _strtoi64("123", NULL, 37);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ures = _strtoui64(NULL, NULL, 10);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ures = _strtoui64("123", NULL, 1);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ures = _strtoui64("123", NULL, 37);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);
}

static void test__itoa_s(void)
{
    errno_t ret;
    char buffer[33];

    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(0, NULL, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(0, buffer, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == 'X', "Expected the output buffer to be untouched\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(0, buffer, sizeof(buffer), 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(0, buffer, sizeof(buffer), 64);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(12345678, buffer, 4, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\000765", 4),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(12345678, buffer, 8, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\0007654321", 8),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = _itoa_s(-12345678, buffer, 9, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\00087654321", 9),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    ret = _itoa_s(12345678, buffer, 9, 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "12345678"),
       "Expected output buffer string to be \"12345678\", got \"%s\"\n",
       buffer);

    ret = _itoa_s(43690, buffer, sizeof(buffer), 2);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "1010101010101010"),
       "Expected output buffer string to be \"1010101010101010\", got \"%s\"\n",
       buffer);

    ret = _itoa_s(1092009, buffer, sizeof(buffer), 36);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "nell"),
       "Expected output buffer string to be \"nell\", got \"%s\"\n",
       buffer);

    ret = _itoa_s(5704, buffer, sizeof(buffer), 18);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "hag"),
       "Expected output buffer string to be \"hag\", got \"%s\"\n",
       buffer);

    ret = _itoa_s(-12345678, buffer, sizeof(buffer), 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "-12345678"),
       "Expected output buffer string to be \"-12345678\", got \"%s\"\n",
       buffer);
}

static void test_wcsncat_s(void)
{
    int ret;
    wchar_t dst[4];
    wchar_t src[4];

    wcscpy(src, L"abc");
    dst[0] = 0;
    SET_EXPECT(invalid_parameter_handler);
    ret = wcsncat_s(NULL, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = wcsncat_s(dst, 0, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = wcsncat_s(dst, 0, src, _TRUNCATE);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    ret = wcsncat_s(dst, 4, NULL, 0);
    ok(ret == 0, "err = %d\n", ret);

    dst[0] = 0;
    SET_EXPECT(invalid_parameter_handler);
    ret = wcsncat_s(dst, 2, src, 4);
    ok(ret == ERANGE, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, ERANGE);

    dst[0] = 0;
    ret = wcsncat_s(dst, 2, src, _TRUNCATE);
    ok(ret == STRUNCATE, "err = %d\n", ret);
    ok(dst[0] == 'a' && dst[1] == 0, "dst is %s\n", wine_dbgstr_w(dst));

    memcpy(dst, L"abcd", 4 * sizeof(wchar_t));
    SET_EXPECT(invalid_parameter_handler);
    ret = wcsncat_s(dst, 4, src, 4);
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
    qsort_s(NULL, 0, 0, NULL, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    qsort_s(NULL, 0, 0, intcomparefunc, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    qsort_s(NULL, 0, sizeof(int), NULL, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    qsort_s(NULL, 1, sizeof(int), intcomparefunc, NULL);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    qsort_s(NULL, 0, sizeof(int), intcomparefunc, NULL);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );

    /* overflow without side effects, other overflow values crash */
    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    qsort_s((void*)arr2, (((size_t)1) << (8*sizeof(size_t) - 1)) + 1, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );
    ok(arr2[0] == 23, "should remain unsorted, arr2[0] is %d\n", arr2[0]);
    ok(arr2[1] == 42, "should remain unsorted, arr2[1] is %d\n", arr2[1]);
    ok(arr2[2] == 8,  "should remain unsorted, arr2[2] is %d\n", arr2[2]);
    ok(arr2[3] == 4,  "should remain unsorted, arr2[3] is %d\n", arr2[3]);

    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    qsort_s((void*)arr, 0, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );
    ok(arr[0] == 23, "badly sorted, nmemb=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=0, arr[4] is %d\n", arr[4]);

    errno = 0xdeadbeef;
    g_qsort_s_context_counter = 0;
    qsort_s((void*)arr, 1, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok( errno == 0xdeadbeef, "wrong errno %u\n", errno );
    ok(arr[0] == 23, "badly sorted, nmemb=1, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=1, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=1, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=1, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=1, arr[4] is %d\n", arr[4]);

    SET_EXPECT(invalid_parameter_handler);
    g_qsort_s_context_counter = 0;
    qsort_s((void*)arr, 5, 0, intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok(arr[0] == 23, "badly sorted, size=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, size=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, size=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, size=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, size=0, arr[4] is %d\n", arr[4]);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    g_qsort_s_context_counter = 0;
    qsort_s((void*)arr, 5, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(arr[0] == 4,  "badly sorted, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 8,  "badly sorted, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 16, "badly sorted, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 23, "badly sorted, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 42, "badly sorted, arr[4] is %d\n", arr[4]);

    g_qsort_s_context_counter = 0;
    qsort_s((void*)carr, 5, sizeof(char), charcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(carr[0] == 4,  "badly sorted, carr[0] is %d\n", carr[0]);
    ok(carr[1] == 8,  "badly sorted, carr[1] is %d\n", carr[1]);
    ok(carr[2] == 16, "badly sorted, carr[2] is %d\n", carr[2]);
    ok(carr[3] == 23, "badly sorted, carr[3] is %d\n", carr[3]);
    ok(carr[4] == 42, "badly sorted, carr[4] is %d\n", carr[4]);

    g_qsort_s_context_counter = 0;
    qsort_s((void*)strarr, 7, sizeof(char*), strcomparefunc, &g_qsort_s_context_counter);
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
    x = bsearch_s(NULL, NULL, 0, 0, NULL, NULL);
    ok(x == NULL, "Expected bsearch_s to return NULL, got %p\n", x);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    g_bsearch_s_context_counter = 0;
    SET_EXPECT(invalid_parameter_handler);
    x = bsearch_s(&l, arr, j, 0, intcomparefunc, &g_bsearch_s_context_counter);
    ok(x == NULL, "Expected bsearch_s to return NULL, got %p\n", x);
    ok(g_bsearch_s_context_counter == 0, "callback shouldn't have been called\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    g_bsearch_s_context_counter = 0;
    SET_EXPECT(invalid_parameter_handler);
    x = bsearch_s(&l, arr, j, sizeof(arr[0]), NULL, &g_bsearch_s_context_counter);
    ok(x == NULL, "Expected bsearch_s to return NULL, got %p\n", x);
    ok(g_bsearch_s_context_counter == 0, "callback shouldn't have been called\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    /* just try all array sizes */
    for (j=1; j<ARRAY_SIZE(arr); j++) {
        for (i=0;i<j;i++) {
            l = arr[i];
            g_bsearch_s_context_counter = 0;
            x = bsearch_s(&l, arr, j, sizeof(arr[0]), intcomparefunc, &g_bsearch_s_context_counter);
            ok (x == &arr[i], "bsearch_s did not find %d entry in loopsize %d.\n", i, j);
            ok(g_bsearch_s_context_counter > 0, "callback wasn't called\n");
        }
        l = 4242;
        g_bsearch_s_context_counter = 0;
        x = bsearch_s(&l, arr, j, sizeof(arr[0]), intcomparefunc, &g_bsearch_s_context_counter);
        ok (x == NULL, "bsearch_s did find 4242 entry in loopsize %d.\n", j);
        ok(g_bsearch_s_context_counter > 0, "callback wasn't called\n");
    }
}

static void test_controlfp_s(void)
{
    unsigned int cur;
    int ret;

    SET_EXPECT(invalid_parameter_handler);
    ret = _controlfp_s( NULL, ~0, ~0 );
    ok( ret == EINVAL, "wrong result %d\n", ret );
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    cur = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = _controlfp_s( &cur, ~0, ~0 );
    ok( ret == EINVAL, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    cur = 0xdeadbeef;
    ret = _controlfp_s( &cur, 0, 0 );
    ok( !ret, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );

    SET_EXPECT(invalid_parameter_handler);
    cur = 0xdeadbeef;
    ret = _controlfp_s( &cur, 0x80000000, 0x80000000 );
    ok( ret == EINVAL, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    cur = 0xdeadbeef;
    /* mask is only checked when setting invalid bits */
    ret = _controlfp_s( &cur, 0, 0x80000000 );
    ok( !ret, "wrong result %d\n", ret );
    ok( cur != 0xdeadbeef, "value not set\n" );
}

static void test_tmpfile_s( void )
{
    int ret;

    SET_EXPECT(invalid_parameter_handler);
    ret = tmpfile_s(NULL);
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
    _atoflt(NULL, NULL);
    _atoflt(NULL, (char*)_atoflt_testdata[0].str);
    _atoflt(&flt, NULL);
}

    while (_atoflt_testdata[i].str)
    {
        ret = _atoflt(&flt, (char*)_atoflt_testdata[i].str);
        ok(ret == _atoflt_testdata[i].ret, "got ret %d, expected ret %d, for %s\n", ret,
            _atoflt_testdata[i].ret, _atoflt_testdata[i].str);

        if (ret == 0)
          ok(flt.f == _atoflt_testdata[i].flt, "got %f, expected %f, for %s\n", flt.f,
              _atoflt_testdata[i].flt, _atoflt_testdata[i].str);

        i++;
    }
}

static void test__set_abort_behavior(void)
{
    unsigned int res;

    /* default is _WRITE_ABORT_MSG | _CALL_REPORTFAULT */
    res = _set_abort_behavior(0, 0);
    ok (res == (_WRITE_ABORT_MSG | _CALL_REPORTFAULT),
        "got 0x%x (expected 0x%x)\n", res, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    /* no internal mask */
    _set_abort_behavior(0xffffffff, 0xffffffff);
    res = _set_abort_behavior(0, 0);
    ok (res == 0xffffffff, "got 0x%x (expected 0x%x)\n", res, 0xffffffff);

    /* set to default value */
    _set_abort_behavior(_WRITE_ABORT_MSG | _CALL_REPORTFAULT, 0xffffffff);
}

static void test__sopen_s(void)
{
    int ret, fd;

    SET_EXPECT(invalid_parameter_handler);
    ret = _sopen_s(NULL, "test", _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == EINVAL, "got %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    fd = 0xdead;
    ret = _sopen_s(&fd, "test", _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == ENOENT, "got %d, expected ENOENT\n", ret);
    ok(fd == -1, "got %d\n", fd);
}

static void test__wsopen_s(void)
{
    int ret, fd;

    SET_EXPECT(invalid_parameter_handler);
    ret = _wsopen_s(NULL, L"test", _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == EINVAL, "got %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    fd = 0xdead;
    ret = _wsopen_s(&fd, L"test", _O_RDONLY, _SH_DENYNO, _S_IREAD);
    ok(ret == ENOENT, "got %d, expected ENOENT\n", ret);
    ok(fd == -1, "got %d\n", fd);
}

static void test__realloc_crt(void)
{
    void *mem;

if (0)
{
    /* crashes on some systems starting Vista */
    _realloc_crt(NULL, 10);
}

    mem = malloc(10);
    ok(mem != NULL, "memory not allocated\n");

    mem = _realloc_crt(mem, 20);
    ok(mem != NULL, "memory not reallocated\n");

    mem = _realloc_crt(mem, 0);
    ok(mem == NULL, "memory not freed\n");

    mem = _realloc_crt(NULL, 0);
    ok(mem != NULL, "memory not (re)allocated for size 0\n");
    free(mem);
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
    void                            *unk6;
    EXCEPTION_RECORD                *exc_record;
};

static void test_getptd(void)
{
    struct __thread_data *ptd = _getptd();
    DWORD tid = GetCurrentThreadId();
    wchar_t testW[] = L"test", *wp;
    char test[] = "test", *p;
    unsigned char mbstok_test[] = "test", *up;
    struct tm time;
    __time64_t secs = 0;
    int dec, sign;
    void *mbcinfo, *locinfo;

    ok(ptd->tid == tid, "ptd->tid = %lx, expected %lx\n", ptd->tid, tid);
    ok(ptd->handle == INVALID_HANDLE_VALUE, "ptd->handle = %p\n", ptd->handle);
    ok(_errno() == &ptd->thread_errno, "ptd->thread_errno is different then _errno()\n");
    ok(__doserrno() == &ptd->thread_doserrno, "ptd->thread_doserrno is different then __doserrno()\n");
    srand(1234);
    ok(ptd->random_seed == 1234, "ptd->random_seed = %d\n", ptd->random_seed);
    p = strtok(test, "t");
    ok(ptd->strtok_next == p+3, "ptd->strtok_next is incorrect\n");
    wp = wcstok(testW, L"t");
    ok(ptd->wcstok_next == wp+3, "ptd->wcstok_next is incorrect\n");
    up = _mbstok(mbstok_test, (unsigned char*)"t");
    ok(ptd->mbstok_next == up+3, "ptd->mbstok_next is incorrect\n");
    ok(strerror(0) == ptd->strerror_buffer, "ptd->strerror_buffer is incorrect\n");
    ok(_wcserror(0) == ptd->wcserror_buffer, "ptd->wcserror_buffer is incorrect\n");
    ok(tmpnam(NULL) == ptd->tmpnam_buffer, "ptd->tmpnam_buffer is incorrect\n");
    ok(_wtmpnam(NULL) == ptd->wtmpnam_buffer, "ptd->wtmpnam_buffer is incorrect\n");
    memset(&time, 0, sizeof(time));
    time.tm_mday = 1;
    ok(asctime(&time) == ptd->asctime_buffer, "ptd->asctime_buffer is incorrect\n");
    ok(_wasctime(&time) == ptd->wasctime_buffer, "ptd->wasctime_buffer is incorrect\n");
    ok(_localtime64(&secs) == ptd->time_buffer, "ptd->time_buffer is incorrect\n");
    ok(_ecvt(3.12, 1, &dec, &sign) == ptd->efcvt_buffer, "ptd->efcvt_buffer is incorrect\n");
    ok(__pxcptinfoptrs() == (void**)&ptd->xcptinfo, "ptd->xcptinfo is incorrect\n");
    ok(__fpecode() == &ptd->fpecode, "ptd->fpecode is incorrect\n");
    mbcinfo = ptd->mbcinfo;
    locinfo = ptd->locinfo;
    ok(ptd->have_locale == 1, "ptd->have_locale = %x\n", ptd->have_locale);
    _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
    ok(mbcinfo == ptd->mbcinfo, "ptd->mbcinfo != mbcinfo\n");
    ok(locinfo == ptd->locinfo, "ptd->locinfo != locinfo\n");
    ok(ptd->have_locale == 3, "ptd->have_locale = %x\n", ptd->have_locale);
    ok(_get_terminate() == ptd->terminate_handler, "ptd->terminate_handler != _get_terminate()\n");
    ok(_get_unexpected() == ptd->unexpected_handler, "ptd->unexpected_handler != _get_unexpected()\n");
    _configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
}

static int WINAPIV __vswprintf_l_wrapper(wchar_t *buf,
        const wchar_t *format, _locale_t locale, ...)
{
    int ret;
    va_list valist;
    va_start(valist, locale);
    ret = __vswprintf_l(buf, format, locale, valist);
    va_end(valist);
    return ret;
}

static int WINAPIV _vswprintf_l_wrapper(wchar_t *buf,
        const wchar_t *format, _locale_t locale, ...)
{
    int ret;
    va_list valist;
    va_start(valist, locale);
    ret = _vswprintf_l(buf, format, locale, valist);
    va_end(valist);
    return ret;
}

static void test__vswprintf_l(void)
{
    wchar_t buf[32];
    int ret;

    ret = __vswprintf_l_wrapper(buf, L"test", NULL);
    ok(ret == 4, "ret = %d\n", ret);
    ok(!wcscmp(buf, L"test"), "buf = %s\n", wine_dbgstr_w(buf));

    ret = _vswprintf_l_wrapper(buf, L"test", NULL);
    ok(ret == 4, "ret = %d\n", ret);
    ok(!wcscmp(buf, L"test"), "buf = %s\n", wine_dbgstr_w(buf));
}

struct block_file_arg
{
    FILE *read;
    FILE *write;
    HANDLE init;
    HANDLE finish;
    LONG deadlock_test;
};

static DWORD WINAPI block_file(void *arg)
{
    struct block_file_arg *files = arg;
    int deadlock_test;

    _lock_file(files->read);
    _lock_file(files->write);
    SetEvent(files->init);

    WaitForSingleObject(files->finish, INFINITE);
    Sleep(200);
    deadlock_test = InterlockedIncrement(&files->deadlock_test);
    ok(deadlock_test == 1, "deadlock_test = %d\n", deadlock_test);
    _unlock_file(files->read);
    _unlock_file(files->write);
    return 0;
}

static void test_nonblocking_file_access(void)
{
    HANDLE thread;
    struct block_file_arg arg;
    FILE *filer, *filew;
    int ret;

    filew = fopen("test_file", "w");
    ok(filew != NULL, "unable to create test file\n");
    if(!filew)
        return;
    filer = fopen("test_file", "r");
    ok(filer != NULL, "unable to open test file\n");
    if(!filer) {
        fclose(filew);
        unlink("test_file");
        return;
    }

    arg.read = filer;
    arg.write = filew;
    arg.init = CreateEventW(NULL, FALSE, FALSE, NULL);
    arg.finish = CreateEventW(NULL, FALSE, FALSE, NULL);
    arg.deadlock_test = 0;
    ok(arg.init != NULL, "CreateEventW failed\n");
    ok(arg.finish != NULL, "CreateEventW failed\n");
    thread = CreateThread(NULL, 0, block_file, (void*)&arg, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(arg.init, INFINITE);

    ret = _fileno(filer);
    ok(ret, "_fileno(filer) returned %d\n", ret);
    ret = _fileno(filew);
    ok(ret, "_fileno(filew) returned %d\n", ret);

    ret = feof(filer);
    ok(ret==0, "feof(filer) returned %d\n", ret);
    ret = feof(filew);
    ok(ret==0, "feof(filew) returned %d\n", ret);

    ret = ferror(filer);
    ok(ret==0, "ferror(filer) returned %d\n", ret);
    ret = ferror(filew);
    ok(ret==0, "ferror(filew) returned %d\n", ret);

    ret = _flsbuf('a', filer);
    ok(ret==-1, "_flsbuf(filer) returned %d\n", ret);
    ret = _flsbuf('a', filew);
    ok(ret=='a', "_flsbuf(filew) returned %d\n", ret);

    ret = _filbuf(filer);
    ok(ret==-1, "_filbuf(filer) returned %d\n", ret);
    ret = _filbuf(filew);
    ok(ret==-1, "_filbuf(filew) returned %d\n", ret);

    ret = _fflush_nolock(filer);
    ok(ret==0, "_fflush_nolock(filer) returned %d\n", ret);
    ret = _fflush_nolock(filew);
    ok(ret==0, "_fflush_nolock(filew) returned %d\n", ret);

    SetEvent(arg.finish);

    ret = _fflush_nolock(NULL);
    ok(ret==0, "_fflush_nolock(NULL) returned %d\n", ret);
    ret = InterlockedIncrement(&arg.deadlock_test);
    ok(ret==2, "InterlockedIncrement returned %d\n", ret);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(arg.init);
    CloseHandle(arg.finish);
    CloseHandle(thread);
    fclose(filer);
    fclose(filew);
    unlink("test_file");
}

static void test_byteswap(void)
{
    unsigned long ret;

    ret = _byteswap_ulong(0x12345678);
    ok(ret == 0x78563412, "ret = %lx\n", ret);

    ret = _byteswap_ulong(0);
    ok(ret == 0, "ret = %lx\n", ret);
}

static void test_access_s(void)
{
    FILE *f;
    int res;

    f = fopen("test_file", "w");
    ok(f != NULL, "unable to create test file\n");
    if(!f)
        return;

    fclose(f);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 0);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 2);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 4);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 6);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    SetFileAttributesA("test_file", FILE_ATTRIBUTE_READONLY);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 0);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 2);
    ok(res == EACCES, "got %x\n", res);
    ok(errno == EACCES, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 4);
    ok(res == 0, "got %x\n", res);
    ok(errno == 0xdeadbeef, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 6);
    ok(res == EACCES, "got %x\n", res);
    ok(errno == EACCES, "got %x\n", res);

    SetFileAttributesA("test_file", FILE_ATTRIBUTE_NORMAL);

    unlink("test_file");

    errno = 0xdeadbeef;
    res = _access_s("test_file", 0);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 2);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 4);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);

    errno = 0xdeadbeef;
    res = _access_s("test_file", 6);
    ok(res == ENOENT, "got %x\n", res);
    ok(errno == ENOENT, "got %x\n", res);
}

#ifdef __i386__
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

#ifndef __i386__
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
    struct {
        int vbase;
        int off;
    } obj = { 0, 0xf0 };
    void *obj1 = &obj.off;
    void *obj2 = &obj;
    struct test_data {
        uintptr_t ptr;
        uintptr_t ret;
        struct {
            int this_offset;
            int vbase_descr;
            int vbase_offset;
        } this_ptr_offsets;
    } data[] = {
        {0, 0, {0, -1, 0}},
        {0xbeef, 0xbef0, {1, -1, 1}},
        {0xbeef, 0xbeee, {-1, -1, 0}},
        {(uintptr_t)&obj1, (uintptr_t)&obj1 + obj.off, {0, 0, 0}},
        {(uintptr_t)&obj1 - 5, (uintptr_t)&obj1 + obj.off, {0, 5, 0}},
        {(uintptr_t)&obj1 - 3, (uintptr_t)&obj1 + obj.off + 24, {24, 3, 0}},
        {(uintptr_t)&obj2 - 17, (uintptr_t)&obj2 + obj.off + 4, {4, 17, sizeof(int)}}
    };
    void *ret;
    int i;

    for(i=0; i<ARRAY_SIZE(data); i++) {
        ret = __AdjustPointer((void*)data[i].ptr, &data[i].this_ptr_offsets);
        ok(ret == (void*)data[i].ret, "%d) __AdjustPointer returned %p, expected %p\n", i, ret, (void*)data[i].ret);
    }
}

static void test_mbstowcs(void)
{
    wchar_t bufw[16];
    char buf[16];
    size_t ret;

    buf[0] = 'a';
    buf[1] = 0;
    memset(bufw, 'x', sizeof(bufw));
    ret = mbstowcs(bufw, buf, -1);
    ok(ret == 1, "ret = %d\n", (int)ret);
    ok(bufw[0] == 'a', "bufw[0] = '%c'\n", bufw[0]);
    ok(bufw[1] == 0, "bufw[1] = '%c'\n", bufw[1]);

    memset(bufw, 'x', sizeof(bufw));
    ret = mbstowcs(bufw, buf, -1000);
    ok(ret == 1, "ret = %d\n", (int)ret);
    ok(bufw[0] == 'a', "bufw[0] = '%c'\n", bufw[0]);
    ok(bufw[1] == 0, "bufw[1] = '%c'\n", bufw[1]);

    memset(buf, 'x', sizeof(buf));
    ret = wcstombs(buf, bufw, -1);
    ok(ret == 1, "ret = %d\n", (int)ret);
    ok(buf[0] == 'a', "buf[0] = '%c'\n", buf[0]);
    ok(buf[1] == 0, "buf[1] = '%c'\n", buf[1]);

    memset(buf, 'x', sizeof(buf));
    ret = wcstombs(buf, bufw, -1000);
    ok(ret == 1, "ret = %d\n", (int)ret);
    ok(buf[0] == 'a', "buf[0] = '%c'\n", buf[0]);
    ok(buf[1] == 0, "buf[1] = '%c'\n", buf[1]);

    if(!setlocale(LC_ALL, "English")) {
        win_skip("English locale not available\n");
        return;
    }

    buf[0] = 'a';
    buf[1] = 0;
    memset(bufw, 'x', sizeof(bufw));
    ret = mbstowcs(bufw, buf, -1);
    ok(ret == -1, "ret = %d\n", (int)ret);
    ok(bufw[0] == 0, "bufw[0] = '%c'\n", bufw[0]);

    memset(bufw, 'x', sizeof(bufw));
    ret = mbstowcs(bufw, buf, -1000);
    ok(ret == -1, "ret = %d\n", (int)ret);
    ok(bufw[0] == 0, "bufw[0] = '%c'\n", bufw[0]);

    memset(buf, 'x', sizeof(buf));
    ret = wcstombs(buf, bufw, -1);
    ok(ret == 0, "ret = %d\n", (int)ret);
    ok(buf[0] == 0, "buf[0] = '%c'\n", buf[0]);

    memset(buf, 'x', sizeof(buf));
    ret = wcstombs(buf, bufw, -1000);
    ok(ret == 0, "ret = %d\n", (int)ret);
    ok(buf[0] == 0, "buf[0] = '%c'\n", buf[0]);

    setlocale(LC_ALL, "C");
}

static void test_strtok_s(void)
{
    char test[] = "a/b";
    char empty[] = "";
    char space[] = " ";
    char delim[] = "/";
    char *strret;
    char *context;

    context = (char*)0xdeadbeef;
    strret = strtok_s( test, delim, &context);
    ok(strret == test, "Expected test, got %p.\n", strret);
    ok(context == test+2, "Expected test+2, got %p.\n", context);

    strret = strtok_s( NULL, delim, &context);
    ok(strret == test+2, "Expected test+2, got %p.\n", strret);
    ok(context == test+3, "Expected test+3, got %p.\n", context);

    strret = strtok_s( NULL, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == test+3, "Expected test+3, got %p.\n", context);

    context = NULL;
    strret = strtok_s( empty, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == empty, "Expected empty, got %p.\n", context);

    strret = strtok_s( NULL, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == empty, "Expected empty, got %p.\n", context);

    context = NULL;
    strret = strtok_s( space, delim, &context);
    ok(strret == space, "Expected space, got %p.\n", strret);
    ok(context == space+1, "Expected space+1, got %p.\n", context);

    strret = strtok_s( NULL, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == space+1, "Expected space+1, got %p.\n", context);
}

static void test__mbstok_s(void)
{
    unsigned char test[] = "a/b";
    unsigned char empty[] = "";
    unsigned char space[] = " ";
    unsigned char delim[] = "/";
    unsigned char *strret;
    unsigned char *context;

    context = (unsigned char*)0xdeadbeef;
    strret = _mbstok_s( test, delim, &context);
    ok(strret == test, "Expected test, got %p.\n", strret);
    ok(context == test+2, "Expected test+2, got %p.\n", context);

    strret = _mbstok_s( NULL, delim, &context);
    ok(strret == test+2, "Expected test+2, got %p.\n", strret);
    ok(context == test+3, "Expected test+3, got %p.\n", context);

    strret = _mbstok_s( NULL, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == test+3, "Expected test+3, got %p.\n", context);

    context = NULL;
    strret = _mbstok_s( empty, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == empty, "Expected empty, got %p.\n", context);

    strret = _mbstok_s( NULL, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == empty, "Expected empty, got %p.\n", context);

    context = NULL;
    strret = _mbstok_s( space, delim, &context);
    ok(strret == space, "Expected space, got %p.\n", strret);
    ok(context == space+1, "Expected space+1, got %p.\n", context);

    strret = _mbstok_s( NULL, delim, &context);
    ok(strret == NULL, "Expected NULL, got %p.\n", strret);
    ok(context == space+1, "Expected space+1, got %p.\n", context);
}

#ifdef __i386__
static _FPIEEE_RECORD fpieee_record;
static int handler_called;
static int __cdecl fpieee_handler(_FPIEEE_RECORD *rec)
{
    handler_called++;
    fpieee_record = *rec;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test__fpieee_flt(void)
{
    double argd = 1.5;
    struct {
        DWORD exception_code;
        WORD opcode;
        DWORD data_offset;
        DWORD control_word;
        DWORD status_word;
        _FPIEEE_RECORD rec;
        int ret;
    } test_data[] = {
        { STATUS_FLOAT_DIVIDE_BY_ZERO, 0x35dc, (DWORD)&argd, 0, 0,
            {0, 2, _FpCodeDivide,
                {0}, {1, 1, 1, 1, 1}, {0},
                {{0}, 1, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 0, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
        { STATUS_FLOAT_INEXACT_RESULT, 0x35dc, (DWORD)&argd, 0, 0,
            {0, 2, _FpCodeDivide,
                {0}, {1, 1, 1, 1, 1}, {0},
                {{0}, 0, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 1, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
        { STATUS_FLOAT_INVALID_OPERATION, 0x35dc, (DWORD)&argd, 0, 0,
            {0, 2, _FpCodeDivide,
                {0}, {1, 1, 1, 1, 1}, {0},
                {{0}, 1, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 0, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
        { STATUS_FLOAT_OVERFLOW, 0x35dc, (DWORD)&argd, 0, 0,
            {0, 2, _FpCodeDivide,
                {0}, {1, 1, 1, 1, 1}, {0},
                {{0}, 0, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 1, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
        { STATUS_FLOAT_UNDERFLOW, 0x35dc, (DWORD)&argd, 0, 0,
            {0, 2, _FpCodeDivide,
                {0}, {1, 1, 1, 1, 1}, {0},
                {{0}, 0, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 1, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
        { STATUS_FLOAT_DIVIDE_BY_ZERO, 0x35dc, (DWORD)&argd, 0, 0xffffffff,
            {0, 2, _FpCodeDivide,
                {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1},
                {{0}, 1, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 0, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
        { STATUS_FLOAT_DIVIDE_BY_ZERO, 0x35dc, (DWORD)&argd, 0xffffffff, 0xffffffff,
            {3, 0, _FpCodeDivide,
                {0}, {0}, {1, 1, 1, 1, 1},
                {{0}, 1, _FpFormatFp80},
                {{0}, 1, _FpFormatFp64},
                {{0}, 0, _FpFormatFp80}},
            EXCEPTION_CONTINUE_EXECUTION },
    };
    EXCEPTION_POINTERS ep;
    EXCEPTION_RECORD rec;
    CONTEXT ctx;
    int i, ret;

    ep.ExceptionRecord = &rec;
    ep.ContextRecord = &ctx;
    memset(&rec, 0, sizeof(rec));
    memset(&ctx, 0, sizeof(ctx));
    ret = _fpieee_flt(1, &ep, fpieee_handler);
    ok(ret == EXCEPTION_CONTINUE_SEARCH, "_fpieee_flt returned %d\n", ret);
    ok(handler_called == 0, "handler_called = %d\n", handler_called);

    for(i=0; i<ARRAY_SIZE(test_data); i++) {
        ep.ExceptionRecord = &rec;
        ep.ContextRecord = &ctx;
        memset(&rec, 0, sizeof(rec));
        memset(&ctx, 0, sizeof(ctx));

        ctx.FloatSave.ErrorOffset = (DWORD)&test_data[i].opcode;
        ctx.FloatSave.DataOffset = test_data[i].data_offset;
        ctx.FloatSave.ControlWord = test_data[i].control_word;
        ctx.FloatSave.StatusWord = test_data[i].status_word;

        handler_called = 0;
        ret = _fpieee_flt(test_data[i].exception_code, &ep, fpieee_handler);
        ok(ret == test_data[i].ret, "%d) _fpieee_flt returned %d\n", i, ret);
        ok(handler_called == 1, "%d) handler_called = %d\n", i, handler_called);

        ok(fpieee_record.RoundingMode == test_data[i].rec.RoundingMode,
                "%d) RoundingMode = %x\n", i, fpieee_record.RoundingMode);
        ok(fpieee_record.Precision == test_data[i].rec.Precision,
                "%d) Precision = %x\n", i, fpieee_record.Precision);
        ok(fpieee_record.Operation == test_data[i].rec.Operation,
                "%d) Operation = %x\n", i, fpieee_record.Operation);
        ok(fpieee_record.Cause.Inexact == test_data[i].rec.Cause.Inexact,
                "%d) Cause.Inexact = %x\n", i, fpieee_record.Cause.Inexact);
        ok(fpieee_record.Cause.Underflow == test_data[i].rec.Cause.Underflow,
                "%d) Cause.Underflow = %x\n", i, fpieee_record.Cause.Underflow);
        ok(fpieee_record.Cause.Overflow == test_data[i].rec.Cause.Overflow,
                "%d) Cause.Overflow = %x\n", i, fpieee_record.Cause.Overflow);
        ok(fpieee_record.Cause.ZeroDivide == test_data[i].rec.Cause.ZeroDivide,
                "%d) Cause.ZeroDivide = %x\n", i, fpieee_record.Cause.ZeroDivide);
        ok(fpieee_record.Cause.InvalidOperation == test_data[i].rec.Cause.InvalidOperation,
                "%d) Cause.InvalidOperation = %x\n", i, fpieee_record.Cause.InvalidOperation);
        ok(fpieee_record.Enable.Inexact == test_data[i].rec.Enable.Inexact,
                "%d) Enable.Inexact = %x\n", i, fpieee_record.Enable.Inexact);
        ok(fpieee_record.Enable.Underflow == test_data[i].rec.Enable.Underflow,
                "%d) Enable.Underflow = %x\n", i, fpieee_record.Enable.Underflow);
        ok(fpieee_record.Enable.Overflow == test_data[i].rec.Enable.Overflow,
                "%d) Enable.Overflow = %x\n", i, fpieee_record.Enable.Overflow);
        ok(fpieee_record.Enable.ZeroDivide == test_data[i].rec.Enable.ZeroDivide,
                "%d) Enable.ZeroDivide = %x\n", i, fpieee_record.Enable.ZeroDivide);
        ok(fpieee_record.Enable.InvalidOperation == test_data[i].rec.Enable.InvalidOperation,
                "%d) Enable.InvalidOperation = %x\n", i, fpieee_record.Enable.InvalidOperation);
        ok(fpieee_record.Status.Inexact == test_data[i].rec.Status.Inexact,
                "%d) Status.Inexact = %x\n", i, fpieee_record.Status.Inexact);
        ok(fpieee_record.Status.Underflow == test_data[i].rec.Status.Underflow,
                "%d) Status.Underflow = %x\n", i, fpieee_record.Status.Underflow);
        ok(fpieee_record.Status.Overflow == test_data[i].rec.Status.Overflow,
                "%d) Status.Overflow = %x\n", i, fpieee_record.Status.Overflow);
        ok(fpieee_record.Status.ZeroDivide == test_data[i].rec.Status.ZeroDivide,
                "%d) Status.ZeroDivide = %x\n", i, fpieee_record.Status.ZeroDivide);
        ok(fpieee_record.Status.InvalidOperation == test_data[i].rec.Status.InvalidOperation,
                "%d) Status.InvalidOperation = %x\n", i, fpieee_record.Status.InvalidOperation);
        ok(fpieee_record.Operand1.OperandValid == test_data[i].rec.Operand1.OperandValid,
                "%d) Operand1.OperandValid = %x\n", i, fpieee_record.Operand1.OperandValid);
        if(fpieee_record.Operand1.OperandValid) {
            ok(fpieee_record.Operand1.Format == test_data[i].rec.Operand1.Format,
                    "%d) Operand1.Format = %x\n", i, fpieee_record.Operand1.Format);
        }
        ok(fpieee_record.Operand2.OperandValid == test_data[i].rec.Operand2.OperandValid,
                "%d) Operand2.OperandValid = %x\n", i, fpieee_record.Operand2.OperandValid);
        ok(fpieee_record.Operand2.Format == test_data[i].rec.Operand2.Format,
                "%d) Operand2.Format = %x\n", i, fpieee_record.Operand2.Format);
        ok(fpieee_record.Result.OperandValid == test_data[i].rec.Result.OperandValid,
                "%d) Result.OperandValid = %x\n", i, fpieee_record.Result.OperandValid);
        ok(fpieee_record.Result.Format == test_data[i].rec.Result.Format,
                "%d) Result.Format = %x\n", i, fpieee_record.Result.Format);
    }
}
#endif

static void test__memicmp(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ret = _memicmp(NULL, NULL, 0);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    ret = _memicmp(NULL, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = _memicmp(s1, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = _memicmp(NULL, s2, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    ret = _memicmp(s1, s2, 2);
    ok(!ret, "got %d\n", ret);

    ret = _memicmp(s1, s2, 3);
    ok(ret == -1, "got %d\n", ret);
}

static void test__memicmp_l(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ret = _memicmp_l(NULL, NULL, 0, NULL);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    ret = _memicmp_l(NULL, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = _memicmp_l(s1, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    SET_EXPECT(invalid_parameter_handler);
    ret = _memicmp_l(NULL, s2, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler, EINVAL);

    ret = _memicmp_l(s1, s2, 2, NULL);
    ok(!ret, "got %d\n", ret);

    ret = _memicmp_l(s1, s2, 3, NULL);
    ok(ret == -1, "got %d\n", ret);
}

static int WINAPIV _vsnwprintf_wrapper(wchar_t *str, size_t len, const wchar_t *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = _vsnwprintf(str, len, format, valist);
    va_end(valist);
    return ret;
}

static void test__vsnwprintf(void)
{
    int ret;
    WCHAR str[2] = {0};

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    str[0] = 'x';
    ret = _vsnwprintf_wrapper(str, 0, NULL);
    ok(ret == -1, "got %d, expected -1\n", ret);
    ok(str[0] == 'x', "Expected string to be unchanged.\n");
    CHECK_CALLED(invalid_parameter_handler, EINVAL);
}

static void test___strncnt(void)
{
    static const struct
    {
        const char *str;
        size_t size;
        size_t ret;
    }
    strncnt_tests[] =
    {
        { NULL, 0, 0 },
        { "a", 0, 0 },
        { "a", 1, 1 },
        { "a", 10, 1 },
        { "abc", 1, 1 },
    };
    unsigned int i;
    size_t ret;

    if (0)
        ret = __strncnt(NULL, 1);

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = __strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
    }
}

static void test_swscanf(void)
{

    wchar_t buffer[100];
    int ret;

    /* check WEOF */
    ret = swscanf(L" \t\n\n", L"%s", buffer);
    ok( ret == (short)WEOF, "ret = %d\n", ret );
}

static void test____mb_cur_max_l_func(void)
{
    int ret;
    _locale_t l;

    ret = ___mb_cur_max_l_func(NULL);
    ok( ret == 1, "MB_CUR_MAX_L(NULL) = %d\n", ret );

    l = _create_locale(LC_ALL, "chinese-traditional");
    if (!l)
    {
        skip("DBCS locale not available\n");
        return;
    }
    ret = ___mb_cur_max_l_func(l);
    ok( ret == 2, "MB_CUR_MAX_L(cht) = %d\n", ret );
    _free_locale(l);
}

static void test__get_current_locale(void)
{
    _locale_t l, l2;
    int i;

    ok(!_setmbcp(1252), "_setmbcp failed\n");
    l = _get_current_locale();
    l2 = _get_current_locale();

    ok(!strcmp(l->locinfo->lc_category[LC_COLLATE].locale, "C"),
            "LC_COLLATE = \"%s\"\n", l->locinfo->lc_category[LC_COLLATE].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_CTYPE].locale, "C"),
            "LC_CTYPE = \"%s\"\n", l->locinfo->lc_category[LC_CTYPE].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_MONETARY].locale, "C"),
            "LC_MONETARY = \"%s\"\n", l->locinfo->lc_category[LC_MONETARY].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_NUMERIC].locale, "C"),
            "LC_NUMERIC = \"%s\"\n", l->locinfo->lc_category[LC_NUMERIC].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_TIME].locale, "C"),
            "LC_TIME = \"%s\"\n", l->locinfo->lc_category[LC_TIME].locale);
    ok(l->mbcinfo->mbcodepage == 1252, "mbcodepage = %d\n", l->mbcinfo->mbcodepage);

    ok(l->locinfo->refcount == 4, "refcount = %d\n", l->locinfo->refcount);

    if(!setlocale(LC_ALL, "english")) {
        win_skip("English locale not available\n");
        _free_locale(l);
        _free_locale(l2);
        return;
    }
    ok(!_setmbcp(932), "_setmbcp failed\n");

    ok(!strcmp(l->locinfo->lc_category[LC_COLLATE].locale, "C"),
            "LC_COLLATE = \"%s\"\n", l->locinfo->lc_category[LC_COLLATE].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_CTYPE].locale, "C"),
            "LC_CTYPE = \"%s\"\n", l->locinfo->lc_category[LC_CTYPE].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_MONETARY].locale, "C"),
            "LC_MONETARY = \"%s\"\n", l->locinfo->lc_category[LC_MONETARY].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_NUMERIC].locale, "C"),
            "LC_NUMERIC = \"%s\"\n", l->locinfo->lc_category[LC_NUMERIC].locale);
    ok(!strcmp(l->locinfo->lc_category[LC_TIME].locale, "C"),
            "LC_TIME = \"%s\"\n", l->locinfo->lc_category[LC_TIME].locale);
    ok(l->mbcinfo->mbcodepage == 1252, "mbcodepage = %d\n", l->mbcinfo->mbcodepage);

    ok(l->locinfo->refcount == 2, "refcount = %d\n", l->locinfo->refcount);
    ok(l->locinfo == l2->locinfo, "different locinfo pointers\n");
    ok(l->mbcinfo == l2->mbcinfo, "different mbcinfo pointers\n");

    _free_locale(l);
    _free_locale(l2);

    l = _get_current_locale();
    setlocale(LC_COLLATE, "C");
    l2 = _get_current_locale();

    ok(l->locinfo->refcount == 1, "refcount = %d\n", l->locinfo->refcount);
    ok(l2->locinfo->refcount == 3, "refcount = %d\n", l2->locinfo->refcount);

    ok(l->locinfo->lc_category[LC_COLLATE].locale != l2->locinfo->lc_category[LC_COLLATE].locale,
            "same locale name pointers for LC_COLLATE\n");
    ok(l->locinfo->lc_category[LC_COLLATE].refcount != l2->locinfo->lc_category[LC_COLLATE].refcount,
            "same refcount pointers for LC_COLLATE\n");
    ok(*l2->locinfo->lc_category[LC_COLLATE].refcount == 3, "refcount = %d\n",
            *l2->locinfo->lc_category[LC_COLLATE].refcount);
    ok(*l->locinfo->lc_category[LC_COLLATE].refcount == 1, "refcount = %d\n",
            *l->locinfo->lc_category[LC_COLLATE].refcount);
    for(i = LC_CTYPE; i <= LC_MAX; i++) {
        ok(l->locinfo->lc_category[i].locale == l2->locinfo->lc_category[i].locale,
                "different locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount == l2->locinfo->lc_category[i].refcount,
                "different refcount pointers for category %d\n", i);
        ok(*l->locinfo->lc_category[i].refcount == 4, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }

    ok(l->locinfo->lc_collate_cp != l2->locinfo->lc_collate_cp, "same lc_collate_cp %u, %u\n",
            l->locinfo->lc_collate_cp, l2->locinfo->lc_collate_cp);

    ok(l->locinfo->lc_codepage == l2->locinfo->lc_codepage, "different lc_codepages %u, %u\n",
            l->locinfo->lc_codepage, l2->locinfo->lc_codepage);
    ok(l->locinfo->lc_clike == l2->locinfo->lc_clike, "different lc_clike values %d, %d\n",
            l->locinfo->lc_clike, l2->locinfo->lc_clike);
    /* The meaning of this member seems to be reversed--go figure */
    ok(l->locinfo->lc_clike, "non-C locale is C-like\n");
    ok(l->locinfo->ctype1 == l2->locinfo->ctype1, "different ctype1 pointers\n");
    ok(l->locinfo->pclmap == l2->locinfo->pclmap, "different clmap pointers\n");
    ok(l->locinfo->pcumap == l2->locinfo->pcumap, "different cumap pointers\n");
    ok(l->locinfo->ctype1_refcount == l2->locinfo->ctype1_refcount, "different ctype1_refcount pointers\n");
    ok(*l->locinfo->ctype1_refcount == 4, "refcount = %d\n", *l->locinfo->ctype1_refcount);

    ok(l->locinfo->lconv == l2->locinfo->lconv, "different lconv pointers\n");
    ok(l->locinfo->lconv_intl_refcount == l2->locinfo->lconv_intl_refcount, "different lconv_intl_refcount pointers\n");
    ok(!!l->locinfo->lconv_intl_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_intl_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_intl_refcount);

    ok(l->locinfo->lconv->decimal_point == l2->locinfo->lconv->decimal_point, "different LC_NUMERIC pointers\n");
    ok(l->locinfo->lconv_num_refcount == l2->locinfo->lconv_num_refcount, "different lconv_num_refcount pointers\n");
    ok(!!l->locinfo->lconv_num_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_num_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_num_refcount);

    ok(l->locinfo->lconv->currency_symbol == l2->locinfo->lconv->currency_symbol, "different LC_MONETARY pointers\n");
    ok(l->locinfo->lconv_mon_refcount == l2->locinfo->lconv_mon_refcount, "different lconv_mon_refcount pointers\n");
    ok(!!l->locinfo->lconv_mon_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_mon_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_mon_refcount);

    ok(l->locinfo->lc_time_curr == l2->locinfo->lc_time_curr, "different lc_time_curr pointers\n");
    ok(l->locinfo->lc_time_curr->unk == 1, "unk = %d\n", l->locinfo->lc_time_curr->unk);
    ok(l->locinfo->lc_time_curr->refcount == 4, "refcount = %d\n", l->locinfo->lc_time_curr->refcount);

    _free_locale(l2);

    setlocale(LC_CTYPE, "C");
    l2 = _get_current_locale();

    ok(l->locinfo->refcount == 1, "refcount = %d\n", l->locinfo->refcount);
    ok(l2->locinfo->refcount == 3, "refcount = %d\n", l2->locinfo->refcount);

    for(i = LC_COLLATE; i < LC_MONETARY; i++) {
        ok(l->locinfo->lc_category[i].locale != l2->locinfo->lc_category[i].locale,
                "same locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount != l2->locinfo->lc_category[i].refcount,
                "same refcount pointers for category %d\n", i);
        ok(*l2->locinfo->lc_category[i].refcount == 3, "refcount = %d for category %d\n",
                *l2->locinfo->lc_category[i].refcount, i);
        ok(*l->locinfo->lc_category[i].refcount == 1, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }
    for(i = LC_MONETARY; i <= LC_MAX; i++) {
        ok(l->locinfo->lc_category[i].locale == l2->locinfo->lc_category[i].locale,
                "different locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount == l2->locinfo->lc_category[i].refcount,
                "different refcount pointers for category %d\n", i);
        ok(*l->locinfo->lc_category[i].refcount == 4, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }

    ok(l->locinfo->lc_collate_cp != l2->locinfo->lc_collate_cp, "same lc_collate_cp %u, %u\n",
            l->locinfo->lc_collate_cp, l2->locinfo->lc_collate_cp);

    ok(l->locinfo->lc_codepage != l2->locinfo->lc_codepage, "same lc_codepages %u, %u\n",
            l->locinfo->lc_codepage, l2->locinfo->lc_codepage);
    todo_wine ok(l->locinfo->lc_clike != l2->locinfo->lc_clike, "same lc_clike values %d, %d\n",
            l->locinfo->lc_clike, l2->locinfo->lc_clike);
    ok(l->locinfo->lc_clike, "non-C locale is C-like\n");
    todo_wine ok(!l2->locinfo->lc_clike, "C locale is not C-like\n");
    ok(l->locinfo->ctype1 != l2->locinfo->ctype1, "same ctype1 pointers\n");
    ok(l->locinfo->pclmap != l2->locinfo->pclmap, "same clmap pointers\n");
    ok(l->locinfo->pcumap != l2->locinfo->pcumap, "same cumap pointers\n");
    ok(l->locinfo->ctype1_refcount != l2->locinfo->ctype1_refcount, "same ctype1_refcount pointers\n");
    ok(!!l->locinfo->ctype1_refcount, "null refcount pointer for non-C locale\n");
    ok(*l->locinfo->ctype1_refcount == 1, "refcount = %d\n", *l->locinfo->ctype1_refcount);
    ok(!l2->locinfo->ctype1_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv == l2->locinfo->lconv, "different lconv pointers\n");
    ok(l->locinfo->lconv_intl_refcount == l2->locinfo->lconv_intl_refcount, "different lconv_intl_refcount pointers\n");
    ok(!!l->locinfo->lconv_intl_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_intl_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_intl_refcount);

    ok(l->locinfo->lconv->decimal_point == l2->locinfo->lconv->decimal_point, "different LC_NUMERIC pointers\n");
    ok(l->locinfo->lconv_num_refcount == l2->locinfo->lconv_num_refcount, "different lconv_num_refcount pointers\n");
    ok(!!l->locinfo->lconv_num_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_num_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_num_refcount);

    ok(l->locinfo->lconv->currency_symbol == l2->locinfo->lconv->currency_symbol, "different LC_MONETARY pointers\n");
    ok(l->locinfo->lconv_mon_refcount == l2->locinfo->lconv_mon_refcount, "different lconv_mon_refcount pointers\n");
    ok(!!l->locinfo->lconv_mon_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_mon_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_mon_refcount);

    ok(l->locinfo->lc_time_curr == l2->locinfo->lc_time_curr, "different lc_time_curr pointers\n");
    ok(l->locinfo->lc_time_curr->unk == 1, "unk = %d\n", l->locinfo->lc_time_curr->unk);
    ok(l->locinfo->lc_time_curr->refcount == 4, "refcount = %d\n", l->locinfo->lc_time_curr->refcount);

    _free_locale(l2);

    setlocale(LC_MONETARY, "C");
    l2 = _get_current_locale();

    ok(l->locinfo->refcount == 1, "refcount = %d\n", l->locinfo->refcount);
    ok(l2->locinfo->refcount == 3, "refcount = %d\n", l2->locinfo->refcount);

    for(i = LC_COLLATE; i <= LC_MONETARY; i++) {
        ok(l->locinfo->lc_category[i].locale != l2->locinfo->lc_category[i].locale,
                "same locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount != l2->locinfo->lc_category[i].refcount,
                "same refcount pointers for category %d\n", i);
        ok(*l2->locinfo->lc_category[i].refcount == 3, "refcount = %d for category %d\n",
                *l2->locinfo->lc_category[i].refcount, i);
        ok(*l->locinfo->lc_category[i].refcount == 1, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }
    for(i = LC_NUMERIC; i <= LC_MAX; i++) {
        ok(l->locinfo->lc_category[i].locale == l2->locinfo->lc_category[i].locale,
                "different locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount == l2->locinfo->lc_category[i].refcount,
                "different refcount pointers for category %d\n", i);
        ok(*l->locinfo->lc_category[i].refcount == 4, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }

    ok(l->locinfo->lc_collate_cp != l2->locinfo->lc_collate_cp, "same lc_collate_cp %u, %u\n",
            l->locinfo->lc_collate_cp, l2->locinfo->lc_collate_cp);

    ok(l->locinfo->lc_codepage != l2->locinfo->lc_codepage, "same lc_codepages %u, %u\n",
            l->locinfo->lc_codepage, l2->locinfo->lc_codepage);
    todo_wine ok(l->locinfo->lc_clike != l2->locinfo->lc_clike, "same lc_clike values %d, %d\n",
            l->locinfo->lc_clike, l2->locinfo->lc_clike);
    ok(l->locinfo->lc_clike, "non-C locale is C-like\n");
    todo_wine ok(!l2->locinfo->lc_clike, "C locale is not C-like\n");
    ok(l->locinfo->ctype1 != l2->locinfo->ctype1, "same ctype1 pointers\n");
    ok(l->locinfo->pclmap != l2->locinfo->pclmap, "same clmap pointers\n");
    ok(l->locinfo->pcumap != l2->locinfo->pcumap, "same cumap pointers\n");
    ok(l->locinfo->ctype1_refcount != l2->locinfo->ctype1_refcount, "same ctype1_refcount pointers\n");
    ok(!!l->locinfo->ctype1_refcount, "null refcount pointer for non-C locale\n");
    ok(*l->locinfo->ctype1_refcount == 1, "refcount = %d\n", *l->locinfo->ctype1_refcount);
    ok(!l2->locinfo->ctype1_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv != l2->locinfo->lconv, "same lconv pointers\n");
    ok(l->locinfo->lconv_intl_refcount != l2->locinfo->lconv_intl_refcount, "same lconv_intl_refcount pointers\n");
    ok(!!l->locinfo->lconv_intl_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_intl_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_intl_refcount);
    ok(!!l2->locinfo->lconv_intl_refcount, "null refcount pointer for C locale\n");
    ok(*l2->locinfo->lconv_intl_refcount == 3, "refcount = %d\n", *l2->locinfo->lconv_intl_refcount);

    ok(l->locinfo->lconv->decimal_point == l2->locinfo->lconv->decimal_point, "different LC_NUMERIC pointers\n");
    ok(l->locinfo->lconv_num_refcount == l2->locinfo->lconv_num_refcount, "different lconv_num_refcount pointers\n");
    ok(!!l->locinfo->lconv_num_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_num_refcount == 4, "refcount = %d\n", *l->locinfo->lconv_num_refcount);

    ok(l->locinfo->lconv->currency_symbol != l2->locinfo->lconv->currency_symbol, "same LC_MONETARY pointers\n");
    ok(l->locinfo->lconv_mon_refcount != l2->locinfo->lconv_mon_refcount, "same lconv_mon_refcount pointers\n");
    ok(!!l->locinfo->lconv_mon_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_mon_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_mon_refcount);
    ok(!l2->locinfo->lconv_mon_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lc_time_curr == l2->locinfo->lc_time_curr, "different lc_time_curr pointers\n");
    ok(l->locinfo->lc_time_curr->unk == 1, "unk = %d\n", l->locinfo->lc_time_curr->unk);
    ok(l->locinfo->lc_time_curr->refcount == 4, "refcount = %d\n", l->locinfo->lc_time_curr->refcount);

    _free_locale(l2);

    setlocale(LC_NUMERIC, "C");
    l2 = _get_current_locale();

    ok(l->locinfo->refcount == 1, "refcount = %d\n", l->locinfo->refcount);
    ok(l2->locinfo->refcount == 3, "refcount = %d\n", l2->locinfo->refcount);

    for(i = LC_COLLATE; i <= LC_NUMERIC; i++) {
        ok(l->locinfo->lc_category[i].locale != l2->locinfo->lc_category[i].locale,
                "same locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount != l2->locinfo->lc_category[i].refcount,
                "same refcount pointers for category %d\n", i);
        ok(*l2->locinfo->lc_category[i].refcount == 3, "refcount = %d for category %d\n",
                *l2->locinfo->lc_category[i].refcount, i);
        ok(*l->locinfo->lc_category[i].refcount == 1, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }
    ok(l->locinfo->lc_category[LC_TIME].locale == l2->locinfo->lc_category[LC_TIME].locale,
            "different locale name pointers for LC_TIME\n");
    ok(l->locinfo->lc_category[LC_TIME].refcount == l2->locinfo->lc_category[LC_TIME].refcount,
            "different refcount pointers for LC_TIME\n");
    ok(*l->locinfo->lc_category[LC_TIME].refcount == 4, "refcount = %d\n",
            *l->locinfo->lc_category[LC_TIME].refcount);

    ok(l->locinfo->lc_collate_cp != l2->locinfo->lc_collate_cp, "same lc_collate_cp %u, %u\n",
            l->locinfo->lc_collate_cp, l2->locinfo->lc_collate_cp);

    ok(l->locinfo->lc_codepage != l2->locinfo->lc_codepage, "same lc_codepages %u, %u\n",
            l->locinfo->lc_codepage, l2->locinfo->lc_codepage);
    todo_wine ok(l->locinfo->lc_clike != l2->locinfo->lc_clike, "same lc_clike values %d, %d\n",
            l->locinfo->lc_clike, l2->locinfo->lc_clike);
    ok(l->locinfo->lc_clike, "non-C locale is C-like\n");
    todo_wine ok(!l2->locinfo->lc_clike, "C locale is not C-like\n");
    ok(l->locinfo->ctype1 != l2->locinfo->ctype1, "same ctype1 pointers\n");
    ok(l->locinfo->pclmap != l2->locinfo->pclmap, "same clmap pointers\n");
    ok(l->locinfo->pcumap != l2->locinfo->pcumap, "same cumap pointers\n");
    ok(l->locinfo->ctype1_refcount != l2->locinfo->ctype1_refcount, "same ctype1_refcount pointers\n");
    ok(!!l->locinfo->ctype1_refcount, "null refcount pointer for non-C locale\n");
    ok(*l->locinfo->ctype1_refcount == 1, "refcount = %d\n", *l->locinfo->ctype1_refcount);
    ok(!l2->locinfo->ctype1_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv != l2->locinfo->lconv, "same lconv pointers\n");
    ok(l->locinfo->lconv_intl_refcount != l2->locinfo->lconv_intl_refcount, "same lconv_intl_refcount pointers\n");
    ok(!!l->locinfo->lconv_intl_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_intl_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_intl_refcount);
    ok(!l2->locinfo->lconv_intl_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv->decimal_point != l2->locinfo->lconv->decimal_point, "same LC_NUMERIC pointers\n");
    ok(l->locinfo->lconv_num_refcount != l2->locinfo->lconv_num_refcount, "same lconv_num_refcount pointers\n");
    ok(!!l->locinfo->lconv_num_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_num_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_num_refcount);
    ok(!l2->locinfo->lconv_num_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv->currency_symbol != l2->locinfo->lconv->currency_symbol, "same LC_MONETARY pointers\n");
    ok(l->locinfo->lconv_mon_refcount != l2->locinfo->lconv_mon_refcount, "same lconv_mon_refcount pointers\n");
    ok(!!l->locinfo->lconv_mon_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_mon_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_mon_refcount);
    ok(!l2->locinfo->lconv_mon_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lc_time_curr == l2->locinfo->lc_time_curr, "different lc_time_curr pointers\n");
    ok(l->locinfo->lc_time_curr->unk == 1, "unk = %d\n", l->locinfo->lc_time_curr->unk);
    ok(l->locinfo->lc_time_curr->refcount == 4, "refcount = %d\n", l->locinfo->lc_time_curr->refcount);

    _free_locale(l2);

    setlocale(LC_TIME, "C");
    l2 = _get_current_locale();

    ok(l->locinfo->refcount == 1, "refcount = %d\n", l->locinfo->refcount);
    ok(l2->locinfo->refcount == 3, "refcount = %d\n", l2->locinfo->refcount);

    for(i = LC_MIN+1; i <= LC_MAX; i++) {
        ok(l->locinfo->lc_category[i].locale != l2->locinfo->lc_category[i].locale,
                "same locale name pointers for category %d\n", i);
        ok(l->locinfo->lc_category[i].refcount != l2->locinfo->lc_category[i].refcount,
                "same refcount pointers for category %d\n", i);
        ok(*l2->locinfo->lc_category[i].refcount == 3, "refcount = %d for category %d\n",
                *l2->locinfo->lc_category[i].refcount, i);
        ok(*l->locinfo->lc_category[i].refcount == 1, "refcount = %d for category %d\n",
                *l->locinfo->lc_category[i].refcount, i);
    }

    ok(l->locinfo->lc_collate_cp != l2->locinfo->lc_collate_cp, "same lc_collate_cp %u, %u\n",
            l->locinfo->lc_collate_cp, l2->locinfo->lc_collate_cp);

    ok(l->locinfo->lc_codepage != l2->locinfo->lc_codepage, "same lc_codepages %u, %u\n",
            l->locinfo->lc_codepage, l2->locinfo->lc_codepage);
    todo_wine ok(l->locinfo->lc_clike != l2->locinfo->lc_clike, "same lc_clike values %d, %d\n",
            l->locinfo->lc_clike, l2->locinfo->lc_clike);
    ok(l->locinfo->lc_clike, "non-C locale is C-like\n");
    todo_wine ok(!l2->locinfo->lc_clike, "C locale is not C-like\n");
    ok(l->locinfo->ctype1 != l2->locinfo->ctype1, "same ctype1 pointers\n");
    ok(l->locinfo->pclmap != l2->locinfo->pclmap, "same clmap pointers\n");
    ok(l->locinfo->pcumap != l2->locinfo->pcumap, "same cumap pointers\n");
    ok(l->locinfo->ctype1_refcount != l2->locinfo->ctype1_refcount, "same ctype1_refcount pointers\n");
    ok(!!l->locinfo->ctype1_refcount, "null refcount pointer for non-C locale\n");
    ok(*l->locinfo->ctype1_refcount == 1, "refcount = %d\n", *l->locinfo->ctype1_refcount);
    ok(!l2->locinfo->ctype1_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv != l2->locinfo->lconv, "same lconv pointers\n");
    ok(l->locinfo->lconv_intl_refcount != l2->locinfo->lconv_intl_refcount, "same lconv_intl_refcount pointers\n");
    ok(!!l->locinfo->lconv_intl_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_intl_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_intl_refcount);
    ok(!l2->locinfo->lconv_intl_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv->decimal_point != l2->locinfo->lconv->decimal_point, "same LC_NUMERIC pointers\n");
    ok(l->locinfo->lconv_num_refcount != l2->locinfo->lconv_num_refcount, "same lconv_num_refcount pointers\n");
    ok(!!l->locinfo->lconv_num_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_num_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_num_refcount);
    ok(!l2->locinfo->lconv_num_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lconv->currency_symbol != l2->locinfo->lconv->currency_symbol, "same LC_MONETARY pointers\n");
    ok(l->locinfo->lconv_mon_refcount != l2->locinfo->lconv_mon_refcount, "same lconv_mon_refcount pointers\n");
    ok(!!l->locinfo->lconv_mon_refcount, "null refcount pointer in non-C locale\n");
    ok(*l->locinfo->lconv_mon_refcount == 1, "refcount = %d\n", *l->locinfo->lconv_mon_refcount);
    ok(!l2->locinfo->lconv_mon_refcount, "nonnull refcount pointer for C locale\n");

    ok(l->locinfo->lc_time_curr != l2->locinfo->lc_time_curr, "same lc_time_curr pointers\n");
    ok(l->locinfo->lc_time_curr->unk == 1, "unk = %d\n", l->locinfo->lc_time_curr->unk);
    ok(l->locinfo->lc_time_curr->refcount == 1, "refcount = %d\n", l->locinfo->lc_time_curr->refcount);
    ok(l2->locinfo->lc_time_curr->unk == 1, "unk = %d\n", l2->locinfo->lc_time_curr->unk);
    ok(l2->locinfo->lc_time_curr->refcount == 2,
            "refcount = %d\n", l2->locinfo->lc_time_curr->refcount);

    _free_locale(l2);

    _free_locale(l);
    setlocale(LC_ALL, "C");
}

static void test_ioinfo_flags(void)
{
    HANDLE handle;
    ioinfo *info;
    char *tempf;
    int tempfd;

    tempf = _tempnam(".","wne");

    tempfd = _open(tempf, _O_CREAT|_O_TRUNC|_O_WRONLY|_O_WTEXT, _S_IWRITE);
    ok(tempfd != -1, "_open failed with error: %d\n", errno);
    handle = (HANDLE)_get_osfhandle(tempfd);
    info = &__pioinfo[tempfd / MSVCRT_FD_BLOCK_SIZE][tempfd % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->handle == handle, "Unexpected handle %p, expected %p.\n", info->handle, handle);
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->wxflag == (WX_TEXT | WX_OPEN), "Unexpected wxflag %#x.\n", info->wxflag);
    ok(info->unicode, "Unicode is not set.\n");
    ok(info->textmode == 2, "Unexpected textmode %d.\n", info->textmode);
    _close(tempfd);

    ok(info->handle == INVALID_HANDLE_VALUE, "Unexpected handle %p.\n", info->handle);
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->unicode, "Unicode is not set.\n");
    ok(!info->wxflag, "Unexpected wxflag %#x.\n", info->wxflag);
    ok(info->textmode == 2, "Unexpected textmode %d.\n", info->textmode);

    info = &__pioinfo[(tempfd + 4) / MSVCRT_FD_BLOCK_SIZE][(tempfd + 4) % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->handle == INVALID_HANDLE_VALUE, "Unexpected handle %p.\n", info->handle);
    ok(!info->exflag, "Unexpected exflag %#x.\n", info->exflag);
    ok(!info->textmode, "Unexpected textmode %d.\n", info->textmode);

    tempfd = _open(tempf, _O_CREAT|_O_TRUNC|_O_WRONLY|_O_TEXT, _S_IWRITE);
    ok(tempfd != -1, "_open failed with error: %d\n", errno);
    info = &__pioinfo[tempfd / MSVCRT_FD_BLOCK_SIZE][tempfd % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->wxflag == (WX_TEXT | WX_OPEN), "Unexpected wxflag %#x.\n", info->wxflag);
    ok(!info->unicode, "Unicode is not set.\n");
    ok(!info->textmode, "Unexpected textmode %d.\n", info->textmode);
    _close(tempfd);

    tempfd = _open(tempf, _O_CREAT|_O_TRUNC|_O_WRONLY|_O_U8TEXT, _S_IWRITE);
    ok(tempfd != -1, "_open failed with error: %d\n", errno);
    info = &__pioinfo[tempfd / MSVCRT_FD_BLOCK_SIZE][tempfd % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->wxflag == (WX_TEXT | WX_OPEN), "Unexpected wxflag %#x.\n", info->wxflag);
    ok(!info->unicode, "Unicode is not set.\n");
    ok(info->textmode == 1, "Unexpected textmode %d.\n", info->textmode);
    _close(tempfd);

    unlink(tempf);
    free(tempf);
}

static void test_strcmp(void)
{
    int ret = strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
#ifdef _WIN64
    ret = strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
#else
    ret = strncmp( "", "abc", 3 );
    ok( ret == 0 - 'a', "wrong ret %d\n", ret );
    ret = strncmp( "abc", "ab\xa0", 4 );
    ok( ret == 'c' - 0xa0, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 0xb0 - 0xa0, "wrong ret %d\n", ret );
#endif
    ret = strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

START_TEST(msvcr90)
{
    if(!init())
        return;

    /* _get_current_locale tests needs to be run first because there's
     * a C-locale refcount leak in native _create_locale implementation. */
    test__get_current_locale();

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
    test_mbstowcs();
    test_strtok_s();
    test__mbstok_s();
    test__memicmp();
    test__memicmp_l();
    test__vsnwprintf();
#ifdef __i386__
    test__fpieee_flt();
#endif
    test___strncnt();
    test_swscanf();
    test____mb_cur_max_l_func();
    test_ioinfo_flags();
    test_strcmp();
}
