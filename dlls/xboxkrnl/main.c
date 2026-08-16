/*
 * xboxkrnl.exe - original Xbox kernel: real forwards.
 *
 * This file hand-implements the subset of the ~378 xboxkrnl.exe ordinal
 * exports (see xboxkrnl.exe.spec) that have an honest, verifiable, near-1:1
 * mapping onto existing ntdll/kernel32 functionality: memory allocation,
 * critical sections, event/semaphore/mutant synchronisation objects,
 * thread control, string/memory helpers and debug output. Every function's
 * *signature* (name, ordinal, argument list) comes from Cxbx-Reloaded's
 * src/core/kernel/common/*.h XBSYSAPI EXPORTNUM() declarations, cross
 * checked against src/core/kernel/exports/KernelThunk.cpp for the ordinal
 * number - not guessed. Two structural simplifications the real Xbox
 * kernel has relative to desktop NT, both because the original Xbox only
 * ever runs one process, are handled the same way throughout:
 *
 *   - Xbox Nt*() calls that take an explicit HANDLE/PVOID for a *process*
 *     on desktop NT (NtAllocateVirtualMemory, NtFreeVirtualMemory,
 *     NtProtectVirtualMemory, NtQueryVirtualMemory, NtDuplicateObject) omit
 *     that parameter entirely; forwarded here with NtCurrentProcess() filled
 *     in for both source and target where relevant.
 *   - Xbox Nt*() object-creation calls that take an explicit ACCESS_MASK on
 *     desktop NT (NtCreateEvent, NtCreateMutant, NtCreateSemaphore) omit it;
 *     forwarded here with the matching *_ALL_ACCESS constant, since nothing
 *     in a single-process Xbox title needs access-checked handles.
 *
 * Every export not implemented here is a FIXME-logging stub in
 * generated_stubs.c (or, for data/variable exports, placeholder storage in
 * generated_data.c) - see those files' header comments.
 */

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

extern void xbox_sync_init(void);
extern void xbox_sync_fini(void);

/* ntdll exports this (see dlls/ntdll/ntdll.spec) but Wine's own winternl.h
 * doesn't declare the plain unicode-to-unicode overload (only the
 * ...ToAnsiString/...ToOemString/...ToCountedOemString variants) */
NTSYSAPI NTSTATUS WINAPI RtlUpcaseUnicodeString( UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN );
/* likewise the plain STRING (ANSI_STRING) versions of these three aren't
 * declared in winternl.h, only their Unicode counterparts are */
NTSYSAPI LONG WINAPI RtlCompareString( const STRING *, const STRING *, BOOLEAN );
NTSYSAPI void WINAPI RtlCopyString( STRING *, const STRING * );
NTSYSAPI BOOLEAN WINAPI RtlEqualString( const STRING *, const STRING *, BOOLEAN );

/* ---- ordinal 5: DbgBreakPoint ---- */
void WINAPI XBOXKRNL_DbgBreakPoint( void )
{
    DebugBreak();
}

/* ---- ordinal 8: DbgPrint (cdecl, varargs) ---- */
void WINAPIV XBOXKRNL_DbgPrint( const char *format, ... )
{
    char buffer[512];
    va_list args;

    va_start( args, format );
    vsnprintf( buffer, sizeof(buffer), format, args );
    va_end( args );

    TRACE( "%s", buffer );
}

/* ---- ordinal 14: ExAllocatePool ---- */
void * WINAPI XBOXKRNL_ExAllocatePool( SIZE_T NumberOfBytes )
{
    return RtlAllocateHeap( GetProcessHeap(), 0, NumberOfBytes );
}

/* ---- ordinal 15: ExAllocatePoolWithTag ---- */
void * WINAPI XBOXKRNL_ExAllocatePoolWithTag( SIZE_T NumberOfBytes, ULONG Tag )
{
    /* the real Xbox kernel uses Tag for pool debugging/leak tracking only;
     * we have no equivalent pool-tag infrastructure, so it's accepted and
     * ignored, same as the plain ExAllocatePool() forward above */
    return RtlAllocateHeap( GetProcessHeap(), 0, NumberOfBytes );
}

