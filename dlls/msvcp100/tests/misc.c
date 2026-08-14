/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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

struct __Container_proxy;

typedef struct {
    struct __Container_proxy *proxy;
} _Container_base12;

typedef struct __Iterator_base12 {
    struct __Container_proxy *proxy;
    struct __Iterator_base12 *next;
} _Iterator_base12;

typedef struct __Container_proxy {
    const _Container_base12 *cont;
    _Iterator_base12 *head;
} _Container_proxy;

typedef struct {
    void *vtable;
    int id;
} _Runtime_object;

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

static _Container_base12* (__thiscall *p__Container_base12_copy_ctor)(_Container_base12*, _Container_base12*);
static _Container_base12* (__thiscall *p__Container_base12_ctor)(_Container_base12*);
static void (__thiscall *p__Container_base12__Orphan_all)(_Container_base12*);
static void (__thiscall *p__Container_base12_dtor)(_Container_base12*);
static _Iterator_base12** (__thiscall *p__Container_base12__Getpfirst)(_Container_base12*);
static void (__thiscall *p__Container_base12__Swap_all)(_Container_base12*, _Container_base12*);
static _Runtime_object* (__thiscall *p__Runtime_object_ctor)(_Runtime_object*);
static _Runtime_object* (__thiscall *p__Runtime_object_ctor_id)(_Runtime_object*, int);

