/*
 * Tokens
 *
 * Copyright (C) 1998 Alexandre Julliard
 * Copyright (C) 2003 Mike McCormack
 * Copyright (C) 2005 Robert Shearman
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "handle.h"
#include "thread.h"
#include "process.h"
#include "request.h"
#include "security.h"

#define MAX_SUBAUTH_COUNT 1

const LUID SeIncreaseQuotaPrivilege        = {  5, 0 };
const LUID SeSecurityPrivilege             = {  8, 0 };
const LUID SeTakeOwnershipPrivilege        = {  9, 0 };
const LUID SeLoadDriverPrivilege           = { 10, 0 };
const LUID SeSystemProfilePrivilege        = { 11, 0 };
const LUID SeSystemtimePrivilege           = { 12, 0 };
const LUID SeProfileSingleProcessPrivilege = { 13, 0 };
const LUID SeIncreaseBasePriorityPrivilege = { 14, 0 };
const LUID SeCreatePagefilePrivilege       = { 15, 0 };
const LUID SeBackupPrivilege               = { 17, 0 };
const LUID SeRestorePrivilege              = { 18, 0 };
const LUID SeShutdownPrivilege             = { 19, 0 };
const LUID SeDebugPrivilege                = { 20, 0 };
const LUID SeSystemEnvironmentPrivilege    = { 22, 0 };
const LUID SeChangeNotifyPrivilege         = { 23, 0 };
const LUID SeRemoteShutdownPrivilege       = { 24, 0 };
const LUID SeUndockPrivilege               = { 25, 0 };
const LUID SeManageVolumePrivilege         = { 28, 0 };
const LUID SeImpersonatePrivilege          = { 29, 0 };
const LUID SeCreateGlobalPrivilege         = { 30, 0 };

#define SID_N(n) struct /* same fields as struct SID */ \
    { \
        BYTE Revision; \
        BYTE SubAuthorityCount; \
        SID_IDENTIFIER_AUTHORITY IdentifierAuthority; \
        DWORD SubAuthority[n]; \
    }

static const SID world_sid = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } };
static const SID local_sid = { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } };
static const SID interactive_sid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } };
static const SID anonymous_logon_sid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } };
static const SID authenticated_user_sid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } };
static const SID local_system_sid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } };
static const SID high_label_sid = { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY }, { SECURITY_MANDATORY_HIGH_RID } };
static const SID_N(5) local_user_sid = { SID_REVISION, 5, { SECURITY_NT_AUTHORITY }, { SECURITY_NT_NON_UNIQUE, 0, 0, 0, 1000 } };
static const SID_N(2) builtin_admins_sid = { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } };
static const SID_N(2) builtin_users_sid = { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } };
static const SID_N(3) builtin_logon_sid = { SID_REVISION, 3, { SECURITY_NT_AUTHORITY }, { SECURITY_LOGON_IDS_RID, 0, 0 } };
static const SID_N(5) domain_users_sid = { SID_REVISION, 5, { SECURITY_NT_AUTHORITY }, { SECURITY_NT_NON_UNIQUE, 0, 0, 0, DOMAIN_GROUP_RID_USERS } };

const PSID security_world_sid = (PSID)&world_sid;
static const PSID security_local_sid = (PSID)&local_sid;
static const PSID security_interactive_sid = (PSID)&interactive_sid;
static const PSID security_authenticated_user_sid = (PSID)&authenticated_user_sid;
const PSID security_local_system_sid = (PSID)&local_system_sid;
const PSID security_local_user_sid = (PSID)&local_user_sid;
const PSID security_builtin_admins_sid = (PSID)&builtin_admins_sid;
const PSID security_builtin_users_sid = (PSID)&builtin_users_sid;
const PSID security_domain_users_sid = (PSID)&domain_users_sid;
const PSID security_high_label_sid = (PSID)&high_label_sid;

static luid_t prev_luid_value = { 1000, 0 };

struct token
{
    struct object  obj;             /* object header */
    luid_t         token_id;        /* system-unique id of token */
    luid_t         modified_id;     /* new id allocated every time token is modified */
    struct list    privileges;      /* privileges available to the token */
    struct list    groups;          /* groups that the user of this token belongs to (sid_and_attributes) */
    SID           *user;            /* SID of user this token represents */
    SID           *owner;           /* SID of owner (points to user or one of groups) */
    SID           *primary_group;   /* SID of user's primary group (points to one of groups) */
    unsigned       primary;         /* is this a primary or impersonation token? */
    ACL           *default_dacl;    /* the default DACL to assign to objects created by this user */
    TOKEN_SOURCE   source;          /* source of the token */
    int            impersonation_level; /* impersonation level this token is capable of if non-primary token */
};

struct privilege
{
    struct list entry;
    LUID        luid;
    unsigned    enabled  : 1; /* is the privilege currently enabled? */
    unsigned    def      : 1; /* is the privilege enabled by default? */
};

struct group
{
    struct list entry;
    unsigned    enabled  : 1; /* is the sid currently enabled? */
    unsigned    def      : 1; /* is the sid enabled by default? */
    unsigned    logon    : 1; /* is this a logon sid? */
    unsigned    mandatory: 1; /* is this sid always enabled? */
    unsigned    owner    : 1; /* can this sid be an owner of an object? */
    unsigned    resource : 1; /* is this a domain-local group? */
    unsigned    deny_only: 1; /* is this a sid that should be use for denying only? */
    SID         sid;
};

static void token_dump( struct object *obj, int verbose );
static struct object_type *token_get_type( struct object *obj );
static unsigned int token_map_access( struct object *obj, unsigned int access );
static void token_destroy( struct object *obj );

static const struct object_ops token_ops =
{
    sizeof(struct token),      /* size */
    token_dump,                /* dump */
    token_get_type,            /* get_type */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    token_map_access,          /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    no_get_full_name,          /* get_full_name */
    no_lookup_name,            /* lookup_name */
    no_link_name,              /* link_name */
    NULL,                      /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    token_destroy              /* destroy */
};

static void token_dump( struct object *obj, int verbose )
{
    struct token *token = (struct token *)obj;
    assert( obj->ops == &token_ops );
    fprintf( stderr, "Token id=%d.%u primary=%u impersonation level=%d\n", token->token_id.high_part,
             token->token_id.low_part, token->primary, token->impersonation_level );
}

static struct object_type *token_get_type( struct object *obj )
{
    static const WCHAR name[] = {'T','o','k','e','n'};
    static const struct unicode_str str = { name, sizeof(name) };
    return get_object_type( &str );
}

