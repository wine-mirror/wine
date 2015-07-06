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
    int stored_char;
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
static void (*__thiscall p_streambuf_gbump)(streambuf*, int);
static void (*__thiscall p_streambuf_lock)(streambuf*);
static int (*__thiscall p_streambuf_pbackfail)(streambuf*, int);
static void (*__thiscall p_streambuf_pbump)(streambuf*, int);
static int (*__thiscall p_streambuf_sbumpc)(streambuf*);
static void (*__thiscall p_streambuf_setb)(streambuf*, char*, char*, int);
static void (*__thiscall p_streambuf_setlock)(streambuf*);
static streambuf* (*__thiscall p_streambuf_setbuf)(streambuf*, char*, int);
static int (*__thiscall p_streambuf_sgetc)(streambuf*);
static int (*__thiscall p_streambuf_snextc)(streambuf*);
static int (*__thiscall p_streambuf_sputc)(streambuf*, int);
static void (*__thiscall p_streambuf_stossc)(streambuf*);
static int (*__thiscall p_streambuf_sync)(streambuf*);
static void (*__thiscall p_streambuf_unlock)(streambuf*);
static int (*__thiscall p_streambuf_xsgetn)(streambuf*, char*, int);
static int (*__thiscall p_streambuf_xsputn)(streambuf*, const char*, int);

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
        SET(p_streambuf_gbump, "?gbump@streambuf@@IEAAXH@Z");
        SET(p_streambuf_lock, "?lock@streambuf@@QEAAXXZ");
        SET(p_streambuf_pbackfail, "?pbackfail@streambuf@@UEAAHH@Z");
        SET(p_streambuf_pbump, "?pbump@streambuf@@IEAAXH@Z");
        SET(p_streambuf_sbumpc, "?sbumpc@streambuf@@QEAAHXZ");
        SET(p_streambuf_setb, "?setb@streambuf@@IEAAXPEAD0H@Z");
        SET(p_streambuf_setbuf, "?setbuf@streambuf@@UEAAPEAV1@PEADH@Z");
        SET(p_streambuf_setlock, "?setlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_sgetc, "?sgetc@streambuf@@QEAAHXZ");
        SET(p_streambuf_snextc, "?snextc@streambuf@@QEAAHXZ");
        SET(p_streambuf_sputc, "?sputc@streambuf@@QEAAHH@Z");
        SET(p_streambuf_stossc, "?stossc@streambuf@@QEAAXXZ");
        SET(p_streambuf_sync, "?sync@streambuf@@UEAAHXZ");
        SET(p_streambuf_unlock, "?unlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_xsgetn, "?xsgetn@streambuf@@UEAAHPEADH@Z");
        SET(p_streambuf_xsputn, "?xsputn@streambuf@@UEAAHPEBDH@Z");
    } else {
        SET(p_streambuf_reserve_ctor, "??0streambuf@@IAE@PADH@Z");
        SET(p_streambuf_ctor, "??0streambuf@@IAE@XZ");
        SET(p_streambuf_dtor, "??1streambuf@@UAE@XZ");
        SET(p_streambuf_allocate, "?allocate@streambuf@@IAEHXZ");
        SET(p_streambuf_clrclock, "?clrlock@streambuf@@QAEXXZ");
        SET(p_streambuf_doallocate, "?doallocate@streambuf@@MAEHXZ");
        SET(p_streambuf_gbump, "?gbump@streambuf@@IAEXH@Z");
        SET(p_streambuf_lock, "?lock@streambuf@@QAEXXZ");
        SET(p_streambuf_pbackfail, "?pbackfail@streambuf@@UAEHH@Z");
        SET(p_streambuf_pbump, "?pbump@streambuf@@IAEXH@Z");
        SET(p_streambuf_sbumpc, "?sbumpc@streambuf@@QAEHXZ");
        SET(p_streambuf_setb, "?setb@streambuf@@IAEXPAD0H@Z");
        SET(p_streambuf_setbuf, "?setbuf@streambuf@@UAEPAV1@PADH@Z");
        SET(p_streambuf_setlock, "?setlock@streambuf@@QAEXXZ");
        SET(p_streambuf_sgetc, "?sgetc@streambuf@@QAEHXZ");
        SET(p_streambuf_snextc, "?snextc@streambuf@@QAEHXZ");
        SET(p_streambuf_sputc, "?sputc@streambuf@@QAEHH@Z");
        SET(p_streambuf_stossc, "?stossc@streambuf@@QAEXXZ");
        SET(p_streambuf_sync, "?sync@streambuf@@UAEHXZ");
        SET(p_streambuf_unlock, "?unlock@streambuf@@QAEXXZ");
        SET(p_streambuf_xsgetn, "?xsgetn@streambuf@@UAEHPADH@Z");
        SET(p_streambuf_xsputn, "?xsputn@streambuf@@UAEHPBDH@Z");
    }

    init_thiscall_thunk();
    return TRUE;
}

