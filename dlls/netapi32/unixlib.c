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

#ifdef SONAME_LIBNETAPI

#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "lm.h"
#include "wine/debug.h"

#include "unixlib.h"

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

static DWORD netapi_wcstoumbs( const WCHAR *src, char *dst, DWORD dstlen )
{
    if (!dst) return 3 * wcslen( src ) + 1;
    return ntdll_wcstoumbs( src, wcslen( src ) + 1, dst, dstlen, FALSE );
}

static DWORD netapi_umbstowcs( const char *src, WCHAR *dst, DWORD dstlen )
{
    if (!dst) return strlen( src ) + 1;
    return ntdll_umbstowcs( src, strlen( src ) + 1, dst, dstlen );
}

static char *strdup_unixcp( const WCHAR *str )
{
    char *ret;

    int len = netapi_wcstoumbs( str, NULL, 0 );
    if ((ret = malloc( len )))
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

static NET_API_STATUS server_info_101_from_samba( const unsigned char *buf, void *buffer, ULONG *size )
{
    SERVER_INFO_101 *ret = (SERVER_INFO_101 *)buffer;
    const struct server_info_101 *info = (const struct server_info_101 *)buf;
    DWORD len = 0;
    WCHAR *ptr;

    if (info->sv101_name) len += netapi_umbstowcs( info->sv101_name, NULL, 0 );
    if (info->sv101_comment) len += netapi_umbstowcs( info->sv101_comment, NULL, 0 );
    if (*size < sizeof(*ret) + (len * sizeof(WCHAR) ))
    {
        *size = sizeof(*ret) + (len * sizeof(WCHAR) );
        return ERROR_INSUFFICIENT_BUFFER;
    }

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
        ptr += netapi_umbstowcs( info->sv101_comment, ptr, len );
    }
    *size = (char *)ptr - (char *)buffer;
    return NERR_Success;
}