static unsigned int token_map_access( struct object *obj, unsigned int access )
{
    if (access & GENERIC_READ)    access |= TOKEN_READ;
    if (access & GENERIC_WRITE)   access |= TOKEN_WRITE;
    if (access & GENERIC_EXECUTE) access |= STANDARD_RIGHTS_EXECUTE;
    if (access & GENERIC_ALL)     access |= TOKEN_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static SID *security_sid_alloc( const SID_IDENTIFIER_AUTHORITY *idauthority, int subauthcount, const unsigned int subauth[] )
{
    int i;
    SID *sid = mem_alloc( FIELD_OFFSET(SID, SubAuthority[subauthcount]) );
    if (!sid) return NULL;
    sid->Revision = SID_REVISION;
    sid->SubAuthorityCount = subauthcount;
    sid->IdentifierAuthority = *idauthority;
    for (i = 0; i < subauthcount; i++)
        sid->SubAuthority[i] = subauth[i];
    return sid;
}

void security_set_thread_token( struct thread *thread, obj_handle_t handle )
{
    if (!handle)
    {
        if (thread->token)
            release_object( thread->token );
        thread->token = NULL;
    }
    else
    {
        struct token *token = (struct token *)get_handle_obj( current->process,
                                                              handle,
                                                              TOKEN_IMPERSONATE,
                                                              &token_ops );
        if (token)
        {
            if (thread->token)
                release_object( thread->token );
            thread->token = token;
        }
    }
}

const SID *security_unix_uid_to_sid( uid_t uid )
{
    /* very simple mapping: either the current user or not the current user */
    if (uid == getuid())
        return (const SID *)&local_user_sid;
    else
        return &anonymous_logon_sid;
}

static int acl_is_valid( const ACL *acl, data_size_t size )
{
    ULONG i;
    const ACE_HEADER *ace;

    if (size < sizeof(ACL))
        return FALSE;

    size = min(size, MAX_ACL_LEN);

    size -= sizeof(ACL);

    ace = (const ACE_HEADER *)(acl + 1);
    for (i = 0; i < acl->AceCount; i++)
    {
        const SID *sid;
        data_size_t sid_size;

        if (size < sizeof(ACE_HEADER))
            return FALSE;
        if (size < ace->AceSize)
            return FALSE;
        size -= ace->AceSize;
        switch (ace->AceType)
        {
        case ACCESS_DENIED_ACE_TYPE:
            sid = (const SID *)&((const ACCESS_DENIED_ACE *)ace)->SidStart;
            sid_size = ace->AceSize - FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart);
            break;
        case ACCESS_ALLOWED_ACE_TYPE:
            sid = (const SID *)&((const ACCESS_ALLOWED_ACE *)ace)->SidStart;
            sid_size = ace->AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);
            break;
        case SYSTEM_AUDIT_ACE_TYPE:
            sid = (const SID *)&((const SYSTEM_AUDIT_ACE *)ace)->SidStart;
            sid_size = ace->AceSize - FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart);
            break;
        case SYSTEM_ALARM_ACE_TYPE:
            sid = (const SID *)&((const SYSTEM_ALARM_ACE *)ace)->SidStart;
            sid_size = ace->AceSize - FIELD_OFFSET(SYSTEM_ALARM_ACE, SidStart);
            break;
        case SYSTEM_MANDATORY_LABEL_ACE_TYPE:
            sid = (const SID *)&((const SYSTEM_MANDATORY_LABEL_ACE *)ace)->SidStart;
            sid_size = ace->AceSize - FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart);
            break;
        default:
            return FALSE;
        }
        if (sid_size < FIELD_OFFSET(SID, SubAuthority[0]) || sid_size < security_sid_len( sid ))
            return FALSE;
        ace = ace_next( ace );
    }
    return TRUE;
}

static unsigned int get_sid_count( const SID *sid, data_size_t size )
{
    unsigned int count;

    for (count = 0; size >= sizeof(SID) && security_sid_len( sid ) <= size; count++)
    {
        size -= security_sid_len( sid );
        sid = (const SID *)((char *)sid + security_sid_len( sid ));
    }

    return count;
}

/* checks whether all members of a security descriptor fit inside the size
 * of memory specified */
int sd_is_valid( const struct security_descriptor *sd, data_size_t size )
{
    size_t offset = sizeof(struct security_descriptor);
    const SID *group;
    const SID *owner;
    const ACL *sacl;
    const ACL *dacl;
    int dummy;

    if (size < offset)
        return FALSE;

    if ((sd->owner_len >= FIELD_OFFSET(SID, SubAuthority[255])) ||
        (offset + sd->owner_len > size))
        return FALSE;
    owner = sd_get_owner( sd );
    if (owner)
    {
        if ((sd->owner_len < sizeof(SID)) || (security_sid_len( owner ) > sd->owner_len))
            return FALSE;
    }
    offset += sd->owner_len;

    if ((sd->group_len >= FIELD_OFFSET(SID, SubAuthority[255])) ||
        (offset + sd->group_len > size))
        return FALSE;
    group = sd_get_group( sd );
    if (group)
    {
        if ((sd->group_len < sizeof(SID)) || (security_sid_len( group ) > sd->group_len))
            return FALSE;
    }
    offset += sd->group_len;

    if ((sd->sacl_len >= MAX_ACL_LEN) || (offset + sd->sacl_len > size))
        return FALSE;
    sacl = sd_get_sacl( sd, &dummy );
    if (sacl && !acl_is_valid( sacl, sd->sacl_len ))
        return FALSE;
    offset += sd->sacl_len;

    if ((sd->dacl_len >= MAX_ACL_LEN) || (offset + sd->dacl_len > size))
        return FALSE;
    dacl = sd_get_dacl( sd, &dummy );
    if (dacl && !acl_is_valid( dacl, sd->dacl_len ))
        return FALSE;
    offset += sd->dacl_len;

    return TRUE;
}

/* extract security labels from SACL */
ACL *extract_security_labels( const ACL *sacl )
{
    size_t size = sizeof(ACL);
    const ACE_HEADER *ace;
    ACE_HEADER *label_ace;
    unsigned int i, count = 0;
    ACL *label_acl;

    ace = (const ACE_HEADER *)(sacl + 1);
    for (i = 0; i < sacl->AceCount; i++, ace = ace_next( ace ))
    {
        if (ace->AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            size += ace->AceSize;
            count++;
        }
    }

    label_acl = mem_alloc( size );
    if (!label_acl) return NULL;

    label_acl->AclRevision = sacl->AclRevision;
    label_acl->Sbz1 = 0;
    label_acl->AclSize = size;
    label_acl->AceCount = count;
    label_acl->Sbz2 = 0;
    label_ace = (ACE_HEADER *)(label_acl + 1);

    ace = (const ACE_HEADER *)(sacl + 1);
    for (i = 0; i < sacl->AceCount; i++, ace = ace_next( ace ))
    {
        if (ace->AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            memcpy( label_ace, ace, ace->AceSize );
            label_ace = (ACE_HEADER *)ace_next( label_ace );
        }
    }
    return label_acl;
}

