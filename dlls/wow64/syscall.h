/*
 * WoW64 syscall definitions
 *
 * Copyright 2021 Alexandre Julliard
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

#ifndef __WOW64_SYSCALL_H
#define __WOW64_SYSCALL_H

#define ALL_SYSCALLS \
    SYSCALL_ENTRY( NtAddAtom ) \
    SYSCALL_ENTRY( NtAllocateLocallyUniqueId ) \
    SYSCALL_ENTRY( NtAllocateUuids ) \
    SYSCALL_ENTRY( NtCancelTimer ) \
    SYSCALL_ENTRY( NtClearEvent ) \
    SYSCALL_ENTRY( NtClose ) \
    SYSCALL_ENTRY( NtCreateDebugObject ) \
    SYSCALL_ENTRY( NtCreateDirectoryObject ) \
    SYSCALL_ENTRY( NtCreateEvent ) \
    SYSCALL_ENTRY( NtCreateIoCompletion ) \
    SYSCALL_ENTRY( NtCreateJobObject ) \
    SYSCALL_ENTRY( NtCreateKeyedEvent ) \
    SYSCALL_ENTRY( NtCreateMutant ) \
    SYSCALL_ENTRY( NtCreateSection ) \
    SYSCALL_ENTRY( NtCreateSemaphore ) \
    SYSCALL_ENTRY( NtCreateSymbolicLinkObject ) \
    SYSCALL_ENTRY( NtCreateTimer ) \
    SYSCALL_ENTRY( NtDebugContinue ) \
    SYSCALL_ENTRY( NtDelayExecution ) \
    SYSCALL_ENTRY( NtDeleteAtom ) \
    SYSCALL_ENTRY( NtFindAtom ) \
    SYSCALL_ENTRY( NtGetCurrentProcessorNumber ) \
    SYSCALL_ENTRY( NtOpenDirectoryObject ) \
    SYSCALL_ENTRY( NtOpenEvent ) \
    SYSCALL_ENTRY( NtOpenIoCompletion ) \
    SYSCALL_ENTRY( NtOpenJobObject ) \
    SYSCALL_ENTRY( NtOpenKeyedEvent ) \
    SYSCALL_ENTRY( NtOpenMutant ) \
    SYSCALL_ENTRY( NtOpenSection ) \
    SYSCALL_ENTRY( NtOpenSemaphore ) \
    SYSCALL_ENTRY( NtOpenSymbolicLinkObject  ) \
    SYSCALL_ENTRY( NtOpenTimer ) \
    SYSCALL_ENTRY( NtPulseEvent ) \
    SYSCALL_ENTRY( NtQueryDefaultLocale ) \
    SYSCALL_ENTRY( NtQueryDefaultUILanguage ) \
    SYSCALL_ENTRY( NtQueryDirectoryObject ) \
    SYSCALL_ENTRY( NtQueryEvent ) \
    SYSCALL_ENTRY( NtQueryInformationAtom ) \
    SYSCALL_ENTRY( NtQueryInstallUILanguage ) \
    SYSCALL_ENTRY( NtQueryIoCompletion ) \
    SYSCALL_ENTRY( NtQueryMutant ) \
    SYSCALL_ENTRY( NtQueryPerformanceCounter ) \
    SYSCALL_ENTRY( NtQuerySection ) \
    SYSCALL_ENTRY( NtQuerySemaphore  ) \
    SYSCALL_ENTRY( NtQuerySymbolicLinkObject ) \
    SYSCALL_ENTRY( NtQueryTimer ) \
    SYSCALL_ENTRY( NtQueryTimerResolution ) \
    SYSCALL_ENTRY( NtReleaseKeyedEvent ) \
    SYSCALL_ENTRY( NtReleaseMutant ) \
    SYSCALL_ENTRY( NtReleaseSemaphore ) \
    SYSCALL_ENTRY( NtResetEvent ) \
    SYSCALL_ENTRY( NtSetDefaultLocale ) \
    SYSCALL_ENTRY( NtSetDefaultUILanguage ) \
    SYSCALL_ENTRY( NtSetEvent ) \
    SYSCALL_ENTRY( NtSetInformationDebugObject ) \
    SYSCALL_ENTRY( NtSetIoCompletion ) \
    SYSCALL_ENTRY( NtSetTimer ) \
    SYSCALL_ENTRY( NtSetTimerResolution ) \
    SYSCALL_ENTRY( NtSignalAndWaitForSingleObject ) \
    SYSCALL_ENTRY( NtTerminateJobObject ) \
    SYSCALL_ENTRY( NtWaitForDebugEvent ) \
    SYSCALL_ENTRY( NtWaitForKeyedEvent ) \
    SYSCALL_ENTRY( NtWaitForMultipleObjects ) \
    SYSCALL_ENTRY( NtWaitForSingleObject ) \
    SYSCALL_ENTRY( NtYieldExecution )

#endif /* __WOW64_SYSCALL_H */
