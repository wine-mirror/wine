/* Copyright 2001 Mike McCormack
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2003 Juan Lang
 * Copyright 2005,2006 Paul Vriens
 * Copyright 2006 Robert Reif
 * Copyright 2013 Hans Leidekker for CodeWeavers
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
#include "winbase.h"
#include "lm.h"
#include "lmaccess.h"
#include "lmat.h"
#include "lmapibuf.h"
#include "lmbrowsr.h"
#include "lmshare.h"
#include "lmwksta.h"
#include "netbios.h"
#include "ifmib.h"
#include "iphlpapi.h"
#include "ntsecapi.h"
#include "winnls.h"
#include "dsrole.h"
#include "dsgetdc.h"
#include "wine/debug.h"
#include "wine/library.h"
#include "wine/list.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

static char *strdup_unixcp( const WCHAR *str )
{
    char *ret;
    int len = WideCharToMultiByte( CP_UNIXCP, 0, str, -1, NULL, 0, NULL, NULL );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
        WideCharToMultiByte( CP_UNIXCP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

#ifdef SONAME_LIBNETAPI

static void *libnetapi_handle;
static void *libnetapi_ctx;

static DWORD (*plibnetapi_init)(void **);
static DWORD (*plibnetapi_free)(void *);
static DWORD (*plibnetapi_set_debuglevel)(void *, const char *);
static DWORD (*plibnetapi_set_username)(void *, const char *);
static DWORD (*plibnetapi_set_password)(void *, const char *);

static NET_API_STATUS (*pNetApiBufferAllocate)(unsigned int, void **);
static NET_API_STATUS (*pNetApiBufferFree)(void *);
static NET_API_STATUS (*pNetServerGetInfo)(const char *, unsigned int, unsigned char **);
static NET_API_STATUS (*pNetShareAdd)(const char *, unsigned int, unsigned char *, unsigned int *);
static NET_API_STATUS (*pNetShareDel)(const char *, const char *, unsigned int);
static NET_API_STATUS (*pNetWkstaGetInfo)(const char *, unsigned int, unsigned char **);

static void destroy_context(void)
{
    TRACE( "destroying %p\n", libnetapi_ctx );
    plibnetapi_free( libnetapi_ctx );
    libnetapi_ctx = NULL;
}

static BOOL init_context(void)
{
    DWORD status;

    if ((status = plibnetapi_init( &libnetapi_ctx )))
    {
        ERR( "Failed to initialize context %u\n", status );
        return FALSE;
    }
    if (TRACE_ON( netapi32 ) && (status = plibnetapi_set_debuglevel( libnetapi_ctx, "10" )))
    {
        ERR( "Failed to set debug level %u\n", status );
        destroy_context();
        return FALSE;
    }
    /* perform an anonymous login by default (avoids a password prompt) */
    if ((status = plibnetapi_set_username( libnetapi_ctx, "Guest" )))
    {
        ERR( "Failed to set username %u\n", status );
        destroy_context();
        return FALSE;
    }
    if ((status = plibnetapi_set_password( libnetapi_ctx, "" )))
    {
        ERR( "Failed to set password %u\n", status );
        destroy_context();
        return FALSE;
    }
    TRACE( "using %p\n", libnetapi_ctx );
    return TRUE;
}