/* replace security labels in an existing SACL */
ACL *replace_security_labels( const ACL *old_sacl, const ACL *new_sacl )
{
    const ACE_HEADER *ace;
    ACE_HEADER *replaced_ace;
    size_t size = sizeof(ACL);
    unsigned int i, count = 0;
    BYTE revision = ACL_REVISION;
    ACL *replaced_acl;

    if (old_sacl)
    {
        revision = max( revision, old_sacl->AclRevision );
        ace = (const ACE_HEADER *)(old_sacl + 1);
        for (i = 0; i < old_sacl->AceCount; i++, ace = ace_next( ace ))
        {
            if (ace->AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
            size += ace->AceSize;
            count++;
        }
    }

    if (new_sacl)
    {
        revision = max( revision, new_sacl->AclRevision );
        ace = (const ACE_HEADER *)(new_sacl + 1);
        for (i = 0; i < new_sacl->AceCount; i++, ace = ace_next( ace ))
        {
            if (ace->AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
            size += ace->AceSize;
            count++;
        }
    }

    replaced_acl = mem_alloc( size );
    if (!replaced_acl) return NULL;

    replaced_acl->AclRevision = revision;
    replaced_acl->Sbz1 = 0;
    replaced_acl->AclSize = size;
    replaced_acl->AceCount = count;
    replaced_acl->Sbz2 = 0;
    replaced_ace = (ACE_HEADER *)(replaced_acl + 1);

    if (old_sacl)
    {
        ace = (const ACE_HEADER *)(old_sacl + 1);
        for (i = 0; i < old_sacl->AceCount; i++, ace = ace_next( ace ))
        {
            if (ace->AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
            memcpy( replaced_ace, ace, ace->AceSize );
            replaced_ace = (ACE_HEADER *)ace_next( replaced_ace );
        }
    }

    if (new_sacl)
    {
        ace = (const ACE_HEADER *)(new_sacl + 1);
        for (i = 0; i < new_sacl->AceCount; i++, ace = ace_next( ace ))
        {
            if (ace->AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
            memcpy( replaced_ace, ace, ace->AceSize );
            replaced_ace = (ACE_HEADER *)ace_next( replaced_ace );
        }
    }

    return replaced_acl;
}

/* maps from generic rights to specific rights as given by a mapping */
static inline void map_generic_mask(unsigned int *mask, const GENERIC_MAPPING *mapping)
{
    if (*mask & GENERIC_READ) *mask |= mapping->GenericRead;
    if (*mask & GENERIC_WRITE) *mask |= mapping->GenericWrite;
    if (*mask & GENERIC_EXECUTE) *mask |= mapping->GenericExecute;
    if (*mask & GENERIC_ALL) *mask |= mapping->GenericAll;
    *mask &= 0x0FFFFFFF;
}

static inline int is_equal_luid( const LUID *luid1, const LUID *luid2 )
{
    return (luid1->LowPart == luid2->LowPart && luid1->HighPart == luid2->HighPart);
}

static inline void allocate_luid( luid_t *luid )
{
    prev_luid_value.low_part++;
    *luid = prev_luid_value;
}

DECL_HANDLER( allocate_locally_unique_id )
{
    allocate_luid( &reply->luid );
}

static inline void luid_and_attr_from_privilege( LUID_AND_ATTRIBUTES *out, const struct privilege *in)
{
    out->Luid = in->luid;
    out->Attributes =
        (in->enabled ? SE_PRIVILEGE_ENABLED : 0) |
        (in->def ? SE_PRIVILEGE_ENABLED_BY_DEFAULT : 0);
}

static struct privilege *privilege_add( struct token *token, const LUID *luid, int enabled )
{
    struct privilege *privilege = mem_alloc( sizeof(*privilege) );
    if (privilege)
    {
        privilege->luid = *luid;
        privilege->def = privilege->enabled = (enabled != 0);
        list_add_tail( &token->privileges, &privilege->entry );
    }
    return privilege;
}

static inline void privilege_remove( struct privilege *privilege )
{
    list_remove( &privilege->entry );
    free( privilege );
}

static void token_destroy( struct object *obj )
{
    struct token* token;
    struct list *cursor, *cursor_next;

    assert( obj->ops == &token_ops );
    token = (struct token *)obj;

    free( token->user );

    LIST_FOR_EACH_SAFE( cursor, cursor_next, &token->privileges )
    {
        struct privilege *privilege = LIST_ENTRY( cursor, struct privilege, entry );
        privilege_remove( privilege );
    }

    LIST_FOR_EACH_SAFE( cursor, cursor_next, &token->groups )
    {
        struct group *group = LIST_ENTRY( cursor, struct group, entry );
        list_remove( &group->entry );
        free( group );
    }

    free( token->default_dacl );
}

/* creates a new token.
 *  groups may be NULL if group_count is 0.
 *  privs may be NULL if priv_count is 0.
 *  default_dacl may be NULL, indicating that all objects created by the user
 *   are unsecured.
 *  modified_id may be NULL, indicating that a new modified_id luid should be
 *   allocated.
 */
static struct token *create_token( unsigned primary, const SID *user,
                                   const SID_AND_ATTRIBUTES *groups, unsigned int group_count,
                                   const LUID_AND_ATTRIBUTES *privs, unsigned int priv_count,
                                   const ACL *default_dacl, TOKEN_SOURCE source,
                                   const luid_t *modified_id,
                                   int impersonation_level )
{
    struct token *token = alloc_object( &token_ops );
    if (token)
    {
        unsigned int i;

        allocate_luid( &token->token_id );
        if (modified_id)
            token->modified_id = *modified_id;
        else
            allocate_luid( &token->modified_id );
        list_init( &token->privileges );
        list_init( &token->groups );
        token->primary = primary;
        /* primary tokens don't have impersonation levels */
        if (primary)
            token->impersonation_level = -1;
        else
            token->impersonation_level = impersonation_level;
        token->default_dacl = NULL;
        token->primary_group = NULL;

        /* copy user */
        token->user = memdup( user, security_sid_len( user ));
        if (!token->user)
        {
            release_object( token );
            return NULL;
        }

        /* copy groups */
        for (i = 0; i < group_count; i++)
        {
            size_t size = FIELD_OFFSET( struct group, sid.SubAuthority[((const SID *)groups[i].Sid)->SubAuthorityCount] );
            struct group *group = mem_alloc( size );

            if (!group)
            {
                release_object( token );
                return NULL;
            }
            memcpy( &group->sid, groups[i].Sid, security_sid_len( groups[i].Sid ));
            group->enabled = TRUE;
            group->def = TRUE;
            group->logon = (groups[i].Attributes & SE_GROUP_LOGON_ID) != 0;
            group->mandatory = (groups[i].Attributes & SE_GROUP_MANDATORY) != 0;
            group->owner = (groups[i].Attributes & SE_GROUP_OWNER) != 0;
            group->resource = FALSE;
            group->deny_only = FALSE;
            list_add_tail( &token->groups, &group->entry );
            /* Use first owner capable group as owner and primary group */
            if (!token->primary_group && group->owner)
            {
                token->owner = &group->sid;
                token->primary_group = &group->sid;
            }
        }

        /* copy privileges */
        for (i = 0; i < priv_count; i++)
        {
            /* note: we don't check uniqueness: the caller must make sure
             * privs doesn't contain any duplicate luids */
            if (!privilege_add( token, &privs[i].Luid,
                                privs[i].Attributes & SE_PRIVILEGE_ENABLED ))
            {
                release_object( token );
                return NULL;
            }
        }

        if (default_dacl)
        {
            token->default_dacl = memdup( default_dacl, default_dacl->AclSize );
            if (!token->default_dacl)
            {
                release_object( token );
                return NULL;
            }
        }

        token->source = source;
    }
    return token;
}

static int filter_group( struct group *group, const SID *filter, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if (security_equal_sid( &group->sid, filter )) return 1;
        filter = (const SID *)((char *)filter + security_sid_len( filter ));
    }

    return 0;
}

static int filter_privilege( struct privilege *privilege, const LUID_AND_ATTRIBUTES *filter, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if (!memcmp( &privilege->luid, &filter[i].Luid, sizeof(LUID) ))
            return 1;
    }

    return 0;
}

struct token *token_duplicate( struct token *src_token, unsigned primary,
                               int impersonation_level, const struct security_descriptor *sd,
                               const LUID_AND_ATTRIBUTES *remove_privs, unsigned int remove_priv_count,
                               const SID *remove_groups, unsigned int remove_group_count)
{
    const luid_t *modified_id =
        primary || (impersonation_level == src_token->impersonation_level) ?
            &src_token->modified_id : NULL;
    struct token *token = NULL;
    struct privilege *privilege;
    struct group *group;

    if (!primary &&
        (impersonation_level < SecurityAnonymous ||
         impersonation_level > SecurityDelegation ||
         (!src_token->primary && (impersonation_level > src_token->impersonation_level))))
    {
        set_error( STATUS_BAD_IMPERSONATION_LEVEL );
        return NULL;
    }

    token = create_token( primary, src_token->user, NULL, 0,
                          NULL, 0, src_token->default_dacl,
                          src_token->source, modified_id,
                          impersonation_level );
    if (!token) return token;

    /* copy groups */
    token->primary_group = NULL;
    LIST_FOR_EACH_ENTRY( group, &src_token->groups, struct group, entry )
    {
        size_t size = FIELD_OFFSET( struct group, sid.SubAuthority[group->sid.SubAuthorityCount] );
        struct group *newgroup = mem_alloc( size );
        if (!newgroup)
        {
            release_object( token );
            return NULL;
        }
        memcpy( newgroup, group, size );
        if (filter_group( group, remove_groups, remove_group_count ))
        {
            newgroup->enabled = 0;
            newgroup->def = 0;
            newgroup->deny_only = 1;
        }
        list_add_tail( &token->groups, &newgroup->entry );
        if (src_token->primary_group == &group->sid)
        {
            token->owner = &newgroup->sid;
            token->primary_group = &newgroup->sid;
        }
    }
    assert( token->primary_group );

    /* copy privileges */
    LIST_FOR_EACH_ENTRY( privilege, &src_token->privileges, struct privilege, entry )
    {
        if (filter_privilege( privilege, remove_privs, remove_priv_count )) continue;
        if (!privilege_add( token, &privilege->luid, privilege->enabled ))
        {
            release_object( token );
            return NULL;
        }
    }

    if (sd) default_set_sd( &token->obj, sd, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION );

    return token;
}

static ACL *create_default_dacl( const SID *user )
{
    ACCESS_ALLOWED_ACE *aaa;
    ACL *default_dacl;
    SID *sid;
    size_t default_dacl_size = sizeof(ACL) +
                               2*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                               sizeof(local_system_sid) +
                               security_sid_len( user );

    default_dacl = mem_alloc( default_dacl_size );
    if (!default_dacl) return NULL;

    default_dacl->AclRevision = ACL_REVISION;
    default_dacl->Sbz1 = 0;
    default_dacl->AclSize = default_dacl_size;
    default_dacl->AceCount = 2;
    default_dacl->Sbz2 = 0;

    /* GENERIC_ALL for Local System */
    aaa = (ACCESS_ALLOWED_ACE *)(default_dacl + 1);
    aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    aaa->Header.AceFlags = 0;
    aaa->Header.AceSize = (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                          sizeof(local_system_sid);
    aaa->Mask = GENERIC_ALL;
    sid = (SID *)&aaa->SidStart;
    memcpy( sid, &local_system_sid, sizeof(local_system_sid) );

    /* GENERIC_ALL for specified user */
    aaa = (ACCESS_ALLOWED_ACE *)((char *)aaa + aaa->Header.AceSize);
    aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    aaa->Header.AceFlags = 0;
    aaa->Header.AceSize = (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) + security_sid_len( user );
    aaa->Mask = GENERIC_ALL;
    sid = (SID *)&aaa->SidStart;
    memcpy( sid, user, security_sid_len( user ));

    return default_dacl;
}

struct sid_data
{
    SID_IDENTIFIER_AUTHORITY idauth;
    int count;
    unsigned int subauth[MAX_SUBAUTH_COUNT];
};

static struct security_descriptor *create_security_label_sd( struct token *token, PSID label_sid )
{
    size_t sid_len = security_sid_len( label_sid ), sacl_size, sd_size;
    SYSTEM_MANDATORY_LABEL_ACE *smla;
    struct security_descriptor *sd;
    ACL *sacl;

    sacl_size = sizeof(ACL) + FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart) + sid_len;
    sd_size = sizeof(struct security_descriptor) + sacl_size;
    if (!(sd = mem_alloc( sd_size )))
        return NULL;

    sd->control   = SE_SACL_PRESENT;
    sd->owner_len = 0;
    sd->group_len = 0;
    sd->sacl_len  = sacl_size;
    sd->dacl_len  = 0;

    sacl = (ACL *)(sd + 1);
    sacl->AclRevision = ACL_REVISION;
    sacl->Sbz1 = 0;
    sacl->AclSize = sacl_size;
    sacl->AceCount = 1;
    sacl->Sbz2 = 0;

    smla = (SYSTEM_MANDATORY_LABEL_ACE *)(sacl + 1);
    smla->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
    smla->Header.AceFlags = 0;
    smla->Header.AceSize = FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart) + sid_len;
    smla->Mask = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP;
    memcpy( &smla->SidStart, label_sid, sid_len );

    assert( sd_is_valid( sd, sd_size ) );
    return sd;
}

int token_assign_label( struct token *token, PSID label )
{
    struct security_descriptor *sd;
    int ret = 0;

    if ((sd = create_security_label_sd( token, label )))
    {
        ret = set_sd_defaults_from_token( &token->obj, sd, LABEL_SECURITY_INFORMATION, token );
        free( sd );
    }

    return ret;
}

struct token *get_token_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct token *)get_handle_obj( process, handle, access, &token_ops );
}