/* ---- ordinal 17: ExFreePool ---- */
void WINAPI XBOXKRNL_ExFreePool( void *P )
{
    RtlFreeHeap( GetProcessHeap(), 0, P );
}

/* ---- ordinals 51-58: fastcall Interlocked / SList primitives.
 * Internal names are prefixed because plain "InterlockedXxx" is a static
 * inline/intrinsic macro in Wine's own winnt.h (so a same-named, differently
 * declared global symbol here would not even compile), and
 * InterlockedFlushSList/PopEntrySList/PushEntrySList are declared __stdcall
 * in winbase.h while the Xbox kernel's versions are __fastcall. */
LONG FASTCALL XBOXKRNL_InterlockedCompareExchange( LONG volatile *Destination, LONG Exchange, LONG Comparand )
{
    return InterlockedCompareExchange( Destination, Exchange, Comparand );
}

LONG FASTCALL XBOXKRNL_InterlockedDecrement( LONG volatile *Addend )
{
    return InterlockedDecrement( Addend );
}

LONG FASTCALL XBOXKRNL_InterlockedIncrement( LONG volatile *Addend )
{
    return InterlockedIncrement( Addend );
}

LONG FASTCALL XBOXKRNL_InterlockedExchange( LONG volatile *Destination, LONG Value )
{
    return InterlockedExchange( Destination, Value );
}

LONG FASTCALL XBOXKRNL_InterlockedExchangeAdd( LONG volatile *Addend, LONG Value )
{
    return InterlockedExchangeAdd( Addend, Value );
}

SINGLE_LIST_ENTRY * FASTCALL XBOXKRNL_InterlockedFlushSList( PSLIST_HEADER ListHead )
{
    return (SINGLE_LIST_ENTRY *)RtlInterlockedFlushSList( ListHead );
}

SLIST_ENTRY * FASTCALL XBOXKRNL_InterlockedPopEntrySList( PSLIST_HEADER ListHead )
{
    return RtlInterlockedPopEntrySList( ListHead );
}

SLIST_ENTRY * FASTCALL XBOXKRNL_InterlockedPushEntrySList( PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry )
{
    return RtlInterlockedPushEntrySList( ListHead, ListEntry );
}

/* ---- ordinal 99: KeDelayExecutionThread. WaitMode (KPROCESSOR_MODE, a
 * single byte on real NT/Xbox) is meaningless outside kernel mode, so it's
 * accepted and dropped, same as the WaitMode param of
 * NtWaitForSingleObjectEx below. ---- */
NTSTATUS WINAPI XBOXKRNL_KeDelayExecutionThread( CHAR WaitMode, BOOLEAN Alertable, LARGE_INTEGER *Interval )
{
    return NtDelayExecution( Alertable, Interval );
}

/* ---- ordinals 126/127: KeQueryPerformanceCounter/Frequency.
 * Unlike Win32 QueryPerformanceCounter(), the Xbox kernel versions return
 * the 64-bit value directly instead of through an out-pointer. */
ULONGLONG WINAPI XBOXKRNL_KeQueryPerformanceCounter( void )
{
    LARGE_INTEGER counter;
    RtlQueryPerformanceCounter( &counter );
    return counter.QuadPart;
}

ULONGLONG WINAPI XBOXKRNL_KeQueryPerformanceFrequency( void )
{
    LARGE_INTEGER freq;
    RtlQueryPerformanceFrequency( &freq );
    return freq.QuadPart;
}

/* ---- ordinal 128: KeQuerySystemTime ---- */
void WINAPI XBOXKRNL_KeQuerySystemTime( LARGE_INTEGER *CurrentTime )
{
    NtQuerySystemTime( CurrentTime );
}

/* ---- ordinal 167: MmAllocateSystemMemory ---- */
void * WINAPI XBOXKRNL_MmAllocateSystemMemory( ULONG NumberOfBytes, ULONG Protect )
{
    /* the original Xbox has a single flat physical/virtual address space;
     * mapping "system memory" onto a VirtualAlloc() in our one process is
     * the closest honest equivalent available in usermode. Protect is
     * passed straight through: Xbox kernel PAGE_* protection constants are
     * the same numeric values as the desktop Win32 ones this was cloned
     * from. */
    return VirtualAlloc( NULL, NumberOfBytes, MEM_COMMIT | MEM_RESERVE, Protect );
}