static BOOL libnetapi_init(void)
{
    char buf[200];

    if (libnetapi_handle) return TRUE;
    if (!(libnetapi_handle = wine_dlopen( SONAME_LIBNETAPI, RTLD_NOW, buf, sizeof(buf) )))
    {
        WARN( "Failed to load libnetapi: %s\n", buf );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libnetapi_handle, #f, buf, sizeof(buf) ))) \
    { \
        ERR( "Failed to load %s: %s\n", #f, buf ); \
        goto error; \
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

    if (init_context()) return TRUE;

error:
    wine_dlclose( libnetapi_handle, NULL, 0 );
    libnetapi_handle = NULL;
    return FALSE;
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

    if (info->sv101_name) len += MultiByteToWideChar( CP_UNIXCP, 0, info->sv101_name, -1, NULL, 0 );
    if (info->sv101_comment) len += MultiByteToWideChar( CP_UNIXCP, 0, info->sv101_comment, -1, NULL, 0 );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, sizeof(*ret) + (len * sizeof(WCHAR) ))))
        return ERROR_OUTOFMEMORY;

    ptr = (WCHAR *)(ret + 1);
    ret->sv101_platform_id = info->sv101_platform_id;
    if (!info->sv101_name) ret->sv101_name = NULL;
    else
    {
        ret->sv101_name = ptr;
        ptr += MultiByteToWideChar( CP_UNIXCP, 0, info->sv101_name, -1, ptr, len );
    }
    ret->sv101_version_major = info->sv101_version_major;
    ret->sv101_version_minor = info->sv101_version_minor;
    ret->sv101_type          = info->sv101_type;
    if (!info->sv101_comment) ret->sv101_comment = NULL;
    else
    {
        ret->sv101_comment = ptr;
        MultiByteToWideChar( CP_UNIXCP, 0, info->sv101_comment, -1, ptr, len );
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

static NET_API_STATUS server_getinfo( LMSTR servername, DWORD level, LPBYTE *bufptr )
{
    NET_API_STATUS status;
    char *server = NULL;
    unsigned char *buf = NULL;

    if (servername && !(server = strdup_unixcp( servername ))) return ERROR_OUTOFMEMORY;
    status = pNetServerGetInfo( server, level, &buf );
    HeapFree( GetProcessHeap(), 0, server );
    if (!status)
    {
        status = server_info_from_samba( level, buf, bufptr );
        pNetApiBufferFree( buf );
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
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_netname, -1, NULL, 0, NULL, NULL );
    if (info->shi2_remark)
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_remark, -1, NULL, 0, NULL, NULL );
    if (info->shi2_path)
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_path, -1, NULL, 0, NULL, NULL );
    if (info->shi2_passwd)
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_passwd, -1, NULL, 0, NULL, NULL );
    if (!(ret = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret) + len )))
        return ERROR_OUTOFMEMORY;

    ptr = (char *)(ret + 1);
    if (!info->shi2_netname) ret->shi2_netname = NULL;
    else
    {
        ret->shi2_netname = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_netname, -1, ptr, len, NULL, NULL );
    }
    ret->shi2_type = info->shi2_type;
    if (!info->shi2_remark) ret->shi2_remark = NULL;
    else
    {
        ret->shi2_remark = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_remark, -1, ptr, len, NULL, NULL );
    }
    ret->shi2_permissions  = info->shi2_permissions;
    ret->shi2_max_uses     = info->shi2_max_uses;
    ret->shi2_current_uses = info->shi2_current_uses;
    if (!info->shi2_path) ret->shi2_path = NULL;
    else
    {
        ret->shi2_path = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_path, -1, ptr, len, NULL, NULL );
    }
    if (!info->shi2_passwd) ret->shi2_passwd = NULL;
    else
    {
        ret->shi2_passwd = ptr;
        WideCharToMultiByte( CP_UNIXCP, 0, info->shi2_passwd, -1, ptr, len, NULL, NULL );
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

#define GENERIC_ALL_ACCESS     (1 << 28)
#define GENERIC_EXECUTE_ACCESS (1 << 29)
#define GENERIC_WRITE_ACCESS   (1 << 30)
#define GENERIC_READ_ACCESS    (1 << 31)

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
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_netname, -1, NULL, 0, NULL, NULL );
    if (info->shi502_remark)
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_remark, -1, NULL, 0, NULL, NULL );
    if (info->shi502_path)
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_path, -1, NULL, 0, NULL, NULL );
    if (info->shi502_passwd)
        len += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_passwd, -1, NULL, 0, NULL, NULL );
    if (info->shi502_security_descriptor)
        size = sd_to_samba_size( info->shi502_security_descriptor );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, sizeof(*ret) + (len * sizeof(WCHAR)) + size )))
        return ERROR_OUTOFMEMORY;

    ptr = (char *)(ret + 1);
    if (!info->shi502_netname) ret->shi502_netname = NULL;
    else
    {
        ret->shi502_netname = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_netname, -1, ptr, len, NULL, NULL );
    }
    ret->shi502_type = info->shi502_type;
    if (!info->shi502_remark) ret->shi502_remark = NULL;
    else
    {
        ret->shi502_remark = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_remark, -1, ptr, len, NULL, NULL );
    }
    ret->shi502_permissions  = info->shi502_permissions;
    ret->shi502_max_uses     = info->shi502_max_uses;
    ret->shi502_current_uses = info->shi502_current_uses;
    if (!info->shi502_path) ret->shi502_path = NULL;
    else
    {
        ret->shi502_path = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_path, -1, ptr, len, NULL, NULL );
    }
    if (!info->shi502_passwd) ret->shi502_passwd = NULL;
    else
    {
        ret->shi502_passwd = ptr;
        ptr += WideCharToMultiByte( CP_UNIXCP, 0, info->shi502_passwd, -1, ptr, len, NULL, NULL );
    }
    ret->shi502_reserved = info->shi502_reserved;
    if (!info->shi502_security_descriptor) ret->shi502_security_descriptor = NULL;
    else
    {
        status = sd_to_samba( info->shi502_security_descriptor, (struct security_descriptor *)ptr );
        if (status)
        {
            HeapFree( GetProcessHeap(), 0, ret );
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

static NET_API_STATUS share_add( LMSTR servername, DWORD level, LPBYTE buf, LPDWORD parm_err )
{
    char *server = NULL;
    unsigned char *info;
    NET_API_STATUS status;

    if (servername && !(server = strdup_unixcp( servername ))) return ERROR_OUTOFMEMORY;
    status = share_info_to_samba( level, buf, &info );
    if (!status)
    {
        unsigned int err;

        status = pNetShareAdd( server, level, info, &err );
        HeapFree( GetProcessHeap(), 0, info );
        if (parm_err) *parm_err = err;
    }
    HeapFree( GetProcessHeap(), 0, server );
    return status;
}

static NET_API_STATUS share_del( LMSTR servername, LMSTR netname, DWORD reserved )
{
    char *server = NULL, *share;
    NET_API_STATUS status;

    if (servername && !(server = strdup_unixcp( servername ))) return ERROR_OUTOFMEMORY;
    if (!(share = strdup_unixcp( netname )))
    {
        HeapFree( GetProcessHeap(), 0, server );
        return ERROR_OUTOFMEMORY;
    }
    status = pNetShareDel( server, share, reserved );
    HeapFree( GetProcessHeap(), 0, server );
    HeapFree( GetProcessHeap(), 0, share );
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
        len += MultiByteToWideChar( CP_UNIXCP, 0, info->wki100_computername, -1, NULL, 0 );
    if (info->wki100_langroup)
        len += MultiByteToWideChar( CP_UNIXCP, 0, info->wki100_langroup, -1, NULL, 0 );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, sizeof(*ret) + (len * sizeof(WCHAR) ))))
        return ERROR_OUTOFMEMORY;

    ptr = (WCHAR *)(ret + 1);
    ret->wki100_platform_id = info->wki100_platform_id;
    if (!info->wki100_computername) ret->wki100_computername = NULL;
    else
    {
        ret->wki100_computername = ptr;
        ptr += MultiByteToWideChar( CP_UNIXCP, 0, info->wki100_computername, -1, ptr, len );
    }
    if (!info->wki100_langroup) ret->wki100_langroup = NULL;
    else
    {
        ret->wki100_langroup = ptr;
        MultiByteToWideChar( CP_UNIXCP, 0, info->wki100_langroup, -1, ptr, len );
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

static NET_API_STATUS wksta_getinfo( LMSTR servername, DWORD level, LPBYTE *bufptr )
{
    NET_API_STATUS status;
    char *wksta = NULL;
    unsigned char *buf = NULL;

    if (servername && !(wksta = strdup_unixcp( servername ))) return ERROR_OUTOFMEMORY;
    status = pNetWkstaGetInfo( wksta, level, &buf );
    HeapFree( GetProcessHeap(), 0, wksta );
    if (!status)
    {
        status = wksta_info_from_samba( level, buf, bufptr );
        pNetApiBufferFree( buf );
    }
    return status;
}

#else

static BOOL libnetapi_init(void)
{
    return FALSE;
}

static NET_API_STATUS server_getinfo( LMSTR servername, DWORD level, LPBYTE *bufptr )
{
    ERR( "\n" );
    return ERROR_NOT_SUPPORTED;
}
static NET_API_STATUS share_add( LMSTR servername, DWORD level, LPBYTE buf, LPDWORD parm_err )
{
    ERR( "\n" );
    return ERROR_NOT_SUPPORTED;
}
static NET_API_STATUS share_del( LMSTR servername, LMSTR netname, DWORD reserved )
{
    ERR( "\n" );
    return ERROR_NOT_SUPPORTED;
}
static NET_API_STATUS wksta_getinfo(  LMSTR servername, DWORD level, LPBYTE *bufptr )
{
    ERR( "\n" );
    return ERROR_NOT_SUPPORTED;
}

#endif /* SONAME_LIBNETAPI */

/************************************************************
 *                NETAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
static BOOL NETAPI_IsLocalComputer( LMCSTR name )
{
    WCHAR buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(buf) / sizeof(buf[0]);
    BOOL ret;

    if (!name || !name[0]) return TRUE;

    ret = GetComputerNameW( buf,  &size );
    if (ret && name[0] == '\\' && name[1] == '\\') name += 2;
    return ret && !strcmpiW( name, buf );
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            NetBIOSInit();
            NetBTInit();
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            NetBIOSShutdown();
            break;
    }

    return TRUE;
}

/************************************************************
 *                NetServerEnum (NETAPI32.@)
 */
NET_API_STATUS  WINAPI NetServerEnum(
  LMCSTR servername,
  DWORD level,
  LPBYTE* bufptr,
  DWORD prefmaxlen,
  LPDWORD entriesread,
  LPDWORD totalentries,
  DWORD servertype,
  LMCSTR domain,
  LPDWORD resume_handle
)
{
    FIXME("Stub (%s %d %p %d %p %p %d %s %p)\n", debugstr_w(servername),
     level, bufptr, prefmaxlen, entriesread, totalentries, servertype,
     debugstr_w(domain), resume_handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerEnumEx (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerEnumEx(
    LMCSTR ServerName,
    DWORD Level,
    LPBYTE *Bufptr,
    DWORD PrefMaxlen,
    LPDWORD EntriesRead,
    LPDWORD totalentries,
    DWORD servertype,
    LMCSTR domain,
    LMCSTR FirstNameToReturn)
{
    FIXME("Stub (%s %d %p %d %p %p %d %s %s)\n",
           debugstr_w(ServerName), Level, Bufptr, PrefMaxlen, EntriesRead, totalentries,
           servertype, debugstr_w(domain), debugstr_w(FirstNameToReturn));

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerDiskEnum (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerDiskEnum(
    LMCSTR ServerName,
    DWORD Level,
    LPBYTE *Bufptr,
    DWORD PrefMaxlen,
    LPDWORD EntriesRead,
    LPDWORD totalentries,
    LPDWORD Resume_Handle)
{
    FIXME("Stub (%s %d %p %d %p %p %p)\n", debugstr_w(ServerName),
     Level, Bufptr, PrefMaxlen, EntriesRead, totalentries, Resume_Handle);

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/************************************************************
 *                NetServerGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetServerGetInfo(LMSTR servername, DWORD level, LPBYTE* bufptr)
{
    NET_API_STATUS ret;
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %d %p\n", debugstr_w( servername ), level, bufptr );

    if (!local)
    {
        if (libnetapi_init()) return server_getinfo( servername, level, bufptr );
        FIXME( "remote computers not supported\n" );
        return ERROR_INVALID_LEVEL;
    }
    if (!bufptr) return ERROR_INVALID_PARAMETER;

    switch (level)
    {
        case 100:
        case 101:
        {
            DWORD computerNameLen, size;
            WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];

            computerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerNameW(computerName, &computerNameLen);
            computerNameLen++; /* include NULL terminator */

            size = sizeof(SERVER_INFO_101) + computerNameLen * sizeof(WCHAR);
            ret = NetApiBufferAllocate(size, (LPVOID *)bufptr);
            if (ret == NERR_Success)
            {
                /* INFO_100 structure is a subset of INFO_101 */
                PSERVER_INFO_101 info = (PSERVER_INFO_101)*bufptr;
                OSVERSIONINFOW verInfo;

                info->sv101_platform_id = PLATFORM_ID_NT;
                info->sv101_name = (LMSTR)(*bufptr + sizeof(SERVER_INFO_101));
                memcpy(info->sv101_name, computerName,
                       computerNameLen * sizeof(WCHAR));
                verInfo.dwOSVersionInfoSize = sizeof(verInfo);
                GetVersionExW(&verInfo);
                info->sv101_version_major = verInfo.dwMajorVersion;
                info->sv101_version_minor = verInfo.dwMinorVersion;
                 /* Use generic type as no wine equivalent of DC / Server */
                info->sv101_type = SV_TYPE_NT;
                info->sv101_comment = NULL;
            }
            break;
        }

        default:
            FIXME("level %d unimplemented\n", level);
            ret = ERROR_INVALID_LEVEL;
    }
    return ret;
}


/************************************************************
 *                NetStatisticsGet  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetStatisticsGet(LMSTR server, LMSTR service,
                                       DWORD level, DWORD options,
                                       LPBYTE *bufptr)
{
    TRACE("(%p, %p, %d, %d, %p)\n", server, service, level, options, bufptr);
    return NERR_InternalError;
}

NET_API_STATUS WINAPI NetUseEnum(LMSTR server, DWORD level, LPBYTE* bufptr, DWORD prefmaxsize,
                          LPDWORD entriesread, LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("stub (%p, %d, %p, %d, %p, %p, %p)\n", server, level, bufptr, prefmaxsize,
           entriesread, totalentries, resumehandle);
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS WINAPI NetScheduleJobAdd(LPCWSTR server, LPBYTE bufptr, LPDWORD jobid)
{
    FIXME("stub (%s, %p, %p)\n", debugstr_w(server), bufptr, jobid);
    return NERR_Success;
}

NET_API_STATUS WINAPI NetScheduleJobDel(LPCWSTR server, DWORD minjobid, DWORD maxjobid)
{
    FIXME("stub (%s, %d, %d)\n", debugstr_w(server), minjobid, maxjobid);
    return NERR_Success;
}

NET_API_STATUS WINAPI NetScheduleJobEnum(LPCWSTR server, LPBYTE* bufptr, DWORD prefmaxsize, LPDWORD entriesread,
                                         LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("stub (%s, %p, %d, %p, %p, %p)\n", debugstr_w(server), bufptr, prefmaxsize, entriesread, totalentries, resumehandle);
    *entriesread = 0;
    *totalentries = 0;
    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseGetInfo(LMSTR server, LMSTR name, DWORD level, LPBYTE *bufptr)
{
    FIXME("stub (%p, %p, %d, %p)\n", server, name, level, bufptr);
    return ERROR_NOT_SUPPORTED;

}

/************************************************************
 *                NetApiBufferAllocate  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferAllocate(DWORD ByteCount, LPVOID* Buffer)
{
    TRACE("(%d, %p)\n", ByteCount, Buffer);

    if (Buffer == NULL) return ERROR_INVALID_PARAMETER;
    *Buffer = HeapAlloc(GetProcessHeap(), 0, ByteCount);
    if (*Buffer)
        return NERR_Success;
    else
        return GetLastError();
}

/************************************************************
 *                NetApiBufferFree  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferFree(LPVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
    return NERR_Success;
}

/************************************************************
 *                NetApiBufferReallocate  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferReallocate(LPVOID OldBuffer, DWORD NewByteCount,
                                             LPVOID* NewBuffer)
{
    TRACE("(%p, %d, %p)\n", OldBuffer, NewByteCount, NewBuffer);
    if (NewByteCount)
    {
        if (OldBuffer)
            *NewBuffer = HeapReAlloc(GetProcessHeap(), 0, OldBuffer, NewByteCount);
        else
            *NewBuffer = HeapAlloc(GetProcessHeap(), 0, NewByteCount);
	return *NewBuffer ? NERR_Success : GetLastError();
    }
    else
    {
	if (!HeapFree(GetProcessHeap(), 0, OldBuffer)) return GetLastError();
	*NewBuffer = 0;
	return NERR_Success;
    }
}

/************************************************************
 *                NetApiBufferSize  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferSize(LPVOID Buffer, LPDWORD ByteCount)
{
    DWORD dw;

    TRACE("(%p, %p)\n", Buffer, ByteCount);
    if (Buffer == NULL)
        return ERROR_INVALID_PARAMETER;
    dw = HeapSize(GetProcessHeap(), 0, Buffer);
    TRACE("size: %d\n", dw);
    if (dw != 0xFFFFFFFF)
        *ByteCount = dw;
    else
        *ByteCount = 0;

    return NERR_Success;
}

/************************************************************
 * NetSessionEnum  (NETAPI32.@)
 *
 * PARAMS
 *   servername    [I]   Pointer to a string with the name of the server
 *   UncClientName [I]   Pointer to a string with the name of the session
 *   username      [I]   Pointer to a string with the name of the user
 *   level         [I]   Data information level
 *   bufptr        [O]   Buffer to the data
 *   prefmaxlen    [I]   Preferred maximum length of the data
 *   entriesread   [O]   Pointer to the number of entries enumerated
 *   totalentries  [O]   Pointer to the possible number of entries
 *   resume_handle [I/O] Pointer to a handle for subsequent searches
 *
 * RETURNS
 *   If successful, the function returns NERR_Success
 *   On failure it returns:
 *     ERROR_ACCESS_DENIED         User has no access to the requested information
 *     ERROR_INVALID_LEVEL         Value of 'level' is not correct
 *     ERROR_INVALID_PARAMETER     Wrong parameter
 *     ERROR_MORE_DATA             Need a larger buffer
 *     ERROR_NOT_ENOUGH_MEMORY     Not enough memory
 *     NERR_ClientNameNotFound     A session does not exist on a given computer
 *     NERR_InvalidComputer        Invalid computer name
 *     NERR_UserNotFound           User name could not be found.
 */
NET_API_STATUS WINAPI NetSessionEnum(LMSTR servername, LMSTR UncClientName,
    LMSTR username, DWORD level, LPBYTE* bufptr, DWORD prefmaxlen, LPDWORD entriesread,
    LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("Stub (%s %s %s %d %p %d %p %p %p)\n", debugstr_w(servername),
        debugstr_w(UncClientName), debugstr_w(username),
        level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

    return NERR_Success;
}

/************************************************************
 * NetShareEnum  (NETAPI32.@)
 *
 * PARAMS
 *   servername    [I]   Pointer to a string with the name of the server
 *   level         [I]   Data information level
 *   bufptr        [O]   Buffer to the data
 *   prefmaxlen    [I]   Preferred maximum length of the data
 *   entriesread   [O]   Pointer to the number of entries enumerated
 *   totalentries  [O]   Pointer to the possible number of entries
 *   resume_handle [I/O] Pointer to a handle for subsequent searches
 *
 * RETURNS
 *   If successful, the function returns NERR_Success
 *   On failure it returns a system error code (FIXME: find out which)
 *
 */
NET_API_STATUS WINAPI NetShareEnum( LMSTR servername, DWORD level, LPBYTE* bufptr,
    DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("Stub (%s %d %p %d %p %p %p)\n", debugstr_w(servername), level, bufptr,
        prefmaxlen, entriesread, totalentries, resume_handle);

    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 * NetShareDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareDel(LMSTR servername, LMSTR netname, DWORD reserved)
{
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %s %d\n", debugstr_w(servername), debugstr_w(netname), reserved);

    if (!local)
    {
        if (libnetapi_init()) return share_del( servername, netname, reserved );
        FIXME( "remote computers not supported\n" );
    }

    FIXME("%s %s %d\n", debugstr_w(servername), debugstr_w(netname), reserved);
    return NERR_Success;
}

/************************************************************
 * NetShareGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareGetInfo(LMSTR servername, LMSTR netname,
    DWORD level, LPBYTE *bufptr)
{
    FIXME("Stub (%s %s %d %p)\n", debugstr_w(servername),
        debugstr_w(netname),level, bufptr);
    return NERR_NetNameNotFound;
}

/************************************************************
 * NetShareAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareAdd(LMSTR servername,
    DWORD level, LPBYTE buf, LPDWORD parm_err)
{
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %d %p %p\n", debugstr_w(servername), level, buf, parm_err);

    if (!local)
    {
        if (libnetapi_init()) return share_add( servername, level, buf, parm_err );
        FIXME( "remote computers not supported\n" );
    }

    FIXME("%s %d %p %p\n", debugstr_w(servername), level, buf, parm_err);
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                NetFileEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetFileEnum(
    LPWSTR ServerName, LPWSTR BasePath, LPWSTR UserName,
    DWORD Level, LPBYTE* BufPtr, DWORD PrefMaxLen,
    LPDWORD EntriesRead, LPDWORD TotalEntries, PDWORD_PTR ResumeHandle)
{
    FIXME("(%s, %s, %s, %u): stub\n", debugstr_w(ServerName), debugstr_w(BasePath),
        debugstr_w(UserName), Level);
    return ERROR_NOT_SUPPORTED;
}

static void wprint_mac(WCHAR* buffer, int len, const MIB_IFROW *ifRow)
{
    int i;
    unsigned char val;

    if (!buffer)
        return;
    if (len < 1)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    for (i = 0; i < ifRow->dwPhysAddrLen && 2 * i < len; i++)
    {
        val = ifRow->bPhysAddr[i];
        if ((val >>4) >9)
            buffer[2*i] = (WCHAR)((val >>4) + 'A' - 10);
        else
            buffer[2*i] = (WCHAR)((val >>4) + '0');
        if ((val & 0xf ) >9)
            buffer[2*i+1] = (WCHAR)((val & 0xf) + 'A' - 10);
        else
            buffer[2*i+1] = (WCHAR)((val & 0xf) + '0');
    }
    buffer[2*i]=0;
}

/* Theoretically this could be too short, except that MS defines
 * MAX_ADAPTER_NAME as 128, and MAX_INTERFACE_NAME_LEN as 256, and both
 * represent a count of WCHARs, so even with an extraordinarily long header
 * this will be plenty
 */
#define MAX_TRANSPORT_NAME MAX_INTERFACE_NAME_LEN
#define MAX_TRANSPORT_ADDR 13

#define NBT_TRANSPORT_NAME_HEADER "\\Device\\NetBT_Tcpip_"
#define UNKNOWN_TRANSPORT_NAME_HEADER "\\Device\\UnknownTransport_"

static void wprint_name(WCHAR *buffer, int len, ULONG transport,
 PMIB_IFROW ifRow)
{
    WCHAR *ptr1, *ptr2;
    const char *name;

    if (!buffer)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
        name = NBT_TRANSPORT_NAME_HEADER;
    else
        name = UNKNOWN_TRANSPORT_NAME_HEADER;

    for (ptr1 = buffer; *name && ptr1 < buffer + len; ptr1++, name++)
        *ptr1 = *name;
    for (ptr2 = ifRow->wszName; *ptr2 && ptr1 < buffer + len; ptr1++, ptr2++)
        *ptr1 = *ptr2;
    *ptr1 = '\0';
}

/***********************************************************************
 *                NetWkstaTransportEnum  (NETAPI32.@)
 */

struct WkstaTransportEnumData
{
    UCHAR          n_adapt;
    UCHAR          n_read;
    DWORD          prefmaxlen;
    LPBYTE        *pbuf;
    NET_API_STATUS ret;
};

/**********************************************************************/

static BOOL WkstaEnumAdaptersCallback(UCHAR totalLANAs, UCHAR lanaIndex,
 ULONG transport, const NetBIOSAdapterImpl *data, void *closure)
{
    BOOL ret;
    struct WkstaTransportEnumData *enumData = closure;

    if (enumData && enumData->pbuf)
    {
        if (lanaIndex == 0)
        {
            DWORD toAllocate;

            enumData->n_adapt = totalLANAs;
            enumData->n_read = 0;

            toAllocate = totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0)
             + MAX_TRANSPORT_NAME * sizeof(WCHAR) +
             MAX_TRANSPORT_ADDR * sizeof(WCHAR));
            if (enumData->prefmaxlen != MAX_PREFERRED_LENGTH)
                toAllocate = enumData->prefmaxlen;
            NetApiBufferAllocate(toAllocate, (LPVOID *)enumData->pbuf);
        }
        if (*(enumData->pbuf))
        {
            UCHAR spaceFor;

            if (enumData->prefmaxlen == MAX_PREFERRED_LENGTH)
                spaceFor = totalLANAs;
            else
                spaceFor = enumData->prefmaxlen /
                 (sizeof(WKSTA_TRANSPORT_INFO_0) + (MAX_TRANSPORT_NAME +
                 MAX_TRANSPORT_ADDR) * sizeof(WCHAR));
            if (enumData->n_read < spaceFor)
            {
                PWKSTA_TRANSPORT_INFO_0 ti;
                LMSTR transport_name, transport_addr;
                MIB_IFROW ifRow;

                ti = (PWKSTA_TRANSPORT_INFO_0)(*(enumData->pbuf) +
                 enumData->n_read * sizeof(WKSTA_TRANSPORT_INFO_0));
                transport_name = (LMSTR)(*(enumData->pbuf) +
                 totalLANAs * sizeof(WKSTA_TRANSPORT_INFO_0) +
                 enumData->n_read * MAX_TRANSPORT_NAME * sizeof(WCHAR));
                transport_addr = (LMSTR)(*(enumData->pbuf) +
                 totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0) +
                 MAX_TRANSPORT_NAME * sizeof(WCHAR)) +
                 enumData->n_read * MAX_TRANSPORT_ADDR * sizeof(WCHAR));

                ifRow.dwIndex = data->ifIndex;
                GetIfEntry(&ifRow);
                ti->wkti0_quality_of_service = 0;
                ti->wkti0_number_of_vcs = 0;
                ti->wkti0_transport_name = transport_name;
                wprint_name(ti->wkti0_transport_name, MAX_TRANSPORT_NAME,
                 transport, &ifRow);
                ti->wkti0_transport_address = transport_addr;
                wprint_mac(ti->wkti0_transport_address, MAX_TRANSPORT_ADDR,
                 &ifRow);
                if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
                    ti->wkti0_wan_ish = TRUE;
                else
                    ti->wkti0_wan_ish = FALSE;
                TRACE("%d of %d:ti at %p\n", lanaIndex, totalLANAs, ti);
                TRACE("transport_name at %p %s\n",
                 ti->wkti0_transport_name,
                 debugstr_w(ti->wkti0_transport_name));
                TRACE("transport_address at %p %s\n",
                 ti->wkti0_transport_address,
                 debugstr_w(ti->wkti0_transport_address));
                enumData->n_read++;
                enumData->ret = NERR_Success;
                ret = TRUE;
            }
            else
            {
                enumData->ret = ERROR_MORE_DATA;
                ret = FALSE;
            }
        }
        else
        {
            enumData->ret = ERROR_OUTOFMEMORY;
            ret = FALSE;
        }
    }
    else
        ret = FALSE;
    return ret;
}

/**********************************************************************/

NET_API_STATUS WINAPI
NetWkstaTransportEnum(LMSTR ServerName, DWORD level, PBYTE* pbuf,
      DWORD prefmaxlen, LPDWORD read_entries,
      PDWORD total_entries, PDWORD hresume)
{
    NET_API_STATUS ret;

    TRACE(":%s, 0x%08x, %p, 0x%08x, %p, %p, %p\n", debugstr_w(ServerName),
     level, pbuf, prefmaxlen, read_entries, total_entries,hresume);
    if (!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME(":not implemented for non-local computers\n");
        ret = ERROR_INVALID_LEVEL;
    }
    else
    {
        if (hresume && *hresume)
        {
          FIXME(":resume handle not implemented\n");
          return ERROR_INVALID_LEVEL;
        }

        switch (level)
        {
            case 0: /* transport info */
            {
                ULONG allTransports;
                struct WkstaTransportEnumData enumData;

                if (NetBIOSNumAdapters() == 0)
                  return ERROR_NETWORK_UNREACHABLE;
                if (!read_entries)
                  return STATUS_ACCESS_VIOLATION;
                if (!total_entries || !pbuf)
                  return RPC_X_NULL_REF_POINTER;

                enumData.prefmaxlen = prefmaxlen;
                enumData.pbuf = pbuf;
                memcpy(&allTransports, ALL_TRANSPORTS, sizeof(ULONG));
                NetBIOSEnumAdapters(allTransports, WkstaEnumAdaptersCallback,
                 &enumData);
                *read_entries = enumData.n_read;
                *total_entries = enumData.n_adapt;
                if (hresume) *hresume= 0;
                ret = enumData.ret;
                break;
            }
            default:
                TRACE("Invalid level %d is specified\n", level);
                ret = ERROR_INVALID_LEVEL;
        }
    }
    return ret;
}

/************************************************************
 *                NetWkstaUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetWkstaUserGetInfo(LMSTR reserved, DWORD level,
                                          PBYTE* bufptr)
{
    NET_API_STATUS nastatus;

    TRACE("(%s, %d, %p)\n", debugstr_w(reserved), level, bufptr);
    switch (level)
    {
    case 0:
    {
        PWKSTA_USER_INFO_0 ui;
        DWORD dwSize = UNLEN + 1;

        /* set up buffer */
        nastatus = NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_0) + dwSize * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        if (nastatus != NERR_Success)
            return ERROR_NOT_ENOUGH_MEMORY;

        ui = (PWKSTA_USER_INFO_0) *bufptr;
        ui->wkui0_username = (LMSTR) (*bufptr + sizeof(WKSTA_USER_INFO_0));

        /* get data */
        if (!GetUserNameW(ui->wkui0_username, &dwSize))
        {
            NetApiBufferFree(ui);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            nastatus = NetApiBufferReallocate(
                *bufptr, sizeof(WKSTA_USER_INFO_0) +
                (lstrlenW(ui->wkui0_username) + 1) * sizeof(WCHAR),
                (LPVOID *) bufptr);
            if (nastatus != NERR_Success)
                return nastatus;
        }
        break;
    }

    case 1:
    {
        PWKSTA_USER_INFO_1 ui;
        PWKSTA_USER_INFO_0 ui0;
        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        LSA_HANDLE PolicyHandle;
        PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
        NTSTATUS NtStatus;

        /* sizes of the field buffers in WCHARS */
        int username_sz, logon_domain_sz, oth_domains_sz, logon_server_sz;

        FIXME("Level 1 processing is partially implemented\n");
        oth_domains_sz = 1;
        logon_server_sz = 1;

        /* get some information first to estimate size of the buffer */
        ui0 = NULL;
        nastatus = NetWkstaUserGetInfo(NULL, 0, (PBYTE *) &ui0);
        if (nastatus != NERR_Success)
            return nastatus;
        username_sz = lstrlenW(ui0->wkui0_username) + 1;

        ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
                                 POLICY_VIEW_LOCAL_INFORMATION,
                                 &PolicyHandle);
        if (NtStatus != STATUS_SUCCESS)
        {
            TRACE("LsaOpenPolicyFailed with NT status %x\n",
                LsaNtStatusToWinError(NtStatus));
            NetApiBufferFree(ui0);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation,
                                  (PVOID*) &DomainInfo);
        logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
        LsaClose(PolicyHandle);

        /* set up buffer */
        nastatus = NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1) +
                             (username_sz + logon_domain_sz +
                              oth_domains_sz + logon_server_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        if (nastatus != NERR_Success) {
            NetApiBufferFree(ui0);
            return nastatus;
        }
        ui = (WKSTA_USER_INFO_1 *) *bufptr;
        ui->wkui1_username = (LMSTR) (*bufptr + sizeof(WKSTA_USER_INFO_1));
        ui->wkui1_logon_domain = (LMSTR) (
            ((PBYTE) ui->wkui1_username) + username_sz * sizeof(WCHAR));
        ui->wkui1_oth_domains = (LMSTR) (
            ((PBYTE) ui->wkui1_logon_domain) +
            logon_domain_sz * sizeof(WCHAR));
        ui->wkui1_logon_server = (LMSTR) (
            ((PBYTE) ui->wkui1_oth_domains) +
            oth_domains_sz * sizeof(WCHAR));

        /* get data */
        lstrcpyW(ui->wkui1_username, ui0->wkui0_username);
        NetApiBufferFree(ui0);

        lstrcpynW(ui->wkui1_logon_domain, DomainInfo->DomainName.Buffer,
                logon_domain_sz);
        LsaFreeMemory(DomainInfo);

        /* FIXME. Not implemented. Populated with empty strings */
        ui->wkui1_oth_domains[0] = 0;
        ui->wkui1_logon_server[0] = 0;
        break;
    }
    case 1101:
    {
        PWKSTA_USER_INFO_1101 ui;
        DWORD dwSize = 1;

        FIXME("Stub. Level 1101 processing is not implemented\n");
        /* FIXME see also wkui1_oth_domains for level 1 */

        /* set up buffer */
        nastatus = NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1101) + dwSize * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        if (nastatus != NERR_Success)
            return nastatus;
        ui = (PWKSTA_USER_INFO_1101) *bufptr;
        ui->wkui1101_oth_domains = (LMSTR)(ui + 1);

        /* get data */
        ui->wkui1101_oth_domains[0] = 0;
        break;
    }
    default:
        TRACE("Invalid level %d is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetWkstaUserEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetWkstaUserEnum(LMSTR servername, DWORD level, LPBYTE* bufptr,
                 DWORD prefmaxlen, LPDWORD entriesread,
                 LPDWORD totalentries, LPDWORD resumehandle)
{
    FIXME("(%s, %d, %p, %d, %p, %p, %p): stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);
    return ERROR_INVALID_PARAMETER;
}

/************************************************************
 *                NetpGetComputerName  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetpGetComputerName(LPWSTR *Buffer)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    TRACE("(%p)\n", Buffer);
    NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) Buffer);
    if (GetComputerNameW(*Buffer,  &dwSize))
    {
        return NetApiBufferReallocate(
            *Buffer, (dwSize + 1) * sizeof(WCHAR),
            (LPVOID *) Buffer);
    }
    else
    {
        NetApiBufferFree(*Buffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

NET_API_STATUS WINAPI I_NetNameCompare(LPVOID p1, LPWSTR wkgrp, LPWSTR comp,
 LPVOID p4, LPVOID p5)
{
    FIXME("(%p %s %s %p %p): stub\n", p1, debugstr_w(wkgrp), debugstr_w(comp),
     p4, p5);
    return ERROR_INVALID_PARAMETER;
}

NET_API_STATUS WINAPI I_NetNameValidate(LPVOID p1, LPWSTR wkgrp, LPVOID p3,
 LPVOID p4)
{
    FIXME("(%p %s %p %p): stub\n", p1, debugstr_w(wkgrp), p3, p4);
    return ERROR_INVALID_PARAMETER;
}

NET_API_STATUS WINAPI NetWkstaGetInfo( LMSTR servername, DWORD level,
                                       LPBYTE* bufptr)
{
    NET_API_STATUS ret;
    BOOL local = NETAPI_IsLocalComputer( servername );

    TRACE("%s %d %p\n", debugstr_w( servername ), level, bufptr );

    if (!local)
    {
        if (libnetapi_init()) return wksta_getinfo( servername, level, bufptr );
        FIXME( "remote computers not supported\n" );
        return ERROR_INVALID_LEVEL;
    }
    if (!bufptr) return ERROR_INVALID_PARAMETER;

    switch (level)
    {
        case 100:
        case 101:
        case 102:
        {
            static const WCHAR lanroot[] = {'c',':','\\','l','a','n','m','a','n',0};  /* FIXME */
            DWORD computerNameLen, domainNameLen, size;
            WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            NTSTATUS NtStatus;

            computerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerNameW(computerName, &computerNameLen);
            computerNameLen++; /* include NULL terminator */

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
                ret = LsaNtStatusToWinError(NtStatus);
            else
            {
                PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;

                LsaQueryInformationPolicy(PolicyHandle,
                 PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
                domainNameLen = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
                size = sizeof(WKSTA_INFO_102) + computerNameLen * sizeof(WCHAR)
                    + domainNameLen * sizeof(WCHAR) + sizeof(lanroot);
                ret = NetApiBufferAllocate(size, (LPVOID *)bufptr);
                if (ret == NERR_Success)
                {
                    /* INFO_100 and INFO_101 structures are subsets of INFO_102 */
                    PWKSTA_INFO_102 info = (PWKSTA_INFO_102)*bufptr;
                    OSVERSIONINFOW verInfo;

                    info->wki102_platform_id = PLATFORM_ID_NT;
                    info->wki102_computername = (LMSTR)(*bufptr +
                     sizeof(WKSTA_INFO_102));
                    memcpy(info->wki102_computername, computerName,
                     computerNameLen * sizeof(WCHAR));
                    info->wki102_langroup = info->wki102_computername + computerNameLen;
                    memcpy(info->wki102_langroup, DomainInfo->DomainName.Buffer,
                     domainNameLen * sizeof(WCHAR));
                    info->wki102_lanroot = info->wki102_langroup + domainNameLen;
                    memcpy(info->wki102_lanroot, lanroot, sizeof(lanroot));
                    memset(&verInfo, 0, sizeof(verInfo));
                    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
                    GetVersionExW(&verInfo);
                    info->wki102_ver_major = verInfo.dwMajorVersion;
                    info->wki102_ver_minor = verInfo.dwMinorVersion;
                    info->wki102_logged_on_users = 1;
                }
                LsaFreeMemory(DomainInfo);
                LsaClose(PolicyHandle);
            }
            break;
        }

        default:
            FIXME("level %d unimplemented\n", level);
            ret = ERROR_INVALID_LEVEL;
    }
    return ret;
}