static int overflow_count, underflow_count;
static streambuf *test_this;
static char test_get_buffer[24];
static int buffer_pos, get_end;

#ifdef __i386__
static int __thiscall test_streambuf_overflow(int ch)
#else
static int __thiscall test_streambuf_overflow(streambuf *this, int ch)
#endif
{
    overflow_count++;
    if (ch == 'L') /* simulate a failure */
        return EOF;
    if (!test_this->unbuffered)
        test_this->pptr = test_this->pbase + 5;
    return ch;
}

#ifdef __i386__
static int __thiscall test_streambuf_underflow(void)
#else
static int __thiscall test_streambuf_underflow(streambuf *this)
#endif
{
    underflow_count++;
    if (test_this->unbuffered) {
        return (buffer_pos < 23) ? test_get_buffer[buffer_pos++] : EOF;
    } else if (test_this->gptr < test_this->egptr) {
        return *test_this->gptr;
    } else {
        return get_end ? EOF : *(test_this->gptr = test_this->eback);
    }
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
    streambuf sb, sb2, sb3, *psb;
    vtable_ptr test_streambuf_vtbl[11];
    struct streambuf_lock_arg lock_arg;
    HANDLE thread;
    char reserve[16];
    int ret, i;
    BOOL locked;

    memset(&sb, 0xab, sizeof(streambuf));
    memset(&sb2, 0xab, sizeof(streambuf));
    memset(&sb3, 0xab, sizeof(streambuf));

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
    call_func1(p_streambuf_ctor, &sb3);
    ok(sb3.allocated == 0, "wrong allocate value, expected 0 got %d\n", sb3.allocated);
    ok(sb3.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb3.unbuffered);
    ok(sb3.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb3.base);
    ok(sb3.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb3.ebuf);

    memcpy(test_streambuf_vtbl, sb.vtable, sizeof(test_streambuf_vtbl));
    test_streambuf_vtbl[7] = (vtable_ptr)&test_streambuf_overflow;
    test_streambuf_vtbl[8] = (vtable_ptr)&test_streambuf_underflow;
    sb2.vtable = test_streambuf_vtbl;
    sb3.vtable = test_streambuf_vtbl;
    overflow_count = underflow_count = 0;
    strcpy(test_get_buffer, "CompuGlobalHyperMegaNet");
    buffer_pos = get_end = 0;

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
    ret = (int) call_func1(p_streambuf_doallocate, &sb3);
    ok(ret == 1, "doallocate failed, got %d\n", ret);
    ok(sb3.allocated == 1, "wrong allocate value, expected 1 got %d\n", sb3.allocated);
    ok(sb3.ebuf - sb3.base == 512 , "wrong reserve area size, expected 512 got %p-%p\n", sb3.ebuf, sb3.base);

    /* sb: buffered, space available */
    sb.eback = sb.gptr = sb.base;
    sb.egptr = sb.base + 256;
    sb.pbase = sb.pptr = sb.base + 256;
    sb.epptr = sb.base + 512;
    /* sb2: buffered, no space available */
    sb2.eback = sb2.base;
    sb2.gptr = sb2.egptr = sb2.base + 256;
    sb2.pbase = sb2.base + 256;
    sb2.pptr = sb2.epptr = sb2.base + 512;
    /* sb3: unbuffered */
    sb3.unbuffered = 1;

    /* gbump */
    call_func2(p_streambuf_gbump, &sb, 10);
    ok(sb.gptr == sb.eback + 10, "advance get pointer failed, expected %p got %p\n", sb.eback + 10, sb.gptr);
    call_func2(p_streambuf_gbump, &sb, -15);
    ok(sb.gptr == sb.eback - 5, "advance get pointer failed, expected %p got %p\n", sb.eback - 5, sb.gptr);
    sb.gptr = sb.eback;

    /* pbump */
    call_func2(p_streambuf_pbump, &sb, -2);
    ok(sb.pptr == sb.pbase - 2, "advance put pointer failed, expected %p got %p\n", sb.pbase - 2, sb.pptr);
    call_func2(p_streambuf_pbump, &sb, 20);
    ok(sb.pptr == sb.pbase + 18, "advance put pointer failed, expected %p got %p\n", sb.pbase + 18, sb.pptr);
    sb.pptr = sb.pbase;

    /* sync */
    ret = (int) call_func1(p_streambuf_sync, &sb);
    ok(ret == EOF, "sync failed, expected EOF got %d\n", ret);
    sb.gptr = sb.egptr;
    ret = (int) call_func1(p_streambuf_sync, &sb);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb.gptr = sb.egptr + 1;
    ret = (int) call_func1(p_streambuf_sync, &sb);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb.gptr = sb.eback;
    ret = (int) call_func1(p_streambuf_sync, &sb2);
    ok(ret == EOF, "sync failed, expected EOF got %d\n", ret);
    sb2.pptr = sb2.pbase;
    ret = (int) call_func1(p_streambuf_sync, &sb2);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb2.pptr = sb2.pbase - 1;
    ret = (int) call_func1(p_streambuf_sync, &sb2);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb2.pptr = sb2.epptr;
    ret = (int) call_func1(p_streambuf_sync, &sb3);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);

    /* sgetc */
    strcpy(sb2.eback, "WorstTestEver");
    test_this = &sb2;
    ret = (int) call_func1(p_streambuf_sgetc, &sb2);
    ok(ret == 'W', "expected 'W' got '%c'\n", ret);
    ok(underflow_count == 1, "expected call to underflow\n");
    ok(sb2.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb2.stored_char);
    sb2.gptr++;
    ret = (int) call_func1(p_streambuf_sgetc, &sb2);
    ok(ret == 'o', "expected 'o' got '%c'\n", ret);
    ok(underflow_count == 2, "expected call to underflow\n");
    ok(sb2.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb2.stored_char);
    sb2.gptr = sb2.egptr;
    test_this = &sb3;
    ret = (int) call_func1(p_streambuf_sgetc, &sb3);
    ok(ret == 'C', "expected 'C' got '%c'\n", ret);
    ok(underflow_count == 3, "expected call to underflow\n");
    ok(sb3.stored_char == 'C', "wrong stored character, expected 'C' got %c\n", sb3.stored_char);
    sb3.stored_char = 'b';
    ret = (int) call_func1(p_streambuf_sgetc, &sb3);
    ok(ret == 'b', "expected 'b' got '%c'\n", ret);
    ok(underflow_count == 3, "no call to underflow expected\n");
    ok(sb3.stored_char == 'b', "wrong stored character, expected 'b' got %c\n", sb3.stored_char);

    /* sputc */
    *sb.pbase = 'a';
    ret = (int) call_func2(p_streambuf_sputc, &sb, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 0, "no call to overflow expected\n");
    ok(*sb.pbase == 'c', "expected 'c' in the put area, got %c\n", *sb.pbase);
    ok(sb.pptr == sb.pbase + 1, "wrong put pointer, expected %p got %p\n", sb.pbase + 1, sb.pptr);
    test_this = &sb2;
    ret = (int) call_func2(p_streambuf_sputc, &sb2, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 1, "expected call to overflow\n");
    ok(sb2.pptr == sb2.pbase + 5, "wrong put pointer, expected %p got %p\n", sb2.pbase + 5, sb2.pptr);
    test_this = &sb3;
    ret = (int) call_func2(p_streambuf_sputc, &sb3, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 2, "expected call to overflow\n");
    sb3.pbase = sb3.pptr = sb3.base;
    sb3.epptr = sb3.ebuf;
    ret = (int) call_func2(p_streambuf_sputc, &sb3, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 2, "no call to overflow expected\n");
    ok(*sb3.pbase == 'c', "expected 'c' in the put area, got %c\n", *sb3.pbase);
    sb3.pbase = sb3.pptr = sb3.epptr = NULL;

    /* xsgetn */
    sb2.gptr = sb2.egptr = sb2.eback + 13;
    test_this = &sb2;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb2, reserve, 5);
    ok(ret == 5, "wrong return value, expected 5 got %d\n", ret);
    ok(!strncmp(reserve, "Worst", 5), "expected 'Worst' got %s\n", reserve);
    ok(sb2.gptr == sb2.eback + 5, "wrong get pointer, expected %p got %p\n", sb2.eback + 5, sb2.gptr);
    ok(underflow_count == 4, "expected call to underflow\n");
    ret = (int) call_func3(p_streambuf_xsgetn, &sb2, reserve, 4);
    ok(ret == 4, "wrong return value, expected 4 got %d\n", ret);
    ok(!strncmp(reserve, "Test", 4), "expected 'Test' got %s\n", reserve);
    ok(sb2.gptr == sb2.eback + 9, "wrong get pointer, expected %p got %p\n", sb2.eback + 9, sb2.gptr);
    ok(underflow_count == 5, "expected call to underflow\n");
    get_end = 1;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb2, reserve, 16);
    ok(ret == 4, "wrong return value, expected 4 got %d\n", ret);
    ok(!strncmp(reserve, "Ever", 4), "expected 'Ever' got %s\n", reserve);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 7, "expected 2 calls to underflow, got %d\n", underflow_count - 5);
    test_this = &sb3;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 11);
    ok(ret == 11, "wrong return value, expected 11 got %d\n", ret);
    ok(!strncmp(reserve, "bompuGlobal", 11), "expected 'bompuGlobal' got %s\n", reserve);
    ok(sb3.stored_char == 'H', "wrong stored character, expected 'H' got %c\n", sb3.stored_char);
    ok(underflow_count == 18, "expected 11 calls to underflow, got %d\n", underflow_count - 7);
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 16);
    ok(ret == 12, "wrong return value, expected 12 got %d\n", ret);
    ok(!strncmp(reserve, "HyperMegaNet", 12), "expected 'HyperMegaNet' got %s\n", reserve);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 30, "expected 12 calls to underflow, got %d\n", underflow_count - 18);
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 3);
    ok(ret == 0, "wrong return value, expected 0 got %d\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 31, "expected call to underflow\n");
    buffer_pos = 0;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 5);
    ok(ret == 5, "wrong return value, expected 5 got %d\n", ret);
    ok(!strncmp(reserve, "Compu", 5), "expected 'Compu' got %s\n", reserve);
    ok(sb3.stored_char == 'G', "wrong stored character, expected 'G' got %c\n", sb3.stored_char);
    ok(underflow_count == 37, "expected 6 calls to underflow, got %d\n", underflow_count - 31);

    /* xsputn */
    ret = (int) call_func3(p_streambuf_xsputn, &sb, "Test\0ing", 8);
    ok(ret == 8, "wrong return value, expected 8 got %d\n", ret);
    ok(sb.pptr == sb.pbase + 9, "wrong put pointer, expected %p got %p\n", sb.pbase + 9, sb.pptr);
    test_this = &sb2;
    sb2.pptr = sb2.epptr - 7;
    ret = (int) call_func3(p_streambuf_xsputn, &sb2, "Testing", 7);
    ok(ret == 7, "wrong return value, expected 7 got %d\n", ret);
    ok(sb2.pptr == sb2.epptr, "wrong put pointer, expected %p got %p\n", sb2.epptr, sb2.pptr);
    ok(overflow_count == 2, "no call to overflow expected\n");
    sb2.pptr = sb2.epptr - 5;
    sb2.pbase[5] = 'a';
    ret = (int) call_func3(p_streambuf_xsputn, &sb2, "Testing", 7);
    ok(ret == 7, "wrong return value, expected 7 got %d\n", ret);
    ok(sb2.pbase[5] == 'g', "expected 'g' got %c\n", sb2.pbase[5]);
    ok(sb2.pptr == sb2.pbase + 6, "wrong put pointer, expected %p got %p\n", sb2.pbase + 6, sb2.pptr);
    ok(overflow_count == 3, "expected call to overflow\n");
    sb2.pptr = sb2.epptr - 4;
    ret = (int) call_func3(p_streambuf_xsputn, &sb2, "TestLing", 8);
    ok(ret == 4, "wrong return value, expected 4 got %d\n", ret);
    ok(sb2.pptr == sb2.epptr, "wrong put pointer, expected %p got %p\n", sb2.epptr, sb2.pptr);
    ok(overflow_count == 4, "expected call to overflow\n");
    test_this = &sb3;
    ret = (int) call_func3(p_streambuf_xsputn, &sb3, "Testing", 7);
    ok(ret == 7, "wrong return value, expected 7 got %d\n", ret);
    ok(sb3.stored_char == 'G', "wrong stored character, expected 'G' got %c\n", sb3.stored_char);
    ok(overflow_count == 11, "expected 7 calls to overflow, got %d\n", overflow_count - 4);
    ret = (int) call_func3(p_streambuf_xsputn, &sb3, "TeLephone", 9);
    ok(ret == 2, "wrong return value, expected 2 got %d\n", ret);
    ok(sb3.stored_char == 'G', "wrong stored character, expected 'G' got %c\n", sb3.stored_char);
    ok(overflow_count == 14, "expected 3 calls to overflow, got %d\n", overflow_count - 11);

    /* snextc */
    strcpy(sb.eback, "Test");
    ret = (int) call_func1(p_streambuf_snextc, &sb);
    ok(ret == 'e', "expected 'e' got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 1, "wrong get pointer, expected %p got %p\n", sb.eback + 1, sb.gptr);
    test_this = &sb2;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr + 1, "wrong get pointer, expected %p got %p\n", sb2.egptr + 1, sb2.gptr);
    ok(underflow_count == 39, "expected 2 calls to underflow, got %d\n", underflow_count - 37);
    sb2.gptr = sb2.egptr - 1;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 40, "expected call to underflow\n");
    get_end = 0;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == 'o', "expected 'o' got '%c'\n", ret);
    ok(sb2.gptr == sb2.eback + 1, "wrong get pointer, expected %p got %p\n", sb2.eback + 1, sb2.gptr);
    ok(underflow_count == 41, "expected call to underflow\n");
    sb2.gptr = sb2.egptr - 1;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == 'W', "expected 'W' got '%c'\n", ret);
    ok(sb2.gptr == sb2.eback, "wrong get pointer, expected %p got %p\n", sb2.eback, sb2.gptr);
    ok(underflow_count == 42, "expected call to underflow\n");
    sb2.gptr = sb2.egptr;
    test_this = &sb3;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 'l', "expected 'l' got '%c'\n", ret);
    ok(sb3.stored_char == 'l', "wrong stored character, expected 'l' got %c\n", sb3.stored_char);
    ok(underflow_count == 43, "expected call to underflow\n");
    buffer_pos = 22;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 't', "expected 't' got '%c'\n", ret);
    ok(sb3.stored_char == 't', "wrong stored character, expected 't' got %c\n", sb3.stored_char);
    ok(underflow_count == 44, "expected call to underflow\n");
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 45, "expected call to underflow\n");
    buffer_pos = 0;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 'o', "expected 'o' got '%c'\n", ret);
    ok(sb3.stored_char == 'o', "wrong stored character, expected 'o' got %c\n", sb3.stored_char);
    ok(underflow_count == 47, "expected 2 calls to underflow, got %d\n", underflow_count - 45);
    sb3.stored_char = EOF;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 'p', "expected 'p' got '%c'\n", ret);
    ok(sb3.stored_char == 'p', "wrong stored character, expected 'p' got %c\n", sb3.stored_char);
    ok(underflow_count == 49, "expected 2 calls to underflow, got %d\n", underflow_count - 47);

    /* sbumpc */
    ret = (int) call_func1(p_streambuf_sbumpc, &sb);
    ok(ret == 'e', "expected 'e' got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 2, "wrong get pointer, expected %p got %p\n", sb.eback + 2, sb.gptr);
    test_this = &sb2;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb2);
    ok(ret == 'W', "expected 'W' got '%c'\n", ret);
    ok(sb2.gptr == sb2.eback + 1, "wrong get pointer, expected %p got %p\n", sb2.eback + 1, sb2.gptr);
    ok(underflow_count == 50, "expected call to underflow\n");
    sb2.gptr = sb2.egptr - 1;
    *sb2.gptr = 't';
    ret = (int) call_func1(p_streambuf_sbumpc, &sb2);
    ok(ret == 't', "expected 't' got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 50, "no call to underflow expected\n");
    get_end = 1;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb2);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr + 1, "wrong get pointer, expected %p got %p\n", sb2.egptr + 1, sb2.gptr);
    ok(underflow_count == 51, "expected call to underflow\n");
    sb2.gptr = sb2.egptr;
    test_this = &sb3;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == 'p', "expected 'p' got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 51, "no call to underflow expected\n");
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == 'u', "expected 'u' got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 52, "expected call to underflow\n");
    buffer_pos = 23;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 53, "expected call to underflow\n");
    buffer_pos = 0;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == 'C', "expected 'C' got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 54, "expected call to underflow\n");

    /* stossc */
    call_func1(p_streambuf_stossc, &sb);
    ok(sb.gptr == sb.eback + 3, "wrong get pointer, expected %p got %p\n", sb.eback + 3, sb.gptr);
    test_this = &sb2;
    call_func1(p_streambuf_stossc, &sb2);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 55, "expected call to underflow\n");
    get_end = 0;
    call_func1(p_streambuf_stossc, &sb2);
    ok(sb2.gptr == sb2.eback + 1, "wrong get pointer, expected %p got %p\n", sb2.eback + 1, sb2.gptr);
    ok(underflow_count == 56, "expected call to underflow\n");
    sb2.gptr = sb2.egptr - 1;
    call_func1(p_streambuf_stossc, &sb2);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 56, "no call to underflow expected\n");
    test_this = &sb3;
    call_func1(p_streambuf_stossc, &sb3);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 57, "expected call to underflow\n");
    sb3.stored_char = 'a';
    call_func1(p_streambuf_stossc, &sb3);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 57, "no call to underflow expected\n");

    /* pbackfail */
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, 'A');
    ok(ret == 'A', "expected 'A' got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 2, "wrong get pointer, expected %p got %p\n", sb.eback + 2, sb.gptr);
    ok(*sb.gptr == 'A', "expected 'A' in the get area, got %c\n", *sb.gptr);
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, EOF);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 1, "wrong get pointer, expected %p got %p\n", sb.eback + 1, sb.gptr);
    ok((signed char)*sb.gptr == EOF, "expected EOF in the get area, got %c\n", *sb.gptr);
    sb.gptr = sb.eback;
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, 'Z');
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb.gptr == sb.eback, "wrong get pointer, expected %p got %p\n", sb.eback, sb.gptr);
    ok(*sb.gptr == 'T', "expected 'T' in the get area, got %c\n", *sb.gptr);
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, EOF);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb.gptr == sb.eback, "wrong get pointer, expected %p got %p\n", sb.eback, sb.gptr);
    ok(*sb.gptr == 'T', "expected 'T' in the get area, got %c\n", *sb.gptr);
    sb2.gptr = sb2.egptr + 1;
    ret = (int) call_func2(p_streambuf_pbackfail, &sb2, 'X');
    ok(ret == 'X', "expected 'X' got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(*sb2.gptr == 'X', "expected 'X' in the get area, got %c\n", *sb2.gptr);

    SetEvent(lock_arg.test[3]);
    WaitForSingleObject(thread, INFINITE);

    call_func1(p_streambuf_dtor, &sb);
    call_func1(p_streambuf_dtor, &sb2);
    call_func1(p_streambuf_dtor, &sb3);
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