/* ---- ordinal 172: MmFreeSystemMemory ---- */
ULONG WINAPI XBOXKRNL_MmFreeSystemMemory( void *BaseAddress, ULONG NumberOfBytes )
{
    return VirtualFree( BaseAddress, 0, MEM_RELEASE ) ? NumberOfBytes : 0;
}

/* ---- ordinal 174: MmIsAddressValid ---- */
BOOLEAN WINAPI XBOXKRNL_MmIsAddressValid( void *VirtualAddress )
{
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery( VirtualAddress, &info, sizeof(info) )) return FALSE;
    return info.State == MEM_COMMIT;
}

/* ---- ordinal 180: MmQueryAllocationSize ---- */
ULONG WINAPI XBOXKRNL_MmQueryAllocationSize( void *BaseAddress )
{
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery( BaseAddress, &info, sizeof(info) )) return 0;
    return (ULONG)info.RegionSize;
}

/* ---- ordinal 184: NtAllocateVirtualMemory (no ProcessHandle - Xbox is
 * always "the current process"; AllocationSize is PULONG not PSIZE_T since
 * Xbox is 32-bit-only, so the two are the same width there) ---- */
NTSTATUS WINAPI XBOXKRNL_NtAllocateVirtualMemory( void **BaseAddress, ULONG ZeroBits, ULONG *AllocationSize,
                                                   DWORD AllocationType, DWORD Protect )
{
    SIZE_T size = *AllocationSize;
    NTSTATUS status = NtAllocateVirtualMemory( NtCurrentProcess(), BaseAddress, ZeroBits, &size,
                                                AllocationType, Protect );
    *AllocationSize = (ULONG)size;
    return status;
}

/* ---- ordinal 186: NtClearEvent ---- */
NTSTATUS WINAPI XBOXKRNL_NtClearEvent( HANDLE EventHandle )
{
    return NtClearEvent( EventHandle );
}

/* ---- ordinal 187: NtClose ---- */
NTSTATUS WINAPI XBOXKRNL_NtClose( HANDLE Handle )
{
    return NtClose( Handle );
}

/* ---- ordinal 189: NtCreateEvent (no ACCESS_MASK param) ---- */
NTSTATUS WINAPI XBOXKRNL_NtCreateEvent( HANDLE *EventHandle, OBJECT_ATTRIBUTES *ObjectAttributes,
                                         EVENT_TYPE EventType, BOOLEAN InitialState )
{
    return NtCreateEvent( EventHandle, EVENT_ALL_ACCESS, ObjectAttributes, EventType, InitialState );
}

/* ---- ordinal 192: NtCreateMutant (no ACCESS_MASK param) ---- */
NTSTATUS WINAPI XBOXKRNL_NtCreateMutant( HANDLE *MutantHandle, OBJECT_ATTRIBUTES *ObjectAttributes, BOOLEAN InitialOwner )
{
    return NtCreateMutant( MutantHandle, MUTANT_ALL_ACCESS, ObjectAttributes, InitialOwner );
}

/* ---- ordinal 193: NtCreateSemaphore (no ACCESS_MASK param) ---- */
NTSTATUS WINAPI XBOXKRNL_NtCreateSemaphore( HANDLE *SemaphoreHandle, OBJECT_ATTRIBUTES *ObjectAttributes,
                                             ULONG InitialCount, ULONG MaximumCount )
{
    return NtCreateSemaphore( SemaphoreHandle, SEMAPHORE_ALL_ACCESS, ObjectAttributes, InitialCount, MaximumCount );
}

/* ---- ordinal 197: NtDuplicateObject (source/target are always the current
 * process on Xbox) ---- */
NTSTATUS WINAPI XBOXKRNL_NtDuplicateObject( HANDLE SourceHandle, HANDLE *TargetHandle, DWORD Options )
{
    return NtDuplicateObject( NtCurrentProcess(), SourceHandle, NtCurrentProcess(), TargetHandle,
                               0, FALSE, Options );
}