/************************************************************
 *                NetGetJoinInformation (NETAPI32.@)
 */
NET_API_STATUS NET_API_FUNCTION NetGetJoinInformation(
    LPCWSTR Server,
    LPWSTR *Name,
    PNETSETUP_JOIN_STATUS type)
{
    static const WCHAR workgroupW[] = {'W','o','r','k','g','r','o','u','p',0};

    FIXME("Semi-stub %s %p %p\n", wine_dbgstr_w(Server), Name, type);

    if (!Name || !type)
        return ERROR_INVALID_PARAMETER;

    NetApiBufferAllocate(sizeof(workgroupW), (LPVOID *)Name);
    lstrcpyW(*Name, workgroupW);
    *type = NetSetupWorkgroupName;

    return NERR_Success;
}

/************************************************************
 *                NetUserGetGroups (NETAPI32.@)
 */
NET_API_STATUS NET_API_FUNCTION NetUserGetGroups(
        LPCWSTR servername,
        LPCWSTR username,
        DWORD level,
        LPBYTE *bufptr,
        DWORD prefixmaxlen,
        LPDWORD entriesread,
        LPDWORD totalentries)
{
    FIXME("%s %s %d %p %d %p %p stub\n", debugstr_w(servername),
          debugstr_w(username), level, bufptr, prefixmaxlen, entriesread,
          totalentries);

    *bufptr = NULL;
    *entriesread = 0;
    *totalentries = 0;

    return ERROR_INVALID_LEVEL;
}

