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

extern struct token *token_create_admin(void);
extern struct token *token_duplicate( struct token *src_token, unsigned primary,
                                      SECURITY_IMPERSONATION_LEVEL impersonation_level );
extern int token_check_privileges( struct token *token, int all_required,
                                   const LUID_AND_ATTRIBUTES *reqprivs,
                                   unsigned int count, LUID_AND_ATTRIBUTES *usedprivs);
extern const ACL *token_get_default_dacl( struct token *token );
extern void security_set_thread_token( struct thread *thread, obj_handle_t handle );
extern int check_object_access( struct object *obj, unsigned int *access );

static inline int thread_single_check_privilege( struct thread *thread, const LUID *priv)
{
    struct token *token = thread_get_impersonation_token( thread );
    const LUID_AND_ATTRIBUTES privs = { *priv, 0 };

    if (!token) return FALSE;

    return token_check_privileges( token, TRUE, &privs, 1, NULL );
}