/* ---- ordinal 199: NtFreeVirtualMemory (no ProcessHandle) ---- */
NTSTATUS WINAPI XBOXKRNL_NtFreeVirtualMemory( void **BaseAddress, ULONG *FreeSize, ULONG FreeType )
{
    SIZE_T size = *FreeSize;
    NTSTATUS status = NtFreeVirtualMemory( NtCurrentProcess(), BaseAddress, &size, FreeType );
    *FreeSize = (ULONG)size;
    return status;
}

/* ---- ordinal 204: NtProtectVirtualMemory (no ProcessHandle) ---- */
NTSTATUS WINAPI XBOXKRNL_NtProtectVirtualMemory( void **BaseAddress, SIZE_T *RegionSize, ULONG NewProtect,
                                                  ULONG *OldProtect )
{
    return NtProtectVirtualMemory( NtCurrentProcess(), BaseAddress, RegionSize, NewProtect, OldProtect );
}

/* ---- ordinal 205: NtPulseEvent ---- */
NTSTATUS WINAPI XBOXKRNL_NtPulseEvent( HANDLE EventHandle, LONG *PreviousState )
{
    return NtPulseEvent( EventHandle, PreviousState );
}

/* ---- ordinal 209: NtQueryEvent ---- */
NTSTATUS WINAPI XBOXKRNL_NtQueryEvent( HANDLE EventHandle, EVENT_BASIC_INFORMATION *EventInformation )
{
    return NtQueryEvent( EventHandle, EventBasicInformation, EventInformation, sizeof(*EventInformation), NULL );
}

/* ---- ordinal 213: NtQueryMutant ---- */
NTSTATUS WINAPI XBOXKRNL_NtQueryMutant( HANDLE MutantHandle, MUTANT_BASIC_INFORMATION *MutantInformation )
{
    return NtQueryMutant( MutantHandle, MutantBasicInformation, MutantInformation, sizeof(*MutantInformation), NULL );
}

/* ---- ordinal 214: NtQuerySemaphore ---- */
NTSTATUS WINAPI XBOXKRNL_NtQuerySemaphore( HANDLE SemaphoreHandle, SEMAPHORE_BASIC_INFORMATION *SemaphoreInformation )
{
    return NtQuerySemaphore( SemaphoreHandle, SemaphoreBasicInformation, SemaphoreInformation,
                              sizeof(*SemaphoreInformation), NULL );
}

/* ---- ordinal 217: NtQueryVirtualMemory (no ProcessHandle, always queries
 * MemoryBasicInformation) ---- */
NTSTATUS WINAPI XBOXKRNL_NtQueryVirtualMemory( void *BaseAddress, MEMORY_BASIC_INFORMATION *Buffer )
{
    return NtQueryVirtualMemory( NtCurrentProcess(), BaseAddress, MemoryBasicInformation,
                                  Buffer, sizeof(*Buffer), NULL );
}

/* ---- ordinal 221: NtReleaseMutant ---- */
NTSTATUS WINAPI XBOXKRNL_NtReleaseMutant( HANDLE MutantHandle, LONG *PreviousCount )
{
    return NtReleaseMutant( MutantHandle, PreviousCount );
}

/* ---- ordinal 222: NtReleaseSemaphore ---- */
NTSTATUS WINAPI XBOXKRNL_NtReleaseSemaphore( HANDLE SemaphoreHandle, ULONG ReleaseCount, ULONG *PreviousCount )
{
    return NtReleaseSemaphore( SemaphoreHandle, ReleaseCount, (LONG *)PreviousCount );
}

/* ---- ordinal 224: NtResumeThread ---- */
NTSTATUS WINAPI XBOXKRNL_NtResumeThread( HANDLE ThreadHandle, ULONG *PreviousSuspendCount )
{
    return NtResumeThread( ThreadHandle, PreviousSuspendCount );
}

/* ---- ordinal 225: NtSetEvent ---- */
NTSTATUS WINAPI XBOXKRNL_NtSetEvent( HANDLE EventHandle, LONG *PreviousState )
{
    return NtSetEvent( EventHandle, PreviousState );
}

/* ---- ordinal 231: NtSuspendThread ---- */
NTSTATUS WINAPI XBOXKRNL_NtSuspendThread( HANDLE ThreadHandle, ULONG *PreviousSuspendCount )
{
    return NtSuspendThread( ThreadHandle, PreviousSuspendCount );
}

