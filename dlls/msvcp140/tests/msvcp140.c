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
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)

#endif /* __i386__ */

typedef struct {
    void *unk0;
    BYTE unk1;
} task_continuation_context;

static unsigned int (__cdecl *p__Thrd_id)(void);
static task_continuation_context* (__thiscall *p_task_continuation_context_ctor)(task_continuation_context*);

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

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_task_continuation_context_ctor, "??0task_continuation_context@Concurrency@@AEAA@XZ");
    } else {
#ifdef __arm__
        SET(p_task_continuation_context_ctor, "??0task_continuation_context@Concurrency@@AAA@XZ");
#else
        SET(p_task_continuation_context_ctor, "??0task_continuation_context@Concurrency@@AAE@XZ");
#endif
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

START_TEST(msvcp140)
{
    if(!init()) return;
    test_thrd();
    test_vbtable_size_exports();
    test_task_continuation_context();
    FreeLibrary(msvcp);
}
