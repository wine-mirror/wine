/*
 * WoW64 private definitions
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

#ifndef __WOW64_PRIVATE_H
#define __WOW64_PRIVATE_H

#include "syscall.h"
#include "struct32.h"

#define SYSCALL_ENTRY(func) extern NTSTATUS WINAPI wow64_ ## func( UINT *args ) DECLSPEC_HIDDEN;
ALL_SYSCALLS
#undef SYSCALL_ENTRY

extern void init_image_mapping( HMODULE module ) DECLSPEC_HIDDEN;
extern void init_file_redirects(void) DECLSPEC_HIDDEN;
extern BOOL get_file_redirect( OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;

extern USHORT native_machine DECLSPEC_HIDDEN;
extern USHORT current_machine DECLSPEC_HIDDEN;
extern ULONG_PTR args_alignment DECLSPEC_HIDDEN;
extern ULONG_PTR highest_user_address DECLSPEC_HIDDEN;
extern ULONG_PTR default_zero_bits DECLSPEC_HIDDEN;
extern SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock DECLSPEC_HIDDEN;
extern void (WINAPI *pBTCpuUpdateProcessorInformation)( SYSTEM_CPU_INFORMATION * ) DECLSPEC_HIDDEN;

struct object_attr64
{
    OBJECT_ATTRIBUTES   attr;
    UNICODE_STRING      str;
    SECURITY_DESCRIPTOR sd;
};

/* cf. GetSystemWow64Directory2 */
static inline const WCHAR *get_machine_wow64_dir( USHORT machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_TARGET_HOST: return L"\\??\\C:\\windows\\system32";
    case IMAGE_FILE_MACHINE_I386:        return L"\\??\\C:\\windows\\syswow64";
    case IMAGE_FILE_MACHINE_ARMNT:       return L"\\??\\C:\\windows\\sysarm32";
    case IMAGE_FILE_MACHINE_AMD64:       return L"\\??\\C:\\windows\\sysx8664";
    case IMAGE_FILE_MACHINE_ARM64:       return L"\\??\\C:\\windows\\sysarm64";
    default: return NULL;
    }
}

static inline ULONG get_ulong( UINT **args ) { return *(*args)++; }
static inline HANDLE get_handle( UINT **args ) { return LongToHandle( *(*args)++ ); }
static inline void *get_ptr( UINT **args ) { return ULongToPtr( *(*args)++ ); }

static inline ULONG64 get_ulong64( UINT **args )
{
    ULONG64 ret;

    *args = (UINT *)(((ULONG_PTR)*args + args_alignment - 1) & ~(args_alignment - 1));
    ret = *(ULONG64 *)*args;
    *args += 2;
    return ret;
}

static inline ULONG_PTR get_zero_bits( ULONG_PTR zero_bits )
{
    return zero_bits ? zero_bits : default_zero_bits;
}

static inline void **addr_32to64( void **addr, ULONG *addr32 )
{
    if (!addr32) return NULL;
    *addr = ULongToPtr( *addr32 );
    return addr;
}

static inline SIZE_T *size_32to64( SIZE_T *size, ULONG *size32 )
{
    if (!size32) return NULL;
    *size = *size32;
    return size;
}

static inline void *apc_32to64( ULONG func )
{
    return func ? Wow64ApcRoutine : NULL;
}

static inline void *apc_param_32to64( ULONG func, ULONG context )
{
    if (!func) return ULongToPtr( context );
    return (void *)(ULONG_PTR)(((ULONG64)func << 32) | context);
}

static inline IO_STATUS_BLOCK *iosb_32to64( IO_STATUS_BLOCK *io, IO_STATUS_BLOCK32 *io32 )
{
    if (!io32) return NULL;
    io->Pointer = io32;
    return io;
}

static inline UNICODE_STRING *unicode_str_32to64( UNICODE_STRING *str, const UNICODE_STRING32 *str32 )
{
    if (!str32) return NULL;
    str->Length = str32->Length;
    str->MaximumLength = str32->MaximumLength;
    str->Buffer = ULongToPtr( str32->Buffer );
    return str;
}

static inline CLIENT_ID *client_id_32to64( CLIENT_ID *id, const CLIENT_ID32 *id32 )
{
    if (!id32) return NULL;
    id->UniqueProcess = LongToHandle( id32->UniqueProcess );
    id->UniqueThread = LongToHandle( id32->UniqueThread );
    return id;
}

