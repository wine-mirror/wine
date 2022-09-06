/*
 * Copyright 2012 Dan Kegel
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

#include <locale.h>

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

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

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(invalid_parameter_handler);
DEFINE_EXPECT(yield_func);

static _invalid_parameter_handler (__cdecl *p_set_invalid_parameter_handler)(_invalid_parameter_handler);

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
}

#ifdef __i386__

#include "pshpack1.h"
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#include "poppack.h"

static ULONG_PTR (WINAPI *call_thiscall_func1)( void *func, void *this );
static ULONG_PTR (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );
static ULONG_PTR (WINAPI *call_thiscall_func3)( void *func, void *this, const void *a, const void *b );

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc( NULL, sizeof(*thunk),
            MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void *)thunk;
    call_thiscall_func2 = (void *)thunk;
    call_thiscall_func3 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)

#endif /* __i386__ */

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

typedef unsigned char MSVCRT_bool;

typedef enum
{
    SPINWAIT_INIT,
    SPINWAIT_SPIN,
    SPINWAIT_YIELD,
    SPINWAIT_DONE
} SpinWait_state;

typedef void (__cdecl *yield_func)(void);

typedef struct
{
    ULONG spin;
    ULONG unknown;
    SpinWait_state state;
    yield_func yield_func;
} SpinWait;

typedef struct {
    CRITICAL_SECTION cs;
} _ReentrantBlockingLock;

typedef struct {
    char pad[64];
} event;

typedef struct {
    void *vtable;
} Context;

typedef struct {
    void *policy_container;
} SchedulerPolicy;

struct SchedulerVtbl;
typedef struct {
    struct SchedulerVtbl *vtable;
} Scheduler;

struct SchedulerVtbl {
    Scheduler* (__thiscall *vector_dor)(Scheduler*);
    unsigned int (__thiscall *Id)(Scheduler*);
    unsigned int (__thiscall *GetNumberOfVirtualProcessors)(Scheduler*);
    SchedulerPolicy* (__thiscall *GetPolicy)(Scheduler*);
    unsigned int (__thiscall *Reference)(Scheduler*);
    unsigned int (__thiscall *Release)(Scheduler*);
    void (__thiscall *RegisterShutdownEvent)(Scheduler*,HANDLE);
    void (__thiscall *Attach)(Scheduler*);
    /* CreateScheduleGroup */
    /* ScheduleTask */
};

static int* (__cdecl *p_errno)(void);
static int (__cdecl *p_wmemcpy_s)(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count);
static int (__cdecl *p_wmemmove_s)(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count);
static FILE* (__cdecl *p_fopen)(const char*,const char*);
static int (__cdecl *p_fclose)(FILE*);
static size_t (__cdecl *p_fread_s)(void*,size_t,size_t,size_t,FILE*);
static void (__cdecl *p_lock_file)(FILE*);
static void (__cdecl *p_unlock_file)(FILE*);
static void* (__cdecl *p__aligned_offset_malloc)(size_t, size_t, size_t);
static void (__cdecl *p__aligned_free)(void*);
static size_t (__cdecl *p__aligned_msize)(void*, size_t, size_t);
static int (__cdecl *p_atoi)(const char*);

static SpinWait* (__thiscall *pSpinWait_ctor_yield)(SpinWait*, yield_func);
static void (__thiscall *pSpinWait_dtor)(SpinWait*);
static void (__thiscall *pSpinWait__DoYield)(SpinWait*);
static ULONG (__thiscall *pSpinWait__NumberOfSpins)(SpinWait*);
static void (__thiscall *pSpinWait__SetSpinCount)(SpinWait*, unsigned int);
static MSVCRT_bool (__thiscall *pSpinWait__ShouldSpinAgain)(SpinWait*);
static MSVCRT_bool (__thiscall *pSpinWait__SpinOnce)(SpinWait*);

static void* (__thiscall *preader_writer_lock_ctor)(void*);
static void (__thiscall *preader_writer_lock_dtor)(void*);
static void (__thiscall *preader_writer_lock_lock)(void*);
static void (__thiscall *preader_writer_lock_lock_read)(void*);
static void (__thiscall *preader_writer_lock_unlock)(void*);
static MSVCRT_bool (__thiscall *preader_writer_lock_try_lock)(void*);
static MSVCRT_bool (__thiscall *preader_writer_lock_try_lock_read)(void*);

static _ReentrantBlockingLock* (__thiscall *p_ReentrantBlockingLock_ctor)(_ReentrantBlockingLock*);
static void (__thiscall *p_ReentrantBlockingLock_dtor)(_ReentrantBlockingLock*);
static void (__thiscall *p_ReentrantBlockingLock__Acquire)(_ReentrantBlockingLock*);
static void (__thiscall *p_ReentrantBlockingLock__Release)(_ReentrantBlockingLock*);
static MSVCRT_bool (__thiscall *p_ReentrantBlockingLock__TryAcquire)(_ReentrantBlockingLock*);
static _ReentrantBlockingLock* (__thiscall *p_NonReentrantBlockingLock_ctor)(_ReentrantBlockingLock*);
static void (__thiscall *p_NonReentrantBlockingLock_dtor)(_ReentrantBlockingLock*);
static void (__thiscall *p_NonReentrantBlockingLock__Acquire)(_ReentrantBlockingLock*);
static void (__thiscall *p_NonReentrantBlockingLock__Release)(_ReentrantBlockingLock*);
static MSVCRT_bool (__thiscall *p_NonReentrantBlockingLock__TryAcquire)(_ReentrantBlockingLock*);

