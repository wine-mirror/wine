/*
 * Copyright 2015 Iván Matellanes
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

typedef void (*vtable_ptr)(void);

/* class streambuf */
typedef struct {
    const vtable_ptr *vtable;
    int allocated;
    int unbuffered;
    int unknown;
    char *base;
    char *ebuf;
    char *pbase;
    char *pptr;
    char *epptr;
    char *eback;
    char *gptr;
    char *egptr;
    int do_lock;
    CRITICAL_SECTION lock;
} streambuf;

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

/* streambuf */
static streambuf* (*__thiscall p_streambuf_reserve_ctor)(streambuf*, char*, int);
static streambuf* (*__thiscall p_streambuf_ctor)(streambuf*);
static void (*__thiscall p_streambuf_dtor)(streambuf*);
static int (*__thiscall p_streambuf_allocate)(streambuf*);
static void (*__thiscall p_streambuf_clrclock)(streambuf*);
static int (*__thiscall p_streambuf_doallocate)(streambuf*);
static void (*__thiscall p_streambuf_lock)(streambuf*);
static void (*__thiscall p_streambuf_setb)(streambuf*, char*, char*, int);
static void (*__thiscall p_streambuf_setlock)(streambuf*);
static streambuf* (*__thiscall p_streambuf_setbuf)(streambuf*, char*, int);
static void (*__thiscall p_streambuf_unlock)(streambuf*);

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
static void * (WINAPI *call_thiscall_func3)( void *func, void *this, const void *a, const void *b );
static void * (WINAPI *call_thiscall_func4)( void *func, void *this, const void *a, const void *b,
        const void *c );
static void * (WINAPI *call_thiscall_func5)( void *func, void *this, const void *a, const void *b,
        const void *c, const void *d );

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
    call_thiscall_func4 = (void *)thunk;
    call_thiscall_func5 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))
#define call_func4(func,_this,a,b,c) call_thiscall_func4(func,_this,(const void*)(a),(const void*)(b), \
        (const void*)(c))
#define call_func5(func,_this,a,b,c,d) call_thiscall_func5(func,_this,(const void*)(a),(const void*)(b), \
        (const void*)(c), (const void *)(d))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func4(func,_this,a,b,c) func(_this,a,b,c)
#define call_func5(func,_this,a,b,c,d) func(_this,a,b,c,d)

#endif /* __i386__ */