struct sam_user
{
    struct list entry;
    WCHAR user_name[LM20_UNLEN+1];
    WCHAR user_password[PWLEN + 1];
    DWORD sec_since_passwd_change;
    DWORD user_priv;
    LPWSTR home_dir;
    LPWSTR user_comment;
    DWORD user_flags;
    LPWSTR user_logon_script_path;
};

static struct list user_list = LIST_INIT( user_list );

/************************************************************
 *                NETAPI_ValidateServername
 *
 * Validates server name
 */
static NET_API_STATUS NETAPI_ValidateServername(LPCWSTR ServerName)
{
    if (ServerName)
    {
        if (ServerName[0] == 0)
            return ERROR_BAD_NETPATH;
        else if (
            ((ServerName[0] == '\\') &&
             (ServerName[1] != '\\'))
            ||
            ((ServerName[0] == '\\') &&
             (ServerName[1] == '\\') &&
             (ServerName[2] == 0))
            )
            return ERROR_INVALID_NAME;
    }
    return NERR_Success;
}

/************************************************************
 *                NETAPI_FindUser
 *
 * Looks for a user in the user database.
 * Returns a pointer to the entry in the user list when the user
 * is found, NULL otherwise.
 */
static struct sam_user* NETAPI_FindUser(LPCWSTR UserName)
{
    struct sam_user *user;