static event* (__thiscall *p_event_ctor)(event*);
static void (__thiscall *p_event_dtor)(event*);
static void (__thiscall *p_event_reset)(event*);
static void (__thiscall *p_event_set)(event*);
static size_t (__thiscall *p_event_wait)(event*, unsigned int);
static int (__cdecl *p_event_wait_for_multiple)(event**, size_t, MSVCRT_bool, unsigned int);

static Context* (__cdecl *p_Context_CurrentContext)(void);
static unsigned int (__cdecl *p_Context_Id)(void);
static SchedulerPolicy* (__thiscall *p_SchedulerPolicy_ctor)(SchedulerPolicy*);
static void (__thiscall *p_SchedulerPolicy_SetConcurrencyLimits)(SchedulerPolicy*, unsigned int, unsigned int);
static void (__thiscall *p_SchedulerPolicy_dtor)(SchedulerPolicy*);
static Scheduler* (__cdecl *p_Scheduler_Create)(SchedulerPolicy*);
static Scheduler* (__cdecl *p_CurrentScheduler_Get)(void);
static void (__cdecl *p_CurrentScheduler_Detach)(void);
static unsigned int (__cdecl *p_CurrentScheduler_Id)(void);

static int (__cdecl *p__memicmp)(const char*, const char*, size_t);
static int (__cdecl *p__memicmp_l)(const char*, const char*, size_t,_locale_t);

static char* (__cdecl *p_setlocale)(int, const char*);
static size_t (__cdecl *p___strncnt)(const char*, size_t);

static int (__cdecl *p_strcmp)(const char *, const char *);
static int (__cdecl *p_strncmp)(const char *, const char *, size_t);