struct token *token_create_admin( void )
{
    struct token *token = NULL;
    static const SID_IDENTIFIER_AUTHORITY nt_authority = { SECURITY_NT_AUTHORITY };
    static const unsigned int alias_admins_subauth[] = { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS };
    static const unsigned int alias_users_subauth[] = { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS };
    /* on Windows, this value changes every time the user logs on */
    static const unsigned int logon_subauth[] = { SECURITY_LOGON_IDS_RID, 0, 1 /* FIXME: should be randomly generated when tokens are inherited by new processes */ };
    PSID alias_admins_sid;
    PSID alias_users_sid;
    PSID logon_sid;
    const SID *user_sid = security_unix_uid_to_sid( getuid() );
    ACL *default_dacl = create_default_dacl( user_sid );

    alias_admins_sid = security_sid_alloc( &nt_authority, ARRAY_SIZE( alias_admins_subauth ),
                                           alias_admins_subauth );
    alias_users_sid = security_sid_alloc( &nt_authority, ARRAY_SIZE( alias_users_subauth ),
                                          alias_users_subauth );
    logon_sid = security_sid_alloc( &nt_authority, ARRAY_SIZE( logon_subauth ), logon_subauth );

    if (alias_admins_sid && alias_users_sid && logon_sid && default_dacl)
    {
        const LUID_AND_ATTRIBUTES admin_privs[] =
        {
            { SeChangeNotifyPrivilege        , SE_PRIVILEGE_ENABLED },
            { SeSecurityPrivilege            , 0                    },
            { SeBackupPrivilege              , 0                    },
            { SeRestorePrivilege             , 0                    },
            { SeSystemtimePrivilege          , 0                    },
            { SeShutdownPrivilege            , 0                    },
            { SeRemoteShutdownPrivilege      , 0                    },
            { SeTakeOwnershipPrivilege       , 0                    },
            { SeDebugPrivilege               , 0                    },
            { SeSystemEnvironmentPrivilege   , 0                    },
            { SeSystemProfilePrivilege       , 0                    },
            { SeProfileSingleProcessPrivilege, 0                    },
            { SeIncreaseBasePriorityPrivilege, 0                    },
            { SeLoadDriverPrivilege          , SE_PRIVILEGE_ENABLED },
            { SeCreatePagefilePrivilege      , 0                    },
            { SeIncreaseQuotaPrivilege       , 0                    },
            { SeUndockPrivilege              , 0                    },
            { SeManageVolumePrivilege        , 0                    },
            { SeImpersonatePrivilege         , SE_PRIVILEGE_ENABLED },
            { SeCreateGlobalPrivilege        , SE_PRIVILEGE_ENABLED },
        };
        /* note: we don't include non-builtin groups here for the user -
         * telling us these is the job of a client-side program */
        const SID_AND_ATTRIBUTES admin_groups[] =
        {
            { security_world_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
            { security_local_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
            { security_interactive_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
            { security_authenticated_user_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
            { security_domain_users_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY|SE_GROUP_OWNER },
            { alias_admins_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY|SE_GROUP_OWNER },
            { alias_users_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
            { logon_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY|SE_GROUP_LOGON_ID },
        };
        static const TOKEN_SOURCE admin_source = {"SeMgr", {0, 0}};
        token = create_token( TRUE, user_sid, admin_groups, ARRAY_SIZE( admin_groups ),
                              admin_privs, ARRAY_SIZE( admin_privs ), default_dacl,
                              admin_source, NULL, -1 );
        /* we really need a primary group */
        assert( token->primary_group );
    }

    free( logon_sid );
    free( alias_admins_sid );
    free( alias_users_sid );
    free( default_dacl );

    return token;
}

static struct privilege *token_find_privilege( struct token *token, const LUID *luid, int enabled_only )
{
    struct privilege *privilege;
    LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
    {
        if (is_equal_luid( luid, &privilege->luid ))
        {
            if (enabled_only && !privilege->enabled)
                return NULL;
            return privilege;
        }
    }
    return NULL;
}

static unsigned int token_adjust_privileges( struct token *token, const LUID_AND_ATTRIBUTES *privs,
                                             unsigned int count, LUID_AND_ATTRIBUTES *mod_privs,
                                             unsigned int mod_privs_count )
{
    unsigned int i, modified_count = 0;

    /* mark as modified */
    allocate_luid( &token->modified_id );

    for (i = 0; i < count; i++)
    {
        struct privilege *privilege =
            token_find_privilege( token, &privs[i].Luid, FALSE );
        if (!privilege)
        {
            set_error( STATUS_NOT_ALL_ASSIGNED );
            continue;
        }

        if (privs[i].Attributes & SE_PRIVILEGE_REMOVED)
            privilege_remove( privilege );
        else
        {
            /* save previous state for caller */
            if (mod_privs_count)
            {
                luid_and_attr_from_privilege(mod_privs, privilege);
                mod_privs++;
                mod_privs_count--;
                modified_count++;
            }

            if (privs[i].Attributes & SE_PRIVILEGE_ENABLED)
                privilege->enabled = TRUE;
            else
                privilege->enabled = FALSE;
        }
    }
    return modified_count;
}

static void token_disable_privileges( struct token *token )
{
    struct privilege *privilege;

    /* mark as modified */
    allocate_luid( &token->modified_id );

    LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
        privilege->enabled = FALSE;
}

int token_check_privileges( struct token *token, int all_required,
                            const LUID_AND_ATTRIBUTES *reqprivs,
                            unsigned int count, LUID_AND_ATTRIBUTES *usedprivs)
{
    unsigned int i, enabled_count = 0;

    for (i = 0; i < count; i++)
    {
        struct privilege *privilege = 
            token_find_privilege( token, &reqprivs[i].Luid, TRUE );

        if (usedprivs)
            usedprivs[i] = reqprivs[i];

        if (privilege && privilege->enabled)
        {
            enabled_count++;
            if (usedprivs)
                usedprivs[i].Attributes |= SE_PRIVILEGE_USED_FOR_ACCESS;
        }
    }

    if (all_required)
        return (enabled_count == count);
    else
        return (enabled_count > 0);
}

int token_sid_present( struct token *token, const SID *sid, int deny )
{
    struct group *group;

    if (security_equal_sid( token->user, sid )) return TRUE;

    LIST_FOR_EACH_ENTRY( group, &token->groups, struct group, entry )
    {
        if (!group->enabled) continue;
        if (group->deny_only && !deny) continue;

        if (security_equal_sid( &group->sid, sid )) return TRUE;
    }

    return FALSE;
}

/* Checks access to a security descriptor. 'sd' must have been validated by
 * caller. It returns STATUS_SUCCESS if call succeeded or an error indicating
 * the reason. 'status' parameter will indicate if access is granted or denied.
 *
 * If both returned value and 'status' are STATUS_SUCCESS then access is granted.
 */
static unsigned int token_access_check( struct token *token,
                                 const struct security_descriptor *sd,
                                 unsigned int desired_access,
                                 LUID_AND_ATTRIBUTES *privs,
                                 unsigned int *priv_count,
                                 const GENERIC_MAPPING *mapping,
                                 unsigned int *granted_access,
                                 unsigned int *status )
{
    unsigned int current_access = 0;
    unsigned int denied_access = 0;
    ULONG i;
    const ACL *dacl;
    int dacl_present;
    const ACE_HEADER *ace;
    const SID *owner;

    /* assume no access rights */
    *granted_access = 0;

    /* fail if desired_access contains generic rights */
    if (desired_access & (GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE|GENERIC_ALL))
    {
        if (priv_count) *priv_count = 0;
        return STATUS_GENERIC_NOT_MAPPED;
    }

    dacl = sd_get_dacl( sd, &dacl_present );
    owner = sd_get_owner( sd );
    if (!owner || !sd_get_group( sd ))
    {
        if (priv_count) *priv_count = 0;
        return STATUS_INVALID_SECURITY_DESCR;
    }

    /* 1: Grant desired access if the object is unprotected */
    if (!dacl_present || !dacl)
    {
        if (priv_count) *priv_count = 0;
        if (desired_access & MAXIMUM_ALLOWED)
            *granted_access = mapping->GenericAll;
        else
            *granted_access = desired_access;
        return *status = STATUS_SUCCESS;
    }

    /* 2: Check if caller wants access to system security part. Note: access
     * is only granted if specifically asked for */
    if (desired_access & ACCESS_SYSTEM_SECURITY)
    {
        const LUID_AND_ATTRIBUTES security_priv = { SeSecurityPrivilege, 0 };
        LUID_AND_ATTRIBUTES retpriv = security_priv;
        if (token_check_privileges( token, TRUE, &security_priv, 1, &retpriv ))
        {
            if (priv_count)
            {
                /* assumes that there will only be one privilege to return */
                if (*priv_count >= 1)
                {
                    *priv_count = 1;
                    *privs = retpriv;
                }
                else
                {
                    *priv_count = 1;
                    return STATUS_BUFFER_TOO_SMALL;
                }
            }
            current_access |= ACCESS_SYSTEM_SECURITY;
            if (desired_access == current_access)
            {
                *granted_access = current_access;
                return *status = STATUS_SUCCESS;
            }
        }
        else
        {
            if (priv_count) *priv_count = 0;
            *status = STATUS_PRIVILEGE_NOT_HELD;
            return STATUS_SUCCESS;
        }
    }
    else if (priv_count) *priv_count = 0;

    /* 3: Check whether the token is the owner */
    /* NOTE: SeTakeOwnershipPrivilege is not checked for here - it is instead
     * checked when a "set owner" call is made, overriding the access rights
     * determined here. */
    if (token_sid_present( token, owner, FALSE ))
    {
        current_access |= (READ_CONTROL | WRITE_DAC);
        if (desired_access == current_access)
        {
            *granted_access = current_access;
            return *status = STATUS_SUCCESS;
        }
    }

    /* 4: Grant rights according to the DACL */
    ace = (const ACE_HEADER *)(dacl + 1);
    for (i = 0; i < dacl->AceCount; i++, ace = ace_next( ace ))
    {
        const ACCESS_ALLOWED_ACE *aa_ace;
        const ACCESS_DENIED_ACE *ad_ace;
        const SID *sid;

        if (ace->AceFlags & INHERIT_ONLY_ACE)
            continue;

        switch (ace->AceType)
        {
        case ACCESS_DENIED_ACE_TYPE:
            ad_ace = (const ACCESS_DENIED_ACE *)ace;
            sid = (const SID *)&ad_ace->SidStart;
            if (token_sid_present( token, sid, TRUE ))
            {
                unsigned int access = ad_ace->Mask;
                map_generic_mask(&access, mapping);
                if (desired_access & MAXIMUM_ALLOWED)
                    denied_access |= access;
                else
                {
                    denied_access |= (access & ~current_access);
                    if (desired_access & access) goto done;
                }
            }
            break;
        case ACCESS_ALLOWED_ACE_TYPE:
            aa_ace = (const ACCESS_ALLOWED_ACE *)ace;
            sid = (const SID *)&aa_ace->SidStart;
            if (token_sid_present( token, sid, FALSE ))
            {
                unsigned int access = aa_ace->Mask;
                map_generic_mask(&access, mapping);
                if (desired_access & MAXIMUM_ALLOWED)
                    current_access |= access;
                else
                    current_access |= (access & ~denied_access);
            }
            break;
        }

        /* don't bother carrying on checking if we've already got all of
            * rights we need */
        if (desired_access == *granted_access)
            break;
    }

done:
    if (desired_access & MAXIMUM_ALLOWED)
        *granted_access = current_access & ~denied_access;
    else
        if ((current_access & desired_access) == desired_access)
            *granted_access = current_access & desired_access;
        else
            *granted_access = 0;

    *status = *granted_access ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
    return STATUS_SUCCESS;
}

const ACL *token_get_default_dacl( struct token *token )
{
    return token->default_dacl;
}

const SID *token_get_user( struct token *token )
{
    return token->user;
}

const SID *token_get_primary_group( struct token *token )
{
    return token->primary_group;
}

int check_object_access(struct token *token, struct object *obj, unsigned int *access)
{
    GENERIC_MAPPING mapping;
    unsigned int status;
    int res;

    if (!token)
        token = current->token ? current->token : current->process->token;

    mapping.GenericAll = obj->ops->map_access( obj, GENERIC_ALL );

    if (!obj->sd)
    {
        if (*access & MAXIMUM_ALLOWED)
            *access = mapping.GenericAll;
        return TRUE;
    }

    mapping.GenericRead  = obj->ops->map_access( obj, GENERIC_READ );
    mapping.GenericWrite = obj->ops->map_access( obj, GENERIC_WRITE );
    mapping.GenericExecute = obj->ops->map_access( obj, GENERIC_EXECUTE );

    res = token_access_check( token, obj->sd, *access, NULL, NULL,
                              &mapping, access, &status ) == STATUS_SUCCESS &&
          status == STATUS_SUCCESS;

    if (!res) set_error( STATUS_ACCESS_DENIED );
    return res;
}


/* open a security token */
DECL_HANDLER(open_token)
{
    if (req->flags & OPEN_TOKEN_THREAD)
    {
        struct thread *thread = get_thread_from_handle( req->handle, 0 );
        if (thread)
        {
            if (thread->token)
            {
                if (!thread->token->primary && thread->token->impersonation_level <= SecurityAnonymous)
                    set_error( STATUS_CANT_OPEN_ANONYMOUS );
                else
                    reply->token = alloc_handle( current->process, thread->token,
                                                 req->access, req->attributes );
            }
            else
                set_error( STATUS_NO_TOKEN );
            release_object( thread );
        }
    }
    else
    {
        struct process *process = get_process_from_handle( req->handle, 0 );
        if (process)
        {
            if (process->token)
                reply->token = alloc_handle( current->process, process->token, req->access,
                                             req->attributes );
            else
                set_error( STATUS_NO_TOKEN );
            release_object( process );
        }
    }
}

/* adjust the privileges held by a token */
DECL_HANDLER(adjust_token_privileges)
{
    struct token *token;
    unsigned int access = TOKEN_ADJUST_PRIVILEGES;

    if (req->get_modified_state) access |= TOKEN_QUERY;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 access, &token_ops )))
    {
        const LUID_AND_ATTRIBUTES *privs = get_req_data();
        LUID_AND_ATTRIBUTES *modified_privs = NULL;
        unsigned int priv_count = get_req_data_size() / sizeof(LUID_AND_ATTRIBUTES);
        unsigned int modified_priv_count = 0;

        if (req->get_modified_state && !req->disable_all)
        {
            unsigned int i;
            /* count modified privs */
            for (i = 0; i < priv_count; i++)
            {
                struct privilege *privilege =
                    token_find_privilege( token, &privs[i].Luid, FALSE );
                if (privilege && req->get_modified_state)
                    modified_priv_count++;
            }
            reply->len = modified_priv_count;
            modified_priv_count = min( modified_priv_count, get_reply_max_size() / sizeof(*modified_privs) );
            if (modified_priv_count)
                modified_privs = set_reply_data_size( modified_priv_count * sizeof(*modified_privs) );
        }
        reply->len = modified_priv_count * sizeof(*modified_privs);

        if (req->disable_all)
            token_disable_privileges( token );
        else
            token_adjust_privileges( token, privs, priv_count, modified_privs, modified_priv_count );

        release_object( token );
    }
}

/* retrieves the list of privileges that may be held be the token */
DECL_HANDLER(get_token_privileges)
{
    struct token *token;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        int priv_count = 0;
        LUID_AND_ATTRIBUTES *privs;
        struct privilege *privilege;

        LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
            priv_count++;

        reply->len = priv_count * sizeof(*privs);
        if (reply->len <= get_reply_max_size())
        {
            privs = set_reply_data_size( priv_count * sizeof(*privs) );
            if (privs)
            {
                int i = 0;
                LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
                {
                    luid_and_attr_from_privilege( &privs[i], privilege );
                    i++;
                }
            }
        }
        else
            set_error(STATUS_BUFFER_TOO_SMALL);

        release_object( token );
    }
}

/* creates a duplicate of the token */
DECL_HANDLER(duplicate_token)
{
    struct token *src_token;
    struct unicode_str name;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, NULL );

