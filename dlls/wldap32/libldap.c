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

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

C_ASSERT( sizeof(BerValueU) == sizeof(BerValue) );
C_ASSERT( sizeof(LDAPModU) == sizeof(LDAPMod) );
C_ASSERT( sizeof(LDAPControlU) == sizeof(LDAPControl) );
C_ASSERT( sizeof(LDAPSortKeyU) == sizeof(LDAPSortKey) );
C_ASSERT( sizeof(LDAPVLVInfoU) == sizeof(LDAPVLVInfo) );

static LDAPMod *nullattrs[] = { NULL };

void * CDECL wrap_ber_alloc_t( int options )
{
    return ber_alloc_t( options );
}

void CDECL wrap_ber_bvecfree( struct bervalU **berval )
{
    ber_bvecfree( (struct berval **)berval );
}

void CDECL wrap_ber_bvfree( struct bervalU *berval )
{
    ber_bvfree( (struct berval *)berval );
}

unsigned int CDECL wrap_ber_first_element( void *ber, ULONG *ret_len, char **last )
{
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_first_element( ber, &len, last )) == LBER_ERROR) return WLDAP32_LBER_ERROR;
    if (ret > ~0u)
    {
        ERR( "ret too large\n" );
        return WLDAP32_LBER_ERROR;
    }
    if (len > ~0u)
    {
        ERR( "len too large\n" );
        return WLDAP32_LBER_ERROR;
    }

    *ret_len = len;
    return ret;
}

int CDECL wrap_ber_flatten( void *ber, struct bervalU **berval )
{
    return ber_flatten( ber, (struct berval **)berval );
}

void CDECL wrap_ber_free( void *ber, int freebuf )
{
    ber_free( ber, freebuf );
}

void * CDECL wrap_ber_init( struct bervalU *berval )
{
    return ber_init( (struct berval *)berval );
}

unsigned int CDECL wrap_ber_next_element( void *ber, unsigned int *ret_len, char *last )
{
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_next_element( ber, &len, last )) == LBER_ERROR) return WLDAP32_LBER_ERROR;
    if (ret > ~0u)
    {
        ERR( "ret too large\n" );
        return WLDAP32_LBER_ERROR;
    }
    if (len > ~0u)
    {
        ERR( "len too large\n" );
        return WLDAP32_LBER_ERROR;
    }

    *ret_len = len;
    return ret;
}

unsigned int CDECL wrap_ber_peek_tag( void *ber, unsigned int *ret_len )
{
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_peek_tag( ber, &len )) == LBER_ERROR) return WLDAP32_LBER_ERROR;
    if (len > ~0u)
    {
        ERR( "len too large\n" );
        return WLDAP32_LBER_ERROR;
    }

    *ret_len = len;
    return ret;
}

unsigned int CDECL wrap_ber_skip_tag( void *ber, unsigned int *ret_len )
{
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_skip_tag( ber, &len )) == LBER_ERROR) return WLDAP32_LBER_ERROR;
    if (len > ~0u)
    {
        ERR( "len too large\n" );
        return WLDAP32_LBER_ERROR;
    }

    *ret_len = len;
    return ret;
}

int WINAPIV wrap_ber_printf( void *ber, char *fmt, ... )
{
    int ret;
    __ms_va_list args;

    assert( strlen(fmt) == 1 );

    __ms_va_start( args, fmt );
    switch (fmt[0])
    {
    case 'b':
    case 'e':
    case 'i':
    {
        int i = va_arg( args, int );
        ret = ber_printf( ber, fmt, i );
        break;
    }
    case 'o':
    case 's':
    {
        char *str = va_arg( args, char * );
        ret = ber_printf( ber, fmt, str );
        break;
    }
    case 't':
    {
        unsigned int tag = va_arg( args, unsigned int );
        ret = ber_printf( ber, fmt, tag );
        break;
    }
    case 'v':
    {
        char **array = va_arg( args, char ** );
        ret = ber_printf( ber, fmt, array );
        break;
    }
    case 'V':
    {
        struct berval **array = va_arg( args, struct berval ** );
        ret = ber_printf( ber, fmt, array );
        break;
    }
    case 'B':
    {
        char *str = va_arg( args, char * );
        int len = va_arg( args, int );
        ret = ber_printf( ber, fmt, str, len );
        break;
    }
    case 'n':
    case '{':
    case '}':
    case '[':
    case ']':
        ret = ber_printf( ber, fmt );
        break;

    default:
        assert( 0 );
    }
    __ms_va_end( args );
    return ret;
}

int WINAPIV wrap_ber_scanf( void *ber, char *fmt, ... )
{
    int ret;
    __ms_va_list args;

    assert( strlen(fmt) == 1 );

    __ms_va_start( args, fmt );
    switch (fmt[0])
    {
    case 'a':
    {
        char **str = va_arg( args, char ** );
        ret = ber_scanf( ber, fmt, str );
        break;
    }
    case 'b':
    case 'e':
    case 'i':
    {
        int *i = va_arg( args, int * );
        ret = ber_scanf( ber, fmt, i );
        break;
    }
    case 't':
    {
        unsigned int *tag = va_arg( args, unsigned int * );
        ret = ber_scanf( ber, fmt, tag );
        break;
    }
    case 'v':
    {
        char ***array = va_arg( args, char *** );
        ret = ber_scanf( ber, fmt, array );
        break;
    }
    case 'B':
    {
        char **str = va_arg( args, char ** );
        int *len = va_arg( args, int * );
        ret = ber_scanf( ber, fmt, str, len );
        break;
    }
    case 'O':
    {
        struct berval **berval = va_arg( args, struct berval ** );
        ret = ber_scanf( ber, fmt, berval );
        break;
    }
    case 'V':
    {
        struct berval ***array = va_arg( args, struct berval *** );
        ret = ber_scanf( ber, fmt, array );
        break;
    }
    case 'n':
    case 'x':
    case '{':
    case '}':
    case '[':
    case ']':
        ret = ber_scanf( ber, fmt );
        break;

    default:
        assert( 0 );
    }
    __ms_va_end( args );
    return ret;
}

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

void CDECL wrap_ldap_memfree( void *ptr )
{
    return ldap_memfree( ptr );
}

static const struct ldap_funcs funcs =
{
    wrap_ber_alloc_t,
    wrap_ber_bvecfree,
    wrap_ber_bvfree,
    wrap_ber_first_element,
    wrap_ber_flatten,
    wrap_ber_free,
    wrap_ber_init,
    wrap_ber_next_element,
    wrap_ber_peek_tag,
    wrap_ber_skip_tag,
    wrap_ber_printf,
    wrap_ber_scanf,
    wrap_ldap_add_ext,
    wrap_ldap_add_ext_s,
    wrap_ldap_memfree,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    *(const struct ldap_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}

#endif /* HAVE_LDAP */
