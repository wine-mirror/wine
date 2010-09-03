/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

/* basic_string<char, char_traits<char>, allocator<char>> */
#define BUF_SIZE_CHAR 16
typedef struct _basic_string_char
{
    void *allocator;
    union {
        char buf[BUF_SIZE_CHAR];
        char *ptr;
    } data;
    size_t size;
    size_t res;
} basic_string_char;

/* basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>> */
#define BUF_SIZE_WCHAR 8
typedef struct _basic_string_wchar
{
    void *allocator;
    union {
        wchar_t buf[BUF_SIZE_WCHAR];
        wchar_t *ptr;
    } data;
    size_t size;
    size_t res;
} basic_string_wchar;

static void* (__cdecl *p_set_invalid_parameter_handler)(void*);

#ifdef __i386__
static basic_string_char* (WINAPI *p_basic_string_char_ctor)(void);
static basic_string_char* (WINAPI *p_basic_string_char_copy_ctor)(basic_string_char*);
static basic_string_char* (WINAPI *p_basic_string_char_ctor_cstr)(const char*);
static void (WINAPI *p_basic_string_char_dtor)(void);
static basic_string_char* (WINAPI *p_basic_string_char_erase)(size_t, size_t);
static basic_string_char* (WINAPI *p_basic_string_char_assign_cstr_len)(const char*, size_t);
static const char* (WINAPI *p_basic_string_char_cstr)(void);
static const char* (WINAPI *p_basic_string_char_data)(void);
static size_t (WINAPI *p_basic_string_char_size)(void);

static basic_string_wchar* (WINAPI *p_basic_string_wchar_ctor)(void);
static basic_string_wchar* (WINAPI *p_basic_string_wchar_copy_ctor)(basic_string_wchar*);
static basic_string_wchar* (WINAPI *p_basic_string_wchar_ctor_cstr)(const wchar_t*);
static void (WINAPI *p_basic_string_wchar_dtor)(void);
static basic_string_wchar* (WINAPI *p_basic_string_wchar_erase)(size_t, size_t);
static basic_string_wchar* (WINAPI *p_basic_string_wchar_assign_cstr_len)(const wchar_t*, size_t);
static const wchar_t* (WINAPI *p_basic_string_wchar_cstr)(void);
static const wchar_t* (WINAPI *p_basic_string_wchar_data)(void);
static size_t (WINAPI *p_basic_string_wchar_size)(void);
#else
static basic_string_char* (__cdecl *p_basic_string_char_ctor)(basic_string_char*);
static basic_string_char* (__cdecl *p_basic_string_char_copy_ctor)(basic_string_char*, basic_string_char*);
static basic_string_char* (__cdecl *p_basic_string_char_ctor_cstr)(basic_string_char*, const char*);
static void (__cdecl *p_basic_string_char_dtor)(basic_string_char*);
static basic_string_char* (__cdecl *p_basic_string_char_erase)(basic_string_char*, size_t, size_t);
static basic_string_char* (__cdecl *p_basic_string_char_assign_cstr_len)(basic_string_char*, const char*, size_t);
static const char* (__cdecl *p_basic_string_char_cstr)(basic_string_char*);
static const char* (__cdecl *p_basic_string_char_data)(basic_string_char*);
static size_t (__cdecl *p_basic_string_char_size)(basic_string_char*);

static basic_string_wchar* (__cdecl *p_basic_string_wchar_ctor)(basic_string_wchar*);
static basic_string_wchar* (__cdecl *p_basic_string_wchar_copy_ctor)(basic_string_wchar*, basic_string_wchar*);
static basic_string_wchar* (__cdecl *p_basic_string_wchar_ctor_cstr)(basic_string_wchar*, const wchar_t*);
static void (__cdecl *p_basic_string_wchar_dtor)(basic_string_wchar*);
static basic_string_wchar* (__cdecl *p_basic_string_wchar_erase)(basic_string_wchar*, size_t, size_t);
static basic_string_wchar* (__cdecl *p_basic_string_wchar_assign_cstr_len)(basic_string_wchar*, const wchar_t*, size_t);
static const wchar_t* (__cdecl *p_basic_string_wchar_cstr)(basic_string_wchar*);
static const wchar_t* (__cdecl *p_basic_string_wchar_data)(basic_string_wchar*);
static size_t (__cdecl *p_basic_string_wchar_size)(basic_string_wchar*);
#endif

static int invalid_parameter = 0;
static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %lx\n", (UINT_PTR)arg);
    invalid_parameter++;
}

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

static inline void* do_call_func2(void *func, void *_this, const void *arg)
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