    if (!objattr) return;

    if ((src_token = (struct token *)get_handle_obj( current->process, req->handle,
                                                     TOKEN_DUPLICATE,
                                                     &token_ops )))
    {
        struct token *token = token_duplicate( src_token, req->primary, req->impersonation_level, sd, NULL, 0, NULL, 0 );
        if (token)
        {
            reply->new_handle = alloc_handle_no_access_check( current->process, token, req->access, objattr->attributes );
            release_object( token );
        }
        release_object( src_token );
    }
}

/* creates a restricted version of a token */
DECL_HANDLER(filter_token)
{
    struct token *src_token;

    if ((src_token = (struct token *)get_handle_obj( current->process, req->handle, TOKEN_DUPLICATE, &token_ops )))
    {
        const LUID_AND_ATTRIBUTES *filter_privileges = get_req_data();
        unsigned int priv_count, group_count;
        const SID *filter_groups;
        struct token *token;

        priv_count = min( req->privileges_size, get_req_data_size() ) / sizeof(LUID_AND_ATTRIBUTES);
        filter_groups = (const SID *)((char *)filter_privileges + priv_count * sizeof(LUID_AND_ATTRIBUTES));
        group_count = get_sid_count( filter_groups, get_req_data_size() - priv_count * sizeof(LUID_AND_ATTRIBUTES) );

        token = token_duplicate( src_token, src_token->primary, src_token->impersonation_level, NULL,
                                 filter_privileges, priv_count, filter_groups, group_count );
        if (token)
        {
            unsigned int access = get_handle_access( current->process, req->handle );
            reply->new_handle = alloc_handle_no_access_check( current->process, token, access, 0 );
            release_object( token );
        }
        release_object( src_token );
    }
}

