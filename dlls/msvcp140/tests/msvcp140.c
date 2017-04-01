/*
 * Copyright 2016 Daniel Lehman
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

#include "wine/test.h"
#include "winbase.h"

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

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#ifdef _WIN64
DEFINE_EXPECT(function_do_call);
DEFINE_EXPECT(function_do_clean);
#endif

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

/* Emulate a __thiscall */
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

static void * (WINAPI *call_thiscall_func1)( void *func, void *this );
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );

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
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,a)

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */
typedef unsigned char MSVCP_bool;

typedef struct {
    void *unk0;
    BYTE unk1;
} task_continuation_context;

typedef struct {
    void *unused;
} _ContextCallback;

typedef struct {
    const void *vtable;
    void (__cdecl *func)(void);
    int unk[4];
    void *unk2[3];
    void *this;
} function_void_cdecl_void;

typedef struct {
    void *task;
    MSVCP_bool scheduled;
    MSVCP_bool started;
} _TaskEventLogger;

typedef struct {
    PTP_WORK work;
    void (__cdecl *callback)(void*);
    void *arg;
} _Threadpool_chore;

static unsigned int (__cdecl *p__Thrd_id)(void);
static task_continuation_context* (__thiscall *p_task_continuation_context_ctor)(task_continuation_context*);
static void (__thiscall *p__ContextCallback__Assign)(_ContextCallback*, void*);
static void (__thiscall *p__ContextCallback__CallInContext)(const _ContextCallback*, function_void_cdecl_void, MSVCP_bool);
static void (__thiscall *p__ContextCallback__Capture)(_ContextCallback*);
static void (__thiscall *p__ContextCallback__Reset)(_ContextCallback*);
static MSVCP_bool (__cdecl *p__ContextCallback__IsCurrentOriginSTA)(_ContextCallback*);
static void (__thiscall *p__TaskEventLogger__LogCancelTask)(_TaskEventLogger*);
static void (__thiscall *p__TaskEventLogger__LogScheduleTask)(_TaskEventLogger*, MSVCP_bool);
static void (__thiscall *p__TaskEventLogger__LogTaskCompleted)(_TaskEventLogger*);
static void (__thiscall *p__TaskEventLogger__LogTaskExecutionCompleted)(_TaskEventLogger*);
static void (__thiscall *p__TaskEventLogger__LogWorkItemCompleted)(_TaskEventLogger*);
static void (__thiscall *p__TaskEventLogger__LogWorkItemStarted)(_TaskEventLogger*);
static int (__cdecl *p__Schedule_chore)(_Threadpool_chore*);
static int (__cdecl *p__Reschedule_chore)(const _Threadpool_chore*);
static void (__cdecl *p__Release_chore)(_Threadpool_chore*);

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp140.dll");
    if(!msvcp)
    {
        win_skip("msvcp140.dll not installed\n");
        return FALSE;
    }

    SET(p__Thrd_id, "_Thrd_id");
    SET(p__ContextCallback__IsCurrentOriginSTA, "?_IsCurrentOriginSTA@_ContextCallback@details@Concurrency@@CA_NXZ");

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_task_continuation_context_ctor, "??0task_continuation_context@Concurrency@@AEAA@XZ");
        SET(p__ContextCallback__Assign, "?_Assign@_ContextCallback@details@Concurrency@@AEAAXPEAX@Z");
        SET(p__ContextCallback__CallInContext, "?_CallInContext@_ContextCallback@details@Concurrency@@QEBAXV?$function@$$A6AXXZ@std@@_N@Z");
        SET(p__ContextCallback__Capture, "?_Capture@_ContextCallback@details@Concurrency@@AEAAXXZ");
        SET(p__ContextCallback__Reset, "?_Reset@_ContextCallback@details@Concurrency@@AEAAXXZ");
        SET(p__TaskEventLogger__LogCancelTask, "?_LogCancelTask@_TaskEventLogger@details@Concurrency@@QEAAXXZ");
        SET(p__TaskEventLogger__LogScheduleTask, "?_LogScheduleTask@_TaskEventLogger@details@Concurrency@@QEAAX_N@Z");
        SET(p__TaskEventLogger__LogTaskCompleted, "?_LogTaskCompleted@_TaskEventLogger@details@Concurrency@@QEAAXXZ");
        SET(p__TaskEventLogger__LogTaskExecutionCompleted, "?_LogTaskExecutionCompleted@_TaskEventLogger@details@Concurrency@@QEAAXXZ");
        SET(p__TaskEventLogger__LogWorkItemCompleted, "?_LogWorkItemCompleted@_TaskEventLogger@details@Concurrency@@QEAAXXZ");
        SET(p__TaskEventLogger__LogWorkItemStarted, "?_LogWorkItemStarted@_TaskEventLogger@details@Concurrency@@QEAAXXZ");
        SET(p__Schedule_chore, "?_Schedule_chore@details@Concurrency@@YAHPEAU_Threadpool_chore@12@@Z");
        SET(p__Reschedule_chore, "?_Reschedule_chore@details@Concurrency@@YAHPEBU_Threadpool_chore@12@@Z");
        SET(p__Release_chore, "?_Release_chore@details@Concurrency@@YAXPEAU_Threadpool_chore@12@@Z");
    } else {
#ifdef __arm__
        SET(p_task_continuation_context_ctor, "??0task_continuation_context@Concurrency@@AAA@XZ");
        SET(p__ContextCallback__Assign, "?_Assign@_ContextCallback@details@Concurrency@@AAAXPAX@Z");
        SET(p__ContextCallback__CallInContext, "?_CallInContext@_ContextCallback@details@Concurrency@@QBAXV?$function@$$A6AXXZ@std@@_N@Z");
        SET(p__ContextCallback__Capture, "?_Capture@_ContextCallback@details@Concurrency@@AAAXXZ");
        SET(p__ContextCallback__Reset, "?_Reset@_ContextCallback@details@Concurrency@@AAAXXZ");
        SET(p__TaskEventLogger__LogCancelTask, "?_LogCancelTask@_TaskEventLogger@details@Concurrency@@QAAXXZ");
        SET(p__TaskEventLogger__LogScheduleTask, "?_LogScheduleTask@_TaskEventLogger@details@Concurrency@@QAEX_N@Z");
        SET(p__TaskEventLogger__LogTaskCompleted, "?_LogTaskCompleted@_TaskEventLogger@details@Concurrency@@QAAXXZ");
        SET(p__TaskEventLogger__LogTaskExecutionCompleted, "?_LogTaskExecutionCompleted@_TaskEventLogger@details@Concurrency@@QAAXXZ");
        SET(p__TaskEventLogger__LogWorkItemCompleted, "?_LogWorkItemCompleted@_TaskEventLogger@details@Concurrency@@QAAXXZ");
        SET(p__TaskEventLogger__LogWorkItemStarted, "?_LogWorkItemStarted@_TaskEventLogger@details@Concurrency@@QAAXXZ");
#else
        SET(p_task_continuation_context_ctor, "??0task_continuation_context@Concurrency@@AAE@XZ");
        SET(p__ContextCallback__Assign, "?_Assign@_ContextCallback@details@Concurrency@@AAEXPAX@Z");
        SET(p__ContextCallback__CallInContext, "?_CallInContext@_ContextCallback@details@Concurrency@@QBEXV?$function@$$A6AXXZ@std@@_N@Z");
        SET(p__ContextCallback__Capture, "?_Capture@_ContextCallback@details@Concurrency@@AAEXXZ");
        SET(p__ContextCallback__Reset, "?_Reset@_ContextCallback@details@Concurrency@@AAEXXZ");
        SET(p__TaskEventLogger__LogCancelTask, "?_LogCancelTask@_TaskEventLogger@details@Concurrency@@QAEXXZ");
        SET(p__TaskEventLogger__LogScheduleTask, "?_LogScheduleTask@_TaskEventLogger@details@Concurrency@@QAEX_N@Z");
        SET(p__TaskEventLogger__LogTaskCompleted, "?_LogTaskCompleted@_TaskEventLogger@details@Concurrency@@QAEXXZ");
        SET(p__TaskEventLogger__LogTaskExecutionCompleted, "?_LogTaskExecutionCompleted@_TaskEventLogger@details@Concurrency@@QAEXXZ");
        SET(p__TaskEventLogger__LogWorkItemCompleted, "?_LogWorkItemCompleted@_TaskEventLogger@details@Concurrency@@QAEXXZ");
        SET(p__TaskEventLogger__LogWorkItemStarted, "?_LogWorkItemStarted@_TaskEventLogger@details@Concurrency@@QAEXXZ");
#endif
        SET(p__Schedule_chore, "?_Schedule_chore@details@Concurrency@@YAHPAU_Threadpool_chore@12@@Z");
        SET(p__Reschedule_chore, "?_Reschedule_chore@details@Concurrency@@YAHPBU_Threadpool_chore@12@@Z");
        SET(p__Release_chore, "?_Release_chore@details@Concurrency@@YAXPAU_Threadpool_chore@12@@Z");
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test_thrd(void)
{
    ok(p__Thrd_id() == GetCurrentThreadId(),
        "expected same id, got _Thrd_id %u GetCurrentThreadId %u\n",
        p__Thrd_id(), GetCurrentThreadId());
}

static struct {
    int value[2];
    const char* export_name;
} vbtable_size_exports_list[] = {
    {{0x20, 0x20}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x20, 0x20}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x20, 0x20}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x18, 0x18}, "??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x18, 0x18}, "??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x18, 0x18}, "??_8?$basic_istream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_ostream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{ 0x0,  0x0}, 0}
};

