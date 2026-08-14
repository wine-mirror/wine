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

#include "../ntdll/ntsyscalls.h"
#include "struct32.h"

#define SYSCALL_ENTRY(id,name,_args) extern NTSTATUS WINAPI wow64_ ## name( UINT *args );
ALL_SYSCALLS32
#undef SYSCALL_ENTRY

extern void init_image_mapping( HMODULE module );
extern void init_file_redirects(void);
extern BOOL get_file_redirect( OBJECT_ATTRIBUTES *attr );

extern USHORT native_machine;
extern USHORT current_machine;
extern ULONG_PTR args_alignment;
extern ULONG_PTR highest_user_address;
extern ULONG_PTR default_zero_bits;
extern SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock;

extern void     (WINAPI *pBTCpuFlushInstructionCache2)( const void *, SIZE_T );
extern void     (WINAPI *pBTCpuFlushInstructionCacheHeavy)( const void *, SIZE_T );
extern NTSTATUS (WINAPI *pBTCpuNotifyMapViewOfSection)( void *, void *, void *, SIZE_T, ULONG, ULONG );
extern void     (WINAPI *pBTCpuNotifyMemoryAlloc)( void *, SIZE_T, ULONG, ULONG, BOOL, NTSTATUS );
extern void     (WINAPI *pBTCpuNotifyMemoryDirty)( void *, SIZE_T );
extern void     (WINAPI *pBTCpuNotifyMemoryFree)( void *, SIZE_T, ULONG, BOOL, NTSTATUS );
extern void     (WINAPI *pBTCpuNotifyMemoryProtect)( void *, SIZE_T, ULONG, BOOL, NTSTATUS );
extern void     (WINAPI *pBTCpuNotifyProcessExecuteFlagsChange)(ULONG);
extern void     (WINAPI *pBTCpuNotifyReadFile)( HANDLE, void *, SIZE_T, BOOL, NTSTATUS );
extern void     (WINAPI *pBTCpuNotifyUnmapViewOfSection)( void *, BOOL, NTSTATUS );
extern void     (WINAPI *pBTCpuUpdateProcessorInformation)( SYSTEM_CPU_INFORMATION * );
extern void     (WINAPI *pBTCpuProcessTerm)( HANDLE, BOOL, NTSTATUS );
extern void     (WINAPI *pBTCpuThreadTerm)( HANDLE, LONG );

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
    default: return NULL;
    }
}