/* Emulate a __thiscall */
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
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp100.dll");
    if(!msvcp) {
        win_skip("msvcp100.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p__Container_base12_copy_ctor, "??0_Container_base12@std@@QEAA@AEBU01@@Z");
        SET(p__Container_base12_ctor, "??0_Container_base12@std@@QEAA@XZ");
        SET(p__Container_base12__Orphan_all, "?_Orphan_all@_Container_base12@std@@QEAAXXZ");
        SET(p__Container_base12_dtor, "??1_Container_base12@std@@QEAA@XZ");
        SET(p__Container_base12__Getpfirst, "?_Getpfirst@_Container_base12@std@@QEBAPEAPEAU_Iterator_base12@2@XZ");
        SET(p__Container_base12__Swap_all, "?_Swap_all@_Container_base12@std@@QEAAXAEAU12@@Z");
        SET(p__Runtime_object_ctor, "??0_Runtime_object@details@Concurrency@@QEAA@XZ");
        SET(p__Runtime_object_ctor_id, "??0_Runtime_object@details@Concurrency@@QEAA@H@Z");
    }else {
#ifdef __arm__
        SET(p__Container_base12_copy_ctor, "??0_Container_base12@std@@QAA@ABU01@@Z");
        SET(p__Container_base12_ctor, "??0_Container_base12@std@@QAA@XZ");
        SET(p__Container_base12__Orphan_all, "?_Orphan_all@_Container_base12@std@@QAAXXZ");
        SET(p__Container_base12_dtor, "??1_Container_base12@std@@QAA@XZ");
        SET(p__Container_base12__Getpfirst, "?_Getpfirst@_Container_base12@std@@QBAPAPAU_Iterator_base12@2@XZ");
        SET(p__Container_base12__Swap_all, "?_Swap_all@_Container_base12@std@@QAAXAAU12@@Z");
        SET(p__Runtime_object_ctor, "??0_Runtime_object@details@Concurrency@@QAA@XZ");
        SET(p__Runtime_object_ctor_id, "??0_Runtime_object@details@Concurrency@@QAA@H@Z");
#else
        SET(p__Container_base12_copy_ctor, "??0_Container_base12@std@@QAE@ABU01@@Z");
        SET(p__Container_base12_ctor, "??0_Container_base12@std@@QAE@XZ");
        SET(p__Container_base12__Orphan_all, "?_Orphan_all@_Container_base12@std@@QAEXXZ");
        SET(p__Container_base12_dtor, "??1_Container_base12@std@@QAE@XZ");
        SET(p__Container_base12__Getpfirst, "?_Getpfirst@_Container_base12@std@@QBEPAPAU_Iterator_base12@2@XZ");
        SET(p__Container_base12__Swap_all, "?_Swap_all@_Container_base12@std@@QAEXAAU12@@Z");
        SET(p__Runtime_object_ctor, "??0_Runtime_object@details@Concurrency@@QAE@XZ");
        SET(p__Runtime_object_ctor_id, "??0_Runtime_object@details@Concurrency@@QAE@H@Z");
#endif /* __arm__ */
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test__Container_base12(void)
{
    _Container_base12 c1, c2;
    _Iterator_base12 i1, i2, **pi;
    _Container_proxy p1, p2;

    call_func1(p__Container_base12_ctor, &c1);
    ok(c1.proxy == NULL, "c1.proxy != NULL\n");

    p1.cont = NULL;
    p1.head = NULL;
    c1.proxy = &p1;
    call_func2(p__Container_base12_copy_ctor, &c2, &c1);
    ok(c1.proxy == &p1, "c1.proxy = %p, expected %p\n", c1.proxy, &p1);
    ok(c2.proxy == NULL, "c2.proxy != NULL\n");

    p1.head = &i1;
    i1.proxy = &p1;
    i1.next = &i2;
    i2.proxy = &p1;
    i2.next = NULL;
    pi = call_func1(p__Container_base12__Getpfirst, &c1);
    ok(pi == &p1.head, "pi = %p, expected %p\n", pi, &p1.head);
    pi = call_func1(p__Container_base12__Getpfirst, &c2);
    ok(pi == NULL, "pi != NULL\n");

    p1.cont = &c1;
    call_func1(p__Container_base12_dtor, &c1);
    ok(p1.cont == &c1, "p1.cont = %p, expected %p\n", p1.cont, &c1);
    ok(p1.head == &i1, "p1.head = %p, expected %p\n", p1.head, &i1);
    ok(i1.proxy == &p1, "i1.proxy = %p, expected %p\n", i1.proxy, &p1);
    ok(i1.next == &i2, "i1.next = %p, expected %p\n", i1.next, &i2);
    ok(i2.proxy == &p1, "i2.proxy = %p, expected %p\n", i2.proxy, &p1);
    ok(i2.next == NULL, "i2.next != NULL\n");

    call_func1(p__Container_base12__Orphan_all, &c1);
    ok(p1.cont == &c1, "p1.cont = %p, expected %p\n", p1.cont, &c1);
    ok(p1.head == &i1, "p1.head = %p, expected %p\n", p1.head, &i1);
    ok(i1.proxy == &p1, "i1.proxy = %p, expected %p\n", i1.proxy, &p1);
    ok(i1.next == &i2, "i1.next = %p, expected %p\n", i1.next, &i2);
    ok(i2.proxy == &p1, "i2.proxy = %p, expected %p\n", i2.proxy, &p1);
    ok(i2.next == NULL, "i2.next != NULL\n");

    c2.proxy = NULL;
    call_func2(p__Container_base12__Swap_all, &c1, &c2);
    ok(c1.proxy == NULL, "c1.proxy != NULL\n");
    ok(c2.proxy == &p1, "c2.proxy = %p, expected %p\n", c2.proxy, &p2);
    ok(p1.cont == &c2, "p1.cont = %p, expected %p\n", p1.cont, &c2);

    c1.proxy = &p2;
    p2.cont = (void*)0xdeadbeef;
    p2.head = (void*)0xdeadbeef;
    p1.head = &i1;
    call_func2(p__Container_base12__Swap_all, &c1, &c2);
    ok(c1.proxy == &p1, "c1.proxt = %p, expected %p\n", c1.proxy, &p1);
    ok(p1.cont == &c1, "p1.cont = %p, expected %p\n", p1.cont, &c1);
    ok(p1.head == &i1, "p1.head = %p, expected %p\n", p1.head, &i1);
    ok(c2.proxy == &p2, "c2.proxy = %p, expected %p\n", c2.proxy, &p2);
    ok(p2.cont == &c2, "p2.cont = %p, expected %p\n", p2.cont, &c2);
    ok(p2.head == (void*)0xdeadbeef, "p2.head = %p, expected 0xdeadbeef\n", p2.head);
}

static struct {
    int value[2];
    const char* export_name;
} vbtable_size_exports_list[] = {
    {{0x18, 0x18}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{ 0x8,  0x8}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x18, 0x18}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{ 0x8,  0x8}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x18, 0x18}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@"},
    {{ 0x8,  0x8}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x10, 0x10}, "??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x10, 0x10}, "??_8?$basic_istream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{ 0x8,  0x8}, "??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x8,  0x8}, "??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0x8,  0x8}, "??_8?$basic_ostream@_WU?$char_traits@_W@std@@@std@@7B@"},
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

static void test__Runtime_object(void)
{
    _Runtime_object ro;
    memset(&ro, 0, sizeof(ro));

    call_func1(p__Runtime_object_ctor, &ro);
    ok(ro.id == 0, "ro.id = %d\n", ro.id);
    call_func1(p__Runtime_object_ctor, &ro);
    ok(ro.id == 2, "ro.id = %d\n", ro.id);
    call_func1(p__Runtime_object_ctor, &ro);
    ok(ro.id == 4, "ro.id = %d\n", ro.id);
    call_func2(p__Runtime_object_ctor_id, &ro, 0);
    ok(ro.id == 0, "ro.id = %d\n", ro.id);
    call_func2(p__Runtime_object_ctor_id, &ro, 1);
    ok(ro.id == 1, "ro.id = %d\n", ro.id);
    call_func2(p__Runtime_object_ctor_id, &ro, 10);
    ok(ro.id == 10, "ro.id = %d\n", ro.id);
    call_func1(p__Runtime_object_ctor, &ro);
    ok(ro.id == 6, "ro.id = %d\n", ro.id);
}

START_TEST(misc)
{
    if(!init())
        return;

    test__Container_base12();
    test_vbtable_size_exports();
    test__Runtime_object();

    FreeLibrary(msvcp);
}
