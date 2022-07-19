/*
 * Security Management
 *
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

#ifndef __WINE_SERVER_SECURITY_H
#define __WINE_SERVER_SECURITY_H

#include <sys/types.h>

extern const struct luid SeIncreaseQuotaPrivilege;
extern const struct luid SeSecurityPrivilege;
extern const struct luid SeTakeOwnershipPrivilege;
extern const struct luid SeLoadDriverPrivilege;
extern const struct luid SeSystemProfilePrivilege;
extern const struct luid SeSystemtimePrivilege;
extern const struct luid SeProfileSingleProcessPrivilege;
extern const struct luid SeIncreaseBasePriorityPrivilege;
extern const struct luid SeCreatePagefilePrivilege;
extern const struct luid SeBackupPrivilege;
extern const struct luid SeRestorePrivilege;
extern const struct luid SeShutdownPrivilege;
extern const struct luid SeDebugPrivilege;
extern const struct luid SeSystemEnvironmentPrivilege;
extern const struct luid SeChangeNotifyPrivilege;
extern const struct luid SeRemoteShutdownPrivilege;
extern const struct luid SeUndockPrivilege;
extern const struct luid SeManageVolumePrivilege;
extern const struct luid SeImpersonatePrivilege;
extern const struct luid SeCreateGlobalPrivilege;

extern const struct sid world_sid;
extern const struct sid local_user_sid;
extern const struct sid local_system_sid;
extern const struct sid builtin_users_sid;
extern const struct sid builtin_admins_sid;
extern const struct sid domain_users_sid;
extern const struct sid high_label_sid;

struct ace
{
    unsigned char  type;
    unsigned char  flags;
    unsigned short size;
    unsigned int   mask;
};

/* token functions */

extern struct token *get_token_obj( struct process *process, obj_handle_t handle, unsigned int access );
extern struct token *token_create_admin( unsigned primary, int impersonation_level, int elevation, unsigned int session_id );
extern int token_assign_label( struct token *token, const struct sid *label );
extern struct token *token_duplicate( struct token *src_token, unsigned primary,
                                      int impersonation_level, const struct security_descriptor *sd,
                                      const struct luid_attr *remove_privs, unsigned int remove_priv_count,
                                      const struct sid *remove_groups, unsigned int remove_group_count );
extern int token_check_privileges( struct token *token, int all_required,
                                   const struct luid_attr *reqprivs,
                                   unsigned int count, struct luid_attr *usedprivs );
extern const struct acl *token_get_default_dacl( struct token *token );
extern const struct sid *token_get_owner( struct token *token );
extern const struct sid *token_get_primary_group( struct token *token );
extern unsigned int token_get_session_id( struct token *token );
extern int token_sid_present( struct token *token, const struct sid *sid, int deny );

static inline struct ace *ace_first( const struct acl *acl )
{
    return (struct ace *)(acl + 1);
}

static inline struct ace *ace_next( const struct ace *ace )
{
    return (struct ace *)((char *)ace + ace->size);
}

static inline size_t sid_len( const struct sid *sid )
{
    return offsetof( struct sid, sub_auth[sid->sub_count] );
}

static inline int equal_sid( const struct sid *sid1, const struct sid *sid2 )
{
    return ((sid1->sub_count == sid2->sub_count) && !memcmp( sid1, sid2, sid_len( sid1 )));
}

static inline void *copy_sid( struct sid *dst, const struct sid *src )
{
    memcpy( dst, src, sid_len( src ));
    return (char *)dst + sid_len( src );
}

static inline int sid_valid_size( const struct sid *sid, data_size_t size )
{
    return (size >= offsetof( struct sid, sub_auth[0] ) && size >= sid_len( sid ));
}

static inline struct ace *set_ace( struct ace *ace, const struct sid *sid, unsigned char type,
                                   unsigned char flags, unsigned int mask )
{
    ace->type  = type;
    ace->flags = flags;
    ace->size  = sizeof(*ace) + sid_len( sid );
    ace->mask  = mask;
    memcpy( ace + 1, sid, sid_len( sid ));
    return ace;
}

extern void security_set_thread_token( struct thread *thread, obj_handle_t handle );
extern const struct sid *security_unix_uid_to_sid( uid_t uid );
extern int check_object_access( struct token *token, struct object *obj, unsigned int *access );

static inline int thread_single_check_privilege( struct thread *thread, struct luid priv )
{
    struct token *token = thread_get_impersonation_token( thread );
    const struct luid_attr privs = { priv, 0 };

    if (!token) return FALSE;

    return token_check_privileges( token, TRUE, &privs, 1, NULL );
}


/* security descriptor helper functions */

extern int sd_is_valid( const struct security_descriptor *sd, data_size_t size );
extern struct acl *extract_security_labels( const struct acl *sacl );
extern struct acl *replace_security_labels( const struct acl *old_sacl, const struct acl *new_sacl );

/* gets the discretionary access control list from a security descriptor */
static inline const struct acl *sd_get_dacl( const struct security_descriptor *sd, int *present )
{
    *present = (sd->control & SE_DACL_PRESENT) != 0;

    if (sd->dacl_len)
        return (const struct acl *)((const char *)(sd + 1) +
            sd->owner_len + sd->group_len + sd->sacl_len);
    else
        return NULL;
}

/* gets the system access control list from a security descriptor */
static inline const struct acl *sd_get_sacl( const struct security_descriptor *sd, int *present )
{
    *present = (sd->control & SE_SACL_PRESENT) != 0;

    if (sd->sacl_len)
        return (const struct acl *)((const char *)(sd + 1) +
            sd->owner_len + sd->group_len);
    else
        return NULL;
}

/* gets the owner from a security descriptor */
static inline const struct sid *sd_get_owner( const struct security_descriptor *sd )
{
    if (sd->owner_len)
        return (const struct sid *)(sd + 1);
    else
        return NULL;
}

/* gets the primary group from a security descriptor */
static inline const struct sid *sd_get_group( const struct security_descriptor *sd )
{
    if (sd->group_len)
        return (const struct sid *)((const char *)(sd + 1) + sd->owner_len);
    else
        return NULL;
}

#endif  /* __WINE_SERVER_SECURITY_H */