static inline void* do_call_func3(void *func, void *_this,
        const void *arg1, const void *arg2)
{
    volatile void* retval = 0;
    __asm
    {
        push ecx
        push arg1
        push arg2
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
    __asm__ __volatile__ (
            "call *%2"
            : "=a" (ret), "=c" (dummy)
            : "g" (func), "1" (_this)
            : "edx", "memory"
            );
    return ret;
}

static void* do_call_func2(void *func, void *_this, const void *arg)
{
    void *ret, *dummy;
    __asm__ __volatile__ (
            "pushl %3\n\tcall *%2"
            : "=a" (ret), "=c" (dummy)
            : "r" (func), "r" (arg), "1" (_this)
            : "edx", "memory"
            );
    return ret;
}

static void* do_call_func3(void *func, void *_this,
        const void *arg1, const void *arg2)
{
    void *ret, *dummy;
    __asm__ __volatile__ (
            "pushl %4\n\tpushl %3\n\tcall *%2"
            : "=a" (ret), "=c" (dummy)
            : "r" (func), "r" (arg1), "r" (arg2), "1" (_this)
            : "edx", "memory"
            );
    return ret;
}
#endif

#define call_func1(func,_this)   do_call_func1(func,_this)
#define call_func2(func,_this,a) do_call_func2(func,_this,(const void*)a)
#define call_func3(func,_this,a,b) do_call_func3(func,_this,(const void*)a,(const void*)b)

#else

#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)

#endif /* __i386__ */

static BOOL init(void)
{
    HMODULE msvcr = LoadLibraryA("msvcr90.dll");
    HMODULE msvcp = LoadLibraryA("msvcp90.dll");
    if(!msvcr || !msvcp) {
        win_skip("msvcp90.dll or msvcrt90.dll not installed\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler = (void*)GetProcAddress(msvcr, "_set_invalid_parameter_handler");
    if(!p_set_invalid_parameter_handler) {
        win_skip("Error setting tests environment\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler(test_invalid_parameter_handler);

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        p_basic_string_char_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ");
        p_basic_string_char_copy_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@@Z");
        p_basic_string_char_ctor_cstr = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD@Z");
        p_basic_string_char_dtor = (void*)GetProcAddress(msvcp,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ");
        p_basic_string_char_erase = (void*)GetProcAddress(msvcp,
                "?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0@Z");
        p_basic_string_char_assign_cstr_len = (void*)GetProcAddress(msvcp,
                "?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z");
        p_basic_string_char_cstr = (void*)GetProcAddress(msvcp,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ");
        p_basic_string_char_data = (void*)GetProcAddress(msvcp,
                "?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ");
        p_basic_string_char_size = (void*)GetProcAddress(msvcp,
                "?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ");

        p_basic_string_wchar_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@XZ");
        p_basic_string_wchar_copy_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV01@@Z");
        p_basic_string_wchar_ctor_cstr = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W@Z");
        p_basic_string_wchar_dtor = (void*)GetProcAddress(msvcp,
                "??1?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@XZ");
        p_basic_string_wchar_erase = (void*)GetProcAddress(msvcp,
                "?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0@Z");
        p_basic_string_wchar_assign_cstr_len = (void*)GetProcAddress(msvcp,
                "?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W_K@Z");
        p_basic_string_wchar_cstr = (void*)GetProcAddress(msvcp,
                "?c_str@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEB_WXZ");
        p_basic_string_wchar_data = (void*)GetProcAddress(msvcp,
                "?data@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEB_WXZ");
        p_basic_string_wchar_size = (void*)GetProcAddress(msvcp,
                "?size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ");

    } else {
        p_basic_string_char_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");
        p_basic_string_char_copy_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z");
        p_basic_string_char_ctor_cstr = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD@Z");
        p_basic_string_char_dtor = (void*)GetProcAddress(msvcp,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");
        p_basic_string_char_erase = (void*)GetProcAddress(msvcp,
                "?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@II@Z");
        p_basic_string_char_assign_cstr_len = (void*)GetProcAddress(msvcp,
                "?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z");
        p_basic_string_char_cstr = (void*)GetProcAddress(msvcp,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");
        p_basic_string_char_data = (void*)GetProcAddress(msvcp,
                "?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");
        p_basic_string_char_size = (void*)GetProcAddress(msvcp,
                "?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ");

        p_basic_string_wchar_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@XZ");
        p_basic_string_wchar_copy_ctor = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV01@@Z");
        p_basic_string_wchar_ctor_cstr = (void*)GetProcAddress(msvcp,
                "??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_W@Z");
        p_basic_string_wchar_dtor = (void*)GetProcAddress(msvcp,
                "??1?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@XZ");
        p_basic_string_wchar_erase = (void*)GetProcAddress(msvcp,
                "?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@II@Z");
        p_basic_string_wchar_assign_cstr_len = (void*)GetProcAddress(msvcp,
                "?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_WI@Z");
        p_basic_string_wchar_cstr = (void*)GetProcAddress(msvcp,
                "?c_str@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPB_WXZ");
        p_basic_string_wchar_data = (void*)GetProcAddress(msvcp,
                "?data@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPB_WXZ");
        p_basic_string_wchar_size = (void*)GetProcAddress(msvcp,
                "?size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ");
    }

    return TRUE;
}

static void test_basic_string_char(void) {
    basic_string_char str1, str2, *pstr;
    const char *str;
    size_t size;

    if(!p_basic_string_char_ctor || !p_basic_string_char_copy_ctor
            || !p_basic_string_char_ctor_cstr || !p_basic_string_char_dtor
            || !p_basic_string_char_erase || !p_basic_string_char_assign_cstr_len
            || !p_basic_string_char_cstr || !p_basic_string_char_data
            || !p_basic_string_char_size) {
        win_skip("basic_string<char> unavailable\n");
        return;
    }

    call_func1(p_basic_string_char_ctor, &str1);
    str = NULL;
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    str = call_func1(p_basic_string_char_data, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    call_func1(p_basic_string_char_dtor, &str1);

    pstr = call_func2(p_basic_string_char_ctor_cstr, &str1, "test");
    ok(pstr == &str1, "pstr != &str1\n");
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str1);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    size = (size_t)call_func1(p_basic_string_char_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);

    pstr = call_func2(p_basic_string_char_copy_ctor, &str2, &str1);
    ok(pstr == &str2, "pstr != &str2\n");
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);

    call_func3(p_basic_string_char_erase, &str2, 1, 2);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "tt", 3), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "tt", 3), "str = %s\n", str);
    size = (size_t)call_func1(p_basic_string_char_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);

    call_func3(p_basic_string_char_erase, &str2, 1, 100);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "t", 2), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "t", 2), "str = %s\n", str);
    size = (size_t)call_func1(p_basic_string_char_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);

    call_func3(p_basic_string_char_assign_cstr_len, &str2, "test", 4);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);

    call_func3(p_basic_string_char_assign_cstr_len, &str2, (str+1), 2);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "es", 3), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "es", 3), "str = %s\n", str);

    call_func1(p_basic_string_char_dtor, &str1);
    call_func1(p_basic_string_char_dtor, &str2);
}

