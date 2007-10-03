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

extern const LUID SeIncreaseQuotaPrivilege;
extern const LUID SeSecurityPrivilege;
extern const LUID SeTakeOwnershipPrivilege;
extern const LUID SeLoadDriverPrivilege;
extern const LUID SeSystemProfilePrivilege;
extern const LUID SeSystemtimePrivilege;
extern const LUID SeProfileSingleProcessPrivilege;
extern const LUID SeIncreaseBasePriorityPrivilege;
extern const LUID SeCreatePagefilePrivilege;
extern const LUID SeBackupPrivilege;
extern const LUID SeRestorePrivilege;
extern const LUID SeShutdownPrivilege;
extern const LUID SeDebugPrivilege;
extern const LUID SeSystemEnvironmentPrivilege;
extern const LUID SeChangeNotifyPrivilege;
extern const LUID SeRemoteShutdownPrivilege;
extern const LUID SeUndockPrivilege;
extern const LUID SeManageVolumePrivilege;
extern const LUID SeImpersonatePrivilege;
extern const LUID SeCreateGlobalPrivilege;

extern const PSID security_interactive_sid;


/* token functions */

extern struct token *token_create_admin(void);
extern struct token *token_duplicate( struct token *src_token, unsigned primary,
                                      SECURITY_IMPERSONATION_LEVEL impersonation_level );
extern int token_check_privileges( struct token *token, int all_required,
                                   const LUID_AND_ATTRIBUTES *reqprivs,
                                   unsigned int count, LUID_AND_ATTRIBUTES *usedprivs);
extern const ACL *token_get_default_dacl( struct token *token );
extern const SID *token_get_user( struct token *token );
extern const SID *token_get_primary_group( struct token *token );

extern void security_set_thread_token( struct thread *thread, obj_handle_t handle );
extern const SID *security_unix_uid_to_sid( uid_t uid );
extern int check_object_access( struct object *obj, unsigned int *access );

static inline int thread_single_check_privilege( struct thread *thread, const LUID *priv)
{
    struct token *token = thread_get_impersonation_token( thread );
    const LUID_AND_ATTRIBUTES privs = { *priv, 0 };

    if (!token) return FALSE;

    return token_check_privileges( token, TRUE, &privs, 1, NULL );
}


/* security descriptor helper functions */

extern int sd_is_valid( const struct security_descriptor *sd, data_size_t size );

/* gets the discretionary access control list from a security descriptor */
static inline const ACL *sd_get_dacl( const struct security_descriptor *sd, int *present )
{
    *present = (sd->control & SE_DACL_PRESENT ? TRUE : FALSE);

    if (sd->dacl_len)
        return (const ACL *)((const char *)(sd + 1) +
            sd->owner_len + sd->group_len + sd->sacl_len);
    else
        return NULL;
}

/* gets the system access control list from a security descriptor */
static inline const ACL *sd_get_sacl( const struct security_descriptor *sd, int *present )
{
    *present = (sd->control & SE_SACL_PRESENT ? TRUE : FALSE);

    if (sd->sacl_len)
        return (const ACL *)((const char *)(sd + 1) +
            sd->owner_len + sd->group_len);
    else
        return NULL;
}

/* gets the owner from a security descriptor */
static inline const SID *sd_get_owner( const struct security_descriptor *sd )
{
    if (sd->owner_len)
        return (const SID *)(sd + 1);
    else
        return NULL;
}

/* gets the primary group from a security descriptor */
static inline const SID *sd_get_group( const struct security_descriptor *sd )
{
    if (sd->group_len)
        return (const SID *)((const char *)(sd + 1) + sd->owner_len);
    else
        return NULL;
}
