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
#include <sys/types.h>
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

const struct luid SeIncreaseQuotaPrivilege        = {  5, 0 };
const struct luid SeTcbPrivilege                  = {  7, 0 };
const struct luid SeSecurityPrivilege             = {  8, 0 };
const struct luid SeTakeOwnershipPrivilege        = {  9, 0 };
const struct luid SeLoadDriverPrivilege           = { 10, 0 };
const struct luid SeSystemProfilePrivilege        = { 11, 0 };
const struct luid SeSystemtimePrivilege           = { 12, 0 };
const struct luid SeProfileSingleProcessPrivilege = { 13, 0 };
const struct luid SeIncreaseBasePriorityPrivilege = { 14, 0 };
const struct luid SeCreatePagefilePrivilege       = { 15, 0 };
const struct luid SeBackupPrivilege               = { 17, 0 };
const struct luid SeRestorePrivilege              = { 18, 0 };
const struct luid SeShutdownPrivilege             = { 19, 0 };
const struct luid SeDebugPrivilege                = { 20, 0 };
const struct luid SeSystemEnvironmentPrivilege    = { 22, 0 };
const struct luid SeChangeNotifyPrivilege         = { 23, 0 };
const struct luid SeRemoteShutdownPrivilege       = { 24, 0 };
const struct luid SeUndockPrivilege               = { 25, 0 };
const struct luid SeManageVolumePrivilege         = { 28, 0 };
const struct luid SeImpersonatePrivilege          = { 29, 0 };
const struct luid SeCreateGlobalPrivilege         = { 30, 0 };

struct sid_attrs
{
    const struct sid *sid;
    unsigned int      attrs;
};

const struct sid world_sid = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, { SECURITY_WORLD_RID } };
const struct sid local_system_sid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_LOCAL_SYSTEM_RID } };
const struct sid local_user_sid = { SID_REVISION, 5, SECURITY_NT_AUTHORITY, { SECURITY_NT_NON_UNIQUE, 0, 0, 0, 1000 } };
const struct sid builtin_admins_sid = { SID_REVISION, 2, SECURITY_NT_AUTHORITY, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } };
const struct sid builtin_users_sid = { SID_REVISION, 2, SECURITY_NT_AUTHORITY, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } };
const struct sid domain_users_sid = { SID_REVISION, 5, SECURITY_NT_AUTHORITY, { SECURITY_NT_NON_UNIQUE, 0, 0, 0, DOMAIN_GROUP_RID_USERS } };

static const struct sid local_sid = { SID_REVISION, 1, SECURITY_LOCAL_SID_AUTHORITY, { SECURITY_LOCAL_RID } };
static const struct sid interactive_sid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_INTERACTIVE_RID } };
static const struct sid anonymous_logon_sid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_ANONYMOUS_LOGON_RID } };
static const struct sid authenticated_user_sid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_AUTHENTICATED_USER_RID } };
static const struct sid high_label_sid = { SID_REVISION, 1, SECURITY_MANDATORY_LABEL_AUTHORITY, { SECURITY_MANDATORY_HIGH_RID } };

static struct luid prev_luid_value = { 1000, 0 };

static const WCHAR token_name[] = {'T','o','k','e','n'};

struct type_descr token_type =
{
    { token_name, sizeof(token_name) },   /* name */
    TOKEN_ALL_ACCESS | SYNCHRONIZE,       /* valid_access */
    {                                     /* mapping */
        STANDARD_RIGHTS_READ | TOKEN_QUERY_SOURCE | TOKEN_QUERY | TOKEN_DUPLICATE,
        STANDARD_RIGHTS_WRITE | TOKEN_ADJUST_SESSIONID | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_GROUPS
        | TOKEN_ADJUST_PRIVILEGES,
        STANDARD_RIGHTS_EXECUTE | TOKEN_IMPERSONATE | TOKEN_ASSIGN_PRIMARY,
        TOKEN_ALL_ACCESS
    },
};

struct token
{
    struct object  obj;             /* object header */
    struct luid    token_id;        /* system-unique id of token */
    struct luid    modified_id;     /* new id allocated every time token is modified */
    struct list    privileges;      /* privileges available to the token */
    struct list    groups;          /* groups that the user of this token belongs to (sid_and_attributes) */
    struct sid    *user;            /* SID of user this token represents */
    struct sid    *owner;           /* SID of owner (points to user or one of groups) */
    struct sid    *primary_group;   /* SID of user's primary group (points to one of groups) */
    unsigned int   primary;         /* is this a primary or impersonation token? */
    unsigned int   session_id;      /* token session id */
    struct acl    *default_dacl;    /* the default DACL to assign to objects created by this user */
    int            impersonation_level; /* impersonation level this token is capable of if non-primary token */
    int            elevation;       /* elevation type */
};