    LIST_FOR_EACH_ENTRY(user, &user_list, struct sam_user, entry)
    {
        if(lstrcmpW(user->user_name, UserName) == 0)
            return user;
    }
    return NULL;
}

static BOOL NETAPI_IsCurrentUser(LPCWSTR username)
{
    LPWSTR curr_user = NULL;
    DWORD dwSize;
    BOOL ret = FALSE;

    dwSize = LM20_UNLEN+1;
    curr_user = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    if(!curr_user)
    {
        ERR("Failed to allocate memory for user name.\n");
        goto end;
    }
    if(!GetUserNameW(curr_user, &dwSize))
    {
        ERR("Failed to get current user's user name.\n");
        goto end;
    }
    if (!lstrcmpW(curr_user, username))
    {
        ret = TRUE;
    }

end:
    HeapFree(GetProcessHeap(), 0, curr_user);
    return ret;
}

/************************************************************
 *                NetUserAdd (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserAdd(LPCWSTR servername,
                  DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    NET_API_STATUS status;
    struct sam_user * su = NULL;

    FIXME("(%s, %d, %p, %p) stub!\n", debugstr_w(servername), level, bufptr, parm_err);

    if((status = NETAPI_ValidateServername(servername)) != NERR_Success)
        return status;

    switch(level)
    {
    /* Level 3 and 4 are identical for the purposes of NetUserAdd */
    case 4:
    case 3:
        FIXME("Level 3 and 4 not implemented.\n");
        /* Fall through */
    case 2:
        FIXME("Level 2 not implemented.\n");
        /* Fall through */
    case 1:
    {
        PUSER_INFO_1 ui = (PUSER_INFO_1) bufptr;
        su = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sam_user));
        if(!su)
        {
            status = NERR_InternalError;
            break;
        }

        if(lstrlenW(ui->usri1_name) > LM20_UNLEN)
        {
            status = NERR_BadUsername;
            break;
        }

        /*FIXME: do other checks for a valid username */
        lstrcpyW(su->user_name, ui->usri1_name);

        if(lstrlenW(ui->usri1_password) > PWLEN)
        {
            /* Always return PasswordTooShort on invalid passwords. */
            status = NERR_PasswordTooShort;
            break;
        }
        lstrcpyW(su->user_password, ui->usri1_password);

        su->sec_since_passwd_change = ui->usri1_password_age;
        su->user_priv = ui->usri1_priv;
        su->user_flags = ui->usri1_flags;

        /*FIXME: set the other LPWSTRs to NULL for now */
        su->home_dir = NULL;
        su->user_comment = NULL;
        su->user_logon_script_path = NULL;

        list_add_head(&user_list, &su->entry);
        return NERR_Success;
    }
    default:
        TRACE("Invalid level %d specified.\n", level);
        status = ERROR_INVALID_LEVEL;
        break;
    }

    HeapFree(GetProcessHeap(), 0, su);

    return status;
}