static inline TEB32 *NtCurrentTeb32(void)
{
    return (TEB32 *)((char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset);
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

static inline ALPC_PORT_ATTRIBUTES *alpc_port_attributes_32to64( ALPC_PORT_ATTRIBUTES *out,
                                                                 const ALPC_PORT_ATTRIBUTES32 *in )
{
    if (!in) return NULL;

    out->Flags = in->Flags;
    out->SecurityQos.Length = in->SecurityQos.Length;
    out->SecurityQos.ImpersonationLevel = in->SecurityQos.ImpersonationLevel;
    out->SecurityQos.ContextTrackingMode = in->SecurityQos.ContextTrackingMode;
    out->SecurityQos.EffectiveOnly = in->SecurityQos.EffectiveOnly;
    out->MaxMessageLength = in->MaxMessageLength + (sizeof(ALPC_PORT_MESSAGE) - sizeof(ALPC_PORT_MESSAGE32));
    out->MemoryBandwidth = in->MemoryBandwidth;
    out->MaxPoolUsage = in->MaxPoolUsage;
    out->MaxSectionSize = in->MaxSectionSize;
    out->MaxViewSize = in->MaxViewSize;
    out->MaxTotalSectionSize = in->MaxTotalSectionSize;
    out->DupObjectTypes = in->DupObjectTypes;
    out->Reserved = 0;
    return out;
}

static inline ALPC_PORT_MESSAGE *alpc_port_message_32to64( ALPC_PORT_MESSAGE **out, SIZE_T out_msg_size,
                                                           const ALPC_PORT_MESSAGE32 *in, BOOL copy_msg )
{
    ALPC_PORT_MESSAGE *msg;

    if (!in || !(msg = Wow64AllocateTemp( out_msg_size )))
    {
        *out = NULL;
        return NULL;
    }

    if (!copy_msg) goto done;

    msg->DataLength = in->DataLength;
    msg->TotalLength = sizeof(*msg) + msg->DataLength;
    msg->Type = in->Type;
    msg->DataInfoOffset = in->DataInfoOffset;
    client_id_32to64( &msg->ClientId, &in->ClientId );
    msg->MessageId = in->MessageId;
    msg->ClientViewSize = in->ClientViewSize;
    memcpy( (unsigned char *)msg + sizeof(*msg), (const unsigned char *)in + sizeof(*in), in->DataLength );
done:
    *out = msg;
    return msg;
}

static inline ALPC_MESSAGE_ATTRIBUTES *alpc_port_message_attributes_32to64( ALPC_MESSAGE_ATTRIBUTES **out,
                                                                            const ALPC_MESSAGE_ATTRIBUTES32 *in,
                                                                            BOOL copy_attributes )
{
    ALPC_MESSAGE_ATTRIBUTES *attr;
    const unsigned char *current_from_attr;

    if (!in || !(attr = Wow64AllocateTemp( AlpcGetHeaderSize( in->AllocatedAttributes ) )))
    {
        *out = NULL;
        return NULL;
    }

    attr->AllocatedAttributes = in->AllocatedAttributes;

    if (!copy_attributes)
    {
        attr->ValidAttributes = 0;
        *out = attr;
        return attr;
    }

    attr->ValidAttributes = in->ValidAttributes;
    current_from_attr = (const unsigned char *)in + sizeof(*in);

    if (in->ValidAttributes & ALPC_MESSAGE_SECURITY_ATTRIBUTE)
    {
        const ALPC_SECURITY_ATTR32 *from_attr = (const ALPC_SECURITY_ATTR32 *)current_from_attr;
        ALPC_SECURITY_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_SECURITY_ATTRIBUTE );

        to_attr->Flags = from_attr->Flags;
        to_attr->ContextHandle = UlongToHandle( from_attr->ContextHandle );
        if (from_attr->QoSPointer)
        {
            SECURITY_QUALITY_OF_SERVICE32 *qos32 = (SECURITY_QUALITY_OF_SERVICE32 *)ULongToPtr( from_attr->QoSPointer );
            SECURITY_QUALITY_OF_SERVICE *qos = Wow64AllocateTemp( sizeof(*qos) );

            to_attr->QoS = qos;
            qos->Length = qos32->Length;
            qos->ImpersonationLevel = qos32->ImpersonationLevel;
            qos->ContextTrackingMode = qos32->ContextTrackingMode;
            qos->EffectiveOnly = qos32->EffectiveOnly;
        }
        else
        {
            to_attr->QoS = NULL;
        }
    }
    if (in->AllocatedAttributes & ALPC_MESSAGE_SECURITY_ATTRIBUTE)
        current_from_attr += sizeof(ALPC_SECURITY_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_VIEW_ATTRIBUTE)
    {
        const ALPC_VIEW_ATTR32 *from_attr = (const ALPC_VIEW_ATTR32 *)current_from_attr;
        ALPC_VIEW_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_VIEW_ATTRIBUTE );

        to_attr->Flags = from_attr->Flags;
        to_attr->SectionHandle = UlongToHandle( from_attr->SectionHandle );
        to_attr->ViewBase = UlongToPtr( from_attr->ViewBase );
        to_attr->ViewSize = from_attr->ViewSize;
    }
    if (in->AllocatedAttributes & ALPC_MESSAGE_VIEW_ATTRIBUTE)
        current_from_attr += sizeof(ALPC_VIEW_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_CONTEXT_ATTRIBUTE)
    {
        const ALPC_CONTEXT_ATTR32 *from_attr = (const ALPC_CONTEXT_ATTR32 *)current_from_attr;
        ALPC_CONTEXT_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_CONTEXT_ATTRIBUTE );

        to_attr->PortContext = UlongToPtr( from_attr->PortContext );
        to_attr->MessageContext = UlongToPtr( from_attr->MessageContext );
        to_attr->Sequence = from_attr->Sequence;
        to_attr->MessageId = from_attr->MessageId;
        to_attr->CallbackId = from_attr->CallbackId;
    }
    if (in->AllocatedAttributes & ALPC_MESSAGE_CONTEXT_ATTRIBUTE)
        current_from_attr += sizeof(ALPC_CONTEXT_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_HANDLE_ATTRIBUTE)
    {
        const ALPC_HANDLE_ATTR32 *from_attr = (const ALPC_HANDLE_ATTR32 *)current_from_attr;
        ALPC_HANDLE_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_HANDLE_ATTRIBUTE );

        to_attr->Flags = from_attr->Flags;
        to_attr->Handle = UlongToHandle( from_attr->Handle );
        to_attr->ObjectType = from_attr->ObjectType;
        to_attr->DesiredAccess = from_attr->DesiredAccess;
    }
    if (in->AllocatedAttributes & ALPC_MESSAGE_HANDLE_ATTRIBUTE)
        current_from_attr += sizeof(ALPC_HANDLE_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_TOKEN_ATTRIBUTE)
    {
        const ALPC_TOKEN_ATTR32 *from_attr = (const ALPC_TOKEN_ATTR32 *)current_from_attr;
        ALPC_TOKEN_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_TOKEN_ATTRIBUTE );

        to_attr->TokenId = from_attr->TokenId;
        to_attr->AuthenticationId = from_attr->AuthenticationId;
        to_attr->ModifiedId = from_attr->ModifiedId;
    }
    if (in->AllocatedAttributes & ALPC_MESSAGE_TOKEN_ATTRIBUTE)
        current_from_attr += sizeof(ALPC_TOKEN_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_DIRECT_ATTRIBUTE)
    {
        const ALPC_DIRECT_ATTR32 *from_attr = (const ALPC_DIRECT_ATTR32 *)current_from_attr;
        ALPC_DIRECT_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_DIRECT_ATTRIBUTE );

        to_attr->Event = UlongToHandle( from_attr->Event );
    }
    if (in->AllocatedAttributes & ALPC_MESSAGE_DIRECT_ATTRIBUTE)
        current_from_attr += sizeof(ALPC_DIRECT_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE)
    {
        const ALPC_WORK_ON_BEHALF_ATTR32 *from_attr = (const ALPC_WORK_ON_BEHALF_ATTR32 *)current_from_attr;
        ALPC_WORK_ON_BEHALF_ATTR *to_attr = AlpcGetMessageAttribute( attr, ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE );

        to_attr->Ticket = from_attr->Ticket;
    }

    *out = attr;
    return attr;
}

