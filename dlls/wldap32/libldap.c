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
#include <assert.h>
#include <stdarg.h>
#include <sys/time.h>
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
#include "libldap.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

C_ASSERT( sizeof(BerValueU) == sizeof(BerValue) );
C_ASSERT( sizeof(LDAPModU) == sizeof(LDAPMod) );
C_ASSERT( sizeof(LDAPControlU) == sizeof(LDAPControl) );
C_ASSERT( sizeof(LDAPSortKeyU) == sizeof(LDAPSortKey) );
C_ASSERT( sizeof(LDAPVLVInfoU) == sizeof(LDAPVLVInfo) );
C_ASSERT( sizeof(LDAPAPIInfoU) == sizeof(LDAPAPIInfo) );
C_ASSERT( sizeof(LDAPAPIFeatureInfoU) == sizeof(LDAPAPIFeatureInfo) );
C_ASSERT( sizeof(struct timevalU) == sizeof(struct timeval) );

#define WLDAP32_LBER_ERROR  (~0l)

static LDAPMod *nullmods[] = { NULL };

static const struct ldap_callbacks *callbacks;

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

int CDECL wrap_ldap_abandon_ext( void *ld, int msgid, LDAPControlU **serverctrls, LDAPControlU **clientctrls )
{
    return ldap_abandon_ext( ld, msgid, (LDAPControl **)serverctrls, (LDAPControl **)clientctrls );
}

int CDECL wrap_ldap_add_ext( void *ld, const char *dn, LDAPModU **attrs, LDAPControlU **serverctrls,
                             LDAPControlU **clientctrls, ULONG *msg )
{
    int dummy;
    return ldap_add_ext( ld, dn ? dn : "", attrs ? (LDAPMod **)attrs : nullmods, (LDAPControl **)serverctrls,
                         (LDAPControl **)clientctrls, msg ? (int *)msg : &dummy );
}

int CDECL wrap_ldap_add_ext_s( void *ld, const char *dn, LDAPModU **attrs, LDAPControlU **serverctrls,
                               LDAPControlU **clientctrls )
{
    return ldap_add_ext_s( ld, dn ? dn : "", attrs ? (LDAPMod **)attrs : nullmods, (LDAPControl **)serverctrls,
                           (LDAPControl **)clientctrls );
}

int CDECL wrap_ldap_compare_ext( void *ld, const char *dn, const char *attrs, struct bervalU *value,
                                 LDAPControlU **serverctrls, LDAPControlU **clientctrls, ULONG *msg )
{
    int dummy;
    return ldap_compare_ext( ld, dn ? dn : "", attrs ? attrs : "", (struct berval *)value,
                             (LDAPControl **)serverctrls, (LDAPControl **)clientctrls, msg ? (int *)msg : &dummy );
}

int CDECL wrap_ldap_compare_ext_s( void *ld, const char *dn, const char *attrs, struct bervalU *value,
                                   LDAPControlU **serverctrls, LDAPControlU **clientctrls )
{
    return ldap_compare_ext_s( ld, dn ? dn : "", attrs ? attrs : "", (struct berval *)value,
                               (LDAPControl **)serverctrls, (LDAPControl **)clientctrls );
}

void CDECL wrap_ldap_control_free( LDAPControlU *control )
{
    ldap_control_free( (LDAPControl *)control );
}

void CDECL wrap_ldap_controls_free( LDAPControlU **control )
{
    ldap_controls_free( (LDAPControl **)control );
}

int CDECL wrap_ldap_count_entries( void *ld, void *chain )
{
    return ldap_count_entries( ld, chain );
}

int CDECL wrap_ldap_count_references( void *ld, void *chain )
{
    return ldap_count_references( ld, chain );
}

int CDECL wrap_ldap_count_values_len( struct bervalU **values )
{
    return ldap_count_values_len( (struct berval **)values );
}

int CDECL wrap_ldap_create_sort_control( void *ld, LDAPSortKeyU **keylist, int critical, LDAPControlU **control )
{
    return ldap_create_sort_control( ld, (LDAPSortKey **)keylist, critical, (LDAPControl **)control );
}

int CDECL wrap_ldap_create_vlv_control( void *ld, LDAPVLVInfoU *info, LDAPControlU **control )
{
    return ldap_create_vlv_control( ld, (LDAPVLVInfo *)info, (LDAPControl **)control );
}