/* checks the specified privileges are held by the token */
DECL_HANDLER(check_token_privileges)
{
    struct token *token;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        unsigned int count = get_req_data_size() / sizeof(LUID_AND_ATTRIBUTES);

        if (!token->primary && token->impersonation_level <= SecurityAnonymous)
            set_error( STATUS_BAD_IMPERSONATION_LEVEL );
        else if (get_reply_max_size() >= count * sizeof(LUID_AND_ATTRIBUTES))
        {
            LUID_AND_ATTRIBUTES *usedprivs = set_reply_data_size( count * sizeof(*usedprivs) );
            reply->has_privileges = token_check_privileges( token, req->all_required, get_req_data(), count, usedprivs );
        }
        else
            set_error( STATUS_BUFFER_OVERFLOW );
        release_object( token );
    }
}

/* checks that a user represented by a token is allowed to access an object
 * represented by a security descriptor */
DECL_HANDLER(access_check)
{
    data_size_t sd_size = get_req_data_size();
    const struct security_descriptor *sd = get_req_data();
    struct token *token;

    if (!sd_is_valid( sd, sd_size ))
    {
        set_error( STATUS_ACCESS_VIOLATION );
        return;
    }

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        GENERIC_MAPPING mapping;
        unsigned int status;
        LUID_AND_ATTRIBUTES priv;
        unsigned int priv_count = 1;

        memset(&priv, 0, sizeof(priv));

        /* only impersonation tokens may be used with this function */
        if (token->primary)
        {
            set_error( STATUS_NO_IMPERSONATION_TOKEN );
            release_object( token );
            return;
        }
        /* anonymous impersonation tokens can't be used */
        if (token->impersonation_level <= SecurityAnonymous)
        {
            set_error( STATUS_BAD_IMPERSONATION_LEVEL );
            release_object( token );
            return;
        }

        mapping.GenericRead = req->mapping_read;
        mapping.GenericWrite = req->mapping_write;
        mapping.GenericExecute = req->mapping_execute;
        mapping.GenericAll = req->mapping_all;

        status = token_access_check(
            token, sd, req->desired_access, &priv, &priv_count, &mapping,
            &reply->access_granted, &reply->access_status );

        reply->privileges_len = priv_count*sizeof(LUID_AND_ATTRIBUTES);

        if ((priv_count > 0) && (reply->privileges_len <= get_reply_max_size()))
        {
            LUID_AND_ATTRIBUTES *privs = set_reply_data_size( priv_count * sizeof(*privs) );
            memcpy( privs, &priv, sizeof(priv) );
        }

        set_error( status );
        release_object( token );
    }
}