static HMODULE msvcirt;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcirt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcirt = LoadLibraryA("msvcirt.dll");
    if(!msvcirt) {
        win_skip("msvcirt.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_streambuf_reserve_ctor, "??0streambuf@@IEAA@PEADH@Z");
        SET(p_streambuf_ctor, "??0streambuf@@IEAA@XZ");
        SET(p_streambuf_dtor, "??1streambuf@@UEAA@XZ");
        SET(p_streambuf_allocate, "?allocate@streambuf@@IEAAHXZ");
        SET(p_streambuf_clrclock, "?clrlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_doallocate, "?doallocate@streambuf@@MEAAHXZ");
        SET(p_streambuf_lock, "?lock@streambuf@@QEAAXXZ");
        SET(p_streambuf_setb, "?setb@streambuf@@IEAAXPEAD0H@Z");
        SET(p_streambuf_setbuf, "?setbuf@streambuf@@UEAAPEAV1@PEADH@Z");
        SET(p_streambuf_setlock, "?setlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_unlock, "?unlock@streambuf@@QEAAXXZ");
    } else {
        SET(p_streambuf_reserve_ctor, "??0streambuf@@IAE@PADH@Z");
        SET(p_streambuf_ctor, "??0streambuf@@IAE@XZ");
        SET(p_streambuf_dtor, "??1streambuf@@UAE@XZ");
        SET(p_streambuf_allocate, "?allocate@streambuf@@IAEHXZ");
        SET(p_streambuf_clrclock, "?clrlock@streambuf@@QAEXXZ");
        SET(p_streambuf_doallocate, "?doallocate@streambuf@@MAEHXZ");
        SET(p_streambuf_lock, "?lock@streambuf@@QAEXXZ");
        SET(p_streambuf_setb, "?setb@streambuf@@IAEXPAD0H@Z");
        SET(p_streambuf_setbuf, "?setbuf@streambuf@@UAEPAV1@PADH@Z");
        SET(p_streambuf_setlock, "?setlock@streambuf@@QAEXXZ");
        SET(p_streambuf_unlock, "?unlock@streambuf@@QAEXXZ");
    }

    init_thiscall_thunk();
    return TRUE;
}

struct streambuf_lock_arg
{
    streambuf *sb;
    HANDLE lock[4];
    HANDLE test[4];
};

static DWORD WINAPI lock_streambuf(void *arg)
{
    struct streambuf_lock_arg *lock_arg = arg;
    call_func1(p_streambuf_lock, lock_arg->sb);
    SetEvent(lock_arg->lock[0]);
    WaitForSingleObject(lock_arg->test[0], INFINITE);
    call_func1(p_streambuf_lock, lock_arg->sb);
    SetEvent(lock_arg->lock[1]);
    WaitForSingleObject(lock_arg->test[1], INFINITE);
    call_func1(p_streambuf_lock, lock_arg->sb);
    SetEvent(lock_arg->lock[2]);
    WaitForSingleObject(lock_arg->test[2], INFINITE);
    call_func1(p_streambuf_unlock, lock_arg->sb);
    SetEvent(lock_arg->lock[3]);
    WaitForSingleObject(lock_arg->test[3], INFINITE);
    call_func1(p_streambuf_unlock, lock_arg->sb);
    return 0;
}

static void test_streambuf(void)
{
    streambuf sb, sb2, *psb;
    struct streambuf_lock_arg lock_arg;
    HANDLE thread;
    char reserve[16];
    int ret, i;
    BOOL locked;

    memset(&sb, 0xab, sizeof(streambuf));
    memset(&sb2, 0xab, sizeof(streambuf));

    /* constructors */
    call_func1(p_streambuf_ctor, &sb);
    ok(sb.allocated == 0, "wrong allocate value, expected 0 got %d\n", sb.allocated);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb.base);
    ok(sb.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb.ebuf);
    ok(sb.lock.LockCount == -1, "wrong critical section state, expected -1 got %d\n", sb.lock.LockCount);
    call_func3(p_streambuf_reserve_ctor, &sb2, reserve, 16);
    ok(sb2.allocated == 0, "wrong allocate value, expected 0 got %d\n", sb2.allocated);
    ok(sb2.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb2.unbuffered);
    ok(sb2.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb2.base);
    ok(sb2.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb2.ebuf);
    ok(sb.lock.LockCount == -1, "wrong critical section state, expected -1 got %d\n", sb.lock.LockCount);

    /* setlock */
    ok(sb.do_lock == -1, "expected do_lock value -1, got %d\n", sb.do_lock);
    call_func1(p_streambuf_setlock, &sb);
    ok(sb.do_lock == -2, "expected do_lock value -2, got %d\n", sb.do_lock);
    call_func1(p_streambuf_setlock, &sb);
    ok(sb.do_lock == -3, "expected do_lock value -3, got %d\n", sb.do_lock);
    sb.do_lock = 3;
    call_func1(p_streambuf_setlock, &sb);
    ok(sb.do_lock == 2, "expected do_lock value 2, got %d\n", sb.do_lock);

    /* clrlock */
    sb.do_lock = -2;
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == -1, "expected do_lock value -1, got %d\n", sb.do_lock);
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == 0, "expected do_lock value 0, got %d\n", sb.do_lock);
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == 1, "expected do_lock value 1, got %d\n", sb.do_lock);
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == 1, "expected do_lock value 1, got %d\n", sb.do_lock);

    /* lock/unlock */
    lock_arg.sb = &sb;
    for (i = 0; i < 4; i++) {
        lock_arg.lock[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
        ok(lock_arg.lock[i] != NULL, "CreateEventW failed\n");
        lock_arg.test[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
        ok(lock_arg.test[i] != NULL, "CreateEventW failed\n");
    }

    sb.do_lock = 0;
    thread = CreateThread(NULL, 0, lock_streambuf, (void*)&lock_arg, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(lock_arg.lock[0], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked != 0, "could not lock the streambuf\n");
    LeaveCriticalSection(&sb.lock);

    sb.do_lock = 1;
    SetEvent(lock_arg.test[0]);
    WaitForSingleObject(lock_arg.lock[1], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked != 0, "could not lock the streambuf\n");
    LeaveCriticalSection(&sb.lock);

    sb.do_lock = -1;
    SetEvent(lock_arg.test[1]);
    WaitForSingleObject(lock_arg.lock[2], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked == 0, "the streambuf was not locked before\n");

    sb.do_lock = 0;
    SetEvent(lock_arg.test[2]);
    WaitForSingleObject(lock_arg.lock[3], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked == 0, "the streambuf was not locked before\n");
    sb.do_lock = -1;

    /* setb */
    call_func4(p_streambuf_setb, &sb, reserve, reserve+16, 0);
    ok(sb.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb.base);
    ok(sb.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb.ebuf);
    call_func4(p_streambuf_setb, &sb, reserve, reserve+16, 4);
    ok(sb.allocated == 4, "wrong allocate value, expected 4 got %d\n", sb.allocated);
    sb.allocated = 0;
    call_func4(p_streambuf_setb, &sb, NULL, NULL, 3);
    ok(sb.allocated == 3, "wrong allocate value, expected 3 got %d\n", sb.allocated);

    /* setbuf */
    psb = call_func3(p_streambuf_setbuf, &sb, NULL, 5);
    ok(psb == &sb, "wrong return value, expected %p got %p\n", &sb, psb);
    ok(sb.allocated == 3, "wrong allocate value, expected 3 got %d\n", sb.allocated);
    ok(sb.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", sb.unbuffered);
    ok(sb.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb.base);
    ok(sb.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb.ebuf);
    psb = call_func3(p_streambuf_setbuf, &sb, reserve, 0);
    ok(psb == &sb, "wrong return value, expected %p got %p\n", &sb, psb);
    ok(sb.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", sb.unbuffered);
    ok(sb.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb.base);
    ok(sb.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb.ebuf);
    psb = call_func3(p_streambuf_setbuf, &sb, reserve, 16);
    ok(psb == &sb, "wrong return value, expected %p got %p\n", &sb, psb);
    ok(sb.allocated == 3, "wrong allocate value, expected 3 got %d\n", sb.allocated);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb.base);
    ok(sb.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb.ebuf);
    psb = call_func3(p_streambuf_setbuf, &sb, NULL, 8);
    ok(psb == NULL, "wrong return value, expected %p got %p\n", NULL, psb);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb.base);
    ok(sb.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb.ebuf);

    /* allocate */
    ret = (int) call_func1(p_streambuf_allocate, &sb);
    ok(ret == 0, "wrong return value, expected 0 got %d\n", ret);
    sb.base = NULL;
    ret = (int) call_func1(p_streambuf_allocate, &sb);
    ok(ret == 1, "wrong return value, expected 1 got %d\n", ret);
    ok(sb.allocated == 1, "wrong allocate value, expected 1 got %d\n", sb.allocated);
    ok(sb.ebuf - sb.base == 512 , "wrong reserve area size, expected 512 got %p-%p\n", sb.ebuf, sb.base);

    /* doallocate */
    ret = (int) call_func1(p_streambuf_doallocate, &sb2);
    ok(ret == 1, "doallocate failed, got %d\n", ret);
    ok(sb2.allocated == 1, "wrong allocate value, expected 1 got %d\n", sb2.allocated);
    ok(sb2.ebuf - sb2.base == 512 , "wrong reserve area size, expected 512 got %p-%p\n", sb2.ebuf, sb2.base);

    SetEvent(lock_arg.test[3]);
    WaitForSingleObject(thread, INFINITE);

    call_func1(p_streambuf_dtor, &sb);
    call_func1(p_streambuf_dtor, &sb2);
    for (i = 0; i < 4; i++) {
        CloseHandle(lock_arg.lock[i]);
        CloseHandle(lock_arg.test[i]);
    }
    CloseHandle(thread);
}

START_TEST(msvcirt)
{
    if(!init())
        return;

    test_streambuf();

    FreeLibrary(msvcirt);
}