static void test_basic_string_wchar(void) {
    static const wchar_t test[] = { 't','e','s','t',0 };

    basic_string_wchar str1, str2, *pstr;
    const wchar_t *str;
    size_t size;

    if(!p_basic_string_wchar_ctor || !p_basic_string_wchar_copy_ctor
            || !p_basic_string_wchar_ctor_cstr || !p_basic_string_wchar_dtor
            || !p_basic_string_wchar_erase || !p_basic_string_wchar_assign_cstr_len
            || !p_basic_string_wchar_cstr || !p_basic_string_wchar_data
            || !p_basic_string_wchar_size) {
        win_skip("basic_string<wchar_t> unavailable\n");
        return;
    }

    call_func1(p_basic_string_wchar_ctor, &str1);
    str = NULL;
    str = call_func1(p_basic_string_wchar_cstr, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    str = call_func1(p_basic_string_wchar_data, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    call_func1(p_basic_string_wchar_dtor, &str1);

    pstr = call_func2(p_basic_string_wchar_ctor_cstr, &str1, test);
    ok(pstr == &str1, "pstr != &str1\n");
    str = call_func1(p_basic_string_wchar_cstr, &str1);
    ok(!memcmp(str, test, 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str1);
    ok(!memcmp(str, test, 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    size = (size_t)call_func1(p_basic_string_wchar_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);

    memset(&str2, 0, sizeof(basic_string_wchar));
    pstr = call_func2(p_basic_string_wchar_copy_ctor, &str2, &str1);
    ok(pstr == &str2, "pstr != &str2\n");
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(!memcmp(str, test, 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(!memcmp(str, test, 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));

    call_func3(p_basic_string_wchar_erase, &str2, 1, 2);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(str[0]=='t' && str[1]=='t' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(str[0]=='t' && str[1]=='t' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));
    size = (size_t)call_func1(p_basic_string_wchar_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);

    call_func3(p_basic_string_wchar_erase, &str2, 1, 100);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(str[0]=='t' && str[1]=='\0', "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(str[0]=='t' && str[1]=='\0', "str = %s\n", wine_dbgstr_w(str));
    size = (size_t)call_func1(p_basic_string_wchar_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);

    call_func3(p_basic_string_wchar_assign_cstr_len, &str2, test, 4);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(!memcmp(str, test, 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(!memcmp(str, test, 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));

    call_func3(p_basic_string_wchar_assign_cstr_len, &str2, (str+1), 2);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(str[0]=='e' && str[1]=='s' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(str[0]=='e' && str[1]=='s' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));

    call_func1(p_basic_string_wchar_dtor, &str1);
    call_func1(p_basic_string_wchar_dtor, &str2);
}

START_TEST(string)
{
    if(!init())
        return;

    test_basic_string_char();
    test_basic_string_wchar();

    ok(!invalid_parameter, "invalid_parameter_handler was invoked too many times\n");
}