/* make sure we use the correct errno */
#undef errno
#define errno (*p_errno())

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hcrt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE hcrt;

    SetLastError(0xdeadbeef);
    hcrt = LoadLibraryA("msvcr100.dll");
    if (!hcrt) {
        win_skip("msvcr100.dll not installed (got %ld)\n", GetLastError());
        return FALSE;
    }

    SET(p_errno, "_errno");
    SET(p_set_invalid_parameter_handler, "_set_invalid_parameter_handler");
    SET(p_wmemcpy_s, "wmemcpy_s");
    SET(p_wmemmove_s, "wmemmove_s");
    SET(p_fopen, "fopen");
    SET(p_fclose, "fclose");
    SET(p_fread_s, "fread_s");
    SET(p_lock_file, "_lock_file");
    SET(p_unlock_file, "_unlock_file");
    SET(p__aligned_offset_malloc, "_aligned_offset_malloc");
    SET(p__aligned_free, "_aligned_free");
    SET(p__aligned_msize, "_aligned_msize");
    SET(p_atoi, "atoi");
    SET(p__memicmp, "_memicmp");
    SET(p__memicmp_l, "_memicmp_l");
    SET(p_setlocale, "setlocale");
    SET(p___strncnt, "__strncnt");

    SET(p_Context_Id, "?Id@Context@Concurrency@@SAIXZ");
    SET(p_CurrentScheduler_Detach, "?Detach@CurrentScheduler@Concurrency@@SAXXZ");
    SET(p_CurrentScheduler_Id, "?Id@CurrentScheduler@Concurrency@@SAIXZ");

    SET(p_strcmp, "strcmp");
    SET(p_strncmp, "strncmp");

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(pSpinWait_ctor_yield, "??0?$_SpinWait@$00@details@Concurrency@@QEAA@P6AXXZ@Z");
        SET(pSpinWait_dtor, "??_F?$_SpinWait@$00@details@Concurrency@@QEAAXXZ");
        SET(pSpinWait__DoYield, "?_DoYield@?$_SpinWait@$00@details@Concurrency@@IEAAXXZ");
        SET(pSpinWait__NumberOfSpins, "?_NumberOfSpins@?$_SpinWait@$00@details@Concurrency@@IEAAKXZ");
        SET(pSpinWait__SetSpinCount, "?_SetSpinCount@?$_SpinWait@$00@details@Concurrency@@QEAAXI@Z");
        SET(pSpinWait__ShouldSpinAgain, "?_ShouldSpinAgain@?$_SpinWait@$00@details@Concurrency@@IEAA_NXZ");
        SET(pSpinWait__SpinOnce, "?_SpinOnce@?$_SpinWait@$00@details@Concurrency@@QEAA_NXZ");

        SET(preader_writer_lock_ctor, "??0reader_writer_lock@Concurrency@@QEAA@XZ");
        SET(preader_writer_lock_dtor, "??1reader_writer_lock@Concurrency@@QEAA@XZ");
        SET(preader_writer_lock_lock, "?lock@reader_writer_lock@Concurrency@@QEAAXXZ");
        SET(preader_writer_lock_lock_read, "?lock_read@reader_writer_lock@Concurrency@@QEAAXXZ");
        SET(preader_writer_lock_unlock, "?unlock@reader_writer_lock@Concurrency@@QEAAXXZ");
        SET(preader_writer_lock_try_lock, "?try_lock@reader_writer_lock@Concurrency@@QEAA_NXZ");
        SET(preader_writer_lock_try_lock_read, "?try_lock_read@reader_writer_lock@Concurrency@@QEAA_NXZ");

        SET(p_ReentrantBlockingLock_ctor, "??0_ReentrantBlockingLock@details@Concurrency@@QEAA@XZ");
        SET(p_ReentrantBlockingLock_dtor, "??1_ReentrantBlockingLock@details@Concurrency@@QEAA@XZ");
        SET(p_ReentrantBlockingLock__Acquire, "?_Acquire@_ReentrantBlockingLock@details@Concurrency@@QEAAXXZ");
        SET(p_ReentrantBlockingLock__Release, "?_Release@_ReentrantBlockingLock@details@Concurrency@@QEAAXXZ");
        SET(p_ReentrantBlockingLock__TryAcquire, "?_TryAcquire@_ReentrantBlockingLock@details@Concurrency@@QEAA_NXZ");
        SET(p_NonReentrantBlockingLock_ctor, "??0_NonReentrantBlockingLock@details@Concurrency@@QEAA@XZ");
        SET(p_NonReentrantBlockingLock_dtor, "??1_NonReentrantBlockingLock@details@Concurrency@@QEAA@XZ");
        SET(p_NonReentrantBlockingLock__Acquire, "?_Acquire@_NonReentrantBlockingLock@details@Concurrency@@QEAAXXZ");
        SET(p_NonReentrantBlockingLock__Release, "?_Release@_NonReentrantBlockingLock@details@Concurrency@@QEAAXXZ");
        SET(p_NonReentrantBlockingLock__TryAcquire, "?_TryAcquire@_NonReentrantBlockingLock@details@Concurrency@@QEAA_NXZ");

        SET(p_event_ctor, "??0event@Concurrency@@QEAA@XZ");
        SET(p_event_dtor, "??1event@Concurrency@@QEAA@XZ");
        SET(p_event_reset, "?reset@event@Concurrency@@QEAAXXZ");
        SET(p_event_set, "?set@event@Concurrency@@QEAAXXZ");
        SET(p_event_wait, "?wait@event@Concurrency@@QEAA_KI@Z");
        SET(p_event_wait_for_multiple, "?wait_for_multiple@event@Concurrency@@SA_KPEAPEAV12@_K_NI@Z");

        SET(p_Context_CurrentContext, "?CurrentContext@Context@Concurrency@@SAPEAV12@XZ");
        SET(p_SchedulerPolicy_ctor, "??0SchedulerPolicy@Concurrency@@QEAA@XZ");
        SET(p_SchedulerPolicy_SetConcurrencyLimits, "?SetConcurrencyLimits@SchedulerPolicy@Concurrency@@QEAAXII@Z");
        SET(p_SchedulerPolicy_dtor, "??1SchedulerPolicy@Concurrency@@QEAA@XZ");
        SET(p_Scheduler_Create, "?Create@Scheduler@Concurrency@@SAPEAV12@AEBVSchedulerPolicy@2@@Z");
        SET(p_CurrentScheduler_Get, "?Get@CurrentScheduler@Concurrency@@SAPEAVScheduler@2@XZ");
    } else {
        SET(pSpinWait_ctor_yield, "??0?$_SpinWait@$00@details@Concurrency@@QAE@P6AXXZ@Z");
        SET(pSpinWait_dtor, "??_F?$_SpinWait@$00@details@Concurrency@@QAEXXZ");
        SET(pSpinWait__DoYield, "?_DoYield@?$_SpinWait@$00@details@Concurrency@@IAEXXZ");
        SET(pSpinWait__NumberOfSpins, "?_NumberOfSpins@?$_SpinWait@$00@details@Concurrency@@IAEKXZ");
        SET(pSpinWait__SetSpinCount, "?_SetSpinCount@?$_SpinWait@$00@details@Concurrency@@QAEXI@Z");
        SET(pSpinWait__ShouldSpinAgain, "?_ShouldSpinAgain@?$_SpinWait@$00@details@Concurrency@@IAE_NXZ");
        SET(pSpinWait__SpinOnce, "?_SpinOnce@?$_SpinWait@$00@details@Concurrency@@QAE_NXZ");

        SET(preader_writer_lock_ctor, "??0reader_writer_lock@Concurrency@@QAE@XZ");
        SET(preader_writer_lock_dtor, "??1reader_writer_lock@Concurrency@@QAE@XZ");
        SET(preader_writer_lock_lock, "?lock@reader_writer_lock@Concurrency@@QAEXXZ");
        SET(preader_writer_lock_lock_read, "?lock_read@reader_writer_lock@Concurrency@@QAEXXZ");
        SET(preader_writer_lock_unlock, "?unlock@reader_writer_lock@Concurrency@@QAEXXZ");
        SET(preader_writer_lock_try_lock, "?try_lock@reader_writer_lock@Concurrency@@QAE_NXZ");
        SET(preader_writer_lock_try_lock_read, "?try_lock_read@reader_writer_lock@Concurrency@@QAE_NXZ");

        SET(p_ReentrantBlockingLock_ctor, "??0_ReentrantBlockingLock@details@Concurrency@@QAE@XZ");
        SET(p_ReentrantBlockingLock_dtor, "??1_ReentrantBlockingLock@details@Concurrency@@QAE@XZ");
        SET(p_ReentrantBlockingLock__Acquire, "?_Acquire@_ReentrantBlockingLock@details@Concurrency@@QAEXXZ");
        SET(p_ReentrantBlockingLock__Release, "?_Release@_ReentrantBlockingLock@details@Concurrency@@QAEXXZ");
        SET(p_ReentrantBlockingLock__TryAcquire, "?_TryAcquire@_ReentrantBlockingLock@details@Concurrency@@QAE_NXZ");
        SET(p_NonReentrantBlockingLock_ctor, "??0_NonReentrantBlockingLock@details@Concurrency@@QAE@XZ");
        SET(p_NonReentrantBlockingLock_dtor, "??1_NonReentrantBlockingLock@details@Concurrency@@QAE@XZ");
        SET(p_NonReentrantBlockingLock__Acquire, "?_Acquire@_NonReentrantBlockingLock@details@Concurrency@@QAEXXZ");
        SET(p_NonReentrantBlockingLock__Release, "?_Release@_NonReentrantBlockingLock@details@Concurrency@@QAEXXZ");
        SET(p_NonReentrantBlockingLock__TryAcquire, "?_TryAcquire@_NonReentrantBlockingLock@details@Concurrency@@QAE_NXZ");

        SET(p_event_ctor, "??0event@Concurrency@@QAE@XZ");
        SET(p_event_dtor, "??1event@Concurrency@@QAE@XZ");
        SET(p_event_reset, "?reset@event@Concurrency@@QAEXXZ");
        SET(p_event_set, "?set@event@Concurrency@@QAEXXZ");
        SET(p_event_wait, "?wait@event@Concurrency@@QAEII@Z");
        SET(p_event_wait_for_multiple, "?wait_for_multiple@event@Concurrency@@SAIPAPAV12@I_NI@Z");

        SET(p_Context_CurrentContext, "?CurrentContext@Context@Concurrency@@SAPAV12@XZ");
        SET(p_SchedulerPolicy_ctor, "??0SchedulerPolicy@Concurrency@@QAE@XZ");
        SET(p_SchedulerPolicy_SetConcurrencyLimits, "?SetConcurrencyLimits@SchedulerPolicy@Concurrency@@QAEXII@Z");
        SET(p_SchedulerPolicy_dtor, "??1SchedulerPolicy@Concurrency@@QAE@XZ");
        SET(p_Scheduler_Create, "?Create@Scheduler@Concurrency@@SAPAV12@ABVSchedulerPolicy@2@@Z");
        SET(p_CurrentScheduler_Get, "?Get@CurrentScheduler@Concurrency@@SAPAVScheduler@2@XZ");
    }

    init_thiscall_thunk();
    return TRUE;
}

