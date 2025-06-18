/*
 * Copyright 2020 Daniel Lehman
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

#include "wine/test.h"

#ifdef __i386__
#pragma pack(push,1)
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#pragma pack(pop)

static ULONG_PTR (WINAPI *call_thiscall_func1)( void *func, void *this );
static ULONG_PTR (WINAPI *call_thiscall_func3)( void *func,
        void *this, const void *a, const void *b );

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
    call_thiscall_func3 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func3(func,_this,a,b) func(_this,a,b)

#endif /* __i386__ */

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

typedef void (*vtable_ptr)(void);

typedef struct {
    const vtable_ptr *vtable;
} Context;

typedef struct {
    Context *ctx;
} _Context;

typedef struct {
    const vtable_ptr *vtable;
    void *timer;
    unsigned int elapse;
    unsigned char repeat;
} _Timer;

static Context* (__cdecl *p_Context_CurrentContext)(void);
static _Context* (__cdecl *p__Context__CurrentContext)(_Context*);

static _Timer* (__thiscall *p__Timer_ctor)(_Timer*,unsigned int,unsigned char);
static void (__thiscall *p__Timer_dtor)(_Timer*);
static void (__thiscall *p__Timer__Start)(_Timer*);
static void (__thiscall *p__Timer__Stop)(_Timer*);

static const BYTE *p_byte_reverse_table;

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(module,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("concrt140.dll");
    if (!module)
    {
        win_skip("concrt140.dll not installed\n");
        return FALSE;
    }

    SET(p__Context__CurrentContext,
        "?_CurrentContext@_Context@details@Concurrency@@SA?AV123@XZ");
    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_Context_CurrentContext,
                "?CurrentContext@Context@Concurrency@@SAPEAV12@XZ");
        SET(p__Timer_ctor,
                "??0_Timer@details@Concurrency@@IEAA@I_N@Z");
        SET(p__Timer_dtor,
                "??1_Timer@details@Concurrency@@MEAA@XZ");
        SET(p__Timer__Start,
                "?_Start@_Timer@details@Concurrency@@IEAAXXZ");
        SET(p__Timer__Stop,
                "?_Stop@_Timer@details@Concurrency@@IEAAXXZ");
    } else {
        SET(p_Context_CurrentContext,
                "?CurrentContext@Context@Concurrency@@SAPAV12@XZ");
#ifdef __arm__
        SET(p__Timer_ctor,
                "??0_Timer@details@Concurrency@@IAA@I_N@Z");
        SET(p__Timer_dtor,
                "??1_Timer@details@Concurrency@@MAA@XZ");
        SET(p__Timer__Start,
                "?_Start@_Timer@details@Concurrency@@IAAXXZ");
        SET(p__Timer__Stop,
                "?_Stop@_Timer@details@Concurrency@@IAAXXZ");
#else
        SET(p__Timer_ctor,
                "??0_Timer@details@Concurrency@@IAE@I_N@Z");
        SET(p__Timer_dtor,
                "??1_Timer@details@Concurrency@@MAE@XZ");
        SET(p__Timer__Start,
                "?_Start@_Timer@details@Concurrency@@IAEXXZ");
        SET(p__Timer__Stop,
                "?_Stop@_Timer@details@Concurrency@@IAEXXZ");
#endif
    }

    init_thiscall_thunk();
    SET(p_byte_reverse_table, "?_Byte_reverse_table@details@Concurrency@@3QBEB");
    return TRUE;
}

static void test_CurrentContext(void)
{
    _Context _ctx, *ret;
    Context *ctx;

    ctx = p_Context_CurrentContext();
    ok(!!ctx, "got NULL\n");

    memset(&_ctx, 0xcc, sizeof(_ctx));
    ret = p__Context__CurrentContext(&_ctx);
    ok(_ctx.ctx == ctx, "expected %p, got %p\n", ctx, _ctx.ctx);
    ok(ret == &_ctx, "expected %p, got %p\n", &_ctx, ret);
}

static HANDLE callback_called;
static void __cdecl timer_callback(_Timer *this)
{
    SetEvent(callback_called);
}

static void test_Timer(void)
{
    vtable_ptr vtable[2];
    _Timer timer;
    DWORD ret;

    callback_called = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(callback_called != NULL, "CreateEvent failed\n");

    call_func3(p__Timer_ctor, &timer, 1, TRUE);
    ok(!timer.timer, "timer already set to %p\n", timer.timer);
    ok(timer.elapse == 1, "elapse = %u, expected 0\n", timer.elapse);
    ok(timer.repeat, "timer.repeat = FALSE\n");
    vtable[0] = timer.vtable[0];
    vtable[1] = (vtable_ptr)timer_callback;
    timer.vtable = vtable;
    call_func1(p__Timer__Start, &timer);
    ok(timer.timer != NULL, "timer = NULL\n");
    ret = WaitForSingleObject(callback_called, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", ret);
    ret = WaitForSingleObject(callback_called, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", ret);
    call_func1(p__Timer__Stop, &timer);
    ok(!timer.timer, "timer != NULL\n");
    call_func1(p__Timer_dtor, &timer);
    ResetEvent(callback_called);

    call_func3(p__Timer_ctor, &timer, 1, FALSE);
    timer.vtable = vtable;
    call_func1(p__Timer__Start, &timer);
    ok(timer.timer != NULL, "timer = NULL\n");
    ret = WaitForSingleObject(callback_called, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", ret);
    ret = WaitForSingleObject(callback_called, 100);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", ret);
    call_func1(p__Timer_dtor, &timer);

    call_func3(p__Timer_ctor, &timer, 0, TRUE);
    timer.vtable = vtable;
    call_func1(p__Timer__Start, &timer);
    ok(timer.timer != NULL, "timer = NULL\n");
    ret = WaitForSingleObject(callback_called, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", ret);
    ret = WaitForSingleObject(callback_called, 100);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", ret);
    call_func1(p__Timer__Stop, &timer);
    ok(!timer.timer, "timer != NULL\n");
    call_func1(p__Timer_dtor, &timer);

    CloseHandle(callback_called);
}

static BYTE byte_reverse(BYTE b)
{
    b = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
    b = ((b & 0xcc) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xaa) >> 1) | ((b & 0x55) << 1);
    return b;
}

static void test_data_exports(void)
{
    unsigned int i;

    ok(IsBadWritePtr((BYTE *)p_byte_reverse_table, 256), "byte_reverse_table is writeable.\n");
    for (i = 0; i < 256; ++i)
    {
        ok(p_byte_reverse_table[i] == byte_reverse(i), "Got unexpected byte %#x, expected %#x.\n",
                p_byte_reverse_table[i], byte_reverse(i));
    }
}

START_TEST(concrt140)
{
    if (!init()) return;
    test_CurrentContext();
    test_Timer();
    test_data_exports();
}
