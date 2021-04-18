/*
 * Unix library for libnetapi functions
 *
 * Copyright 2013 Hans Leidekker for CodeWeavers
 * Copyright 2021 Zebediah Figura for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "lm.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "unixlib.h"

#ifdef SONAME_LIBNETAPI

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static void *libnetapi_handle;
static void *libnetapi_ctx;

static DWORD (*plibnetapi_init)( void ** );
static DWORD (*plibnetapi_free)( void * );
static DWORD (*plibnetapi_set_debuglevel)( void *, const char * );
static DWORD (*plibnetapi_set_username)( void *, const char * );
static DWORD (*plibnetapi_set_password)( void *, const char * );

static NET_API_STATUS (*pNetApiBufferAllocate)( unsigned int, void ** );
static NET_API_STATUS (*pNetApiBufferFree)( void * );
static NET_API_STATUS (*pNetServerGetInfo)( const char *, unsigned int, unsigned char ** );
static NET_API_STATUS (*pNetShareAdd)( const char *, unsigned int, unsigned char *, unsigned int * );
static NET_API_STATUS (*pNetShareDel)( const char *, const char *, unsigned int );
static NET_API_STATUS (*pNetWkstaGetInfo)( const char *, unsigned int, unsigned char ** );

static CPTABLEINFO unix_cptable;
static ULONG unix_cp;

static BOOL get_unix_codepage(void)
{
    static const WCHAR wineunixcpW[] = {'W','I','N','E','U','N','I','X','C','P',0};
    UNICODE_STRING name, value;
    WCHAR value_buffer[13];
    SIZE_T size;
    void *ptr;

    if (unix_cp) return TRUE;

    RtlInitUnicodeString( &name, wineunixcpW );
    value.Buffer = value_buffer;
    value.MaximumLength = sizeof(value_buffer);
    if (!RtlQueryEnvironmentVariable_U( NULL, &name, &value ))
        RtlUnicodeStringToInteger( &value, 10, &unix_cp );
    if (NtGetNlsSectionPtr( 11, unix_cp, NULL, &ptr, &size ))
        return FALSE;
    RtlInitCodePageTable( ptr, &unix_cptable );
    return TRUE;
}

static DWORD netapi_wcstoumbs( const WCHAR *src, char *dst, DWORD dstlen )
{
    DWORD srclen = (strlenW( src ) + 1) * sizeof(WCHAR);
    DWORD len;

    get_unix_codepage();

    if (unix_cp == CP_UTF8)
    {
        RtlUnicodeToUTF8N( dst, dstlen, &len, src, srclen );
        return len;
    }
    else
    {
        len = (strlenW( src ) * 2) + 1;
        if (dst) RtlUnicodeToCustomCPN( &unix_cptable, dst, dstlen, &len, src, srclen );
        return len;
    }
}

static DWORD netapi_umbstowcs( const char *src, WCHAR *dst, DWORD dstlen )
{
    DWORD srclen = strlen( src ) + 1;
    DWORD len;

    get_unix_codepage();

    if (unix_cp == CP_UTF8)
    {
        RtlUTF8ToUnicodeN( dst, dstlen, &len, src, srclen );
        return len;
    }
    else
    {
        len = srclen * sizeof(WCHAR);
        if (dst) RtlCustomCPToUnicodeN( &unix_cptable, dst, dstlen, &len, src, srclen );
        return len;
    }
}

static char *strdup_unixcp( const WCHAR *str )
{
    char *ret;

    int len = netapi_wcstoumbs( str, NULL, 0 );
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, len )))
        netapi_wcstoumbs( str, ret, len );
    return ret;
}

struct server_info_101
{
    unsigned int sv101_platform_id;
    const char  *sv101_name;
    unsigned int sv101_version_major;
    unsigned int sv101_version_minor;
    unsigned int sv101_type;
    const char  *sv101_comment;
};

static NET_API_STATUS server_info_101_from_samba( const unsigned char *buf, BYTE **bufptr )
{
    SERVER_INFO_101 *ret;
    struct server_info_101 *info = (struct server_info_101 *)buf;
    DWORD len = 0;
    WCHAR *ptr;

    if (info->sv101_name) len += netapi_umbstowcs( info->sv101_name, NULL, 0 );
    if (info->sv101_comment) len += netapi_umbstowcs( info->sv101_comment, NULL, 0 );
    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*ret) + (len * sizeof(WCHAR) ))))
        return ERROR_OUTOFMEMORY;

    ptr = (WCHAR *)(ret + 1);
    ret->sv101_platform_id = info->sv101_platform_id;
    if (!info->sv101_name) ret->sv101_name = NULL;
    else
    {
        ret->sv101_name = ptr;
        ptr += netapi_umbstowcs( info->sv101_name, ptr, len );
    }
    ret->sv101_version_major = info->sv101_version_major;
    ret->sv101_version_minor = info->sv101_version_minor;
    ret->sv101_type          = info->sv101_type;
    if (!info->sv101_comment) ret->sv101_comment = NULL;
    else
    {
        ret->sv101_comment = ptr;
        netapi_umbstowcs( info->sv101_comment, ptr, len );
    }
    *bufptr = (BYTE *)ret;
    return NERR_Success;
}

static NET_API_STATUS server_info_from_samba( DWORD level, const unsigned char *buf, BYTE **bufptr )
{
    switch (level)
    {
    case 101: return server_info_101_from_samba( buf, bufptr );
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NET_API_STATUS WINAPI server_getinfo( const WCHAR *server, DWORD level, BYTE **buffer )
{
    NET_API_STATUS status;
    char *samba_server = NULL;
    unsigned char *samba_buffer = NULL;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (server && !(samba_server = strdup_unixcp( server ))) return ERROR_OUTOFMEMORY;
    status = pNetServerGetInfo( samba_server, level, &samba_buffer );
    RtlFreeHeap( GetProcessHeap(), 0, samba_server );
    if (!status)
    {
        status = server_info_from_samba( level, samba_buffer, buffer );
        pNetApiBufferFree( samba_buffer );
    }
    return status;
}

struct share_info_2
{
    const char  *shi2_netname;
    unsigned int shi2_type;
    const char  *shi2_remark;
    unsigned int shi2_permissions;
    unsigned int shi2_max_uses;
    unsigned int shi2_current_uses;
    const char  *shi2_path;
    const char  *shi2_passwd;
};

static NET_API_STATUS share_info_2_to_samba( const BYTE *buf, unsigned char **bufptr )
{
    struct share_info_2 *ret;
    SHARE_INFO_2 *info = (SHARE_INFO_2 *)buf;
    DWORD len = 0;
    char *ptr;

    if (info->shi2_netname)
        len += netapi_wcstoumbs( info->shi2_netname, NULL, 0 );
    if (info->shi2_remark)
        len += netapi_wcstoumbs( info->shi2_remark, NULL, 0 );
    if (info->shi2_path)
        len += netapi_wcstoumbs( info->shi2_path, NULL, 0 );
    if (info->shi2_passwd)
        len += netapi_wcstoumbs( info->shi2_passwd, NULL, 0 );
    if (!(ret = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret) + len )))
        return ERROR_OUTOFMEMORY;

    ptr = (char *)(ret + 1);
    if (!info->shi2_netname) ret->shi2_netname = NULL;
    else
    {
        ret->shi2_netname = ptr;
        ptr += netapi_wcstoumbs( info->shi2_netname, ptr, len );
    }
    ret->shi2_type = info->shi2_type;
    if (!info->shi2_remark) ret->shi2_remark = NULL;
    else
    {
        ret->shi2_remark = ptr;
        ptr += netapi_wcstoumbs( info->shi2_remark, ptr, len );
    }
    ret->shi2_permissions  = info->shi2_permissions;
    ret->shi2_max_uses     = info->shi2_max_uses;
    ret->shi2_current_uses = info->shi2_current_uses;
    if (!info->shi2_path) ret->shi2_path = NULL;
    else
    {
        ret->shi2_path = ptr;
        ptr += netapi_wcstoumbs( info->shi2_path, ptr, len );
    }
    if (!info->shi2_passwd) ret->shi2_passwd = NULL;
    else
    {
        ret->shi2_passwd = ptr;
        netapi_wcstoumbs( info->shi2_passwd, ptr, len );
    }
    *bufptr = (unsigned char *)ret;
    return NERR_Success;
}

struct sid
{
    unsigned char sid_rev_num;
    unsigned char num_auths;
    unsigned char id_auth[6];
    unsigned int  sub_auths[15];
};

enum ace_type
{
    ACE_TYPE_ACCESS_ALLOWED,
    ACE_TYPE_ACCESS_DENIED,
    ACE_TYPE_SYSTEM_AUDIT,
    ACE_TYPE_SYSTEM_ALARM,
    ACE_TYPE_ALLOWED_COMPOUND,
    ACE_TYPE_ACCESS_ALLOWED_OBJECT,
    ACE_TYPE_ACCESS_DENIED_OBJECT,
    ACE_TYPE_SYSTEM_AUDIT_OBJECT,
    ACE_TYPE_SYSTEM_ALARM_OBJECT
};

#define SEC_ACE_FLAG_OBJECT_INHERIT         0x01
#define SEC_ACE_FLAG_CONTAINER_INHERIT      0x02
#define SEC_ACE_FLAG_NO_PROPAGATE_INHERIT   0x04
#define SEC_ACE_FLAG_INHERIT_ONLY           0x08
#define SEC_ACE_FLAG_INHERITED_ACE          0x10
#define SEC_ACE_FLAG_SUCCESSFUL_ACCESS      0x40
#define SEC_ACE_FLAG_FAILED_ACCESS          0x80

struct guid
{
    unsigned int   time_low;
    unsigned short time_mid;
    unsigned short time_hi_and_version;
    unsigned char  clock_seq[2];
    unsigned char  node[6];
};

union ace_object_type
{
    struct guid type;
};

union ace_object_inherited_type
{
    struct guid inherited_type;
};

struct ace_object
{
    unsigned int flags;
    union ace_object_type type;
    union ace_object_inherited_type inherited_type;
};

union ace_object_ctr
{
    struct ace_object object;
};

struct ace
{
    enum ace_type  type;
    unsigned char  flags;
    unsigned short size;
    unsigned int   access_mask;
    union ace_object_ctr object;
    struct sid     trustee;
};

enum acl_revision
{
    ACL_REVISION_NT4 = 2,
    ACL_REVISION_ADS = 4
};

struct acl
{
    enum acl_revision revision;
    unsigned short size;
    unsigned int   num_aces;
    struct ace    *aces;
};

enum security_descriptor_revision
{
    SECURITY_DESCRIPTOR_REVISION_1 = 1
};

#define SEC_DESC_OWNER_DEFAULTED        0x0001
#define SEC_DESC_GROUP_DEFAULTED        0x0002
#define SEC_DESC_DACL_PRESENT           0x0004
#define SEC_DESC_DACL_DEFAULTED         0x0008
#define SEC_DESC_SACL_PRESENT           0x0010
#define SEC_DESC_SACL_DEFAULTED         0x0020
#define SEC_DESC_DACL_TRUSTED           0x0040
#define SEC_DESC_SERVER_SECURITY        0x0080
#define SEC_DESC_DACL_AUTO_INHERIT_REQ  0x0100
#define SEC_DESC_SACL_AUTO_INHERIT_REQ  0x0200
#define SEC_DESC_DACL_AUTO_INHERITED    0x0400
#define SEC_DESC_SACL_AUTO_INHERITED    0x0800
#define SEC_DESC_DACL_PROTECTED         0x1000
#define SEC_DESC_SACL_PROTECTED         0x2000
#define SEC_DESC_RM_CONTROL_VALID       0x4000
#define SEC_DESC_SELF_RELATIVE          0x8000

struct security_descriptor
{
    enum security_descriptor_revision revision;
    unsigned short type;
    struct sid    *owner_sid;
    struct sid    *group_sid;
    struct acl    *sacl;
    struct acl    *dacl;
};

struct share_info_502
{
    const char  *shi502_netname;
    unsigned int shi502_type;
    const char  *shi502_remark;
    unsigned int shi502_permissions;
    unsigned int shi502_max_uses;
    unsigned int shi502_current_uses;
    const char  *shi502_path;
    const char  *shi502_passwd;
    unsigned int shi502_reserved;
    struct security_descriptor *shi502_security_descriptor;
};

static unsigned short sd_control_to_samba( SECURITY_DESCRIPTOR_CONTROL control )
{
    unsigned short ret = 0;

    if (control & SE_OWNER_DEFAULTED)       ret |= SEC_DESC_OWNER_DEFAULTED;
    if (control & SE_GROUP_DEFAULTED)       ret |= SEC_DESC_GROUP_DEFAULTED;
    if (control & SE_DACL_PRESENT)          ret |= SEC_DESC_DACL_PRESENT;
    if (control & SE_DACL_DEFAULTED)        ret |= SEC_DESC_DACL_DEFAULTED;
    if (control & SE_SACL_PRESENT)          ret |= SEC_DESC_SACL_PRESENT;
    if (control & SE_SACL_DEFAULTED)        ret |= SEC_DESC_SACL_DEFAULTED;
    if (control & SE_DACL_AUTO_INHERIT_REQ) ret |= SEC_DESC_DACL_AUTO_INHERIT_REQ;
    if (control & SE_SACL_AUTO_INHERIT_REQ) ret |= SEC_DESC_SACL_AUTO_INHERIT_REQ;
    if (control & SE_DACL_AUTO_INHERITED)   ret |= SEC_DESC_DACL_AUTO_INHERITED;
    if (control & SE_SACL_AUTO_INHERITED)   ret |= SEC_DESC_SACL_AUTO_INHERITED;
    if (control & SE_DACL_PROTECTED)        ret |= SEC_DESC_DACL_PROTECTED;
    if (control & SE_SACL_PROTECTED)        ret |= SEC_DESC_SACL_PROTECTED;
    if (control & SE_RM_CONTROL_VALID)      ret |= SEC_DESC_RM_CONTROL_VALID;
    return ret;
}

static NET_API_STATUS sid_to_samba( const SID *src, struct sid *dst )
{
    unsigned int i;

    if (src->Revision != 1)
    {
        ERR( "unknown revision %u\n", src->Revision );
        return ERROR_UNKNOWN_REVISION;
    }
    if (src->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)
    {
        WARN( "invalid subauthority count %u\n", src->SubAuthorityCount );
        return ERROR_INVALID_PARAMETER;
    }
    dst->sid_rev_num = SECURITY_DESCRIPTOR_REVISION_1;
    dst->num_auths   = src->SubAuthorityCount;
    for (i = 0; i < 6; i++) dst->id_auth[i] = src->IdentifierAuthority.Value[i];
    for (i = 0; i < dst->num_auths; i++) dst->sub_auths[i] = src->SubAuthority[i];
    return NERR_Success;
}

static enum ace_type ace_type_to_samba( BYTE type )
{
    switch (type)
    {
    case ACCESS_ALLOWED_ACE_TYPE: return ACE_TYPE_ACCESS_ALLOWED;
    case ACCESS_DENIED_ACE_TYPE:  return ACE_TYPE_ACCESS_DENIED;
    case SYSTEM_AUDIT_ACE_TYPE:   return ACE_TYPE_SYSTEM_AUDIT;
    case SYSTEM_ALARM_ACE_TYPE:   return ACE_TYPE_SYSTEM_ALARM;
    default:
        ERR( "unhandled type %u\n", type );
        return 0;
    }
}

static unsigned char ace_flags_to_samba( BYTE flags )
{
    static const BYTE known_flags =
        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | NO_PROPAGATE_INHERIT_ACE |
        INHERIT_ONLY_ACE | INHERITED_ACE | SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG;
    unsigned char ret = 0;

    if (flags & ~known_flags)
    {
        ERR( "unknown flags %x\n", flags & ~known_flags );
        return 0;
    }
    if (flags & OBJECT_INHERIT_ACE)         ret |= SEC_ACE_FLAG_OBJECT_INHERIT;
    if (flags & CONTAINER_INHERIT_ACE)      ret |= SEC_ACE_FLAG_NO_PROPAGATE_INHERIT;
    if (flags & NO_PROPAGATE_INHERIT_ACE)   ret |= SEC_ACE_FLAG_NO_PROPAGATE_INHERIT;
    if (flags & INHERIT_ONLY_ACE)           ret |= SEC_ACE_FLAG_INHERIT_ONLY;
    if (flags & INHERITED_ACE)              ret |= SEC_ACE_FLAG_INHERITED_ACE;
    if (flags & SUCCESSFUL_ACCESS_ACE_FLAG) ret |= SEC_ACE_FLAG_SUCCESSFUL_ACCESS;
    if (flags & FAILED_ACCESS_ACE_FLAG)     ret |= SEC_ACE_FLAG_FAILED_ACCESS;
    return ret;
}

#define GENERIC_ALL_ACCESS     (1u << 28)
#define GENERIC_EXECUTE_ACCESS (1u << 29)
#define GENERIC_WRITE_ACCESS   (1u << 30)
#define GENERIC_READ_ACCESS    (1u << 31)

static unsigned int access_mask_to_samba( DWORD mask )
{
    static const DWORD known_rights =
        GENERIC_ALL | GENERIC_EXECUTE | GENERIC_WRITE | GENERIC_READ;
    unsigned int ret = 0;

    if (mask & ~known_rights)
    {
        ERR( "unknown rights %x\n", mask & ~known_rights );
        return 0;
    }
    if (mask & GENERIC_ALL)     ret |= GENERIC_ALL_ACCESS;
    if (mask & GENERIC_EXECUTE) ret |= GENERIC_EXECUTE_ACCESS;
    if (mask & GENERIC_WRITE)   ret |= GENERIC_WRITE_ACCESS;
    if (mask & GENERIC_READ)    ret |= GENERIC_READ_ACCESS;
    return ret;
}

static NET_API_STATUS ace_to_samba( const ACE_HEADER *src, struct ace *dst )
{
    dst->type  = ace_type_to_samba( src->AceType );
    dst->flags = ace_flags_to_samba( src->AceFlags );
    dst->size  = sizeof(*dst);
    switch (src->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    {
        ACCESS_ALLOWED_ACE *ace = (ACCESS_ALLOWED_ACE *)src;
        dst->access_mask = access_mask_to_samba( ace->Mask );
        memset( &dst->object, 0, sizeof(dst->object) );
        sid_to_samba( (const SID *)&ace->SidStart, &dst->trustee );
	break;
    }
    case ACCESS_DENIED_ACE_TYPE:
    {
        ACCESS_DENIED_ACE *ace = (ACCESS_DENIED_ACE *)src;
        dst->access_mask = access_mask_to_samba( ace->Mask );
        memset( &dst->object, 0, sizeof(dst->object) );
        sid_to_samba( (const SID *)&ace->SidStart, &dst->trustee );
	break;
    }
    case SYSTEM_AUDIT_ACE_TYPE:
    {
        SYSTEM_AUDIT_ACE *ace = (SYSTEM_AUDIT_ACE *)src;
        dst->access_mask = access_mask_to_samba( ace->Mask );
        memset( &dst->object, 0, sizeof(dst->object) );
        sid_to_samba( (const SID *)&ace->SidStart, &dst->trustee );
	break;
    }
    case SYSTEM_ALARM_ACE_TYPE:
    {
        SYSTEM_ALARM_ACE *ace = (SYSTEM_ALARM_ACE *)src;
        dst->access_mask = access_mask_to_samba( ace->Mask );
        memset( &dst->object, 0, sizeof(dst->object) );
        sid_to_samba( (const SID *)&ace->SidStart, &dst->trustee );
	break;
    }
    default:
        ERR( "unhandled type %u\n", src->AceType );
        return ERROR_INVALID_PARAMETER;
    }
    return NERR_Success;
}

static NET_API_STATUS acl_to_samba( const ACL *src, struct acl *dst )
{
    NET_API_STATUS status;
    ACE_HEADER *src_ace;
    unsigned int i;

    switch (src->AclRevision)
    {
    case ACL_REVISION4:
        dst->revision = ACL_REVISION_ADS;
        break;
    default:
        ERR( "unkhandled revision %u\n", src->AclRevision );
        return ERROR_UNKNOWN_REVISION;
    }
    dst->size = sizeof(*dst);
    src_ace = (ACE_HEADER *)(src + 1);
    dst->aces = (struct ace *)(dst + 1);
    for (i = 0; i < src->AceCount; i++)
    {
        if ((status = ace_to_samba( src_ace, &dst->aces[i] ))) return status;
        src_ace = (ACE_HEADER *)((char *)src_ace + src_ace->AceSize);
        dst->size += dst->aces[i].size;
    }
    return NERR_Success;
}

#define SELF_RELATIVE_FIELD(sd,field)\
    ((char *)(sd) + ((SECURITY_DESCRIPTOR_RELATIVE *)(sd))->field)

static NET_API_STATUS sd_to_samba( const SECURITY_DESCRIPTOR *src, struct security_descriptor *dst )
{
    NET_API_STATUS status;
    const SID *owner, *group;
    const ACL *dacl, *sacl;
    unsigned int offset = sizeof(*dst);

    if (src->Revision != SECURITY_DESCRIPTOR_REVISION1)
        return ERROR_UNKNOWN_REVISION;

    dst->revision = SECURITY_DESCRIPTOR_REVISION_1;
    dst->type = sd_control_to_samba( src->Control );

    if (src->Control & SE_SELF_RELATIVE)
    {
        if (!src->Owner) dst->owner_sid = NULL;
        else
        {
            dst->owner_sid = (struct sid *)((char *)dst + offset);
            owner = (const SID *)SELF_RELATIVE_FIELD( src, Owner );
            if ((status = sid_to_samba( owner, dst->owner_sid ))) return status;
            offset += sizeof(struct sid);
        }
        if (!src->Group) dst->group_sid = NULL;
        else
        {
            dst->group_sid = (struct sid *)((char *)dst + offset);
            group = (const SID *)SELF_RELATIVE_FIELD( src, Group );
            if ((status = sid_to_samba( group, dst->group_sid ))) return status;
            offset += sizeof(struct sid);
        }
        if (!(src->Control & SE_SACL_PRESENT)) dst->sacl = NULL;
        else
        {
            dst->sacl = (struct acl *)((char *)dst + offset);
            sacl = (const ACL *)SELF_RELATIVE_FIELD( src, Sacl );
            if ((status = acl_to_samba( sacl, dst->sacl ))) return status;
            offset += dst->sacl->size;
        }
        if (!(src->Control & SE_DACL_PRESENT)) dst->dacl = NULL;
        else
        {
            dst->dacl = (struct acl *)((char *)dst + offset);
            dacl = (const ACL *)SELF_RELATIVE_FIELD( src, Dacl );
            if ((status = acl_to_samba( dacl, dst->dacl ))) return status;
        }
    }
    else
    {
        if (!src->Owner) dst->owner_sid = NULL;
        else
        {
            dst->owner_sid = (struct sid *)((char *)dst + offset);
            if ((status = sid_to_samba( src->Owner, dst->owner_sid ))) return status;
            offset += sizeof(struct sid);
        }
        if (!src->Group) dst->group_sid = NULL;
        else
        {
            dst->group_sid = (struct sid *)((char *)dst + offset);
            if ((status = sid_to_samba( src->Group, dst->group_sid ))) return status;
            offset += sizeof(struct sid);
        }
        if (!(src->Control & SE_SACL_PRESENT)) dst->sacl = NULL;
        else
        {
            dst->sacl = (struct acl *)((char *)dst + offset);
            if ((status = acl_to_samba( src->Sacl, dst->sacl ))) return status;
            offset += dst->sacl->size;
        }
        if (!(src->Control & SE_DACL_PRESENT)) dst->dacl = NULL;
        else
        {
            dst->dacl = (struct acl *)((char *)dst + offset);
            if ((status = acl_to_samba( src->Dacl, dst->dacl ))) return status;
        }
    }
    return NERR_Success;
}

static unsigned int sd_to_samba_size( const SECURITY_DESCRIPTOR *sd )
{
    unsigned int ret = sizeof(struct security_descriptor);

    if (sd->Owner) ret += sizeof(struct sid);
    if (sd->Group) ret += sizeof(struct sid);
    if (sd->Control & SE_SACL_PRESENT)
        ret += sizeof(struct acl) + sd->Sacl->AceCount * sizeof(struct ace);
    if (sd->Control & SE_DACL_PRESENT)
        ret += sizeof(struct acl) + sd->Dacl->AceCount * sizeof(struct ace);
    return ret;
}

static NET_API_STATUS share_info_502_to_samba( const BYTE *buf, unsigned char **bufptr )
{
    NET_API_STATUS status;
    struct share_info_502 *ret;
    SHARE_INFO_502 *info = (SHARE_INFO_502 *)buf;
    DWORD len = 0, size = 0;
    char *ptr;

    *bufptr = NULL;
    if (info->shi502_netname)
        len += netapi_wcstoumbs( info->shi502_netname, NULL, 0 );
    if (info->shi502_remark)
        len += netapi_wcstoumbs( info->shi502_remark, NULL, 0 );
    if (info->shi502_path)
        len += netapi_wcstoumbs( info->shi502_path, NULL, 0 );
    if (info->shi502_passwd)
        len += netapi_wcstoumbs( info->shi502_passwd, NULL, 0 );
    if (info->shi502_security_descriptor)
        size = sd_to_samba_size( info->shi502_security_descriptor );
    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*ret) + (len * sizeof(WCHAR)) + size )))
        return ERROR_OUTOFMEMORY;

    ptr = (char *)(ret + 1);
    if (!info->shi502_netname) ret->shi502_netname = NULL;
    else
    {
        ret->shi502_netname = ptr;
        ptr += netapi_wcstoumbs( info->shi502_netname, ptr, len );
    }
    ret->shi502_type = info->shi502_type;
    if (!info->shi502_remark) ret->shi502_remark = NULL;
    else
    {
        ret->shi502_remark = ptr;
        ptr += netapi_wcstoumbs( info->shi502_remark, ptr, len );
    }
    ret->shi502_permissions  = info->shi502_permissions;
    ret->shi502_max_uses     = info->shi502_max_uses;
    ret->shi502_current_uses = info->shi502_current_uses;
    if (!info->shi502_path) ret->shi502_path = NULL;
    else
    {
        ret->shi502_path = ptr;
        ptr += netapi_wcstoumbs( info->shi502_path, ptr, len );
    }
    if (!info->shi502_passwd) ret->shi502_passwd = NULL;
    else
    {
        ret->shi502_passwd = ptr;
        ptr += netapi_wcstoumbs( info->shi502_passwd, ptr, len );
    }
    ret->shi502_reserved = info->shi502_reserved;
    if (!info->shi502_security_descriptor) ret->shi502_security_descriptor = NULL;
    else
    {
        status = sd_to_samba( info->shi502_security_descriptor, (struct security_descriptor *)ptr );
        if (status)
        {
            RtlFreeHeap( GetProcessHeap(), 0, ret );
            return status;
        }
        ret->shi502_security_descriptor = (struct security_descriptor *)ptr;
    }
    *bufptr = (unsigned char *)ret;
    return NERR_Success;
}

static NET_API_STATUS share_info_to_samba( DWORD level, const BYTE *buf, unsigned char **bufptr )
{
    switch (level)
    {
    case 2:     return share_info_2_to_samba( buf, bufptr );
    case 502:   return share_info_502_to_samba( buf, bufptr );
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NET_API_STATUS WINAPI share_add( const WCHAR *server, DWORD level, const BYTE *info, DWORD *err )
{
    char *samba_server = NULL;
    unsigned char *samba_info;
    NET_API_STATUS status;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (server && !(samba_server = strdup_unixcp( server ))) return ERROR_OUTOFMEMORY;
    status = share_info_to_samba( level, info, &samba_info );
    if (!status)
    {
        unsigned int samba_err;

        status = pNetShareAdd( samba_server, level, samba_info, &samba_err );
        RtlFreeHeap( GetProcessHeap(), 0, samba_info );
        if (err) *err = samba_err;
    }
    RtlFreeHeap( GetProcessHeap(), 0, samba_server );
    return status;
}

static NET_API_STATUS WINAPI share_del( const WCHAR *server, const WCHAR *share, DWORD reserved )
{
    char *samba_server = NULL, *samba_share;
    NET_API_STATUS status;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (server && !(samba_server = strdup_unixcp( server ))) return ERROR_OUTOFMEMORY;
    if (!(samba_share = strdup_unixcp( share )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, samba_server );
        return ERROR_OUTOFMEMORY;
    }
    status = pNetShareDel( samba_server, samba_share, reserved );
    RtlFreeHeap( GetProcessHeap(), 0, samba_server );
    RtlFreeHeap( GetProcessHeap(), 0, samba_share );
    return status;
}

struct wksta_info_100
{
    unsigned int wki100_platform_id;
    const char  *wki100_computername;
    const char  *wki100_langroup;
    unsigned int wki100_ver_major;
    unsigned int wki100_ver_minor;
};

static NET_API_STATUS wksta_info_100_from_samba( const unsigned char *buf, BYTE **bufptr )
{
    WKSTA_INFO_100 *ret;
    struct wksta_info_100 *info = (struct wksta_info_100 *)buf;
    DWORD len = 0;
    WCHAR *ptr;

    if (info->wki100_computername)
        len += netapi_umbstowcs( info->wki100_computername, NULL, 0 );
    if (info->wki100_langroup)
        len += netapi_umbstowcs( info->wki100_langroup, NULL, 0 );
    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*ret) + (len * sizeof(WCHAR) ))))
        return ERROR_OUTOFMEMORY;

    ptr = (WCHAR *)(ret + 1);
    ret->wki100_platform_id = info->wki100_platform_id;
    if (!info->wki100_computername) ret->wki100_computername = NULL;
    else
    {
        ret->wki100_computername = ptr;
        ptr += netapi_umbstowcs( info->wki100_computername, ptr, len );
    }
    if (!info->wki100_langroup) ret->wki100_langroup = NULL;
    else
    {
        ret->wki100_langroup = ptr;
        netapi_umbstowcs( info->wki100_langroup, ptr, len );
    }
    ret->wki100_ver_major = info->wki100_ver_major;
    ret->wki100_ver_minor = info->wki100_ver_minor;
    *bufptr = (BYTE *)ret;
    return NERR_Success;
}

static NET_API_STATUS wksta_info_from_samba( DWORD level, const unsigned char *buf, BYTE **bufptr )
{
    switch (level)
    {
    case 100: return wksta_info_100_from_samba( buf, bufptr );
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NET_API_STATUS WINAPI wksta_getinfo( const WCHAR *server, DWORD level, BYTE **buffer )
{
    unsigned char *samba_buffer = NULL;
    char *samba_server = NULL;
    NET_API_STATUS status;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (server && !(samba_server = strdup_unixcp( server ))) return ERROR_OUTOFMEMORY;
    status = pNetWkstaGetInfo( samba_server, level, &samba_buffer );
    RtlFreeHeap( GetProcessHeap(), 0, samba_server );
    if (!status)
    {
        status = wksta_info_from_samba( level, samba_buffer, buffer );
        pNetApiBufferFree( samba_buffer );
    }
    return status;
}

static void libnetapi_init(void)
{
    DWORD status;
    void *ctx;

    if (!(libnetapi_handle = dlopen( SONAME_LIBNETAPI, RTLD_NOW )))
    {
        ERR_(winediag)( "failed to load %s\n", SONAME_LIBNETAPI );
        return;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libnetapi_handle, #f ))) \
    { \
        ERR_(winediag)( "%s not found in %s\n", #f, SONAME_LIBNETAPI ); \
        return; \
    }

    LOAD_FUNCPTR(libnetapi_init)
    LOAD_FUNCPTR(libnetapi_free)
    LOAD_FUNCPTR(libnetapi_set_debuglevel)
    LOAD_FUNCPTR(libnetapi_set_username)
    LOAD_FUNCPTR(libnetapi_set_password)

    LOAD_FUNCPTR(NetApiBufferAllocate)
    LOAD_FUNCPTR(NetApiBufferFree)
    LOAD_FUNCPTR(NetServerGetInfo)
    LOAD_FUNCPTR(NetShareAdd)
    LOAD_FUNCPTR(NetShareDel)
    LOAD_FUNCPTR(NetWkstaGetInfo)
#undef LOAD_FUNCPTR

    if ((status = plibnetapi_init( &ctx )))
    {
        ERR( "Failed to initialize context, status %u\n", status );
        return;
    }
    if (TRACE_ON(netapi32) && (status = plibnetapi_set_debuglevel( ctx, "10" )))
    {
        ERR( "Failed to set debug level, status %u\n", status );
        plibnetapi_free( ctx );
        return;
    }
    /* perform an anonymous login by default (avoids a password prompt) */
    if ((status = plibnetapi_set_username( ctx, "Guest" )))
    {
        ERR( "Failed to set username, status %u\n", status );
        plibnetapi_free( ctx );
        return;
    }
    if ((status = plibnetapi_set_password( ctx, "" )))
    {
        ERR( "Failed to set password, status %u\n", status );
        plibnetapi_free( ctx );
        return;
    }

    libnetapi_ctx = ctx;
}