/* ---- ordinal 233: NtWaitForSingleObject ---- */
NTSTATUS WINAPI XBOXKRNL_NtWaitForSingleObject( HANDLE Handle, BOOLEAN Alertable, LARGE_INTEGER *Timeout )
{
    return NtWaitForSingleObject( Handle, Alertable, Timeout );
}

/* ---- ordinal 234: NtWaitForSingleObjectEx (the WaitMode param is a
 * kernel/user-mode indicator that's meaningless in our usermode-only
 * implementation, so it's accepted and dropped) ---- */
NTSTATUS WINAPI XBOXKRNL_NtWaitForSingleObjectEx( HANDLE Handle, CHAR WaitMode, BOOLEAN Alertable,
                                                   LARGE_INTEGER *Timeout )
{
    return NtWaitForSingleObject( Handle, Alertable, Timeout );
}

/* ---- ordinal 238: NtYieldExecution ---- */
void WINAPI XBOXKRNL_NtYieldExecution( void )
{
    NtYieldExecution();
}

/* ---- ordinal 260: RtlAnsiStringToUnicodeString ---- */
NTSTATUS WINAPI XBOXKRNL_RtlAnsiStringToUnicodeString( UNICODE_STRING *DestinationString, STRING *SourceString,
                                                         BOOLEAN AllocateDestinationString )
{
    return RtlAnsiStringToUnicodeString( DestinationString, SourceString, AllocateDestinationString );
}

/* ---- ordinal 264: RtlAssert ---- */
void WINAPI XBOXKRNL_RtlAssert( char *FailedAssertion, char *FileName, ULONG LineNumber, char *Message )
{
    ERR( "assertion failed: %s(%lu): %s (%s)\n", debugstr_a(FileName), LineNumber,
         debugstr_a(FailedAssertion), Message ? debugstr_a(Message) : "" );
    DebugBreak();
}

/* ---- ordinal 267: RtlCharToInteger ---- */
NTSTATUS WINAPI XBOXKRNL_RtlCharToInteger( const char *String, ULONG Base, ULONG *Value )
{
    return RtlCharToInteger( String, Base, Value );
}

/* ---- ordinal 268: RtlCompareMemory ---- */
SIZE_T WINAPI XBOXKRNL_RtlCompareMemory( const void *Source1, const void *Source2, SIZE_T Length )
{
    return RtlCompareMemory( Source1, Source2, Length );
}

/* ---- ordinal 269: RtlCompareMemoryUlong ---- */
SIZE_T WINAPI XBOXKRNL_RtlCompareMemoryUlong( void *Source, SIZE_T Length, ULONG Pattern )
{
    return RtlCompareMemoryUlong( Source, Length, Pattern );
}

/* ---- ordinal 270: RtlCompareString ---- */
LONG WINAPI XBOXKRNL_RtlCompareString( const STRING *String1, const STRING *String2, BOOLEAN CaseInSensitive )
{
    return RtlCompareString( String1, String2, CaseInSensitive );
}

/* ---- ordinal 271: RtlCompareUnicodeString ---- */
LONG WINAPI XBOXKRNL_RtlCompareUnicodeString( const UNICODE_STRING *String1, const UNICODE_STRING *String2,
                                               BOOLEAN CaseInSensitive )
{
    return RtlCompareUnicodeString( String1, String2, CaseInSensitive );
}

/* ---- ordinal 272: RtlCopyString ---- */
void WINAPI XBOXKRNL_RtlCopyString( STRING *DestinationString, const STRING *SourceString )
{
    RtlCopyString( DestinationString, SourceString );
}

/* ---- ordinal 273: RtlCopyUnicodeString ---- */
void WINAPI XBOXKRNL_RtlCopyUnicodeString( UNICODE_STRING *DestinationString, const UNICODE_STRING *SourceString )
{
    RtlCopyUnicodeString( DestinationString, SourceString );
}

/* ---- ordinal 275: RtlDowncaseUnicodeChar ---- */
WCHAR WINAPI XBOXKRNL_RtlDowncaseUnicodeChar( WCHAR SourceCharacter )
{
    return RtlDowncaseUnicodeChar( SourceCharacter );
}