/************************************************************
 *                NetUserDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetUserDel(LPCWSTR servername, LPCWSTR username)
{
    NET_API_STATUS status;
    struct sam_user *user;

    TRACE("(%s, %s)\n", debugstr_w(servername), debugstr_w(username));

    if((status = NETAPI_ValidateServername(servername))!= NERR_Success)
        return status;

    if ((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    list_remove(&user->entry);

    HeapFree(GetProcessHeap(), 0, user->home_dir);
    HeapFree(GetProcessHeap(), 0, user->user_comment);
    HeapFree(GetProcessHeap(), 0, user->user_logon_script_path);
    HeapFree(GetProcessHeap(), 0, user);

    return NERR_Success;
}

/************************************************************
 *                NetUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetInfo(LPCWSTR servername, LPCWSTR username, DWORD level,
               LPBYTE* bufptr)
{
    NET_API_STATUS status;
    TRACE("(%s, %s, %d, %p)\n", debugstr_w(servername), debugstr_w(username),
          level, bufptr);
    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    if(!NETAPI_IsLocalComputer(servername))
    {
        FIXME("Only implemented for local computer, but remote server"
              "%s was requested.\n", debugstr_w(servername));
        return NERR_InvalidComputer;
    }

    if(!NETAPI_FindUser(username) && !NETAPI_IsCurrentUser(username))
    {
        TRACE("User %s is unknown.\n", debugstr_w(username));
        return NERR_UserNotFound;
    }

    switch (level)
    {
    case 0:
    {
        PUSER_INFO_0 ui;
        int name_sz;

        name_sz = lstrlenW(username) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_0) + name_sz * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_0) *bufptr;
        ui->usri0_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_0));

        /* get data */
        lstrcpyW(ui->usri0_name, username);
        break;
    }

    case 10:
    {
        PUSER_INFO_10 ui;
        PUSER_INFO_0 ui0;
        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, usr_comment_sz, full_name_sz;

        comment_sz = 1;
        usr_comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_10) +
                             (name_sz + comment_sz + usr_comment_sz +
                              full_name_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);
        ui = (PUSER_INFO_10) *bufptr;
        ui->usri10_name = (LPWSTR) (*bufptr + sizeof(USER_INFO_10));
        ui->usri10_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_name) + name_sz * sizeof(WCHAR));
        ui->usri10_usr_comment = (LPWSTR) (
            ((PBYTE) ui->usri10_comment) + comment_sz * sizeof(WCHAR));
        ui->usri10_full_name = (LPWSTR) (
            ((PBYTE) ui->usri10_usr_comment) + usr_comment_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(ui->usri10_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri10_comment[0] = 0;
        ui->usri10_usr_comment[0] = 0;
        ui->usri10_full_name[0] = 0;
        break;
    }

    case 1:
      {
        static const WCHAR homedirW[] = {'H','O','M','E',0};
        PUSER_INFO_1 ui;
        PUSER_INFO_0 ui0;
        /* sizes of the field buffers in WCHARS */
        int name_sz, password_sz, home_dir_sz, comment_sz, script_path_sz;

        password_sz = 1; /* not filled out for security reasons for NetUserGetInfo*/
        comment_sz = 1;
        script_path_sz = 1;

       /* get data */
        status = NetUserGetInfo(servername, username, 0, (LPBYTE *) &ui0);
        if (status != NERR_Success)
        {
            NetApiBufferFree(ui0);
            return status;
        }
        name_sz = lstrlenW(ui0->usri0_name) + 1;
        home_dir_sz = GetEnvironmentVariableW(homedirW, NULL,0);
        /* set up buffer */
        NetApiBufferAllocate(sizeof(USER_INFO_1) +
                             (name_sz + password_sz + home_dir_sz +
                              comment_sz + script_path_sz) * sizeof(WCHAR),
                             (LPVOID *) bufptr);

        ui = (PUSER_INFO_1) *bufptr;
        ui->usri1_name = (LPWSTR) (ui + 1);
        ui->usri1_password = ui->usri1_name + name_sz;
        ui->usri1_home_dir = ui->usri1_password + password_sz;
        ui->usri1_comment = ui->usri1_home_dir + home_dir_sz;
        ui->usri1_script_path = ui->usri1_comment + comment_sz;
        /* set data */
        lstrcpyW(ui->usri1_name, ui0->usri0_name);
        NetApiBufferFree(ui0);
        ui->usri1_password[0] = 0;
        ui->usri1_password_age = 0;
        ui->usri1_priv = 0;
        GetEnvironmentVariableW(homedirW, ui->usri1_home_dir,home_dir_sz);
        ui->usri1_comment[0] = 0;
        ui->usri1_flags = 0;
        ui->usri1_script_path[0] = 0;
        break;
      }
    case 2:
    case 3:
    case 4:
    case 11:
    case 20:
    case 23:
    case 1003:
    case 1005:
    case 1006:
    case 1007:
    case 1008:
    case 1009:
    case 1010:
    case 1011:
    case 1012:
    case 1013:
    case 1014:
    case 1017:
    case 1018:
    case 1020:
    case 1023:
    case 1024:
    case 1025:
    case 1051:
    case 1052:
    case 1053:
    {
        FIXME("Level %d is not implemented\n", level);
        return NERR_InternalError;
    }
    default:
        TRACE("Invalid level %d is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetUserGetLocalGroups  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetLocalGroups(LPCWSTR servername, LPCWSTR username, DWORD level,
                      DWORD flags, LPBYTE* bufptr, DWORD prefmaxlen,
                      LPDWORD entriesread, LPDWORD totalentries)
{
    NET_API_STATUS status;
    const WCHAR admins[] = {'A','d','m','i','n','i','s','t','r','a','t','o','r','s',0};
    LPWSTR currentuser;
    LOCALGROUP_USERS_INFO_0* info;
    DWORD size;

    FIXME("(%s, %s, %d, %08x, %p %d, %p, %p) stub!\n",
          debugstr_w(servername), debugstr_w(username), level, flags, bufptr,
          prefmaxlen, entriesread, totalentries);

    status = NETAPI_ValidateServername(servername);
    if (status != NERR_Success)
        return status;

    size = UNLEN + 1;
    NetApiBufferAllocate(size * sizeof(WCHAR), (LPVOID*)&currentuser);
    if (!GetUserNameW(currentuser, &size)) {
        NetApiBufferFree(currentuser);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (lstrcmpiW(username, currentuser) && NETAPI_FindUser(username))
    {
        NetApiBufferFree(currentuser);
        return NERR_UserNotFound;
    }

    NetApiBufferFree(currentuser);
    *totalentries = 1;
    size = sizeof(*info) + sizeof(admins);

    if(prefmaxlen < size)
        status = ERROR_MORE_DATA;
    else
        status = NetApiBufferAllocate(size, (LPVOID*)&info);

    if(status != NERR_Success)
    {
        *bufptr = NULL;
        *entriesread = 0;
        return status;
    }

    info->lgrui0_name = (LPWSTR)((LPBYTE)info + sizeof(*info));
    lstrcpyW(info->lgrui0_name, admins);

    *bufptr = (LPBYTE)info;
    *entriesread = 1;

    return NERR_Success;
}

/************************************************************
 *                NetUserEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserEnum(LPCWSTR servername, DWORD level, DWORD filter, LPBYTE* bufptr,
	    DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries,
	    LPDWORD resume_handle)
{
  FIXME("(%s,%d, 0x%d,%p,%d,%p,%p,%p) stub!\n", debugstr_w(servername), level,
        filter, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

  return ERROR_ACCESS_DENIED;
}

/************************************************************
 *                ACCESS_QueryAdminDisplayInformation
 *
 *  Creates a buffer with information for the Admin User
 */
static void ACCESS_QueryAdminDisplayInformation(PNET_DISPLAY_USER *buf, PDWORD pdwSize)
{
    static const WCHAR sAdminUserName[] = {
        'A','d','m','i','n','i','s','t','r','a','t','o','r',0};

    /* sizes of the field buffers in WCHARS */
    int name_sz, comment_sz, full_name_sz;
    PNET_DISPLAY_USER usr;

    /* set up buffer */
    name_sz = lstrlenW(sAdminUserName) + 1;
    comment_sz = 1;
    full_name_sz = 1;

    *pdwSize = sizeof(NET_DISPLAY_USER);
    *pdwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);
    NetApiBufferAllocate(*pdwSize, (LPVOID *) buf);

    usr = *buf;
    usr->usri1_name = (LPWSTR) ((PBYTE) usr + sizeof(NET_DISPLAY_USER));
    usr->usri1_comment = (LPWSTR) (
        ((PBYTE) usr->usri1_name) + name_sz * sizeof(WCHAR));
    usr->usri1_full_name = (LPWSTR) (
        ((PBYTE) usr->usri1_comment) + comment_sz * sizeof(WCHAR));

    /* set data */
    lstrcpyW(usr->usri1_name, sAdminUserName);
    usr->usri1_comment[0] = 0;
    usr->usri1_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
    usr->usri1_full_name[0] = 0;
    usr->usri1_user_id = DOMAIN_USER_RID_ADMIN;
    usr->usri1_next_index = 0;
}

/************************************************************
 *                ACCESS_QueryGuestDisplayInformation
 *
 *  Creates a buffer with information for the Guest User
 */
static void ACCESS_QueryGuestDisplayInformation(PNET_DISPLAY_USER *buf, PDWORD pdwSize)
{
    static const WCHAR sGuestUserName[] = {
        'G','u','e','s','t',0 };

    /* sizes of the field buffers in WCHARS */
    int name_sz, comment_sz, full_name_sz;
    PNET_DISPLAY_USER usr;

    /* set up buffer */
    name_sz = lstrlenW(sGuestUserName) + 1;
    comment_sz = 1;
    full_name_sz = 1;

    *pdwSize = sizeof(NET_DISPLAY_USER);
    *pdwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);
    NetApiBufferAllocate(*pdwSize, (LPVOID *) buf);

    usr = *buf;
    usr->usri1_name = (LPWSTR) ((PBYTE) usr + sizeof(NET_DISPLAY_USER));
    usr->usri1_comment = (LPWSTR) (
        ((PBYTE) usr->usri1_name) + name_sz * sizeof(WCHAR));
    usr->usri1_full_name = (LPWSTR) (
        ((PBYTE) usr->usri1_comment) + comment_sz * sizeof(WCHAR));

    /* set data */
    lstrcpyW(usr->usri1_name, sGuestUserName);
    usr->usri1_comment[0] = 0;
    usr->usri1_flags = UF_ACCOUNTDISABLE | UF_SCRIPT | UF_NORMAL_ACCOUNT |
        UF_DONT_EXPIRE_PASSWD;
    usr->usri1_full_name[0] = 0;
    usr->usri1_user_id = DOMAIN_USER_RID_GUEST;
    usr->usri1_next_index = 0;
}

/************************************************************
 * Copies NET_DISPLAY_USER record.
 */