static void test_vbtable_size_exports(void)
{
    int i;
    const int *p_vbtable;
    int arch_idx = (sizeof(void*) == 8);

    for (i = 0; vbtable_size_exports_list[i].export_name; i++)
    {
        SET(p_vbtable, vbtable_size_exports_list[i].export_name);

        ok(p_vbtable[0] == 0, "vbtable[0] wrong, got 0x%x\n", p_vbtable[0]);
        ok(p_vbtable[1] == vbtable_size_exports_list[i].value[arch_idx],
                "%d: %s[1] wrong, got 0x%x\n", i, vbtable_size_exports_list[i].export_name, p_vbtable[1]);
    }
}

static void test_task_continuation_context(void)
{
    task_continuation_context tcc;

    memset(&tcc, 0xdead, sizeof(tcc));
    call_func1(p_task_continuation_context_ctor, &tcc);
    ok(!tcc.unk0, "tcc.unk0 != NULL (%p)\n", tcc.unk0);
    ok(!tcc.unk1, "tcc.unk1 != 0 (%x)\n", tcc.unk1);
}

#ifdef _WIN64
static void __cdecl function_do_call(void *this)
{
    CHECK_EXPECT(function_do_call);
}

static void __cdecl function_do_clean(void *this, MSVCP_bool b)
{
    CHECK_EXPECT(function_do_clean);
    ok(b, "b == FALSE\n");
}
#endif

