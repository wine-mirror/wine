/*
 * Unix interface for libldap
 *
 * Copyright 2021 Hans Leidekker for CodeWeavers
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

#ifdef HAVE_LDAP
#include <stdarg.h>
#ifdef HAVE_LDAP_H
# include <ldap.h>
#endif
#ifdef HAVE_SASL_SASL_H
# include <sasl/sasl.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"

#include "wine/debug.h"
#include "winldap_private.h"

C_ASSERT( sizeof(BerValueU) == sizeof(BerValue) );
C_ASSERT( sizeof(LDAPModU) == sizeof(LDAPMod) );
C_ASSERT( sizeof(LDAPControlU) == sizeof(LDAPControl) );
C_ASSERT( sizeof(LDAPSortKeyU) == sizeof(LDAPSortKey) );
C_ASSERT( sizeof(LDAPVLVInfoU) == sizeof(LDAPVLVInfo) );

static LDAPMod *nullattrs[] = { NULL };

int CDECL wrap_ldap_add_ext( void *ld, char *dn, LDAPModU **attrs, LDAPControlU **serverctrls,
                             LDAPControlU **clientctrls, ULONG *msg )
{
    int dummy;
    return ldap_add_ext( ld, dn ? dn : "", attrs ? (LDAPMod **)attrs : nullattrs, (LDAPControl **)serverctrls,
                         (LDAPControl **)clientctrls, msg ? (int *)msg : &dummy );
}

int CDECL wrap_ldap_add_ext_s( void *ld, char *dn, LDAPModU **attrs, LDAPControlU **serverctrls,
                               LDAPControlU **clientctrls )
{
    return ldap_add_ext_s( ld, dn ? dn : "", attrs ? (LDAPMod **)attrs : nullattrs, (LDAPControl **)serverctrls,
                           (LDAPControl **)clientctrls );
}

static const struct ldap_funcs funcs =
{
    wrap_ldap_add_ext,
    wrap_ldap_add_ext_s,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    *(const struct ldap_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}

#endif /* HAVE_LDAP */