struct privilege
{
    struct list entry;
    struct luid luid;
    unsigned    enabled  : 1; /* is the privilege currently enabled? */
    unsigned    def      : 1; /* is the privilege enabled by default? */
};

struct group
{
    struct list    entry;
    unsigned int   attrs;
    struct sid     sid;
};

static void token_dump( struct object *obj, int verbose );
static void token_destroy( struct object *obj );
static int token_set_sd( struct object *obj, const struct security_descriptor *sd,
                         unsigned int set_info );

static const struct object_ops token_ops =
{
    sizeof(struct token),      /* size */
    &token_type,               /* type */
    token_dump,                /* dump */
    no_add_queue,              /* add_queue */
    NULL,                      /* remove_queue */
    NULL,                      /* signaled */
    NULL,                      /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    token_set_sd,              /* set_sd */
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

static int token_set_sd( struct object *obj, const struct security_descriptor *sd,
                         unsigned int set_info )
{
    return default_set_sd( obj, sd, set_info & ~LABEL_SECURITY_INFORMATION );
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

const struct sid *security_unix_uid_to_sid( uid_t uid )
{
    /* very simple mapping: either the current user or not the current user */
    if (uid == getuid())
        return &local_user_sid;
    else
        return &anonymous_logon_sid;
}

static int acl_is_valid( const struct acl *acl, data_size_t size )
{
    ULONG i;
    const struct ace *ace;

    if (size < sizeof(*acl)) return FALSE;

    size = min(size, MAX_ACL_LEN);
    size -= sizeof(*acl);

    for (i = 0, ace = ace_first( acl ); i < acl->count; i++, ace = ace_next( ace ))
    {
        if (size < sizeof(*ace) || size < ace->size) return FALSE;
        size -= ace->size;
        switch (ace->type)
        {
        case ACCESS_DENIED_ACE_TYPE:
        case ACCESS_ALLOWED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
        case SYSTEM_MANDATORY_LABEL_ACE_TYPE:
            break;
        default:
            return FALSE;
        }
        if (!sid_valid_size( (const struct sid *)(ace + 1), ace->size - sizeof(*ace) )) return FALSE;
    }
    return TRUE;
}

static unsigned int get_sid_count( const struct sid *sid, data_size_t size )
{
    unsigned int count;

    for (count = 0; sid_valid_size( sid, size ); count++)
    {
        size -= sid_len( sid );
        sid = (const struct sid *)((char *)sid + sid_len( sid ));
    }
    return count;
}

/* checks whether all members of a security descriptor fit inside the size
 * of memory specified */
int sd_is_valid( const struct security_descriptor *sd, data_size_t size )
{
    size_t offset = sizeof(struct security_descriptor);
    const struct sid *group;
    const struct sid *owner;
    const struct acl *sacl;
    const struct acl *dacl;
    int dummy;

    if (size < offset)
        return FALSE;

    if (sd->owner_len >= offsetof(struct sid, sub_auth[255]) || offset + sd->owner_len > size) return FALSE;
    owner = sd_get_owner( sd );
    offset += sd->owner_len;
    if (owner && !sid_valid_size( owner, sd->owner_len )) return FALSE;

    if (sd->group_len >= offsetof(struct sid, sub_auth[255]) || offset + sd->group_len > size) return FALSE;
    group = sd_get_group( sd );
    offset += sd->group_len;
    if (group && !sid_valid_size( group, sd->group_len )) return FALSE;

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

    return TRUE;
}

/* extract security labels from SACL */
struct acl *extract_security_labels( const struct acl *sacl )
{
    size_t size = sizeof(*sacl);
    const struct ace *ace;
    struct ace *label_ace;
    unsigned int i, count = 0;
    struct acl *label_acl;

    for (i = 0, ace = ace_first( sacl ); i < sacl->count; i++, ace = ace_next( ace ))
    {
        if (ace->type == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            size += ace->size;
            count++;
        }
    }

    label_acl = mem_alloc( size );
    if (!label_acl) return NULL;

    label_acl->revision = sacl->revision;
    label_acl->pad1     = 0;
    label_acl->size     = size;
    label_acl->count    = count;
    label_acl->pad2     = 0;

    label_ace = ace_first( label_acl );
    for (i = 0, ace = ace_first( sacl ); i < sacl->count; i++, ace = ace_next( ace ))
        if (ace->type == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
            label_ace = mem_append( label_ace, ace, ace->size );

    return label_acl;
}

/* replace security labels in an existing SACL */
struct acl *replace_security_labels( const struct acl *old_sacl, const struct acl *new_sacl )
{
    const struct ace *ace;
    struct ace *replaced_ace;
    unsigned int i, count = 0;
    unsigned char revision = ACL_REVISION;
    struct acl *replaced_acl;
    data_size_t size = sizeof(*replaced_acl);

    if (old_sacl)
    {
        revision = max( revision, old_sacl->revision );
        for (i = 0, ace = ace_first( old_sacl ); i < old_sacl->count; i++, ace = ace_next( ace ))
        {
            if (ace->type == SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
            size += ace->size;
            count++;
        }
    }

    if (new_sacl)
    {
        revision = max( revision, new_sacl->revision );
        for (i = 0, ace = ace_first( new_sacl ); i < new_sacl->count; i++, ace = ace_next( ace ))
        {
            if (ace->type != SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
            size += ace->size;
            count++;
        }
    }
    if (size > MAX_ACL_LEN)
    {
        set_error( STATUS_INVALID_ACL );
        return NULL;
    }

    replaced_acl = mem_alloc( size );
    if (!replaced_acl) return NULL;

    replaced_acl->revision = revision;
    replaced_acl->pad1     = 0;
    replaced_acl->size     = size;
    replaced_acl->count    = count;
    replaced_acl->pad2     = 0;

    replaced_ace = (struct ace *)(replaced_acl + 1);
    if (old_sacl)
    {
        for (i = 0, ace = ace_first( old_sacl ); i < old_sacl->count; i++, ace = ace_next( ace ))
            if (ace->type != SYSTEM_MANDATORY_LABEL_ACE_TYPE)
                replaced_ace = mem_append( replaced_ace, ace, ace->size );
    }

    if (new_sacl)
    {
        for (i = 0, ace = ace_first( new_sacl ); i < new_sacl->count; i++, ace = ace_next( ace ))
            if (ace->type == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
                replaced_ace = mem_append( replaced_ace, ace, ace->size );
    }

    return replaced_acl;
}

static inline int is_equal_luid( struct luid luid1, struct luid luid2 )
{
    return (luid1.low_part == luid2.low_part && luid1.high_part == luid2.high_part);
}

static inline void allocate_luid( struct luid *luid )
{
    prev_luid_value.low_part++;
    *luid = prev_luid_value;
}

DECL_HANDLER( allocate_locally_unique_id )
{
    allocate_luid( &reply->luid );
}

static inline struct luid_attr luid_and_attr_from_privilege( const struct privilege *in )
{
    struct luid_attr ret = { in->luid };

    ret.attrs = (in->enabled ? SE_PRIVILEGE_ENABLED : 0) |
                (in->def ? SE_PRIVILEGE_ENABLED_BY_DEFAULT : 0);
    return ret;
}

static struct privilege *privilege_add( struct token *token, struct luid luid, int enabled )
{
    struct privilege *privilege = mem_alloc( sizeof(*privilege) );
    if (privilege)
    {
        privilege->luid = luid;
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
static struct token *create_token( unsigned int primary, unsigned int session_id, const struct sid *user,
                                   const struct sid_attrs *groups, unsigned int group_count,
                                   const struct luid_attr *privs, unsigned int priv_count,
                                   const struct acl *default_dacl, const struct luid *modified_id,
                                   unsigned int primary_group, int impersonation_level, int elevation )
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
        token->session_id = session_id;
        /* primary tokens don't have impersonation levels */
        if (primary)
            token->impersonation_level = -1;
        else
            token->impersonation_level = impersonation_level;
        token->default_dacl = NULL;
        token->primary_group = NULL;
        token->elevation = elevation;

        /* copy user */
        token->user = memdup( user, sid_len( user ));
        if (!token->user)
        {
            release_object( token );
            return NULL;
        }

        /* copy groups */
        for (i = 0; i < group_count; i++)
        {
            size_t size = offsetof( struct group, sid.sub_auth[groups[i].sid->sub_count] );
            struct group *group = mem_alloc( size );

            if (!group)
            {
                release_object( token );
                return NULL;
            }
            group->attrs = groups[i].attrs;
            copy_sid( &group->sid, groups[i].sid );
            list_add_tail( &token->groups, &group->entry );

            if (primary_group == i)
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
            if (!privilege_add( token, privs[i].luid, privs[i].attrs & SE_PRIVILEGE_ENABLED ))
            {
                release_object( token );
                return NULL;
            }
        }

        if (default_dacl)
        {
            token->default_dacl = memdup( default_dacl, default_dacl->size );
            if (!token->default_dacl)
            {
                release_object( token );
                return NULL;
            }
        }
    }
    return token;
}

static int filter_group( struct group *group, const struct sid *filter, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if (equal_sid( &group->sid, filter )) return 1;
        filter = (const struct sid *)((char *)filter + sid_len( filter ));
    }

    return 0;
}

static int filter_privilege( struct privilege *privilege, const struct luid_attr *filter, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
        if (is_equal_luid( privilege->luid, filter[i].luid )) return 1;

    return 0;
}

struct token *token_duplicate( struct token *src_token, unsigned primary,
                               int impersonation_level, const struct security_descriptor *sd,
                               const struct luid_attr *remove_privs, unsigned int remove_priv_count,
                               const struct sid *remove_groups, unsigned int remove_group_count)
{
    const struct luid *modified_id =
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

    token = create_token( primary, src_token->session_id, src_token->user, NULL, 0,
                          NULL, 0, src_token->default_dacl, modified_id,
                          0, impersonation_level, src_token->elevation );
    if (!token) return token;

    /* copy groups */
    token->primary_group = NULL;
    LIST_FOR_EACH_ENTRY( group, &src_token->groups, struct group, entry )
    {
        size_t size = offsetof( struct group, sid.sub_auth[group->sid.sub_count] );
        struct group *newgroup = mem_alloc( size );
        if (!newgroup)
        {
            release_object( token );
            return NULL;
        }
        memcpy( newgroup, group, size );
        if (filter_group( group, remove_groups, remove_group_count ))
        {
            newgroup->attrs &= ~(SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT);
            newgroup->attrs |= SE_GROUP_USE_FOR_DENY_ONLY;
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
        if (!privilege_add( token, privilege->luid, privilege->enabled ))
        {
            release_object( token );
            return NULL;
        }
    }

    if (sd) default_set_sd( &token->obj, sd, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION );

    if (src_token->obj.sd)
    {
        const struct acl *sacl;
        const struct ace *ace;
        unsigned int i;
        int present;

        sacl = sd_get_sacl( src_token->obj.sd, &present );
        if (present)
        {
            for (i = 0, ace = ace_first( sacl ); i < sacl->count; i++, ace = ace_next( ace ))
            {
                if (ace->type != SYSTEM_MANDATORY_LABEL_ACE_TYPE) continue;
                token_assign_label( token, (const struct sid *)(ace + 1) );
            }
        }
    }

    return token;
}

static struct acl *create_default_dacl( const struct sid *user )
{
    struct ace *ace;
    struct acl *default_dacl;
    size_t default_dacl_size = sizeof(*default_dacl) + 2 * sizeof(*ace) +
                               sid_len( &local_system_sid ) + sid_len( user );

    default_dacl = mem_alloc( default_dacl_size );
    if (!default_dacl) return NULL;

    default_dacl->revision = ACL_REVISION;
    default_dacl->pad1     = 0;
    default_dacl->size     = default_dacl_size;
    default_dacl->count    = 2;
    default_dacl->pad2     = 0;

    /* GENERIC_ALL for Local System */
    ace = set_ace( ace_first( default_dacl ), &local_system_sid, ACCESS_ALLOWED_ACE_TYPE, 0, GENERIC_ALL );
    /* GENERIC_ALL for specified user */
    set_ace( ace_next( ace ), user, ACCESS_ALLOWED_ACE_TYPE, 0, GENERIC_ALL );

    return default_dacl;
}

struct sid_data
{
    SID_IDENTIFIER_AUTHORITY idauth;
    int count;
    unsigned int subauth[MAX_SUBAUTH_COUNT];
};

static struct security_descriptor *create_security_label_sd( struct token *token, const struct sid *label_sid )
{
    size_t sid_size = sid_len( label_sid ), sacl_size, sd_size;
    struct security_descriptor *sd;
    struct acl *sacl;

    sacl_size = sizeof(*sacl) + sizeof(struct ace) + sid_size;
    sd_size = sizeof(struct security_descriptor) + sacl_size;
    if (!(sd = mem_alloc( sd_size )))
        return NULL;

    sd->control   = SE_SACL_PRESENT;
    sd->owner_len = 0;
    sd->group_len = 0;
    sd->sacl_len  = sacl_size;
    sd->dacl_len  = 0;

    sacl = (struct acl *)(sd + 1);
    sacl->revision = ACL_REVISION;
    sacl->pad1     = 0;
    sacl->size     = sacl_size;
    sacl->count    = 1;
    sacl->pad2     = 0;

    set_ace( ace_first( sacl ), label_sid, SYSTEM_MANDATORY_LABEL_ACE_TYPE, 0,
             SYSTEM_MANDATORY_LABEL_NO_WRITE_UP );
    assert( sd_is_valid( sd, sd_size ) );
    return sd;
}

int token_assign_label( struct token *token, const struct sid *label )
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

struct token *token_create_admin( unsigned primary, int impersonation_level, int elevation, unsigned int session_id )
{
    struct token *token = NULL;
    struct sid alias_admins_sid = { SID_REVISION, 2, SECURITY_NT_AUTHORITY, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS }};
    struct sid alias_users_sid = { SID_REVISION, 2, SECURITY_NT_AUTHORITY, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS }};
    /* on Windows, this value changes every time the user logs on */
    struct sid logon_sid = { SID_REVISION, 3, SECURITY_NT_AUTHORITY, { SECURITY_LOGON_IDS_RID, 0, 0 /* FIXME: should be randomly generated when tokens are inherited by new processes */ }};
    const struct sid *user_sid = security_unix_uid_to_sid( getuid() );
    struct acl *default_dacl = create_default_dacl( &domain_users_sid );
    const struct luid_attr admin_privs[] =
    {
        { SeChangeNotifyPrivilege, SE_PRIVILEGE_ENABLED },
        { SeTcbPrivilege, 0 },
        { SeSecurityPrivilege, 0 },
        { SeBackupPrivilege, 0 },
        { SeRestorePrivilege, 0 },
        { SeSystemtimePrivilege, 0 },
        { SeShutdownPrivilege, 0 },
        { SeRemoteShutdownPrivilege, 0 },
        { SeTakeOwnershipPrivilege, 0 },
        { SeDebugPrivilege, 0 },
        { SeSystemEnvironmentPrivilege, 0 },
        { SeSystemProfilePrivilege, 0 },
        { SeProfileSingleProcessPrivilege, 0 },
        { SeIncreaseBasePriorityPrivilege, 0 },
        { SeLoadDriverPrivilege, SE_PRIVILEGE_ENABLED },
        { SeCreatePagefilePrivilege, 0 },
        { SeIncreaseQuotaPrivilege, 0 },
        { SeUndockPrivilege, 0 },
        { SeManageVolumePrivilege, 0 },
        { SeImpersonatePrivilege, SE_PRIVILEGE_ENABLED },
        { SeCreateGlobalPrivilege, SE_PRIVILEGE_ENABLED },
    };
    /* note: we don't include non-builtin groups here for the user -
     * telling us these is the job of a client-side program */
    const struct sid_attrs admin_groups[] =
    {
        { &world_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
        { &local_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
        { &interactive_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
        { &authenticated_user_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
        { &domain_users_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY|SE_GROUP_OWNER },
        { &alias_admins_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY|SE_GROUP_OWNER },
        { &alias_users_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY },
        { &logon_sid, SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY|SE_GROUP_LOGON_ID },
    };

    token = create_token( primary, session_id, user_sid, admin_groups, ARRAY_SIZE( admin_groups ),
                          admin_privs, ARRAY_SIZE( admin_privs ), default_dacl,
                          NULL, 4 /* domain_users */, impersonation_level, elevation );
    /* we really need a primary group */
    assert( token->primary_group );

    /* Assign a high security label to the token. The default would be medium
     * but Wine provides admin access to all applications right now so high
     * makes more sense for the time being. */
    if (!token_assign_label( token, &high_label_sid ))
    {
        release_object( token );
        return NULL;
    }

    free( default_dacl );
    return token;
}

static struct privilege *token_find_privilege( struct token *token, struct luid luid, int enabled_only )
{
    struct privilege *privilege;
    LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
    {
        if (is_equal_luid( luid, privilege->luid ))
        {
            if (enabled_only && !privilege->enabled)
                return NULL;
            return privilege;
        }
    }
    return NULL;
}

static unsigned int token_adjust_privileges( struct token *token, const struct luid_attr *privs,
                                             unsigned int count, struct luid_attr *mod_privs,
                                             unsigned int mod_privs_count )
{
    unsigned int i, modified_count = 0;

    /* mark as modified */
    allocate_luid( &token->modified_id );

    for (i = 0; i < count; i++)
    {
        struct privilege *privilege = token_find_privilege( token, privs[i].luid, FALSE );
        if (!privilege)
        {
            set_error( STATUS_NOT_ALL_ASSIGNED );
            continue;
        }

        if (privs[i].attrs & SE_PRIVILEGE_REMOVED) privilege_remove( privilege );
        else
        {
            /* save previous state for caller */
            if (mod_privs_count)
            {
                *mod_privs++ = luid_and_attr_from_privilege( privilege );
                mod_privs_count--;
                modified_count++;
            }
            privilege->enabled = !!(privs[i].attrs & SE_PRIVILEGE_ENABLED);
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

int token_check_privileges( struct token *token, int all_required, const struct luid_attr *reqprivs,
                            unsigned int count, struct luid_attr *usedprivs)
{
    unsigned int i, enabled_count = 0;

    for (i = 0; i < count; i++)
    {
        struct privilege *privilege = token_find_privilege( token, reqprivs[i].luid, TRUE );

        if (usedprivs)
            usedprivs[i] = reqprivs[i];

        if (privilege && privilege->enabled)
        {
            enabled_count++;
            if (usedprivs) usedprivs[i].attrs |= SE_PRIVILEGE_USED_FOR_ACCESS;
        }
    }

    if (all_required)
        return (enabled_count == count);
    else
        return (enabled_count > 0);
}

int token_sid_present( struct token *token, const struct sid *sid, int deny )
{
    struct group *group;

    if (equal_sid( token->user, sid )) return TRUE;

    LIST_FOR_EACH_ENTRY( group, &token->groups, struct group, entry )
    {
        if (!(group->attrs & SE_GROUP_ENABLED)) continue;
        if (!deny && (group->attrs & SE_GROUP_USE_FOR_DENY_ONLY)) continue;
        if (equal_sid( &group->sid, sid )) return TRUE;
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
                                 struct luid_attr *privs,
                                 unsigned int *priv_count,
                                 const struct generic_map *mapping,
                                 unsigned int *granted_access,
                                 unsigned int *status )
{
    unsigned int current_access = 0;
    unsigned int denied_access = 0;
    ULONG i;
    const struct acl *dacl;
    int dacl_present;
    const struct ace *ace;
    const struct sid *owner;

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
            *granted_access = mapping->all;
        else
            *granted_access = desired_access;
        return *status = STATUS_SUCCESS;
    }

    /* 2: Check if caller wants access to system security part. Note: access
     * is only granted if specifically asked for */
    if (desired_access & ACCESS_SYSTEM_SECURITY)
    {
        const struct luid_attr security_priv = { SeSecurityPrivilege, 0 };
        struct luid_attr retpriv = security_priv;
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
        current_access |= (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE);
        if (desired_access == current_access)
        {
            *granted_access = current_access;
            return *status = STATUS_SUCCESS;
        }
    }

    /* 4: Grant rights according to the DACL */
    for (i = 0, ace = ace_first( dacl ); i < dacl->count; i++, ace = ace_next( ace ))
    {
        const struct sid *sid = (const struct sid *)(ace + 1);

        if (ace->flags & INHERIT_ONLY_ACE) continue;

        switch (ace->type)
        {
        case ACCESS_DENIED_ACE_TYPE:
            if (token_sid_present( token, sid, TRUE ))
            {
                unsigned int access = map_access( ace->mask, mapping );
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
            if (token_sid_present( token, sid, FALSE ))
            {
                unsigned int access = map_access( ace->mask, mapping );
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

const struct acl *token_get_default_dacl( struct token *token )
{
    return token->default_dacl;
}

const struct sid *token_get_owner( struct token *token )
{
    return token->owner;
}

const struct sid *token_get_primary_group( struct token *token )
{
    return token->primary_group;
}

unsigned int token_get_session_id( struct token *token )
{
    return token->session_id;
}

int check_object_access(struct token *token, struct object *obj, unsigned int *access)
{
    struct generic_map mapping;
    unsigned int status;
    int res;

    if (!token)
        token = current->token ? current->token : current->process->token;

    mapping.all = obj->ops->map_access( obj, GENERIC_ALL );

    if (!obj->sd)
    {
        if (*access & MAXIMUM_ALLOWED) *access = mapping.all;
        return TRUE;
    }

    mapping.read  = obj->ops->map_access( obj, GENERIC_READ );
    mapping.write = obj->ops->map_access( obj, GENERIC_WRITE );
    mapping.exec = obj->ops->map_access( obj, GENERIC_EXECUTE );

    res = token_access_check( token, obj->sd, *access, NULL, NULL,
                              &mapping, access, &status ) == STATUS_SUCCESS &&
          status == STATUS_SUCCESS;

    if (!res) set_error( STATUS_ACCESS_DENIED );
    return res;
}


/* create a security token */
DECL_HANDLER(create_token)
{
    struct token *token;
    struct object_attributes *objattr;
    struct sid *user;
    struct sid_attrs *groups;
    struct luid_attr *privs;
    struct acl *dacl = NULL;
    unsigned int i;
    data_size_t data_size, groups_size;
    struct acl *default_dacl = NULL;
    unsigned int *attrs;
    struct sid *sid;

    objattr = (struct object_attributes *)get_req_data();
    user = (struct sid *)get_req_data_after_objattr( objattr, &data_size );

    if (!user || !sid_valid_size( user, data_size ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    data_size -= sid_len( user );
    groups_size = req->group_count * sizeof( attrs[0] );

    if (data_size < groups_size)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    if (req->primary_group < 0 || req->primary_group >= req->group_count)
    {
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    groups = mem_alloc( req->group_count * sizeof( groups[0] ) );
    if (!groups) return;

    attrs = (unsigned int *)((char *)user + sid_len( user ));
    sid = (struct sid *)&attrs[req->group_count];

    for (i = 0; i < req->group_count; i++)
    {
        groups[i].attrs = attrs[i];
        groups[i].sid = sid;

        if (!sid_valid_size( sid, data_size - groups_size ))
        {
            free( groups );
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }

        groups_size += sid_len( sid );
        sid = (struct sid *)((char *)sid + sid_len( sid ));
    }

    data_size -= groups_size;

    if (data_size < req->priv_count * sizeof( privs[0] ))
    {
        free( groups );
        set_error( STATUS_INVALID_PARAMETER );
        return;
    }

    privs = (struct luid_attr *)((char *)attrs + groups_size);
    data_size -= req->priv_count * sizeof( privs[0] );

    if (data_size)
    {
        dacl = (struct acl *)((char *)privs + req->priv_count * sizeof(privs[0]));
        if (!acl_is_valid( dacl, data_size ))
        {
            free( groups );
            set_error( STATUS_INVALID_PARAMETER );
            return;
        }
    }
    else
        dacl = default_dacl = create_default_dacl( &domain_users_sid );

    token = create_token( req->primary, default_session_id, user, groups, req->group_count,
                          privs, req->priv_count, dacl, NULL, req->primary_group, req->impersonation_level, 0 );
    if (token)
    {
        reply->token = alloc_handle( current->process, token, req->access, objattr->attributes );
        release_object( token );
    }
    free( default_dacl );
    free( groups );
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
        const struct luid_attr *privs = get_req_data();
        struct luid_attr *modified_privs = NULL;
        unsigned int priv_count = get_req_data_size() / sizeof(*privs);
        unsigned int modified_priv_count = 0;

        if (req->get_modified_state && !req->disable_all)
        {
            unsigned int i;
            /* count modified privs */
            for (i = 0; i < priv_count; i++)
            {
                struct privilege *privilege = token_find_privilege( token, privs[i].luid, FALSE );
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
        struct luid_attr *privs;
        struct privilege *privilege;

        LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
            priv_count++;

        reply->len = priv_count * sizeof(*privs);
        if (reply->len <= get_reply_max_size())
        {
            privs = set_reply_data_size( priv_count * sizeof(*privs) );
            if (privs)
                LIST_FOR_EACH_ENTRY( privilege, &token->privileges, struct privilege, entry )
                    *privs++ = luid_and_attr_from_privilege( privilege );
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
            unsigned int access = req->access ? req->access : get_handle_access( current->process, req->handle );
            reply->new_handle = alloc_handle_no_access_check( current->process, token, access, objattr->attributes );
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
        const struct luid_attr *filter_privileges = get_req_data();
        unsigned int priv_count, group_count;
        const struct sid *filter_groups;
        struct token *token;

        priv_count = min( req->privileges_size, get_req_data_size() ) / sizeof(struct luid_attr);
        filter_groups = (const struct sid *)((char *)filter_privileges + priv_count * sizeof(struct luid_attr));
        group_count = get_sid_count( filter_groups, get_req_data_size() - priv_count * sizeof(struct luid_attr) );

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
        unsigned int count = get_req_data_size() / sizeof(struct luid_attr);

        if (!token->primary && token->impersonation_level <= SecurityAnonymous)
            set_error( STATUS_BAD_IMPERSONATION_LEVEL );
        else if (get_reply_max_size() >= count * sizeof(struct luid_attr))
        {
            struct luid_attr *usedprivs = set_reply_data_size( count * sizeof(*usedprivs) );
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
        unsigned int status;
        struct luid_attr priv;
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

        status = token_access_check( token, sd, req->desired_access, &priv, &priv_count, &req->mapping,
                                     &reply->access_granted, &reply->access_status );

        reply->privileges_len = priv_count*sizeof(struct luid_attr);

        if ((priv_count > 0) && (reply->privileges_len <= get_reply_max_size()))
        {
            struct luid_attr *privs = set_reply_data_size( priv_count * sizeof(*privs) );
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
        const struct sid *sid = NULL;

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
        default:
            set_error( STATUS_INVALID_PARAMETER );
            break;
        }

        if (sid)
        {
            reply->sid_len = sid_len( sid );
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

    if ((token = (struct token *)get_handle_obj( current->process, req->handle, TOKEN_QUERY, &token_ops )))
    {
        unsigned int group_count = 0;
        const struct group *group;

        LIST_FOR_EACH_ENTRY( group, &token->groups, const struct group, entry )
        {
            if (req->attr_mask && !(group->attrs & req->attr_mask)) continue;
            group_count++;
            reply->sid_len += sid_len( &group->sid );
        }
        reply->attr_len = sizeof(unsigned int) * group_count;

        if (reply->attr_len + reply->sid_len <= get_reply_max_size())
        {
            unsigned int *attr_ptr = set_reply_data_size( reply->attr_len + reply->sid_len );
            struct sid *sid = (struct sid *)(attr_ptr + group_count);

            if (attr_ptr)
            {
                LIST_FOR_EACH_ENTRY( group, &token->groups, const struct group, entry )
                {
                    if (req->attr_mask && !(group->attrs & req->attr_mask)) continue;
                    sid = copy_sid( sid, &group->sid );
                    *attr_ptr++ = group->attrs;
                }
            }
        }
        else set_error( STATUS_BUFFER_TOO_SMALL );

        release_object( token );
    }
}

DECL_HANDLER(get_token_info)
{
    struct token *token;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle, TOKEN_QUERY, &token_ops )))
    {
        reply->token_id = token->token_id;
        reply->modified_id = token->modified_id;
        reply->session_id = token->session_id;
        reply->primary = token->primary;
        reply->impersonation_level = token->impersonation_level;
        reply->elevation_type = token->elevation;
        /* Tokens with TokenElevationTypeDefault are considered elevated for the
           purposes of GetTokenInformation(TokenElevation) if they belong to the
           admins group. */
        if (token->elevation == TokenElevationTypeDefault)
            reply->is_elevated = token_sid_present( token, &builtin_admins_sid, FALSE );
        else
            reply->is_elevated = token->elevation == TokenElevationTypeFull;
        reply->group_count = list_count( &token->groups );
        reply->privilege_count = list_count( &token->privileges );
        release_object( token );
    }
}

DECL_HANDLER(get_token_default_dacl)
{
    struct token *token;

    if (!(token = (struct token *)get_handle_obj( current->process, req->handle,
                                                  TOKEN_QUERY, &token_ops )))
        return;

    if (token->default_dacl)
    {
        reply->acl_len = token->default_dacl->size;
        if (reply->acl_len <= get_reply_max_size())
        {
            struct acl *acl_reply = set_reply_data_size( reply->acl_len );
            if (acl_reply) memcpy( acl_reply, token->default_dacl, reply->acl_len );
        }
        else set_error( STATUS_BUFFER_TOO_SMALL );
    }
    release_object( token );
}

DECL_HANDLER(set_token_default_dacl)
{
    struct token *token;
    const struct acl *acl = get_req_data();
    unsigned int acl_size = get_req_data_size();

    if (acl_size && !acl_is_valid( acl, acl_size ))
    {
        set_error( STATUS_INVALID_ACL );
        return;
    }

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_ADJUST_DEFAULT,
                                                 &token_ops )))
    {
        free( token->default_dacl );
        token->default_dacl = NULL;

        if (acl_size)
            token->default_dacl = memdup( acl, acl_size );

        release_object( token );
    }
}

DECL_HANDLER(create_linked_token)
{
    struct token *token, *linked;
    int elevation;

    if ((token = (struct token *)get_handle_obj( current->process, req->handle,
                                                 TOKEN_QUERY, &token_ops )))
    {
        switch (token->elevation)
        {
        case TokenElevationTypeFull:
            elevation = TokenElevationTypeLimited;
            break;
        case TokenElevationTypeLimited:
            elevation = TokenElevationTypeFull;
            break;
        default:
            release_object( token );
            return;
        }
        if ((linked = token_create_admin( FALSE, SecurityIdentification, elevation, token->session_id )))
        {
            reply->linked = alloc_handle( current->process, linked, TOKEN_ALL_ACCESS, 0 );
            release_object( linked );
        }
        release_object( token );
    }
}