static NET_API_STATUS WINAPI change_password( const WCHAR *domainname, const WCHAR *username,
                                              const WCHAR *oldpassword, const WCHAR *newpassword )
{
    NET_API_STATUS ret = NERR_Success;
    static char option_silent[] = "-s";
    static char option_user[] = "-U";
    static char option_remote[] = "-r";
    static char smbpasswd[] = "smbpasswd";
    int pipe_out[2];
    pid_t pid, wret;
    int status;
    char *server = NULL, *user, *argv[7], *old = NULL, *new = NULL;

    if (domainname && !(server = strdup_unixcp( domainname ))) return ERROR_OUTOFMEMORY;
    if (!(user = strdup_unixcp( username )))
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }
    if (!(old = strdup_unixcp( oldpassword )))
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }
    if (!(new = strdup_unixcp( newpassword )))
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }
    argv[0] = smbpasswd;
    argv[1] = option_silent;
    argv[2] = option_user;
    argv[3] = user;
    if (server)
    {
        argv[4] = option_remote;
        argv[5] = server;
        argv[6] = NULL;
    }
    else argv[4] = NULL;

    if (pipe( pipe_out ) == -1)
    {
        ret = NERR_InternalError;
        goto end;
    }
    fcntl( pipe_out[0], F_SETFD, FD_CLOEXEC );
    fcntl( pipe_out[1], F_SETFD, FD_CLOEXEC );

    switch ((pid = fork()))
    {
    case -1:
        close( pipe_out[0] );
        close( pipe_out[1] );
        ret = NERR_InternalError;
        goto end;
    case 0:
        dup2( pipe_out[0], 0 );
        close( pipe_out[0] );
        close( pipe_out[1] );
        execvp( "smbpasswd", argv );
        ERR( "can't execute smbpasswd, is it installed?\n" );
        _exit(1);
    default:
        close( pipe_out[0] );
        break;
    }
    write( pipe_out[1], old, strlen( old ) );
    write( pipe_out[1], "\n", 1 );
    write( pipe_out[1], new, strlen( new ) );
    write( pipe_out[1], "\n", 1 );
    write( pipe_out[1], new, strlen( new ) );
    write( pipe_out[1], "\n", 1 );
    close( pipe_out[1] );

    do {
        wret = waitpid(pid, &status, 0);
    } while (wret < 0 && errno == EINTR);

    if (ret == NERR_Success && (wret < 0 || !WIFEXITED(status) || WEXITSTATUS(status)))
        ret = NERR_InternalError;