#define okwchars(dst, b0, b1, b2, b3, b4, b5, b6, b7) \
    ok(dst[0] == b0 && dst[1] == b1 && dst[2] == b2 && dst[3] == b3 && \
       dst[4] == b4 && dst[5] == b5 && dst[6] == b6 && dst[7] == b7, \
       "Bad result: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",\
       dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7])

static void test_wmemcpy_s(void)
{
    static wchar_t dest[8], buf[32];
    static const wchar_t tiny[] = L"T\0INY";
    static const wchar_t big[] = L"atoolongstring";
    const wchar_t XX = 0x5858;     /* two 'X' bytes */
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    /* Normal */
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(ret == 0, "Copying a buffer into a big enough destination returned %d, expected 0\n", ret);
    okwchars(dest, tiny[0], tiny[1], tiny[2], tiny[3], tiny[4], tiny[5], XX, XX);

    /* Vary source size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, ARRAY_SIZE(dest), big, ARRAY_SIZE(big));
    ok(errno == ERANGE, "Copying a big buffer to a small destination errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Copying a big buffer to a small destination returned %d, expected ERANGE\n", ret);
    okwchars(dest, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace source with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, ARRAY_SIZE(dest), NULL, ARRAY_SIZE(tiny));
    ok(errno == EINVAL, "Copying a NULL source buffer errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Copying a NULL source buffer returned %d, expected EINVAL\n", ret);
    okwchars(dest, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK_CALLED(invalid_parameter_handler);

    /* Vary dest size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, 0, tiny, ARRAY_SIZE(tiny));
    ok(errno == ERANGE, "Copying into a destination of size 0 errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Copying into a destination of size 0 returned %d, expected ERANGE\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace dest with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wmemcpy_s(NULL, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(errno == EINVAL, "Copying a tiny buffer to a big NULL destination errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Copying a tiny buffer to a big NULL destination returned %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* Combinations */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, 0, NULL, ARRAY_SIZE(tiny));
    ok(errno == EINVAL, "Copying a NULL buffer into a destination of size 0 errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Copying a NULL buffer into a destination of size 0 returned %d, expected EINVAL\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    ret = p_wmemcpy_s(buf, ARRAY_SIZE(buf), big, ARRAY_SIZE(big));
    ok(!ret, "wmemcpy_s returned %d\n", ret);
    ok(!memcmp(buf, big, sizeof(big)), "unexpected buf\n");

    ret = p_wmemcpy_s(buf + 1, ARRAY_SIZE(buf) - 1, buf, ARRAY_SIZE(big));
    ok(!ret, "wmemcpy_s returned %d\n", ret);
    ok(!memcmp(buf + 1, big, sizeof(big)), "unexpected buf\n");

    ret = p_wmemcpy_s(buf, ARRAY_SIZE(buf), buf + 1, ARRAY_SIZE(big));
    ok(!ret, "wmemcpy_s returned %d\n", ret);
    ok(!memcmp(buf, big, sizeof(big)), "unexpected buf\n");

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

static void test_wmemmove_s(void)
{
    static wchar_t dest[8];
    static const wchar_t tiny[] = L"T\0INY";
    static const wchar_t big[] = L"atoolongstring";
    const wchar_t XX = 0x5858;     /* two 'X' bytes */
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    /* Normal */
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(ret == 0, "Moving a buffer into a big enough destination returned %d, expected 0\n", ret);
    okwchars(dest, tiny[0], tiny[1], tiny[2], tiny[3], tiny[4], tiny[5], XX, XX);

    /* Overlapping */
    memcpy(dest, big, sizeof(dest));
    ret = p_wmemmove_s(dest+1, ARRAY_SIZE(dest)-1, dest, ARRAY_SIZE(dest)-1);
    ok(ret == 0, "Moving a buffer up one char returned %d, expected 0\n", ret);
    okwchars(dest, big[0], big[0], big[1], big[2], big[3], big[4], big[5], big[6]);

    /* Vary source size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, ARRAY_SIZE(dest), big, ARRAY_SIZE(big));
    ok(errno == ERANGE, "Moving a big buffer to a small destination errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Moving a big buffer to a small destination returned %d, expected ERANGE\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace source with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, ARRAY_SIZE(dest), NULL, ARRAY_SIZE(tiny));
    ok(errno == EINVAL, "Moving a NULL source buffer errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Moving a NULL source buffer returned %d, expected EINVAL\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Vary dest size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, 0, tiny, ARRAY_SIZE(tiny));
    ok(errno == ERANGE, "Moving into a destination of size 0 errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Moving into a destination of size 0 returned %d, expected ERANGE\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace dest with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wmemmove_s(NULL, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(errno == EINVAL, "Moving a tiny buffer to a big NULL destination errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Moving a tiny buffer to a big NULL destination returned %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* Combinations */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, 0, NULL, ARRAY_SIZE(tiny));
    ok(errno == EINVAL, "Moving a NULL buffer into a destination of size 0 errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Moving a NULL buffer into a destination of size 0 returned %d, expected EINVAL\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

struct block_file_arg
{
    FILE *test_file;
    HANDLE init;
    HANDLE finish;
};

static DWORD WINAPI block_file(void *arg)
{
    struct block_file_arg *files = arg;
    p_lock_file(files->test_file);
    SetEvent(files->init);
    WaitForSingleObject(files->finish, INFINITE);
    p_unlock_file(files->test_file);
    return 0;
}

static void test_fread_s(void)
{
    static const char test_file[] = "fread_s.tst";
    int ret;
    char buf[10];
    HANDLE thread;
    struct block_file_arg arg;

    FILE *f = fopen(test_file, "w");
    if(!f) {
        skip("Error creating test file\n");
        return;
    }
    fwrite("test", 1, 4, f);
    fclose(f);

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p_fread_s(buf, sizeof(buf), 1, 1, NULL);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    f = p_fopen(test_file, "r");
    arg.test_file = f;
    arg.init = CreateEventW(NULL, FALSE, FALSE, NULL);
    arg.finish = CreateEventW(NULL, FALSE, FALSE, NULL);
    thread = CreateThread(NULL, 0, block_file, (void*)&arg, 0, NULL);
    WaitForSingleObject(arg.init, INFINITE);

    errno = 0xdeadbeef;
    ret = p_fread_s(NULL, sizeof(buf), 0, 1, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d, expected 0xdeadbeef\n", errno);
    ret = p_fread_s(NULL, sizeof(buf), 1, 0, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d, expected 0xdeadbeef\n", errno);

    SetEvent(arg.finish);
    WaitForSingleObject(thread, INFINITE);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p_fread_s(NULL, sizeof(buf), 1, 1, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    buf[1] = 'a';
    ret = p_fread_s(buf, 3, 1, 10, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(buf[0] == 0, "buf[0] = '%c', expected 0\n", buf[0]);
    ok(buf[1] == 0, "buf[1] = '%c', expected 0\n", buf[1]);
    ok(errno == ERANGE, "errno = %d, expected ERANGE\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p_fread_s(buf, 2, 1, 10, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(buf[0] == 0, "buf[0] = '%c', expected 0\n", buf[0]);
    ok(errno == ERANGE, "errno = %d, expected ERANGE\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    memset(buf, 'a', sizeof(buf));
    ret = p_fread_s(buf, sizeof(buf), 3, 10, f);
    ok(ret==1, "fread_s returned %d, expected 1\n", ret);
    ok(buf[0] == 'e', "buf[0] = '%c', expected 'e'\n", buf[0]);
    ok(buf[1] == 's', "buf[1] = '%c', expected 's'\n", buf[1]);
    ok(buf[2] == 't', "buf[2] = '%c', expected 't'\n", buf[2]);
    ok(buf[3] == 'a', "buf[3] = '%c', expected 'a'\n", buf[3]);
    p_fclose(f);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
    CloseHandle(arg.init);
    CloseHandle(arg.finish);
    CloseHandle(thread);
    unlink(test_file);
}

static void test__aligned_msize(void)
{
    void *mem;
    int ret;

    mem = p__aligned_offset_malloc(23, 16, 7);
    ret = p__aligned_msize(mem, 16, 7);
    ok(ret == 23, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 15, 7);
    ok(ret == 24, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 11, 7);
    ok(ret == 28, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 1, 7);
    ok(ret == 39-sizeof(void*), "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 8, 0);
    todo_wine ok(ret == 32, "_aligned_msize returned %d\n", ret);
    p__aligned_free(mem);

    mem = p__aligned_offset_malloc(3, 16, 0);
    ret = p__aligned_msize(mem, 16, 0);
    ok(ret == 3, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 11, 0);
    ok(ret == 8, "_aligned_msize returned %d\n", ret);
    p__aligned_free(mem);
}

static void test_atoi(void)
{
    int r;

    r = p_atoi("0");
    ok(r == 0, "atoi(0) = %d\n", r);

    r = p_atoi("-1");
    ok(r == -1, "atoi(-1) = %d\n", r);

    r = p_atoi("1");
    ok(r == 1, "atoi(1) = %d\n", r);

    r = p_atoi("4294967296");
    ok(r == 2147483647, "atoi(4294967296) = %d\n", r);
}

static void __cdecl test_yield_func(void)
{
    CHECK_EXPECT(yield_func);
}

static void test__SpinWait(void)
{
    SpinWait sp;
    ULONG ul;
    MSVCRT_bool b;

    call_func2(pSpinWait_ctor_yield, &sp, test_yield_func);

    SET_EXPECT(yield_func);
    call_func1(pSpinWait__DoYield, &sp);
    CHECK_CALLED(yield_func);

    ul = call_func1(pSpinWait__NumberOfSpins, &sp);
    ok(ul == 1, "_SpinWait::_NumberOfSpins returned %lu\n", ul);

    sp.spin = 2;
    b = call_func1(pSpinWait__ShouldSpinAgain, &sp);
    ok(b, "_SpinWait::_ShouldSpinAgain returned %x\n", b);
    ok(sp.spin == 1, "sp.spin = %lu\n", sp.spin);
    b = call_func1(pSpinWait__ShouldSpinAgain, &sp);
    ok(!b, "_SpinWait::_ShouldSpinAgain returned %x\n", b);
    ok(sp.spin == 0, "sp.spin = %lu\n", sp.spin);
    b = call_func1(pSpinWait__ShouldSpinAgain, &sp);
    ok(b, "_SpinWait::_ShouldSpinAgain returned %x\n", b);
    ok(sp.spin == -1, "sp.spin = %lu\n", sp.spin);

    call_func2(pSpinWait__SetSpinCount, &sp, 2);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin == 1, "sp.spin = %lu\n", sp.spin);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin == 0, "sp.spin = %lu\n", sp.spin);
    SET_EXPECT(yield_func);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin == 0, "sp.spin = %lu\n", sp.spin);
    CHECK_CALLED(yield_func);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(!b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin==0 || sp.spin==4000, "sp.spin = %lu\n", sp.spin);

    call_func1(pSpinWait_dtor, &sp);
}

static DWORD WINAPI lock_read_thread(void *rw_lock)
{
    return (MSVCRT_bool)call_func1(preader_writer_lock_try_lock_read, rw_lock);
}

static void test_reader_writer_lock(void)
{
    /* define reader_writer_lock data big enough to hold every version of structure */
    char rw_lock[100];
    MSVCRT_bool ret;
    HANDLE thread;
    DWORD d;

    call_func1(preader_writer_lock_ctor, rw_lock);

    ret = call_func1(preader_writer_lock_try_lock, rw_lock);
    ok(ret, "reader_writer_lock:try_lock returned %x\n", ret);
    ret = call_func1(preader_writer_lock_try_lock, rw_lock);
    ok(!ret, "reader_writer_lock:try_lock returned %x\n", ret);
    ret = call_func1(preader_writer_lock_try_lock_read, rw_lock);
    ok(!ret, "reader_writer_lock:try_lock_read returned %x\n", ret);
    call_func1(preader_writer_lock_unlock, rw_lock);

    /* test lock for read on already locked section */
    ret = call_func1(preader_writer_lock_try_lock_read, rw_lock);
    ok(ret, "reader_writer_lock:try_lock_read returned %x\n", ret);
    ret = call_func1(preader_writer_lock_try_lock_read, rw_lock);
    ok(ret, "reader_writer_lock:try_lock_read returned %x\n", ret);
    ret = call_func1(preader_writer_lock_try_lock, rw_lock);
    ok(!ret, "reader_writer_lock:try_lock returned %x\n", ret);
    call_func1(preader_writer_lock_unlock, rw_lock);
    ret = call_func1(preader_writer_lock_try_lock, rw_lock);
    ok(!ret, "reader_writer_lock:try_lock returned %x\n", ret);
    call_func1(preader_writer_lock_unlock, rw_lock);
    ret = call_func1(preader_writer_lock_try_lock, rw_lock);
    ok(ret, "reader_writer_lock:try_lock returned %x\n", ret);
    call_func1(preader_writer_lock_unlock, rw_lock);

    call_func1(preader_writer_lock_lock, rw_lock);
    thread = CreateThread(NULL, 0, lock_read_thread, rw_lock, 0, NULL);
    ok(thread != NULL, "CreateThread failed: %ld\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    ok(GetExitCodeThread(thread, &d), "GetExitCodeThread failed\n");
    ok(!d, "reader_writer_lock::try_lock_read succeeded on already locked object\n");
    CloseHandle(thread);
    call_func1(preader_writer_lock_unlock, rw_lock);

    call_func1(preader_writer_lock_lock_read, rw_lock);
    thread = CreateThread(NULL, 0, lock_read_thread, rw_lock, 0, NULL);
    ok(thread != NULL, "CreateThread failed: %ld\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    ok(GetExitCodeThread(thread, &d), "GetExitCodeThread failed\n");
    ok(d, "reader_writer_lock::try_lock_read failed on object locked for reading\n");
    CloseHandle(thread);
    call_func1(preader_writer_lock_unlock, rw_lock);

    call_func1(preader_writer_lock_dtor, rw_lock);
}

static void test__ReentrantBlockingLock(void)
{
    _ReentrantBlockingLock rbl;
    MSVCRT_bool ret;

    call_func1(p_ReentrantBlockingLock_ctor, &rbl);
    ret = call_func1(p_ReentrantBlockingLock__TryAcquire, &rbl);
    ok(ret, "_ReentrantBlockingLock__TryAcquire failed\n");
    call_func1(p_ReentrantBlockingLock__Acquire, &rbl);
    ret = call_func1(p_ReentrantBlockingLock__TryAcquire, &rbl);
    ok(ret, "_ReentrantBlockingLock__TryAcquire failed\n");
    call_func1(p_ReentrantBlockingLock__Release, &rbl);
    call_func1(p_ReentrantBlockingLock__Release, &rbl);
    call_func1(p_ReentrantBlockingLock__Release, &rbl);
    call_func1(p_ReentrantBlockingLock_dtor, &rbl);

    call_func1(p_NonReentrantBlockingLock_ctor, &rbl);
    ret = call_func1(p_NonReentrantBlockingLock__TryAcquire, &rbl);
    ok(ret, "_NonReentrantBlockingLock__TryAcquire failed\n");
    call_func1(p_NonReentrantBlockingLock__Acquire, &rbl);
    ret = call_func1(p_NonReentrantBlockingLock__TryAcquire, &rbl);
    ok(ret, "_NonReentrantBlockingLock__TryAcquire failed\n");
    call_func1(p_NonReentrantBlockingLock__Release, &rbl);
    call_func1(p_NonReentrantBlockingLock__Release, &rbl);
    call_func1(p_NonReentrantBlockingLock__Release, &rbl);
    call_func1(p_NonReentrantBlockingLock_dtor, &rbl);
}

static DWORD WINAPI test_event_thread(void *arg)
{
    event *evt = arg;
    call_func1(p_event_set, evt);
    return 0;
}

static DWORD WINAPI multiple_events_thread(void *arg)
{
     event **events = arg;

     Sleep(50);
     call_func1(p_event_set, events[0]);
     call_func1(p_event_reset, events[0]);
     call_func1(p_event_set, events[1]);
     call_func1(p_event_reset, events[1]);
     return 0;
}

static void test_event(void)
{
    int i;
    int ret;
    event evt;
    event *evts[70];
    HANDLE thread;
    HANDLE threads[ARRAY_SIZE(evts)];

    call_func1(p_event_ctor, &evt);

    ret = call_func2(p_event_wait, &evt, 100);
    ok(ret == -1, "expected -1, got %d\n", ret);

    call_func1(p_event_set, &evt);
    ret = call_func2(p_event_wait, &evt, 100);
    ok(!ret, "expected 0, got %d\n", ret);

    ret = call_func2(p_event_wait, &evt, 100);
    ok(!ret, "expected 0, got %d\n", ret);

    call_func1(p_event_reset, &evt);
    ret = call_func2(p_event_wait, &evt, 100);
    ok(ret == -1, "expected -1, got %d\n", ret);

    thread = CreateThread(NULL, 0, test_event_thread, (void*)&evt, 0, NULL);
    ret = call_func2(p_event_wait, &evt, 5000);
    ok(!ret, "expected 0, got %d\n", ret);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    if (0) /* crashes on Windows */
        p_event_wait_for_multiple(NULL, 10, TRUE, 0);

    for (i = 0; i < ARRAY_SIZE(evts); i++) {
        evts[i] = malloc(sizeof(evt));
        call_func1(p_event_ctor, evts[i]);
    }

    ret = p_event_wait_for_multiple(evts, 0, TRUE, 100);
    ok(!ret, "expected 0, got %d\n", ret);

    ret = p_event_wait_for_multiple(evts, ARRAY_SIZE(evts), TRUE, 100);
    ok(ret == -1, "expected -1, got %d\n", ret);

    /* reset and test wait for multiple with all */
    for (i = 0; i < ARRAY_SIZE(evts); i++)
        threads[i] = CreateThread(NULL, 0, test_event_thread, (void*)evts[i], 0, NULL);

    ret = p_event_wait_for_multiple(evts, ARRAY_SIZE(evts), TRUE, 5000);
    ok(ret != -1, "didn't expect -1\n");

    for (i = 0; i < ARRAY_SIZE(evts); i++) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    /* reset and test wait for multiple with any */
    call_func1(p_event_reset, evts[0]);

    thread = CreateThread(NULL, 0, test_event_thread, (void*)evts[0], 0, NULL);
    ret = p_event_wait_for_multiple(evts, 1, FALSE, 5000);
    ok(!ret, "expected 0, got %d\n", ret);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    call_func1(p_event_reset, evts[0]);
    call_func1(p_event_reset, evts[1]);
    thread = CreateThread(NULL, 0, multiple_events_thread, (void*)evts, 0, NULL);
    ret = p_event_wait_for_multiple(evts, 2, TRUE, 500);
    ok(ret == -1, "expected -1, got %d\n", ret);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    call_func1(p_event_reset, evts[0]);
    call_func1(p_event_set, evts[1]);
    ret = p_event_wait_for_multiple(evts, 2, FALSE, 0);
    ok(ret == 1, "expected 1, got %d\n", ret);

    for (i = 0; i < ARRAY_SIZE(evts); i++) {
        call_func1(p_event_dtor, evts[i]);
        free(evts[i]);
    }

    call_func1(p_event_dtor, &evt);
}

static DWORD WINAPI external_context_thread(void *arg)
{
    unsigned int id;
    Context *ctx;

    id = p_Context_Id();
    ok(id == -1, "Context::Id() = %u\n", id);

    ctx = p_Context_CurrentContext();
    ok(ctx != NULL, "Context::CurrentContext() = NULL\n");
    id = p_Context_Id();
    ok(id == 1, "Context::Id() = %u\n", id);
    return 0;
}

static void test_ExternalContextBase(void)
{
    unsigned int id;
    Context *ctx;
    HANDLE thread;

    id = p_Context_Id();
    ok(id == -1, "Context::Id() = %u\n", id);

    ctx = p_Context_CurrentContext();
    ok(ctx != NULL, "Context::CurrentContext() = NULL\n");
    id = p_Context_Id();
    ok(id == 0, "Context::Id() = %u\n", id);

    ctx = p_Context_CurrentContext();
    ok(ctx != NULL, "Context::CurrentContext() = NULL\n");
    id = p_Context_Id();
    ok(id == 0, "Context::Id() = %u\n", id);

    thread = CreateThread(NULL, 0, external_context_thread, NULL, 0, NULL);
    ok(thread != NULL, "CreateThread failed: %ld\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

static void test_Scheduler(void)
{
    Scheduler *scheduler, *current_scheduler;
    SchedulerPolicy policy;
    unsigned int i;

    call_func1(p_SchedulerPolicy_ctor, &policy);
    scheduler = p_Scheduler_Create(&policy);
    ok(scheduler != NULL, "Scheduler::Create() = NULL\n");

    call_func1(scheduler->vtable->Attach, scheduler);
    current_scheduler = p_CurrentScheduler_Get();
    ok(current_scheduler == scheduler, "CurrentScheduler::Get() = %p, expected %p\n",
            current_scheduler, scheduler);
    p_CurrentScheduler_Detach();

    current_scheduler = p_CurrentScheduler_Get();
    ok(current_scheduler != scheduler, "scheduler has not changed after detach\n");
    call_func1(scheduler->vtable->Release, scheduler);

    i = p_CurrentScheduler_Id();
    ok(!i, "CurrentScheduler::Id() = %u\n", i);

    call_func3(p_SchedulerPolicy_SetConcurrencyLimits, &policy, 1, 1);
    scheduler = p_Scheduler_Create(&policy);
    ok(scheduler != NULL, "Scheduler::Create() = NULL\n");

    i = call_func1(scheduler->vtable->GetNumberOfVirtualProcessors, scheduler);
    ok(i == 1, "Scheduler::GetNumberOfVirtualProcessors() = %u\n", i);
    call_func1(scheduler->vtable->Release, scheduler);
    call_func1(p_SchedulerPolicy_dtor, &policy);
}

static void test__memicmp(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    ret = p__memicmp(NULL, NULL, 0);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    ret = p__memicmp(NULL, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ret = p__memicmp(s1, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ret = p__memicmp(NULL, s2, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    ret = p__memicmp(s1, s2, 2);
    ok(!ret, "got %d\n", ret);

    ret = p__memicmp(s1, s2, 3);
    ok(ret == -1, "got %d\n", ret);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

static void test__memicmp_l(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    ret = p__memicmp_l(NULL, NULL, 0, NULL);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    ret = p__memicmp_l(NULL, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ret = p__memicmp_l(s1, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ret = p__memicmp_l(NULL, s2, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    ret = p__memicmp_l(s1, s2, 2, NULL);
    ok(!ret, "got %d\n", ret);

    ret = p__memicmp_l(s1, s2, 3, NULL);
    ok(ret == -1, "got %d\n", ret);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

static void test_setlocale(void)
{
    char *ret;

    ret = p_setlocale(LC_ALL, "en-US");
    ok(!ret, "got %p\n", ret);
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

    if (0) /* crashes */
        ret = p___strncnt(NULL, 1);

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = p___strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
    }
}

static void test_strcmp(void)
{
    int ret = p_strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = p_strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
#ifdef _WIN64
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
#else
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == 0 - 'a', "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == 'c' - 0xa0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 0xb0 - 0xa0, "wrong ret %d\n", ret );
#endif
    ret = p_strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

START_TEST(msvcr100)
{
    if (!init())
        return;

    test_ExternalContextBase();
    test_Scheduler();
    test_wmemcpy_s();
    test_wmemmove_s();
    test_fread_s();
    test__aligned_msize();
    test_atoi();
    test__SpinWait();
    test_reader_writer_lock();
    test__ReentrantBlockingLock();
    test_event();
    test__memicmp();
    test__memicmp_l();
    test_setlocale();
    test___strncnt();
    test_strcmp();
}