/* retrieves an SID from the token */
DECL_HANDLER(get_token_sid)
{
    struct token *token;

    reply->sid_len = 0;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle, TOKEN_QUERY, &token_ops )))
    {
        const SID *sid = NULL;

        switch (req->which_sid)
        {
        case TokenUser:
            assert(token->user);
            sid = token->user;
            break;
        case TokenPrimaryGroup:
            sid = token->primary_group;
            break;
        case TokenOwner:
            sid = token->owner;
            break;
        case TokenLogonSid:
            sid = (const SID *)&builtin_logon_sid;
            break;
        default:
            set_error( STATUS_INVALID_PARAMETER );
            break;
        }

        if (sid)
        {
            reply->sid_len = security_sid_len( sid );
            if (reply->sid_len <= get_reply_max_size()) set_reply_data( sid, reply->sid_len );
            else set_error( STATUS_BUFFER_TOO_SMALL );
        }
        release_object( token );
    }
}

/* retrieves the groups that the user represented by the token belongs to */
DECL_HANDLER(get_token_groups)
{
    struct token *token;

    reply->user_len = 0;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        size_t size_needed = sizeof(struct token_groups);
        size_t sid_size = 0;
        unsigned int group_count = 0;
        const struct group *group;

        LIST_FOR_EACH_ENTRY( group, &token->groups, const struct group, entry )
        {
            group_count++;
            sid_size += security_sid_len( &group->sid );
        }
        size_needed += sid_size;
        /* attributes size */
        size_needed += sizeof(unsigned int) * group_count;

        /* reply buffer contains size_needed bytes formatted as:

           unsigned int count;
           unsigned int attrib[count];
           char sid_data[];

           user_len includes extra data needed for TOKEN_GROUPS representation,
           required caller buffer size calculated here to avoid extra server call */
        reply->user_len = FIELD_OFFSET( TOKEN_GROUPS, Groups[group_count] ) + sid_size;

        if (reply->user_len <= get_reply_max_size())
        {
            struct token_groups *tg = set_reply_data_size( size_needed );
            if (tg)
            {
                unsigned int *attr_ptr = (unsigned int *)(tg + 1);
                SID *sid_ptr = (SID *)(attr_ptr + group_count);

                tg->count = group_count;

                LIST_FOR_EACH_ENTRY( group, &token->groups, const struct group, entry )
                {

                    *attr_ptr = 0;
                    if (group->mandatory) *attr_ptr |= SE_GROUP_MANDATORY;
                    if (group->def) *attr_ptr |= SE_GROUP_ENABLED_BY_DEFAULT;
                    if (group->enabled) *attr_ptr |= SE_GROUP_ENABLED;
                    if (group->owner) *attr_ptr |= SE_GROUP_OWNER;
                    if (group->deny_only) *attr_ptr |= SE_GROUP_USE_FOR_DENY_ONLY;
                    if (group->resource) *attr_ptr |= SE_GROUP_RESOURCE;
                    if (group->logon) *attr_ptr |= SE_GROUP_LOGON_ID;

                    memcpy(sid_ptr, &group->sid, security_sid_len( &group->sid ));

                    sid_ptr = (SID *)((char *)sid_ptr + security_sid_len( &group->sid ));
                    attr_ptr++;
                }
            }
        }
        else set_error( STATUS_BUFFER_TOO_SMALL );

        release_object( token );
    }
}