static NET_API_STATUS server_info_from_samba( unsigned int level, const unsigned char *buf, void *buffer, ULONG *size )
{
    switch (level)
    {
    case 101: return server_info_101_from_samba( buf, buffer, size );
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NTSTATUS server_getinfo( void *args )
{
    const struct server_getinfo_params *params = args;
    NET_API_STATUS status;
    char *samba_server = NULL;
    unsigned char *samba_buffer = NULL;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (params->server && !(samba_server = strdup_unixcp( params->server ))) return ERROR_OUTOFMEMORY;
    status = pNetServerGetInfo( samba_server, params->level, &samba_buffer );
    free( samba_server );
    if (!status)
    {
        status = server_info_from_samba( params->level, samba_buffer, params->buffer, params->size );
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
    if (!(ret = malloc( sizeof(*ret) + len )))
        return ERROR_OUTOFMEMORY;

    ptr = (char *)(ret + 1);
    if (!info->shi2_netname) ret->shi2_netname = NULL;
    else
    {
        ret->shi2_netname = ptr;
        ptr += netapi_wcstoumbs( info->shi2_netname, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    ret->shi2_type = info->shi2_type;
    if (!info->shi2_remark) ret->shi2_remark = NULL;
    else
    {
        ret->shi2_remark = ptr;
        ptr += netapi_wcstoumbs( info->shi2_remark, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    ret->shi2_permissions  = info->shi2_permissions;
    ret->shi2_max_uses     = info->shi2_max_uses;
    ret->shi2_current_uses = info->shi2_current_uses;
    if (!info->shi2_path) ret->shi2_path = NULL;
    else
    {
        ret->shi2_path = ptr;
        ptr += netapi_wcstoumbs( info->shi2_path, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    if (!info->shi2_passwd) ret->shi2_passwd = NULL;
    else
    {
        ret->shi2_passwd = ptr;
        netapi_wcstoumbs( info->shi2_passwd, ptr, len - (ptr - (char *)(ret + 1)) );
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

static unsigned int access_mask_to_samba( unsigned int mask )
{
    static const unsigned int known_rights =
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
    if (!(ret = malloc( sizeof(*ret) + (len * sizeof(WCHAR)) + size )))
        return ERROR_OUTOFMEMORY;

    ptr = (char *)(ret + 1);
    if (!info->shi502_netname) ret->shi502_netname = NULL;
    else
    {
        ret->shi502_netname = ptr;
        ptr += netapi_wcstoumbs( info->shi502_netname, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    ret->shi502_type = info->shi502_type;
    if (!info->shi502_remark) ret->shi502_remark = NULL;
    else
    {
        ret->shi502_remark = ptr;
        ptr += netapi_wcstoumbs( info->shi502_remark, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    ret->shi502_permissions  = info->shi502_permissions;
    ret->shi502_max_uses     = info->shi502_max_uses;
    ret->shi502_current_uses = info->shi502_current_uses;
    if (!info->shi502_path) ret->shi502_path = NULL;
    else
    {
        ret->shi502_path = ptr;
        ptr += netapi_wcstoumbs( info->shi502_path, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    if (!info->shi502_passwd) ret->shi502_passwd = NULL;
    else
    {
        ret->shi502_passwd = ptr;
        ptr += netapi_wcstoumbs( info->shi502_passwd, ptr, len - (ptr - (char *)(ret + 1)) );
    }
    ret->shi502_reserved = info->shi502_reserved;
    if (!info->shi502_security_descriptor) ret->shi502_security_descriptor = NULL;
    else
    {
        status = sd_to_samba( info->shi502_security_descriptor, (struct security_descriptor *)ptr );
        if (status)
        {
            free( ret );
            return status;
        }
        ret->shi502_security_descriptor = (struct security_descriptor *)ptr;
    }
    *bufptr = (unsigned char *)ret;
    return NERR_Success;
}

static NET_API_STATUS share_info_to_samba( unsigned int level, const BYTE *buf, unsigned char **bufptr )
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

static NTSTATUS share_add( void *args )
{
    const struct share_add_params *params = args;
    char *samba_server = NULL;
    unsigned char *samba_info;
    NET_API_STATUS status;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (params->server && !(samba_server = strdup_unixcp( params->server ))) return ERROR_OUTOFMEMORY;
    status = share_info_to_samba( params->level, params->info, &samba_info );
    if (!status)
    {
        unsigned int samba_err;

        status = pNetShareAdd( samba_server, params->level, samba_info, &samba_err );
        free( samba_info );
        if (params->err) *params->err = samba_err;
    }
    free( samba_server );
    return status;
}

static NTSTATUS share_del( void *args )
{
    const struct share_del_params *params = args;
    char *samba_server = NULL, *samba_share;
    NET_API_STATUS status;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (params->server && !(samba_server = strdup_unixcp( params->server ))) return ERROR_OUTOFMEMORY;
    if (!(samba_share = strdup_unixcp( params->share )))
    {
        free( samba_server );
        return ERROR_OUTOFMEMORY;
    }
    status = pNetShareDel( samba_server, samba_share, params->reserved );
    free( samba_server );
    free( samba_share );
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

static NET_API_STATUS wksta_info_100_from_samba( const unsigned char *buf, void *buffer, ULONG *size )
{
    WKSTA_INFO_100 *ret = (WKSTA_INFO_100 *)buffer;
    const struct wksta_info_100 *info = (const struct wksta_info_100 *)buf;
    DWORD len = 0;
    WCHAR *ptr;

    if (info->wki100_computername)
        len += netapi_umbstowcs( info->wki100_computername, NULL, 0 );
    if (info->wki100_langroup)
        len += netapi_umbstowcs( info->wki100_langroup, NULL, 0 );
    if (*size < sizeof(*ret) + (len * sizeof(WCHAR) ))
    {
        *size = sizeof(*ret) + (len * sizeof(WCHAR) );
        return ERROR_INSUFFICIENT_BUFFER;
    }

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
        ptr += netapi_umbstowcs( info->wki100_langroup, ptr, len );
    }
    ret->wki100_ver_major = info->wki100_ver_major;
    ret->wki100_ver_minor = info->wki100_ver_minor;
    *size = (char *)ptr - (char *)buffer;
    return NERR_Success;
}

static NET_API_STATUS wksta_info_from_samba( unsigned int level, const unsigned char *buf, void *buffer, ULONG *size )
{
    switch (level)
    {
    case 100: return wksta_info_100_from_samba( buf, buffer, size );
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NTSTATUS wksta_getinfo( void *args )
{
    const struct wksta_getinfo_params *params = args;
    unsigned char *samba_buffer = NULL;
    char *samba_server = NULL;
    NET_API_STATUS status;

    if (!libnetapi_ctx) return ERROR_NOT_SUPPORTED;

    if (params->server && !(samba_server = strdup_unixcp( params->server ))) return ERROR_OUTOFMEMORY;
    status = pNetWkstaGetInfo( samba_server, params->level, &samba_buffer );
    free( samba_server );
    if (!status)
    {
        status = wksta_info_from_samba( params->level, samba_buffer, params->buffer, params->size );
        pNetApiBufferFree( samba_buffer );
    }
    return status;
}

static NTSTATUS netapi_init( void *args )
{
    unsigned int status;
    void *ctx;

    if (!(libnetapi_handle = dlopen( SONAME_LIBNETAPI, RTLD_NOW )))
    {
        ERR_(winediag)( "failed to load %s\n", SONAME_LIBNETAPI );
        return STATUS_DLL_NOT_FOUND;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libnetapi_handle, #f ))) \
    { \
        ERR_(winediag)( "%s not found in %s\n", #f, SONAME_LIBNETAPI ); \
        return STATUS_DLL_NOT_FOUND; \
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
        return STATUS_DLL_NOT_FOUND;
    }
    if (TRACE_ON(netapi32) && (status = plibnetapi_set_debuglevel( ctx, "10" )))
    {
        ERR( "Failed to set debug level, status %u\n", status );
        plibnetapi_free( ctx );
        return STATUS_DLL_NOT_FOUND;
    }
    /* perform an anonymous login by default (avoids a password prompt) */
    if ((status = plibnetapi_set_username( ctx, "Guest" )))
    {
        ERR( "Failed to set username, status %u\n", status );
        plibnetapi_free( ctx );
        return STATUS_DLL_NOT_FOUND;
    }
    if ((status = plibnetapi_set_password( ctx, "" )))
    {
        ERR( "Failed to set password, status %u\n", status );
        plibnetapi_free( ctx );
        return STATUS_DLL_NOT_FOUND;
    }

    libnetapi_ctx = ctx;
    return STATUS_SUCCESS;
}

static NTSTATUS change_password( void *args )
{
    const struct change_password_params *params = args;
    NET_API_STATUS ret = NERR_Success;
    static char option_silent[] = "-s";
    static char option_user[] = "-U";
    static char option_remote[] = "-r";
    static char smbpasswd[] = "smbpasswd";
    int pipe_out[2];
    pid_t pid, wret;
    int status;
    char *server = NULL, *user, *argv[7], *old = NULL, *new = NULL;

    if (params->domain && !(server = strdup_unixcp( params->domain ))) return ERROR_OUTOFMEMORY;
    if (!(user = strdup_unixcp( params->user )))
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }
    if (!(old = strdup_unixcp( params->old )))
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }
    if (!(new = strdup_unixcp( params->new )))
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
    free( server );
    free( user );
    free( old );
    free( new );
    return ret;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    netapi_init,
    server_getinfo,
    share_add,
    share_del,
    wksta_getinfo,
    change_password,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

struct server_info_101_32
{
    unsigned int sv101_platform_id;
    PTR32        sv101_name;
    unsigned int sv101_version_major;
    unsigned int sv101_version_minor;
    unsigned int sv101_type;
    PTR32        sv101_comment;
};

static NTSTATUS create_server_info32( unsigned int level, void *buffer )
{
    switch (level)
    {
    case 101:
    {
        struct server_info_101_32 *si32 = (struct server_info_101_32 *)buffer;
        struct server_info_101 *si = (struct server_info_101 *)buffer;

        si32->sv101_platform_id = si->sv101_platform_id;
        si32->sv101_name = PtrToUlong( si->sv101_name );
        si32->sv101_version_major = si->sv101_version_major;
        si32->sv101_version_minor = si->sv101_version_minor;
        si32->sv101_type = si->sv101_type;
        si32->sv101_comment = PtrToUlong( si->sv101_comment );
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NTSTATUS wow64_server_getinfo( void *args )
{
    struct
    {
        PTR32 server;
        DWORD level;
        PTR32 buffer;
        PTR32 size;
    } const *params32 = args;

    struct server_getinfo_params params =
    {
        ULongToPtr(params32->server),
        params32->level,
        ULongToPtr(params32->buffer),
        ULongToPtr(params32->size)
    };
    NTSTATUS status;

    status = server_getinfo( &params );
    if (!status) status = create_server_info32( params.level, params.buffer );
    return status;
}

struct share_info_2_32
{
    PTR32        shi2_netname;
    unsigned int shi2_type;
    PTR32        shi2_remark;
    unsigned int shi2_permissions;
    unsigned int shi2_max_uses;
    unsigned int shi2_current_uses;
    PTR32        shi2_path;
    PTR32        shi2_passwd;
};

struct share_info
{
    union
    {
        struct share_info_2 si2;
        struct share_info_502 si502;
    };
    struct security_descriptor sd;
    struct acl sacl;
    struct acl dacl;
};

struct acl_32
{
    enum acl_revision revision;
    unsigned short size;
    unsigned int   num_aces;
    PTR32          aces;
};

static void create_acl64( const struct acl_32 *acl32, struct acl *acl )
{
    acl->revision = acl32->revision;
    acl->size = acl32->size;
    acl->num_aces = acl32->num_aces;
    acl->aces = ULongToPtr( acl32->aces );
}

struct security_descriptor_32
{
    enum security_descriptor_revision revision;
    unsigned short type;
    PTR32          owner_sid;
    PTR32          group_sid;
    PTR32          sacl;
    PTR32          dacl;
};

static void create_security_descriptor64( const struct security_descriptor_32 *sd32,
        struct share_info *si )
{
    struct security_descriptor *sd = &si->sd;

    sd->revision = sd32->revision;
    sd->type = sd32->type;
    sd->owner_sid = ULongToPtr( sd32->owner_sid );
    sd->group_sid = ULongToPtr( sd32->group_sid );
    create_acl64( ULongToPtr(sd32->sacl), &si->sacl );
    sd->sacl = &si->sacl;
    create_acl64( ULongToPtr(sd32->dacl), &si->dacl );
    sd->dacl = &si->dacl;
}

struct share_info_502_32
{
    PTR32        shi502_netname;
    unsigned int shi502_type;
    PTR32        shi502_remark;
    unsigned int shi502_permissions;
    unsigned int shi502_max_uses;
    unsigned int shi502_current_uses;
    PTR32        shi502_path;
    PTR32        shi502_passwd;
    unsigned int shi502_reserved;
    PTR32        shi502_security_descriptor;
};

static NTSTATUS create_share_info64( unsigned int level, void *buffer, struct share_info *si )
{
    switch (level)
    {
    case 2:
    {
        struct share_info_2_32 *si32 = buffer;

        si->si2.shi2_netname = ULongToPtr( si32->shi2_netname );
        si->si2.shi2_type = si32->shi2_type;
        si->si2.shi2_remark = ULongToPtr( si32->shi2_remark );
        si->si2.shi2_permissions = si32->shi2_permissions;
        si->si2.shi2_max_uses = si32->shi2_max_uses;
        si->si2.shi2_current_uses = si32->shi2_current_uses;
        si->si2.shi2_path = ULongToPtr( si32->shi2_path );
        si->si2.shi2_passwd = ULongToPtr( si32->shi2_passwd );
        return STATUS_SUCCESS;
    }
    case 502:
    {
        struct share_info_502_32 *si32 = buffer;

        si->si502.shi502_netname = ULongToPtr( si32->shi502_netname );
        si->si502.shi502_type = si32->shi502_type;
        si->si502.shi502_remark = ULongToPtr( si32->shi502_remark );
        si->si502.shi502_permissions = si32->shi502_permissions;
        si->si502.shi502_max_uses = si32->shi502_max_uses;
        si->si502.shi502_current_uses = si32->shi502_current_uses;
        si->si502.shi502_path = ULongToPtr( si32->shi502_path );
        si->si502.shi502_passwd = ULongToPtr( si32->shi502_passwd );
        si->si502.shi502_reserved = si32->shi502_reserved;
        create_security_descriptor64( ULongToPtr(si32->shi502_security_descriptor), si );
        si->si502.shi502_security_descriptor = &si->sd;
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NTSTATUS wow64_share_add( void *args )
{
    struct
    {
        PTR32 server;
        DWORD level;
        PTR32 info;
        PTR32 err;
    } const *params32 = args;

    struct share_info si;
    struct share_add_params params =
    {
        ULongToPtr(params32->server),
        params32->level,
        (BYTE *)&si,
        ULongToPtr(params32->err)
    };
    NTSTATUS status;

    status = create_share_info64( params.level, ULongToPtr(params32->info), &si );
    if (!status) status = share_add( &params );
    return status;
}

static NTSTATUS wow64_share_del( void *args )
{
    struct
    {
        PTR32 server;
        PTR32 share;
        DWORD reserved;
    } const *params32 = args;

    struct share_del_params params =
    {
        ULongToPtr(params32->server),
        ULongToPtr(params32->share),
        params32->reserved
    };

    return share_del( &params );
}

struct wksta_info_100_32
{
    unsigned int wki100_platform_id;
    PTR32        wki100_computername;
    PTR32        wki100_langroup;
    unsigned int wki100_ver_major;
    unsigned int wki100_ver_minor;
};

static NTSTATUS create_wksta_info32( DWORD level, void *buffer )
{
    switch (level)
    {
    case 100:
    {
        struct wksta_info_100_32 *wi32 = buffer;
        struct wksta_info_100 *wi = buffer;

        wi32->wki100_platform_id = wi->wki100_platform_id;
        wi32->wki100_computername = PtrToUlong( wi->wki100_computername );
        wi32->wki100_langroup = PtrToUlong( wi->wki100_langroup );
        wi32->wki100_ver_major = wi->wki100_ver_major;
        wi32->wki100_ver_minor = wi->wki100_ver_minor;
        return ERROR_SUCCESS;
    }
    default:
        FIXME( "level %u not supported\n", level );
        return ERROR_NOT_SUPPORTED;
    }
}

static NTSTATUS wow64_wksta_getinfo( void *args )
{
    struct
    {
        PTR32 server;
        DWORD level;
        PTR32 buffer;
        PTR32 size;
    } const *params32 = args;

    struct wksta_getinfo_params params =
    {
        ULongToPtr(params32->server),
        params32->level,
        ULongToPtr(params32->buffer),
        ULongToPtr(params32->size)
    };
    NTSTATUS status;

    status = wksta_getinfo( &params );
    if (!status) status = create_wksta_info32( params.level, params.buffer );
    return status;
}

static NTSTATUS wow64_change_password( void *args )
{
    struct
    {
        PTR32 domain;
        PTR32 user;
        PTR32 old;
        PTR32 new;
    } const *params32 = args;

    struct change_password_params params =
    {
        ULongToPtr(params32->domain),
        ULongToPtr(params32->user),
        ULongToPtr(params32->old),
        ULongToPtr(params32->new)
    };

    return change_password( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    netapi_init,
    wow64_server_getinfo,
    wow64_share_add,
    wow64_share_del,
    wow64_wksta_getinfo,
    wow64_change_password,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */

#endif /* SONAME_LIBNETAPI */