static inline ALPC_PORT_MESSAGE32 *alpc_port_message_64to32( ALPC_PORT_MESSAGE32 *out,
                                                             const ALPC_PORT_MESSAGE *in )
{
    if (!in) return NULL;

    out->DataLength = in->DataLength;
    out->TotalLength = sizeof(*out) + in->DataLength;
    out->Type = in->Type;
    out->DataInfoOffset = in->DataInfoOffset;
    out->ClientId.UniqueProcess = HandleToUlong( in->ClientId.UniqueProcess );
    out->ClientId.UniqueThread = HandleToUlong( in->ClientId.UniqueThread );
    out->MessageId = in->MessageId;
    out->ClientViewSize = in->ClientViewSize;
    memcpy( (unsigned char *)out + sizeof(*out), (const unsigned char *)in + sizeof(*in), in->DataLength );
    return out;
}

static inline ALPC_MESSAGE_ATTRIBUTES32 *alpc_port_message_attributes_64to32( ALPC_MESSAGE_ATTRIBUTES32 *out,
                                                                              ALPC_MESSAGE_ATTRIBUTES *in )
{
    unsigned char *current_to_attr;

    if (!in) return NULL;

    out->AllocatedAttributes = in->AllocatedAttributes;
    out->ValidAttributes = in->ValidAttributes;

    current_to_attr = (unsigned char *)out + sizeof(*out);

    if (in->ValidAttributes & ALPC_MESSAGE_SECURITY_ATTRIBUTE)
    {
        const ALPC_SECURITY_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_SECURITY_ATTRIBUTE );
        ALPC_SECURITY_ATTR32 *to_attr = (ALPC_SECURITY_ATTR32 *)current_to_attr;

        to_attr->Flags = from_attr->Flags;
        to_attr->ContextHandle = HandleToUlong( from_attr->ContextHandle );
        if (to_attr->QoSPointer && from_attr->QoS)
        {
            SECURITY_QUALITY_OF_SERVICE32 *qos32 = ULongToPtr( to_attr->QoSPointer );
            qos32->ImpersonationLevel = from_attr->QoS->ImpersonationLevel;
            qos32->ContextTrackingMode = from_attr->QoS->ContextTrackingMode;
            qos32->EffectiveOnly = from_attr->QoS->EffectiveOnly;
        }
    }
    if (out->AllocatedAttributes & ALPC_MESSAGE_SECURITY_ATTRIBUTE)
        current_to_attr += sizeof(ALPC_SECURITY_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_VIEW_ATTRIBUTE)
    {
        const ALPC_VIEW_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_VIEW_ATTRIBUTE );
        ALPC_VIEW_ATTR32 *to_attr = (ALPC_VIEW_ATTR32 *)current_to_attr;

        to_attr->Flags = from_attr->Flags;
        to_attr->SectionHandle = HandleToUlong( from_attr->SectionHandle );
        to_attr->ViewBase = PtrToUlong( from_attr->ViewBase );
        to_attr->ViewSize = from_attr->ViewSize;
    }
    if (out->AllocatedAttributes & ALPC_MESSAGE_VIEW_ATTRIBUTE)
        current_to_attr += sizeof(ALPC_VIEW_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_CONTEXT_ATTRIBUTE)
    {
        const ALPC_CONTEXT_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_CONTEXT_ATTRIBUTE );
        ALPC_CONTEXT_ATTR32 *to_attr = (ALPC_CONTEXT_ATTR32 *)current_to_attr;

        to_attr->PortContext = PtrToUlong( from_attr->PortContext );
        to_attr->MessageContext = PtrToUlong( from_attr->MessageContext );
        to_attr->Sequence = from_attr->Sequence;
        /* Should be from_attr->MessageId. But tests show that it's always 0 on 32-bit */
        to_attr->MessageId = 0;
        to_attr->CallbackId = from_attr->CallbackId;
    }
    if (out->AllocatedAttributes & ALPC_MESSAGE_CONTEXT_ATTRIBUTE)
        current_to_attr += sizeof(ALPC_CONTEXT_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_HANDLE_ATTRIBUTE)
    {
        const ALPC_HANDLE_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_HANDLE_ATTRIBUTE );
        ALPC_HANDLE_ATTR32 *to_attr = (ALPC_HANDLE_ATTR32 *)current_to_attr;

        to_attr->Flags = from_attr->Flags;
        to_attr->Handle = HandleToUlong( from_attr->Handle );
        to_attr->ObjectType = from_attr->ObjectType;
        to_attr->DesiredAccess = from_attr->DesiredAccess;
    }
    if (out->AllocatedAttributes & ALPC_MESSAGE_HANDLE_ATTRIBUTE)
        current_to_attr += sizeof(ALPC_HANDLE_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_TOKEN_ATTRIBUTE)
    {
        const ALPC_TOKEN_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_TOKEN_ATTRIBUTE );
        ALPC_TOKEN_ATTR32 *to_attr = (ALPC_TOKEN_ATTR32 *)current_to_attr;

        to_attr->TokenId = from_attr->TokenId;
        to_attr->AuthenticationId = from_attr->AuthenticationId;
        to_attr->ModifiedId = from_attr->ModifiedId;
    }
    if (out->AllocatedAttributes & ALPC_MESSAGE_TOKEN_ATTRIBUTE)
        current_to_attr += sizeof(ALPC_TOKEN_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_DIRECT_ATTRIBUTE)
    {
        const ALPC_DIRECT_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_DIRECT_ATTRIBUTE );
        ALPC_DIRECT_ATTR32 *to_attr = (ALPC_DIRECT_ATTR32 *)current_to_attr;

        to_attr->Event = HandleToUlong( from_attr->Event );
    }
    if (out->AllocatedAttributes & ALPC_MESSAGE_DIRECT_ATTRIBUTE)
        current_to_attr += sizeof(ALPC_DIRECT_ATTR32);

    if (in->ValidAttributes & ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE)
    {
        const ALPC_WORK_ON_BEHALF_ATTR *from_attr = AlpcGetMessageAttribute( in, ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE );
        ALPC_WORK_ON_BEHALF_ATTR32 *to_attr = (ALPC_WORK_ON_BEHALF_ATTR32 *)current_to_attr;

        to_attr->Ticket = from_attr->Ticket;
    }

    return out;
}