static void test__ContextCallback(void)
{
    _ContextCallback cc = {0};
    void *v = (void*)0xdeadbeef;
#ifdef _WIN64
    void* function_vtbl[] = {
        NULL,
        NULL,
        (void*)function_do_call,
        NULL,
        (void*)function_do_clean,
        NULL
    };
    function_void_cdecl_void function = { function_vtbl, NULL, {0}, {NULL}, &function };
    function_void_cdecl_void function2 = { NULL, NULL, {0}, {NULL}, &function };
#endif

    call_func2(p__ContextCallback__Assign, &cc, v);
    ok(!cc.unused, "cc.unused = %p\n", cc.unused);
    call_func1(p__ContextCallback__Reset, &cc);
    ok(!cc.unused, "cc.unused = %p\n", cc.unused);
    call_func1(p__ContextCallback__Capture, &cc);
    ok(!cc.unused, "cc.unused = %p\n", cc.unused);
    ok(!p__ContextCallback__IsCurrentOriginSTA(&cc), "IsCurrentOriginSTA returned TRUE\n");

    cc.unused = v;
    call_func2(p__ContextCallback__Assign, &cc, NULL);
    ok(cc.unused == v, "cc.unused = %p\n", cc.unused);
    call_func1(p__ContextCallback__Reset, &cc);
    ok(cc.unused == v, "cc.unused = %p\n", cc.unused);
    call_func1(p__ContextCallback__Capture, &cc);
    ok(cc.unused == v, "cc.unused = %p\n", cc.unused);
    ok(!p__ContextCallback__IsCurrentOriginSTA(&cc), "IsCurrentOriginSTA returned TRUE\n");
    ok(cc.unused == v, "cc.unused = %p\n", cc.unused);

#ifdef _WIN64
    SET_EXPECT(function_do_call);
    SET_EXPECT(function_do_clean);
    p__ContextCallback__CallInContext(&cc, function, FALSE);
    CHECK_CALLED(function_do_call);
    CHECK_CALLED(function_do_clean);

    SET_EXPECT(function_do_call);
    SET_EXPECT(function_do_clean);
    p__ContextCallback__CallInContext(&cc, function, TRUE);
    CHECK_CALLED(function_do_call);
    CHECK_CALLED(function_do_clean);

    SET_EXPECT(function_do_call);
    SET_EXPECT(function_do_clean);
    p__ContextCallback__CallInContext(&cc, function2, FALSE);
    CHECK_CALLED(function_do_call);
    CHECK_CALLED(function_do_clean);

    SET_EXPECT(function_do_call);
    SET_EXPECT(function_do_clean);
    p__ContextCallback__CallInContext(&cc, function2, TRUE);
    CHECK_CALLED(function_do_call);
    CHECK_CALLED(function_do_clean);
#endif
}