end:
    RtlFreeHeap( GetProcessHeap(), 0, server );
    RtlFreeHeap( GetProcessHeap(), 0, user );
    RtlFreeHeap( GetProcessHeap(), 0, old );
    RtlFreeHeap( GetProcessHeap(), 0, new );
    return ret;
}

#else

static NET_API_STATUS WINAPI server_getinfo( const WCHAR *server, DWORD level, BYTE **buffer )
{
    return ERROR_NOT_SUPPORTED;
}

static NET_API_STATUS WINAPI share_add( const WCHAR *server, DWORD level, const BYTE *info, DWORD *err )
{
    return ERROR_NOT_SUPPORTED;
}

static NET_API_STATUS WINAPI share_del( const WCHAR *server, const WCHAR *share, DWORD reserved )
{
    return ERROR_NOT_SUPPORTED;
}

static NET_API_STATUS WINAPI wksta_getinfo( const WCHAR *server, DWORD level, BYTE **buffer )
{
    return ERROR_NOT_SUPPORTED;
}

static NET_API_STATUS WINAPI change_password( const WCHAR *domainname, const WCHAR *username,
                                              const WCHAR *oldpassword, const WCHAR *newpassword )
{
    return ERROR_NOT_SUPPORTED;
}

static void libnetapi_init(void)
{
}

#endif /* SONAME_LIBNETAPI */

static const struct samba_funcs samba_funcs =
{
    server_getinfo,
    share_add,
    share_del,
    wksta_getinfo,
    change_password,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;

    libnetapi_init();
    *(const struct samba_funcs **)ptr_out = &samba_funcs;
    return STATUS_SUCCESS;
}