DECL_HANDLER(get_token_impersonation_level)
{
    struct token *token;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        if (token->primary)
            set_error( STATUS_INVALID_PARAMETER );
        else
            reply->impersonation_level = token->impersonation_level;

        release_object( token );
    }
}

DECL_HANDLER(get_token_statistics)
{
    struct token *token;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        reply->token_id = token->token_id;
        reply->modified_id = token->modified_id;
        reply->primary = token->primary;
        reply->impersonation_level = token->impersonation_level;
        reply->group_count = list_count( &token->groups );
        reply->privilege_count = list_count( &token->privileges );

        release_object( token );
    }
}

DECL_HANDLER(get_token_default_dacl)
{
    struct token *token;

    reply->acl_len = 0;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY,
                                                 &token_ops )))
    {
        if (token->default_dacl)
            reply->acl_len = token->default_dacl->AclSize;

        if (reply->acl_len <= get_reply_max_size())
        {
            ACL *acl_reply = set_reply_data_size( reply->acl_len );
            if (acl_reply)
                memcpy( acl_reply, token->default_dacl, reply->acl_len );
        }
        else set_error( STATUS_BUFFER_TOO_SMALL );

        release_object( token );
    }
}

DECL_HANDLER(set_token_default_dacl)
{
    struct token *token;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_ADJUST_DEFAULT,
                                                 &token_ops )))
    {
        const ACL *acl = get_req_data();
        unsigned int acl_size = get_req_data_size();

        free( token->default_dacl );
        token->default_dacl = NULL;

        if (acl_size)
            token->default_dacl = memdup( acl, acl_size );

        release_object( token );
    }
}