static void test__TaskEventLogger(void)
{
    _TaskEventLogger logger;
    memset(&logger, 0, sizeof(logger));

    call_func1(p__TaskEventLogger__LogCancelTask, &logger);
    ok(!logger.task, "logger.task = %p\n", logger.task);
    ok(!logger.scheduled, "logger.scheduled = %x\n", logger.scheduled);
    ok(!logger.started, "logger.started = %x\n", logger.started);

    call_func2(p__TaskEventLogger__LogScheduleTask, &logger, FALSE);
    ok(!logger.task, "logger.task = %p\n", logger.task);
    ok(!logger.scheduled, "logger.scheduled = %x\n", logger.scheduled);
    ok(!logger.started, "logger.started = %x\n", logger.started);

    call_func1(p__TaskEventLogger__LogTaskCompleted, &logger);
    ok(!logger.task, "logger.task = %p\n", logger.task);
    ok(!logger.scheduled, "logger.scheduled = %x\n", logger.scheduled);
    ok(!logger.started, "logger.started = %x\n", logger.started);

    call_func1(p__TaskEventLogger__LogTaskExecutionCompleted, &logger);
    ok(!logger.task, "logger.task = %p\n", logger.task);
    ok(!logger.scheduled, "logger.scheduled = %x\n", logger.scheduled);
    ok(!logger.started, "logger.started = %x\n", logger.started);

    call_func1(p__TaskEventLogger__LogWorkItemCompleted, &logger);
    ok(!logger.task, "logger.task = %p\n", logger.task);
    ok(!logger.scheduled, "logger.scheduled = %x\n", logger.scheduled);
    ok(!logger.started, "logger.started = %x\n", logger.started);

    call_func1(p__TaskEventLogger__LogWorkItemStarted, &logger);
    ok(!logger.task, "logger.task = %p\n", logger.task);
    ok(!logger.scheduled, "logger.scheduled = %x\n", logger.scheduled);
    ok(!logger.started, "logger.started = %x\n", logger.started);

    logger.task = (void*)0xdeadbeef;
    logger.scheduled = TRUE;
    logger.started = TRUE;

    call_func1(p__TaskEventLogger__LogCancelTask, &logger);
    ok(logger.task == (void*)0xdeadbeef, "logger.task = %p\n", logger.task);
    ok(logger.scheduled, "logger.scheduled = FALSE\n");
    ok(logger.started, "logger.started = FALSE\n");

    call_func2(p__TaskEventLogger__LogScheduleTask, &logger, FALSE);
    ok(logger.task == (void*)0xdeadbeef, "logger.task = %p\n", logger.task);
    ok(logger.scheduled, "logger.scheduled = FALSE\n");
    ok(logger.started, "logger.started = FALSE\n");

    call_func1(p__TaskEventLogger__LogTaskCompleted, &logger);
    ok(logger.task == (void*)0xdeadbeef, "logger.task = %p\n", logger.task);
    ok(logger.scheduled, "logger.scheduled = FALSE\n");
    ok(logger.started, "logger.started = FALSE\n");

    call_func1(p__TaskEventLogger__LogTaskExecutionCompleted, &logger);
    ok(logger.task == (void*)0xdeadbeef, "logger.task = %p\n", logger.task);
    ok(logger.scheduled, "logger.scheduled = FALSE\n");
    ok(logger.started, "logger.started = FALSE\n");

    call_func1(p__TaskEventLogger__LogWorkItemCompleted, &logger);
    ok(logger.task == (void*)0xdeadbeef, "logger.task = %p\n", logger.task);
    ok(logger.scheduled, "logger.scheduled = FALSE\n");
    ok(logger.started, "logger.started = FALSE\n");

    call_func1(p__TaskEventLogger__LogWorkItemStarted, &logger);
    ok(logger.task == (void*)0xdeadbeef, "logger.task = %p\n", logger.task);
    ok(logger.scheduled, "logger.scheduled = FALSE\n");
    ok(logger.started, "logger.started = FALSE\n");
}