static inline TOKEN_USER *token_user_32to64( TOKEN_USER *out, const TOKEN_USER32 *in )
{
    out->User.Sid = ULongToPtr( in->User.Sid );
    out->User.Attributes = in->User.Attributes;
    return out;
}

static inline TOKEN_OWNER *token_owner_32to64( TOKEN_OWNER *out, const TOKEN_OWNER32 *in )
{
    out->Owner = ULongToPtr( in->Owner );
    return out;
}

static inline TOKEN_PRIMARY_GROUP *token_primary_group_32to64( TOKEN_PRIMARY_GROUP *out, const TOKEN_PRIMARY_GROUP32 *in )
{
    out->PrimaryGroup = ULongToPtr( in->PrimaryGroup );
    return out;
}

static inline TOKEN_DEFAULT_DACL *token_default_dacl_32to64( TOKEN_DEFAULT_DACL *out, const TOKEN_DEFAULT_DACL32 *in )
{
    out->DefaultDacl = ULongToPtr( in->DefaultDacl );
    return out;
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
                                    const SECTION_IMAGE_INFORMATION *info );
extern void put_vm_counters( VM_COUNTERS_EX32 *info32, const VM_COUNTERS_EX *info,
                             ULONG size );

#endif /* __WOW64_PRIVATE_H */
