/*
 * x86-64 emulation on ARM64
 *
 * Copyright 2024 Alexandre Julliard
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xtajit);


/**********************************************************************
 *           DispatchJump  (xtajit64.@)
 *
 * Implementation of __os_arm64x_x64_jump.
 */
void WINAPI DispatchJump(void)
{
    ERR( "x64 emulation not implemented\n" );
    NtTerminateProcess( GetCurrentProcess(), 1 );
}


/**********************************************************************
 *           RetToEntryThunk  (xtajit64.@)
 *
 * Implementation of __os_arm64x_dispatch_ret.
 */
void WINAPI RetToEntryThunk(void)
{
    ERR( "x64 emulation not implemented\n" );
    NtTerminateProcess( GetCurrentProcess(), 1 );
}


/**********************************************************************
 *           ExitToX64  (xtajit64.@)
 *
 * Implementation of __os_arm64x_dispatch_call_no_redirect.
 */
void WINAPI ExitToX64(void)
{
    ERR( "x64 emulation not implemented\n" );
    NtTerminateProcess( GetCurrentProcess(), 1 );
}


/**********************************************************************
 *           BeginSimulation  (xtajit64.@)
 */
void WINAPI BeginSimulation(void)
{
    ERR( "x64 emulation not implemented\n" );
    NtTerminateProcess( GetCurrentProcess(), 1 );
}


/**********************************************************************
 *           BTCpu64FlushInstructionCache  (xtajit64.@)
 */
void WINAPI BTCpu64FlushInstructionCache( void *addr, SIZE_T size )
{
    TRACE( "%p %Ix\n", addr, size );
}


/**********************************************************************
 *           BTCpu64IsProcessorFeaturePresent  (xtajit64.@)
 */
BOOLEAN WINAPI BTCpu64IsProcessorFeaturePresent( UINT feature )
{
    static const ULONGLONG x86_features =
        (1ull << PF_COMPARE_EXCHANGE_DOUBLE) |
        (1ull << PF_MMX_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_XMMI_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_RDTSC_INSTRUCTION_AVAILABLE) |
        (1ull << PF_XMMI64_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_NX_ENABLED) |
        (1ull << PF_SSE3_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_COMPARE_EXCHANGE128) |
        (1ull << PF_FASTFAIL_AVAILABLE) |
        (1ull << PF_RDTSCP_INSTRUCTION_AVAILABLE) |
        (1ull << PF_SSSE3_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_SSE4_1_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_SSE4_2_INSTRUCTIONS_AVAILABLE);

    return feature < 64 && (x86_features & (1ull << feature));
}


/**********************************************************************
 *           BTCpu64NotifyMemoryDirty  (xtajit64.@)
 */
void WINAPI BTCpu64NotifyMemoryDirty( void *addr, SIZE_T size )
{
    TRACE( "%p %Ix\n", addr, size );
}


/**********************************************************************
 *           BTCpu64NotifyReadFile  (xtajit64.@)
 */
void WINAPI BTCpu64NotifyReadFile( HANDLE handle, void *addr, SIZE_T size, BOOL is_post, NTSTATUS status )
{
    TRACE( "%p %p %Ix\n", handle, addr, size );
}


/**********************************************************************
 *           FlushInstructionCacheHeavy  (xtajit64.@)
 */
void WINAPI FlushInstructionCacheHeavy( void *addr, SIZE_T size )
{
    TRACE( "%p %Ix\n", addr, size );
}


/**********************************************************************
 *           NotifyMapViewOfSection  (xtajit64.@)
 */
NTSTATUS WINAPI NotifyMapViewOfSection( void *unk1, void *addr, void *unk2, SIZE_T size,
                                        ULONG alloc_type, ULONG protect )
{
    TRACE( "%p %Ix %lx %lx\n", addr, size, alloc_type, protect );
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           NotifyMemoryAlloc  (xtajit64.@)
 */
void WINAPI NotifyMemoryAlloc( void *addr, SIZE_T size, ULONG type, ULONG prot, BOOL is_post, NTSTATUS status )
{
    TRACE( "%p %Ix\n", addr, size );
}


/**********************************************************************
 *           NotifyMemoryFree  (xtajit64.@)
 */
void WINAPI NotifyMemoryFree( void *addr, SIZE_T size, ULONG type, BOOL is_post, NTSTATUS status )
{
    TRACE( "%p %Ix %lx\n", addr, size, type );
}


/**********************************************************************
 *           NotifyMemoryProtect  (xtajit64.@)
 */
void WINAPI NotifyMemoryProtect( void *addr, SIZE_T size, ULONG prot, BOOL is_post, NTSTATUS status )
{
    TRACE( "%p %Ix %lx\n", addr, size, prot );
}


/**********************************************************************
 *           NotifyUnmapViewOfSection  (xtajit64.@)
 */
void WINAPI NotifyUnmapViewOfSection( void *addr, BOOL is_post, NTSTATUS status )
{
    TRACE( "%p\n", addr );
}


/**********************************************************************
 *           ProcessInit  (xtajit64.@)
 */
NTSTATUS WINAPI ProcessInit(void)
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           ProcessTerm  (xtajit64.@)
 */
void WINAPI ProcessTerm( HANDLE handle, BOOL is_post, NTSTATUS status )
{
    TRACE( "%p\n", handle );
}


/**********************************************************************
 *           ResetToConsistentState  (xtajit64.@)
 */
void WINAPI ResetToConsistentState( EXCEPTION_RECORD *rec, CONTEXT *context, ARM64_NT_CONTEXT *arm_ctx )
{
    TRACE( "%p %p %p\n", rec, context, arm_ctx );
}


/**********************************************************************
 *           ThreadInit  (xtajit64.@)
 */
NTSTATUS WINAPI ThreadInit(void)
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           ThreadTerm  (xtajit64.@)
 */
void WINAPI ThreadTerm( HANDLE handle, LONG exit_code )
{
    TRACE( "%p %lx\n", handle, exit_code );
}


/**********************************************************************
 *           UpdateProcessorInformation  (xtajit64.@)
 */
void WINAPI UpdateProcessorInformation( SYSTEM_CPU_INFORMATION *info )
{
    info->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
    info->ProcessorLevel = 21;
    info->ProcessorRevision = 1;
}


/**********************************************************************
 *           DllMain
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}
