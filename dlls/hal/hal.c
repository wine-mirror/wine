/*
 * hal.dll implementation
 *
 * Copyright (C) 2007 Chris Wulff
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "excpt.h"
#include "ddk/ntddk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);

#ifdef __i386__
#define DEFINE_FASTCALL1_ENTRYPOINT( name ) \
    __ASM_STDCALL_FUNC( name, 4, \
                       "popl %eax\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name __ASM_STDCALL(4))
#define DEFINE_FASTCALL2_ENTRYPOINT( name ) \
    __ASM_STDCALL_FUNC( name, 8, \
                       "popl %eax\n\t" \
                       "pushl %edx\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name __ASM_STDCALL(8))

extern void * WINAPI call_fastcall_func1( void *func, const void *a );
__ASM_STDCALL_FUNC( call_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax" );

extern void * WINAPI call_fastcall_func2( void *func, const void *a, const void *b );
__ASM_STDCALL_FUNC( call_fastcall_func2, 12,
                   "popl %edx\n\t"
                   "popl %eax\n\t"
                   "popl %ecx\n\t"
                   "xchgl (%esp),%edx\n\t"
                   "jmp *%eax" );

extern void WINAPI ExAcquireFastMutexUnsafe( FAST_MUTEX * );

DEFINE_FASTCALL1_ENTRYPOINT( ExAcquireFastMutex )
void WINAPI DECLSPEC_HIDDEN __regs_ExAcquireFastMutex( FAST_MUTEX *mutex )
{
    call_fastcall_func1( ExAcquireFastMutexUnsafe, mutex );
}

extern void WINAPI ExReleaseFastMutexUnsafe( FAST_MUTEX * );

DEFINE_FASTCALL1_ENTRYPOINT( ExReleaseFastMutex )
void WINAPI DECLSPEC_HIDDEN __regs_ExReleaseFastMutex( FAST_MUTEX *mutex )
{
    call_fastcall_func1( ExReleaseFastMutexUnsafe, mutex );
}

DEFINE_FASTCALL1_ENTRYPOINT( ExTryToAcquireFastMutex )
BOOLEAN WINAPI DECLSPEC_HIDDEN __regs_ExTryToAcquireFastMutex( FAST_MUTEX *mutex )
{
    TRACE("mutex %p.\n", mutex);

    return (InterlockedCompareExchange( &mutex->Count, 0, 1 ) == 1);
}

DEFINE_FASTCALL1_ENTRYPOINT( KfAcquireSpinLock )
KIRQL WINAPI DECLSPEC_HIDDEN __regs_KfAcquireSpinLock( KSPIN_LOCK *lock )
{
    KIRQL irql;
    KeAcquireSpinLock( lock, &irql );
    return irql;
}

void WINAPI KeAcquireSpinLock( KSPIN_LOCK *lock, KIRQL *irql )
{
    TRACE("lock %p, irql %p.\n", lock, irql);
    KeAcquireSpinLockAtDpcLevel( lock );
    *irql = 0;
}

DEFINE_FASTCALL2_ENTRYPOINT( KfReleaseSpinLock )
void WINAPI DECLSPEC_HIDDEN __regs_KfReleaseSpinLock( KSPIN_LOCK *lock, KIRQL irql )
{
    KeReleaseSpinLock( lock, irql );
}

void WINAPI KeReleaseSpinLock( KSPIN_LOCK *lock, KIRQL irql )
{
    TRACE("lock %p, irql %u.\n", lock, irql);
    KeReleaseSpinLockFromDpcLevel( lock );
}

extern void WINAPI KeAcquireInStackQueuedSpinLockAtDpcLevel( KSPIN_LOCK *, KLOCK_QUEUE_HANDLE * );

DEFINE_FASTCALL2_ENTRYPOINT( KeAcquireInStackQueuedSpinLock )
void WINAPI DECLSPEC_HIDDEN __regs_KeAcquireInStackQueuedSpinLock( KSPIN_LOCK *lock, KLOCK_QUEUE_HANDLE *queue )
{
    call_fastcall_func2( KeAcquireInStackQueuedSpinLockAtDpcLevel, lock, queue );
}

extern void WINAPI KeReleaseInStackQueuedSpinLockFromDpcLevel( KLOCK_QUEUE_HANDLE * );

DEFINE_FASTCALL1_ENTRYPOINT( KeReleaseInStackQueuedSpinLock )
void WINAPI DECLSPEC_HIDDEN __regs_KeReleaseInStackQueuedSpinLock( KLOCK_QUEUE_HANDLE *queue )
{
    call_fastcall_func1( KeReleaseInStackQueuedSpinLockFromDpcLevel, queue );
}
#endif /* __i386__ */