int CDECL wrap_ldap_delete_ext( void *ld, const char *dn, LDAPControlU **serverctrls, LDAPControlU **clientctrls,
                                ULONG *msg )
{
    int dummy;
    return ldap_delete_ext( ld, dn ? dn : "", (LDAPControl **)serverctrls, (LDAPControl **)clientctrls,
                            msg ? (int *)msg : &dummy );
}

int CDECL wrap_ldap_delete_ext_s( void *ld, const char *dn, LDAPControlU **serverctrls, LDAPControlU **clientctrls )
{
    return ldap_delete_ext_s( ld, dn ? dn : "", (LDAPControl **)serverctrls, (LDAPControl **)clientctrls );
}

char * CDECL wrap_ldap_dn2ufn( const char *dn )
{
    return ldap_dn2ufn( dn );
}

char ** CDECL wrap_ldap_explode_dn( const char *dn, int notypes )
{
    return ldap_explode_dn( dn, notypes );
}

int CDECL wrap_ldap_extended_operation( void *ld, const char *oid, struct bervalU *data, LDAPControlU **serverctrls,
                                        LDAPControlU **clientctrls, ULONG *msg )
{
    int dummy;
    return ldap_extended_operation( ld, oid ? oid : "", (struct berval *)data, (LDAPControl **)serverctrls,
                                    (LDAPControl **)clientctrls, msg ? (int *)msg : &dummy );
}

int CDECL wrap_ldap_extended_operation_s( void *ld, const char *oid, struct bervalU *data, LDAPControlU **serverctrls,
                                          LDAPControlU **clientctrls, char **retoid, struct bervalU **retdata )
{
    return ldap_extended_operation_s( ld, oid ? oid : "", (struct berval *)data, (LDAPControl **)serverctrls,
                                      (LDAPControl **)clientctrls, retoid, (struct berval **)retdata );
}

char * CDECL wrap_ldap_get_dn( void *ld, void *entry )
{
    return ldap_get_dn( ld, entry );
}

char * CDECL wrap_ldap_first_attribute( void *ld, void *entry, void **ber )
{
    return ldap_first_attribute( ld, entry, (BerElement **)ber );
}

void * CDECL wrap_ldap_first_entry( void *ld, void *chain )
{
    return ldap_first_entry( ld, chain );
}

void * CDECL wrap_ldap_first_reference( void *ld, void *chain )
{
    return ldap_first_reference( ld, chain );
}

int CDECL wrap_ldap_get_option( void *ld, int option, void *value )
{
    return ldap_get_option( ld, option, value );
}

struct bervalU ** CDECL wrap_ldap_get_values_len( void *ld, void *entry, const char *attr )
{
    return (struct bervalU **)ldap_get_values_len( ld, entry, attr );
}

int CDECL wrap_ldap_initialize( void **ld, const char *url )
{
    return ldap_initialize( (LDAP **)ld, url );
}

void CDECL wrap_ldap_memfree( void *ptr )
{
    return ldap_memfree( ptr );
}

void CDECL wrap_ldap_memvfree( void **ptr )
{
    ldap_memvfree( ptr );
}

int CDECL wrap_ldap_modify_ext( void *ld, const char *dn, LDAPModU **mods, LDAPControlU **serverctrls,
                                LDAPControlU **clientctrls, ULONG *msg )
{
    int dummy;
    return ldap_modify_ext( ld, dn ? dn : "", mods ? (LDAPMod **)mods : nullmods, (LDAPControl **)serverctrls,
                            (LDAPControl **)clientctrls, msg ? (int *)msg : &dummy );
}

int CDECL wrap_ldap_modify_ext_s( void *ld, const char *dn, LDAPModU **mods, LDAPControlU **serverctrls,
                                  LDAPControlU **clientctrls )
{
    return ldap_modify_ext_s( ld, dn ? dn : "", mods ? (LDAPMod **)mods : nullmods, (LDAPControl **)serverctrls,
                              (LDAPControl **)clientctrls );
}

int CDECL wrap_ldap_msgfree( void *msg )
{
    return ldap_msgfree( msg );
}

char * CDECL wrap_ldap_next_attribute( void *ld, void *entry, void *ber )
{
    return ldap_next_attribute( ld, entry, ber );
}