static void ACCESS_CopyDisplayUser(const NET_DISPLAY_USER *dest, LPWSTR *dest_buf,
                            PNET_DISPLAY_USER src)
{
    LPWSTR str = *dest_buf;

    src->usri1_name = str;
    lstrcpyW(src->usri1_name, dest->usri1_name);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_comment = str;
    lstrcpyW(src->usri1_comment, dest->usri1_comment);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_flags = dest->usri1_flags;

    src->usri1_full_name = str;
    lstrcpyW(src->usri1_full_name, dest->usri1_full_name);
    str = (LPWSTR) (
        ((PBYTE) str) + (lstrlenW(str) + 1) * sizeof(WCHAR));

    src->usri1_user_id = dest->usri1_user_id;
    src->usri1_next_index = dest->usri1_next_index;
    *dest_buf = str;
}

/************************************************************
 *                NetQueryDisplayInformation  (NETAPI32.@)
 *
 * The buffer structure:
 * - array of fixed size record of the level type
 * - strings, referenced by the record of the level type
 */
NET_API_STATUS WINAPI
NetQueryDisplayInformation(
    LPCWSTR ServerName, DWORD Level, DWORD Index, DWORD EntriesRequested,
    DWORD PreferredMaximumLength, LPDWORD ReturnedEntryCount,
    PVOID *SortedBuffer)
{
    TRACE("(%s, %d, %d, %d, %d, %p, %p)\n", debugstr_w(ServerName),
          Level, Index, EntriesRequested, PreferredMaximumLength,
          ReturnedEntryCount, SortedBuffer);

    if(!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME("Only implemented on local computer, but requested for "
              "remote server %s\n", debugstr_w(ServerName));
        return ERROR_ACCESS_DENIED;
    }

    switch (Level)
    {
    case 1:
    {
        /* current record */
        PNET_DISPLAY_USER inf;
        /* current available strings buffer */
        LPWSTR str;
        PNET_DISPLAY_USER admin, guest;
        DWORD admin_size, guest_size;
        LPWSTR name = NULL;
        DWORD dwSize;

        /* sizes of the field buffers in WCHARS */
        int name_sz, comment_sz, full_name_sz;

        /* number of the records, returned in SortedBuffer
           3 - for current user, Administrator and Guest users
         */
        int records = 3;

        FIXME("Level %d partially implemented\n", Level);
        *ReturnedEntryCount = records;
        comment_sz = 1;
        full_name_sz = 1;

        /* get data */
        dwSize = UNLEN + 1;
        NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) &name);
        if (!GetUserNameW(name, &dwSize))
        {
            NetApiBufferFree(name);
            return ERROR_ACCESS_DENIED;
        }
        name_sz = dwSize;
        ACCESS_QueryAdminDisplayInformation(&admin, &admin_size);
        ACCESS_QueryGuestDisplayInformation(&guest, &guest_size);

        /* set up buffer */
        dwSize = sizeof(NET_DISPLAY_USER) * records;
        dwSize += (name_sz + comment_sz + full_name_sz) * sizeof(WCHAR);

        NetApiBufferAllocate(dwSize +
                             admin_size - sizeof(NET_DISPLAY_USER) +
                             guest_size - sizeof(NET_DISPLAY_USER),
                             SortedBuffer);
        inf = *SortedBuffer;
        str = (LPWSTR) ((PBYTE) inf + sizeof(NET_DISPLAY_USER) * records);
        inf->usri1_name = str;
        str = (LPWSTR) (
            ((PBYTE) str) + name_sz * sizeof(WCHAR));
        inf->usri1_comment = str;
        str = (LPWSTR) (
            ((PBYTE) str) + comment_sz * sizeof(WCHAR));
        inf->usri1_full_name = str;
        str = (LPWSTR) (
            ((PBYTE) str) + full_name_sz * sizeof(WCHAR));

        /* set data */
        lstrcpyW(inf->usri1_name, name);
        NetApiBufferFree(name);
        inf->usri1_comment[0] = 0;
        inf->usri1_flags =
            UF_SCRIPT | UF_NORMAL_ACCOUNT | UF_DONT_EXPIRE_PASSWD;
        inf->usri1_full_name[0] = 0;
        inf->usri1_user_id = 0;
        inf->usri1_next_index = 0;

        inf++;
        ACCESS_CopyDisplayUser(admin, &str, inf);
        NetApiBufferFree(admin);

        inf++;
        ACCESS_CopyDisplayUser(guest, &str, inf);
        NetApiBufferFree(guest);
        break;
    }

    case 2:
    case 3:
    {
        FIXME("Level %d is not implemented\n", Level);
        break;
    }

    default:
        TRACE("Invalid level %d is specified\n", Level);
        return ERROR_INVALID_LEVEL;
    }
    return NERR_Success;
}

/************************************************************
 *                NetGetDCName  (NETAPI32.@)
 *
 *  Return the name of the primary domain controller (PDC)
 */

NET_API_STATUS WINAPI
NetGetDCName(LPCWSTR servername, LPCWSTR domainname, LPBYTE *bufptr)
{
  FIXME("(%s, %s, %p) stub!\n", debugstr_w(servername),
                 debugstr_w(domainname), bufptr);
  return NERR_DCNotFound; /* say we can't find a domain controller */
}

/************************************************************
 *                NetGetAnyDCName  (NETAPI32.@)
 *
 *  Return the name of any domain controller (DC) for a
 *   domain that is directly trusted by the specified server
 */

NET_API_STATUS WINAPI NetGetAnyDCName(LPCWSTR servername, LPCWSTR domainname, LPBYTE *bufptr)
{
    FIXME("(%s, %s, %p) stub!\n", debugstr_w(servername),
          debugstr_w(domainname), bufptr);
    return ERROR_NO_SUCH_DOMAIN;
}

/************************************************************
 *                NetGroupEnum  (NETAPI32.@)
 *
 */