/* ---- ordinal 277: RtlEnterCriticalSection ---- */
void WINAPI XBOXKRNL_RtlEnterCriticalSection( RTL_CRITICAL_SECTION *CriticalSection )
{
    RtlEnterCriticalSection( CriticalSection );
}

/* ---- ordinal 279: RtlEqualString ---- */
BOOLEAN WINAPI XBOXKRNL_RtlEqualString( const STRING *String1, const STRING *String2, BOOLEAN CaseInsensitive )
{
    return RtlEqualString( String1, String2, CaseInsensitive );
}

/* ---- ordinal 280: RtlEqualUnicodeString ---- */
BOOLEAN WINAPI XBOXKRNL_RtlEqualUnicodeString( const UNICODE_STRING *String1, const UNICODE_STRING *String2,
                                                BOOLEAN CaseInSensitive )
{
    return RtlEqualUnicodeString( String1, String2, CaseInSensitive );
}

/* ---- ordinal 284: RtlFillMemory (name collides with a macro in
 * winnt.h/winternl.h) ---- */
void WINAPI XBOXKRNL_RtlFillMemory( void *Destination, ULONG Length, BYTE Fill )
{
    memset( Destination, Fill, Length );
}

/* ---- ordinal 285: RtlFillMemoryUlong. ntdll doesn't export this one (only
 * the byte-granular RtlCompareMemoryUlong has an NTSYSAPI declaration in
 * Wine's headers), so it's implemented directly instead of forwarded: fill
 * Length bytes with the 4-byte Pattern repeated, exactly like the real
 * RTL routine of the same name. ---- */
void WINAPI XBOXKRNL_RtlFillMemoryUlong( void *Destination, SIZE_T Length, ULONG Pattern )
{
    ULONG *p = Destination;
    SIZE_T count = Length / sizeof(ULONG);
    while (count--) *p++ = Pattern;
}

/* ---- ordinal 286: RtlFreeAnsiString ---- */
void WINAPI XBOXKRNL_RtlFreeAnsiString( STRING *AnsiString )
{
    RtlFreeAnsiString( AnsiString );
}

/* ---- ordinal 287: RtlFreeUnicodeString ---- */
void WINAPI XBOXKRNL_RtlFreeUnicodeString( UNICODE_STRING *UnicodeString )
{
    RtlFreeUnicodeString( UnicodeString );
}

/* ---- ordinal 289: RtlInitAnsiString ---- */
void WINAPI XBOXKRNL_RtlInitAnsiString( STRING *DestinationString, const char *SourceString )
{
    RtlInitAnsiString( DestinationString, SourceString );
}

/* ---- ordinal 290: RtlInitUnicodeString ---- */
void WINAPI XBOXKRNL_RtlInitUnicodeString( UNICODE_STRING *DestinationString, const WCHAR *SourceString )
{
    RtlInitUnicodeString( DestinationString, SourceString );
}

/* ---- ordinal 291: RtlInitializeCriticalSection ---- */
void WINAPI XBOXKRNL_RtlInitializeCriticalSection( RTL_CRITICAL_SECTION *CriticalSection )
{
    RtlInitializeCriticalSection( CriticalSection );
}

/* ---- ordinal 292: RtlIntegerToChar ---- */
NTSTATUS WINAPI XBOXKRNL_RtlIntegerToChar( ULONG Value, ULONG Base, LONG OutputLength, char *String )
{
    return RtlIntegerToChar( Value, Base, OutputLength, String );
}

/* ---- ordinal 293: RtlIntegerToUnicodeString ---- */
NTSTATUS WINAPI XBOXKRNL_RtlIntegerToUnicodeString( ULONG Value, ULONG Base, UNICODE_STRING *String )
{
    return RtlIntegerToUnicodeString( Value, Base, String );
}

/* ---- ordinal 294: RtlLeaveCriticalSection ---- */
void WINAPI XBOXKRNL_RtlLeaveCriticalSection( RTL_CRITICAL_SECTION *CriticalSection )
{
    RtlLeaveCriticalSection( CriticalSection );
}

/* ---- ordinal 298: RtlMoveMemory (name collides with a macro) ---- */
void WINAPI XBOXKRNL_RtlMoveMemory( void *Destination, const void *Source, SIZE_T Length )
{
    memmove( Destination, Source, Length );
}