#if defined(__i386__) || defined(__arm__) || defined(__aarch64__)
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( KfLowerIrql )
VOID WINAPI DECLSPEC_HIDDEN __regs_KfLowerIrql(KIRQL NewIrql)
#else
VOID WINAPI KfLowerIrql(KIRQL NewIrql)
#endif
{
    FIXME( "(%u) stub!\n", NewIrql );
}


#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( KfRaiseIrql )
KIRQL WINAPI DECLSPEC_HIDDEN __regs_KfRaiseIrql(KIRQL NewIrql)
#else
KIRQL WINAPI KfRaiseIrql(KIRQL NewIrql)
#endif
{
    FIXME( "(%u) stub!\n", NewIrql );

    return 0;
}

KIRQL WINAPI KeGetCurrentIrql(VOID)
{
    FIXME( " stub!\n");
    return 0;
}

UCHAR WINAPI READ_PORT_UCHAR(UCHAR *port)
{
    FIXME("(%p) stub!\n", port);
    return 0;
}

ULONG WINAPI READ_PORT_ULONG(ULONG *port)
{
    FIXME("(%p) stub!\n", port);
    return 0;
}

void WINAPI WRITE_PORT_UCHAR(UCHAR *port, UCHAR value)
{
    FIXME("(%p %d) stub!\n", port, value);
}

void WINAPI WRITE_PORT_ULONG(ULONG *port, ULONG value)
{
    FIXME("(%p %d) stub!\n", port, value);
}
#endif /* __i386__ || __arm__ || __arm64__ */

ULONG WINAPI HalGetBusData(BUS_DATA_TYPE BusDataType, ULONG BusNumber, ULONG SlotNumber, PVOID Buffer, ULONG Length)
{
    FIXME("(%u %u %u %p %u) stub!\n", BusDataType, BusNumber, SlotNumber, Buffer, Length);
    /* Claim that there is no such bus */
    return 0;
}

ULONG WINAPI HalGetBusDataByOffset(BUS_DATA_TYPE BusDataType, ULONG BusNumber, ULONG SlotNumber, PVOID Buffer, ULONG Offset, ULONG Length)
{
    FIXME("(%u %u %u %p %u %u) stub!\n", BusDataType, BusNumber, SlotNumber, Buffer, Offset, Length);
    /* Claim that there is no such bus */
    return 0;
}

BOOLEAN WINAPI HalTranslateBusAddress(INTERFACE_TYPE InterfaceType, ULONG BusNumber, PHYSICAL_ADDRESS BusAddress,
		                              PULONG AddressSpace, PPHYSICAL_ADDRESS TranslatedAddress)
{
    FIXME("(%d %d %s %p %p) stub!\n", InterfaceType, BusNumber,
		wine_dbgstr_longlong(BusAddress.QuadPart), AddressSpace, TranslatedAddress);
    return FALSE;
}

ULONGLONG WINAPI KeQueryPerformanceCounter(LARGE_INTEGER *frequency)
{
    LARGE_INTEGER counter;

    TRACE("(%p)\n", frequency);

    NtQueryPerformanceCounter(&counter, frequency);
    return counter.QuadPart;
}