void * CDECL wrap_ldap_next_entry( void *ld, void *entry )
{
    return ldap_next_entry( ld, entry );
}

void * CDECL wrap_ldap_next_reference( void *ld, void *entry )
{
    return ldap_next_reference( ld, entry );
}

int CDECL wrap_ldap_parse_extended_result( void *ld, void *result, char **retoid, struct bervalU **retdata, int free )
{
    return ldap_parse_extended_result( ld, result, retoid, (struct berval **)retdata, free );
}

int CDECL wrap_ldap_parse_reference( void *ld, void *ref, char ***referrals, LDAPControlU ***serverctrls, int free )
{
    return ldap_parse_reference( ld, ref, referrals, (LDAPControl ***)serverctrls, free );
}

int CDECL wrap_ldap_parse_result( void *ld, void *res, int *errcode, char **matcheddn, char **errmsg,
                                  char ***referrals, LDAPControlU ***serverctrls, int free )
{
    return ldap_parse_result( ld, res, errcode, matcheddn, errmsg, referrals, (LDAPControl ***)serverctrls, free );
}

int CDECL wrap_ldap_parse_sortresponse_control( void *ld, LDAPControlU *ctrl, int *result, char **attr )
{
    return ldap_parse_sortresponse_control( ld, (LDAPControl *)ctrl, result, attr );
}

int CDECL wrap_ldap_parse_vlvresponse_control( void *ld, LDAPControlU *ctrls, int *target_pos, int *list_count,
                                               struct bervalU **ctx, int *errcode )
{
    return ldap_parse_vlvresponse_control( ld, (LDAPControl *)ctrls, target_pos, list_count, (struct berval **)ctx,
                                           errcode );
}

int CDECL wrap_ldap_rename( void *ld, const char *dn, const char *newrdn, const char *newparent, int delete,
                            LDAPControlU **clientctrls, LDAPControlU **serverctrls, ULONG *msg )
{
    return ldap_rename( ld, dn ? dn : "", newrdn, newparent, delete, (LDAPControl **)clientctrls,
                        (LDAPControl **)serverctrls, (int *)msg );
}

int CDECL wrap_ldap_rename_s( void *ld, const char *dn, const char *newrdn, const char *newparent, int delete,
                              LDAPControlU **clientctrls, LDAPControlU **serverctrls )
{
    return ldap_rename_s( ld, dn ? dn : "", newrdn, newparent, delete, (LDAPControl **)clientctrls,
                          (LDAPControl **)serverctrls );
}

int CDECL wrap_ldap_result( void *ld, int msgid, int all, struct timevalU *timeout, void **result )
{
    return ldap_result( ld, msgid, all, (struct timeval *)timeout, (LDAPMessage **)result );
}

int CDECL wrap_ldap_sasl_bind( void *ld, const char *dn, const char *mech, struct bervalU *cred,
                               LDAPControlU **serverctrls, LDAPControlU **clientctrls, int *msgid )
{
    return ldap_sasl_bind( ld, dn, mech, (struct berval *)cred, (LDAPControl **)serverctrls,
                           (LDAPControl **)clientctrls, msgid );
}

int CDECL wrap_ldap_sasl_bind_s( void *ld, const char *dn, const char *mech, struct bervalU *cred,
                                 LDAPControlU **serverctrls, LDAPControlU **clientctrls, struct bervalU **servercred )
{
    return ldap_sasl_bind_s( ld, dn, mech, (struct berval *)cred, (LDAPControl **)serverctrls,
                             (LDAPControl **)clientctrls, (struct berval **)servercred );
}

static int wrap_sasl_interact( LDAP *ld, unsigned int flags, void *defaults, void *interact )
{
#ifdef HAVE_SASL_SASL_H
    C_ASSERT( sizeof(struct sasl_interactU) == sizeof(struct sasl_interact) );
    return callbacks->sasl_interact( ld, flags, defaults, interact );
#endif
    return -1;
}

int CDECL wrap_ldap_sasl_interactive_bind_s( void *ld, const char *dn, const char *mech, LDAPControlU **serverctrls,
                                             LDAPControlU **clientctrls, unsigned int flags, void *defaults )
{
    return ldap_sasl_interactive_bind_s( ld, dn, mech, (LDAPControl **)serverctrls, (LDAPControl **)clientctrls, flags,
                                         wrap_sasl_interact, defaults );
}