/* ---- ordinal 299: RtlMultiByteToUnicodeN ---- */
NTSTATUS WINAPI XBOXKRNL_RtlMultiByteToUnicodeN( WCHAR *UnicodeString, ULONG MaxBytesInUnicodeString,
                                                  ULONG *BytesInUnicodeString, const char *MultiByteString,
                                                  ULONG BytesInMultiByteString )
{
    return RtlMultiByteToUnicodeN( UnicodeString, MaxBytesInUnicodeString, BytesInUnicodeString,
                                    MultiByteString, BytesInMultiByteString );
}

/* ---- ordinal 301: RtlNtStatusToDosError ---- */
ULONG WINAPI XBOXKRNL_RtlNtStatusToDosError( NTSTATUS Status )
{
    return RtlNtStatusToDosError( Status );
}

/* ---- ordinal 306: RtlTryEnterCriticalSection ---- */
BOOLEAN WINAPI XBOXKRNL_RtlTryEnterCriticalSection( RTL_CRITICAL_SECTION *CriticalSection )
{
    return RtlTryEnterCriticalSection( CriticalSection );
}

/* ---- ordinal 307: RtlUlongByteSwap (name collides with a static inline in
 * winternl.h) ---- */
ULONG FASTCALL XBOXKRNL_RtlUlongByteSwap( ULONG Source )
{
    return RtlUlongByteSwap( Source );
}

/* ---- ordinal 308: RtlUnicodeStringToAnsiString ---- */
NTSTATUS WINAPI XBOXKRNL_RtlUnicodeStringToAnsiString( STRING *DestinationString, const UNICODE_STRING *SourceString,
                                                         BOOLEAN AllocateDestinationString )
{
    return RtlUnicodeStringToAnsiString( DestinationString, SourceString, AllocateDestinationString );
}

/* ---- ordinal 309: RtlUnicodeStringToInteger ---- */
NTSTATUS WINAPI XBOXKRNL_RtlUnicodeStringToInteger( const UNICODE_STRING *String, ULONG Base, ULONG *Value )
{
    return RtlUnicodeStringToInteger( String, Base, Value );
}

/* ---- ordinal 310: RtlUnicodeToMultiByteN ---- */
NTSTATUS WINAPI XBOXKRNL_RtlUnicodeToMultiByteN( char *MultiByteString, ULONG MaxBytesInMultiByteString,
                                                  ULONG *BytesInMultiByteString, const WCHAR *UnicodeString,
                                                  ULONG BytesInUnicodeString )
{
    return RtlUnicodeToMultiByteN( MultiByteString, MaxBytesInMultiByteString, BytesInMultiByteString,
                                    UnicodeString, BytesInUnicodeString );
}

/* ---- ordinal 313: RtlUpcaseUnicodeChar ---- */
WCHAR WINAPI XBOXKRNL_RtlUpcaseUnicodeChar( WCHAR SourceCharacter )
{
    return RtlUpcaseUnicodeChar( SourceCharacter );
}

/* ---- ordinal 314: RtlUpcaseUnicodeString ---- */
NTSTATUS WINAPI XBOXKRNL_RtlUpcaseUnicodeString( UNICODE_STRING *DestinationString, const UNICODE_STRING *SourceString,
                                                  BOOLEAN AllocateDestinationString )
{
    return RtlUpcaseUnicodeString( DestinationString, SourceString, AllocateDestinationString );
}

/* ---- ordinal 318: RtlUshortByteSwap ---- */
USHORT FASTCALL XBOXKRNL_RtlUshortByteSwap( USHORT Source )
{
    return RtlUshortByteSwap( Source );
}

/* ---- ordinal 320: RtlZeroMemory (name collides with a macro) ---- */
void WINAPI XBOXKRNL_RtlZeroMemory( void *Destination, SIZE_T Length )
{
    memset( Destination, 0, Length );
}


/*********************************************************************
 *           DllMain
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        xbox_sync_init();
        break;
    case DLL_PROCESS_DETACH:
        if (!reserved) xbox_sync_fini();
        break;
    }
    return TRUE;
}