NET_API_STATUS WINAPI
NetGroupEnum(LPCWSTR servername, DWORD level, LPBYTE *bufptr, DWORD prefmaxlen,
             LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("(%s, %d, %p, %d, %p, %p, %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);
    return ERROR_ACCESS_DENIED;
}

/************************************************************
 *                NetGroupGetInfo  (NETAPI32.@)
 *
 */
NET_API_STATUS WINAPI NetGroupGetInfo(LPCWSTR servername, LPCWSTR groupname, DWORD level, LPBYTE *bufptr)
{
    FIXME("(%s, %s, %d, %p) stub!\n", debugstr_w(servername), debugstr_w(groupname), level, bufptr);
    return ERROR_ACCESS_DENIED;
}

/******************************************************************************
 * NetUserModalsGet  (NETAPI32.@)
 *
 * Retrieves global information for all users and global groups in the security
 * database.
 *
 * PARAMS
 *  szServer   [I] Specifies the DNS or the NetBIOS name of the remote server
 *                 on which the function is to execute.
 *  level      [I] Information level of the data.
 *     0   Return global passwords parameters. bufptr points to a
 *         USER_MODALS_INFO_0 struct.
 *     1   Return logon server and domain controller information. bufptr
 *         points to a USER_MODALS_INFO_1 struct.
 *     2   Return domain name and identifier. bufptr points to a
 *         USER_MODALS_INFO_2 struct.
 *     3   Return lockout information. bufptr points to a USER_MODALS_INFO_3
 *         struct.
 *  pbuffer    [I] Buffer that receives the data.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure:
 *     ERROR_ACCESS_DENIED - the user does not have access to the info.
 *     NERR_InvalidComputer - computer name is invalid.
 */
NET_API_STATUS WINAPI NetUserModalsGet(
    LPCWSTR szServer, DWORD level, LPBYTE *pbuffer)
{
    TRACE("(%s %d %p)\n", debugstr_w(szServer), level, pbuffer);

    switch (level)
    {
        case 0:
            /* return global passwords parameters */
            FIXME("level 0 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        case 1:
            /* return logon server and domain controller info */
            FIXME("level 1 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        case 2:
        {
            /* return domain name and identifier */
            PUSER_MODALS_INFO_2 umi;
            LSA_HANDLE policyHandle;
            LSA_OBJECT_ATTRIBUTES objectAttributes;
            PPOLICY_ACCOUNT_DOMAIN_INFO domainInfo;
            NTSTATUS ntStatus;
            PSID domainIdentifier = NULL;
            int domainNameLen;

            ZeroMemory(&objectAttributes, sizeof(objectAttributes));
            objectAttributes.Length = sizeof(objectAttributes);

            ntStatus = LsaOpenPolicy(NULL, &objectAttributes,
                                     POLICY_VIEW_LOCAL_INFORMATION,
                                     &policyHandle);
            if (ntStatus != STATUS_SUCCESS)
            {
                WARN("LsaOpenPolicy failed with NT status %x\n",
                     LsaNtStatusToWinError(ntStatus));
                return ntStatus;
            }

            ntStatus = LsaQueryInformationPolicy(policyHandle,
                                                 PolicyAccountDomainInformation,
                                                 (PVOID *)&domainInfo);
            if (ntStatus != STATUS_SUCCESS)
            {
                WARN("LsaQueryInformationPolicy failed with NT status %x\n",
                     LsaNtStatusToWinError(ntStatus));
                LsaClose(policyHandle);
                return ntStatus;
            }

            domainIdentifier = domainInfo->DomainSid;
            domainNameLen = lstrlenW(domainInfo->DomainName.Buffer) + 1;
            LsaClose(policyHandle);

            ntStatus = NetApiBufferAllocate(sizeof(USER_MODALS_INFO_2) +
                                            GetLengthSid(domainIdentifier) +
                                            domainNameLen * sizeof(WCHAR),
                                            (LPVOID *)pbuffer);

            if (ntStatus != NERR_Success)
            {
                WARN("NetApiBufferAllocate() failed\n");
                LsaFreeMemory(domainInfo);
                return ntStatus;
            }

            umi = (USER_MODALS_INFO_2 *) *pbuffer;
            umi->usrmod2_domain_id = *pbuffer + sizeof(USER_MODALS_INFO_2);
            umi->usrmod2_domain_name = (LPWSTR)(*pbuffer +
                sizeof(USER_MODALS_INFO_2) + GetLengthSid(domainIdentifier));

            lstrcpynW(umi->usrmod2_domain_name,
                      domainInfo->DomainName.Buffer,
                      domainNameLen);
            CopySid(GetLengthSid(domainIdentifier), umi->usrmod2_domain_id,
                    domainIdentifier);

            LsaFreeMemory(domainInfo);

            break;
        }
        case 3:
            /* return lockout information */
            FIXME("level 3 not implemented!\n");
            *pbuffer = NULL;
            return NERR_InternalError;
        default:
            TRACE("Invalid level %d is specified\n", level);
            *pbuffer = NULL;
            return ERROR_INVALID_LEVEL;
    }

    return NERR_Success;
}

static NET_API_STATUS change_password_smb( LPCWSTR domainname, LPCWSTR username,
    LPCWSTR oldpassword, LPCWSTR newpassword )
{
#ifdef HAVE_FORK
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
    HeapFree( GetProcessHeap(), 0, server );
    HeapFree( GetProcessHeap(), 0, user );
    HeapFree( GetProcessHeap(), 0, old );
    HeapFree( GetProcessHeap(), 0, new );
    return ret;
#else
    ERR( "no fork support on this platform\n" );
    return NERR_InternalError;
#endif
}

/******************************************************************************
 *                NetUserChangePassword  (NETAPI32.@)
 * PARAMS
 *  domainname  [I] Optional. Domain on which the user resides or the logon
 *                  domain of the current user if NULL.
 *  username    [I] Optional. Username to change the password for or the name
 *                  of the current user if NULL.
 *  oldpassword [I] The user's current password.
 *  newpassword [I] The password that the user will be changed to using.
 *
 * RETURNS
 *  Success: NERR_Success.
 *  Failure: NERR_* failure code or win error code.
 *
 */
NET_API_STATUS WINAPI NetUserChangePassword(LPCWSTR domainname, LPCWSTR username,
    LPCWSTR oldpassword, LPCWSTR newpassword)
{
    struct sam_user *user;

    TRACE("(%s, %s, ..., ...)\n", debugstr_w(domainname), debugstr_w(username));

    if (!change_password_smb( domainname, username, oldpassword, newpassword ))
        return NERR_Success;

    if(domainname)
        FIXME("Ignoring domainname %s.\n", debugstr_w(domainname));

    if((user = NETAPI_FindUser(username)) == NULL)
        return NERR_UserNotFound;

    if(lstrcmpW(user->user_password, oldpassword) != 0)
        return ERROR_INVALID_PASSWORD;

    if(lstrlenW(newpassword) > PWLEN)
        return ERROR_PASSWORD_RESTRICTION;

    lstrcpyW(user->user_password, newpassword);

    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseAdd(LMSTR servername, DWORD level, LPBYTE bufptr, LPDWORD parm_err)
{
    FIXME("%s %d %p %p stub\n", debugstr_w(servername), level, bufptr, parm_err);
    return NERR_Success;
}

NET_API_STATUS WINAPI NetUseDel(LMSTR servername, LMSTR usename, DWORD forcecond)
{
    FIXME("%s %s %d stub\n", debugstr_w(servername), debugstr_w(usename), forcecond);
    return NERR_Success;
}

/************************************************************
 *                I_BrowserSetNetlogonState  (NETAPI32.@)
 */
NET_API_STATUS WINAPI I_BrowserSetNetlogonState(
    LPWSTR ServerName, LPWSTR DomainName, LPWSTR EmulatedServerName,
    DWORD Role)
{
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                I_BrowserQueryEmulatedDomains  (NETAPI32.@)
 */
NET_API_STATUS WINAPI I_BrowserQueryEmulatedDomains(
    LPWSTR ServerName, PBROWSER_EMULATED_DOMAIN *EmulatedDomains,
    LPDWORD EntriesRead)
{
    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI DsGetDcNameW(LPCWSTR ComputerName, LPCWSTR AvoidDCName,
 GUID* DomainGuid, LPCWSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08x, %p): stub\n", debugstr_w(ComputerName),
     debugstr_w(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_w(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetDcNameA(LPCSTR ComputerName, LPCSTR AvoidDCName,
 GUID* DomainGuid, LPCSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08x, %p): stub\n", debugstr_a(ComputerName),
     debugstr_a(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_a(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetSiteNameW(LPCWSTR ComputerName, LPWSTR *SiteName)
{
    FIXME("(%s, %p): stub\n", debugstr_w(ComputerName), SiteName);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetSiteNameA(LPCSTR ComputerName, LPSTR *SiteName)
{
    FIXME("(%s, %p): stub\n", debugstr_a(ComputerName), SiteName);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/************************************************************
 *  DsRoleFreeMemory (NETAPI32.@)
 *
 * PARAMS
 *  Buffer [I] Pointer to the to-be-freed buffer.
 *
 * RETURNS
 *  Nothing
 */
VOID WINAPI DsRoleFreeMemory(PVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
}

/************************************************************
 *  DsRoleGetPrimaryDomainInformation  (NETAPI32.@)
 *
 * PARAMS
 *  lpServer  [I] Pointer to UNICODE string with ComputerName
 *  InfoLevel [I] Type of data to retrieve
 *  Buffer    [O] Pointer to to the requested data
 *
 * RETURNS
 *
 * NOTES
 *  When lpServer is NULL, use the local computer
 */
DWORD WINAPI DsRoleGetPrimaryDomainInformation(
    LPCWSTR lpServer, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    PBYTE* Buffer)
{
    DWORD ret;

    FIXME("(%p, %d, %p) stub\n", lpServer, InfoLevel, Buffer);

    /* Check some input parameters */

    if (!Buffer) return ERROR_INVALID_PARAMETER;
    if ((InfoLevel < DsRolePrimaryDomainInfoBasic) || (InfoLevel > DsRoleOperationState)) return ERROR_INVALID_PARAMETER;

    *Buffer = NULL;
    switch (InfoLevel)
    {
        case DsRolePrimaryDomainInfoBasic:
        {
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
            NTSTATUS NtStatus;
            int logon_domain_sz;
            DWORD size;
            PDSROLE_PRIMARY_DOMAIN_INFO_BASIC basic;

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
            {
                TRACE("LsaOpenPolicyFailed with NT status %x\n",
                    LsaNtStatusToWinError(NtStatus));
                return ERROR_OUTOFMEMORY;
            }
            LsaQueryInformationPolicy(PolicyHandle,
             PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
            logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
            LsaClose(PolicyHandle);

            size = sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC) +
             logon_domain_sz * sizeof(WCHAR);
            basic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            if (basic)
            {
                basic->MachineRole = DsRole_RoleStandaloneWorkstation;
                basic->DomainNameFlat = (LPWSTR)((LPBYTE)basic +
                 sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC));
                lstrcpyW(basic->DomainNameFlat, DomainInfo->DomainName.Buffer);
                ret = ERROR_SUCCESS;
            }
            else
                ret = ERROR_OUTOFMEMORY;
            *Buffer = (PBYTE)basic;
            LsaFreeMemory(DomainInfo);
        }
        break;
    default:
        ret = ERROR_CALL_NOT_IMPLEMENTED;
    }
    return ret;
}

/************************************************************
 *                NetLocalGroupAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAdd(
    LPCWSTR servername,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %d %p %p) stub!\n", debugstr_w(servername), level, buf,
          parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDel(
    LPCWSTR servername,
    LPCWSTR groupname)
{
    FIXME("(%s %s) stub!\n", debugstr_w(servername), debugstr_w(groupname));
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupEnum(
    LPCWSTR servername,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %d %p %d %p %p %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);
    *entriesread = 0;
    *totalentries = 0;
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE* bufptr)
{
    static const WCHAR commentW[]={'N','o',' ','c','o','m','m','e','n','t',0};
    LOCALGROUP_INFO_1* info;
    DWORD size;

    FIXME("(%s %s %d %p) semi-stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr);

    size = sizeof(*info) + sizeof(WCHAR) * (lstrlenW(groupname)+1) + sizeof(commentW);
    NetApiBufferAllocate(size, (LPVOID*)&info);

    info->lgrpi1_name = (LPWSTR)(info + 1);
    lstrcpyW(info->lgrpi1_name, groupname);

    info->lgrpi1_comment = info->lgrpi1_name + lstrlenW(groupname) + 1;
    lstrcpyW(info->lgrpi1_comment, commentW);

    *bufptr = (LPBYTE)info;

    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetMembers(
    LPCWSTR servername,
    LPCWSTR localgroupname,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %s %d %p %d, %p %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(localgroupname), level, bufptr, prefmaxlen, entriesread,
          totalentries, resumehandle);

    if (level == 3)
    {
        WCHAR userName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD userNameLen;
        DWORD len,needlen;
        PLOCALGROUP_MEMBERS_INFO_3 ptr;

        /* still a stub,  current user is belonging to all groups */

        *totalentries = 1;
        *entriesread = 0;

        userNameLen = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetUserNameW(userName,&userNameLen))
            return ERROR_NOT_ENOUGH_MEMORY;

        needlen = sizeof(LOCALGROUP_MEMBERS_INFO_3) +
             (userNameLen+2) * sizeof(WCHAR);
        if (prefmaxlen != MAX_PREFERRED_LENGTH)
            len = min(prefmaxlen,needlen);
        else
            len = needlen;

        NetApiBufferAllocate(len, (LPVOID *) bufptr);
        if (len < needlen)
            return ERROR_MORE_DATA;

        ptr = (PLOCALGROUP_MEMBERS_INFO_3)*bufptr;
        ptr->lgrmi3_domainandname = (LPWSTR)(*bufptr+sizeof(LOCALGROUP_MEMBERS_INFO_3));
        lstrcpyW(ptr->lgrmi3_domainandname,userName);

        *entriesread = 1;
    }

    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %s %d %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetMember (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
            debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}