int CDECL wrap_ldap_search_ext( void *ld, const char *base, int scope, const char *filter, char **attrs, int attrsonly,
                                LDAPControlU **serverctrls, LDAPControlU **clientctrls, struct timevalU *timeout,
                                int sizelimit, ULONG *msg )
{
    return ldap_search_ext( ld, base, scope, filter, attrs, attrsonly, (LDAPControl **)serverctrls,
                            (LDAPControl **)clientctrls, (struct timeval *)timeout, sizelimit, (int *)msg );
}

int CDECL wrap_ldap_search_ext_s( void *ld, const char *base, int scope, const char *filter, char **attrs,
                                  int attrsonly, LDAPControlU **serverctrls, LDAPControlU **clientctrls,
                                  struct timevalU *timeout, int sizelimit, void **result )
{
    return ldap_search_ext_s( ld, base, scope, filter, attrs, attrsonly, (LDAPControl **)serverctrls,
                              (LDAPControl **)clientctrls, (struct timeval *)timeout, sizelimit,
                              (LDAPMessage **)result );
}

int CDECL wrap_ldap_set_option( void *ld, int option, const void *value )
{
    return ldap_set_option( ld, option, value );
}

int CDECL wrap_ldap_start_tls_s( void *ld, LDAPControlU **serverctrls, LDAPControlU **clientctrls )
{
    return ldap_start_tls_s( ld, (LDAPControl **)serverctrls, (LDAPControl **)clientctrls );
}

int CDECL wrap_ldap_unbind_ext( void *ld, LDAPControlU **serverctrls, LDAPControlU **clientctrls )
{
    return ldap_unbind_ext( ld, (LDAPControl **)serverctrls, (LDAPControl **)clientctrls );
}

int CDECL wrap_ldap_unbind_ext_s( void *ld, LDAPControlU **serverctrls, LDAPControlU **clientctrls )
{
    return ldap_unbind_ext_s( ld, (LDAPControl **)serverctrls, (LDAPControl **)clientctrls );
}

void CDECL wrap_ldap_value_free_len( struct bervalU **values )
{
    ldap_value_free_len( (struct berval **)values );
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
    wrap_ldap_abandon_ext,
    wrap_ldap_add_ext,
    wrap_ldap_add_ext_s,
    wrap_ldap_compare_ext,
    wrap_ldap_compare_ext_s,
    wrap_ldap_control_free,
    wrap_ldap_controls_free,
    wrap_ldap_count_entries,
    wrap_ldap_count_references,
    wrap_ldap_count_values_len,
    wrap_ldap_create_sort_control,
    wrap_ldap_create_vlv_control,
    wrap_ldap_delete_ext,
    wrap_ldap_delete_ext_s,
    wrap_ldap_dn2ufn,
    wrap_ldap_explode_dn,
    wrap_ldap_extended_operation,
    wrap_ldap_extended_operation_s,
    wrap_ldap_get_dn,
    wrap_ldap_get_option,
    wrap_ldap_get_values_len,
    wrap_ldap_initialize,
    wrap_ldap_first_attribute,
    wrap_ldap_first_entry,
    wrap_ldap_first_reference,
    wrap_ldap_memfree,
    wrap_ldap_memvfree,
    wrap_ldap_modify_ext,
    wrap_ldap_modify_ext_s,
    wrap_ldap_msgfree,
    wrap_ldap_next_attribute,
    wrap_ldap_next_entry,
    wrap_ldap_next_reference,
    wrap_ldap_parse_extended_result,
    wrap_ldap_parse_reference,
    wrap_ldap_parse_result,
    wrap_ldap_parse_sortresponse_control,
    wrap_ldap_parse_vlvresponse_control,
    wrap_ldap_rename,
    wrap_ldap_rename_s,
    wrap_ldap_result,
    wrap_ldap_sasl_bind,
    wrap_ldap_sasl_bind_s,
    wrap_ldap_sasl_interactive_bind_s,
    wrap_ldap_search_ext,
    wrap_ldap_search_ext_s,
    wrap_ldap_set_option,
    wrap_ldap_start_tls_s,
    wrap_ldap_unbind_ext,
    wrap_ldap_unbind_ext_s,
    wrap_ldap_value_free_len,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    callbacks = ptr_in;
    *(const struct ldap_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
#endif /* HAVE_LDAP */
