/*
 * Copyright 1999 Ian Schmidt
 * Copyright 2001 Travis Michielsen
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
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static CRITICAL_SECTION random_cs;
static CRITICAL_SECTION_DEBUG random_debug =
{
    0, 0, &random_cs,
    { &random_debug.ProcessLocksList, &random_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": random_cs") }
};
static CRITICAL_SECTION random_cs = { &random_debug, -1, 0, 0, 0, 0 };

#define MAX_CPUS 256
static char random_buf[sizeof(SYSTEM_INTERRUPT_INFORMATION) * MAX_CPUS];
static ULONG random_len;
static ULONG random_pos;

/* FIXME: assumes interrupt information provides sufficient randomness */
static BOOL fill_random_buffer(void)
{
    ULONG len = sizeof(SYSTEM_INTERRUPT_INFORMATION) * min( NtCurrentTeb()->Peb->NumberOfProcessors, MAX_CPUS );
    NTSTATUS status;

    if ((status = NtQuerySystemInformation( SystemInterruptInformation, random_buf, len, NULL )))
    {
        WARN( "failed to get random bytes, status %#lx\n", status );
        return FALSE;
    }
    random_len = len;
    random_pos = 0;
    return TRUE;
}

/******************************************************************************
 *     SystemFunction036   (cryptbase.@)
 */
BOOLEAN WINAPI SystemFunction036( void *buffer, ULONG len )
{
    char *ptr = buffer;

    EnterCriticalSection( &random_cs );
    while (len)
    {
        ULONG size;
        if (random_pos >= random_len && !fill_random_buffer())
        {
            SetLastError( NTE_FAIL );
            LeaveCriticalSection( &random_cs );
            return FALSE;
        }
        size = min( len, random_len - random_pos );
        memcpy( ptr, random_buf + random_pos, size );
        random_pos += size;
        ptr += size;
        len -= size;
    }
    LeaveCriticalSection( &random_cs );
    return TRUE;
}

/******************************************************************************
 *     SystemFunction040   (cryptbase.@)
 *
 * MSDN documents this function as RtlEncryptMemory and declares it in ntsecapi.h.
 *
 * PARAMS
 *  memory [I/O] Pointer to memory to encrypt.
 *  length [I] Length of region to encrypt in bytes.
 *  flags  [I] Control whether other processes are able to decrypt the memory.
 *    RTL_ENCRYPT_OPTION_SAME_PROCESS
 *    RTL_ENCRYPT_OPTION_CROSS_PROCESS
 *    RTL_ENCRYPT_OPTION_SAME_LOGON
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: NTSTATUS error code
 *
 * NOTES
 *  length must be a multiple of RTL_ENCRYPT_MEMORY_SIZE.
 *  If flags are specified when encrypting, the same flag value must be given
 *  when decrypting the memory.
 */
NTSTATUS WINAPI SystemFunction040( void *memory, ULONG length, ULONG flags )
{
    FIXME("(%p, %lu, %#lx): stub [RtlEncryptMemory]\n", memory, length, flags);
    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction041   (cryptbase.@)
 *
 * MSDN documents this function as RtlDecryptMemory and declares it in ntsecapi.h.
 *
 * PARAMS
 *  memory [I/O] Pointer to memory to decrypt.
 *  length [I] Length of region to decrypt in bytes.
 *  flags  [I] Control whether other processes are able to decrypt the memory.
 *    RTL_ENCRYPT_OPTION_SAME_PROCESS
 *    RTL_ENCRYPT_OPTION_CROSS_PROCESS
 *    RTL_ENCRYPT_OPTION_SAME_LOGON
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: NTSTATUS error code
 *
 * NOTES
 *  length must be a multiple of RTL_ENCRYPT_MEMORY_SIZE.
 *  If flags are specified when encrypting, the same flag value must be given
 *  when decrypting the memory.
 */
NTSTATUS WINAPI SystemFunction041( void *memory, ULONG length, ULONG flags )
{
    FIXME( "(%p, %lu, %#lx): stub [RtlDecryptMemory]\n", memory, length, flags );
    return STATUS_SUCCESS;
}