static inline SECURITY_DESCRIPTOR *secdesc_32to64( SECURITY_DESCRIPTOR *out, const SECURITY_DESCRIPTOR *in )
{
    /* relative descr has the same layout for 32 and 64 */
    const SECURITY_DESCRIPTOR_RELATIVE *sd = (const SECURITY_DESCRIPTOR_RELATIVE *)in;

    if (!in) return NULL;
    out->Revision = sd->Revision;
    out->Sbz1     = sd->Sbz1;
    out->Control  = sd->Control & ~SE_SELF_RELATIVE;
    if (sd->Control & SE_SELF_RELATIVE)
    {
        out->Owner = sd->Owner ? (PSID)((BYTE *)sd + sd->Owner) : NULL;
        out->Group = sd->Group ? (PSID)((BYTE *)sd + sd->Group) : NULL;
        out->Sacl = ((sd->Control & SE_SACL_PRESENT) && sd->Sacl) ? (PSID)((BYTE *)sd + sd->Sacl) : NULL;
        out->Dacl = ((sd->Control & SE_DACL_PRESENT) && sd->Dacl) ? (PSID)((BYTE *)sd + sd->Dacl) : NULL;
    }
    else
    {
        out->Owner = ULongToPtr( sd->Owner );
        out->Group = ULongToPtr( sd->Group );
        out->Sacl = (sd->Control & SE_SACL_PRESENT) ? ULongToPtr( sd->Sacl ) : NULL;
        out->Dacl = (sd->Control & SE_DACL_PRESENT) ? ULongToPtr( sd->Dacl ) : NULL;
    }
    return out;
}

static inline OBJECT_ATTRIBUTES *objattr_32to64( struct object_attr64 *out, const OBJECT_ATTRIBUTES32 *in )
{
    memset( out, 0, sizeof(*out) );
    if (!in) return NULL;
    if (in->Length != sizeof(*in)) return &out->attr;

    out->attr.Length = sizeof(out->attr);
    out->attr.RootDirectory = LongToHandle( in->RootDirectory );
    out->attr.Attributes = in->Attributes;
    out->attr.ObjectName = unicode_str_32to64( &out->str, ULongToPtr( in->ObjectName ));
    out->attr.SecurityQualityOfService = ULongToPtr( in->SecurityQualityOfService );
    out->attr.SecurityDescriptor = secdesc_32to64( &out->sd, ULongToPtr( in->SecurityDescriptor ));
    return &out->attr;
}

static inline OBJECT_ATTRIBUTES *objattr_32to64_redirect( struct object_attr64 *out,
                                                          const OBJECT_ATTRIBUTES32 *in )
{
    OBJECT_ATTRIBUTES *attr = objattr_32to64( out, in );

    if (attr) get_file_redirect( attr );
    return attr;
}

static inline void put_handle( ULONG *handle32, HANDLE handle )
{
    *handle32 = HandleToULong( handle );
}

static inline void put_addr( ULONG *addr32, void *addr )
{
    if (addr32) *addr32 = PtrToUlong( addr );
}

static inline void put_size( ULONG *size32, SIZE_T size )
{
    if (size32) *size32 = min( size, MAXDWORD );
}

static inline void put_client_id( CLIENT_ID32 *id32, const CLIENT_ID *id )
{
    if (!id32) return;
    id32->UniqueProcess = HandleToLong( id->UniqueProcess );
    id32->UniqueThread = HandleToLong( id->UniqueThread );
}

static inline void put_iosb( IO_STATUS_BLOCK32 *io32, const IO_STATUS_BLOCK *io )
{
    /* sync I/O modifies the 64-bit iosb right away, so in that case we update the 32-bit one */
    /* async I/O leaves the 64-bit one untouched and updates the 32-bit one directly later on */
    if (io32 && io->Pointer != io32)
    {
        io32->Status = io->Status;
        io32->Information = io->Information;
    }
}

extern void put_section_image_info( SECTION_IMAGE_INFORMATION32 *info32,
                                    const SECTION_IMAGE_INFORMATION *info ) DECLSPEC_HIDDEN;
extern void put_vm_counters( VM_COUNTERS_EX32 *info32, const VM_COUNTERS_EX *info,
                             ULONG size ) DECLSPEC_HIDDEN;

#endif /* __WOW64_PRIVATE_H */