static void __cdecl chore_callback(void *arg)
{
    HANDLE event = arg;
    SetEvent(event);
}

static void test_chore(void)
{
    HANDLE event = CreateEventW(NULL, FALSE, FALSE, NULL);
    _Threadpool_chore chore, old_chore;
    DWORD wait;
    int ret;

    memset(&chore, 0, sizeof(chore));
    ret = p__Schedule_chore(&chore);
    ok(!ret, "_Schedule_chore returned %d\n", ret);
    ok(chore.work != NULL, "chore.work == NULL\n");
    ok(!chore.callback, "chore.callback != NULL\n");
    p__Release_chore(&chore);

    chore.work = NULL;
    chore.callback = chore_callback;
    chore.arg = event;
    ret = p__Schedule_chore(&chore);
    ok(!ret, "_Schedule_chore returned %d\n", ret);
    ok(chore.work != NULL, "chore.work == NULL\n");
    ok(chore.callback == chore_callback, "chore.callback = %p, expected %p\n", chore.callback, chore_callback);
    ok(chore.arg == event, "chore.arg = %p, expected %p\n", chore.arg, event);
    wait = WaitForSingleObject(event, 500);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %d\n", wait);

    old_chore = chore;
    ret = p__Schedule_chore(&chore);
    ok(!ret, "_Schedule_chore returned %d\n", ret);
    ok(old_chore.work != chore.work, "new threadpool work was not created\n");
    p__Release_chore(&old_chore);
    wait = WaitForSingleObject(event, 500);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %d\n", wait);

    ret = p__Reschedule_chore(&chore);
    ok(!ret, "_Reschedule_chore returned %d\n", ret);
    wait = WaitForSingleObject(event, 500);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %d\n", wait);

    p__Release_chore(&chore);
    ok(!chore.work, "chore.work != NULL\n");
    ok(chore.callback == chore_callback, "chore.callback = %p, expected %p\n", chore.callback, chore_callback);
    ok(chore.arg == event, "chore.arg = %p, expected %p\n", chore.arg, event);
    p__Release_chore(&chore);
}

START_TEST(msvcp140)
{
    if(!init()) return;
    test_thrd();
    test_vbtable_size_exports();
    test_task_continuation_context();
    test__ContextCallback();
    test__TaskEventLogger();
    test_chore();
    FreeLibrary(msvcp);
}
