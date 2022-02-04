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

static struct timeval *convert_timeval(const struct timevalU *tvu, struct timeval *tv)
{
    if (!tvu) return NULL;
    tv->tv_sec = tvu->tv_sec;
    tv->tv_usec = tvu->tv_usec;
    return tv;
}

#define WLDAP32_LBER_ERROR  (~0l)

static LDAPMod *nullmods[] = { NULL };

static NTSTATUS wrap_ber_alloc_t( void *args )
{
    struct ber_alloc_t_params *params = args;
    *params->ret = ber_alloc_t( params->options );
    return *params->ret ? LDAP_SUCCESS : WLDAP32_LBER_ERROR;
}

static NTSTATUS wrap_ber_bvecfree( void *args )
{
    ber_bvecfree( args );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ber_bvfree( void *args )
{
    ber_bvfree( args );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ber_first_element( void *args )
{
    struct ber_first_element_params *params = args;
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_first_element( params->ber, &len, params->last )) == LBER_ERROR)
        return WLDAP32_LBER_ERROR;
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

    *params->ret_len = len;
    return ret;
}

static NTSTATUS wrap_ber_flatten( void *args )
{
    struct ber_flatten_params *params = args;
    return ber_flatten( params->ber, (struct berval **)params->berval );
}

static NTSTATUS wrap_ber_free( void *args )
{
    struct ber_free_params *params = args;

    ber_free( params->ber, params->freebuf );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ber_init( void *args )
{
    struct ber_init_params *params = args;
    *params->ret = ber_init( (struct berval *)params->berval );
    return *params->ret ? LDAP_SUCCESS : WLDAP32_LBER_ERROR;
}

static NTSTATUS wrap_ber_next_element( void *args )
{
    struct ber_next_element_params *params = args;
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_next_element( params->ber, &len, params->last )) == LBER_ERROR)
        return WLDAP32_LBER_ERROR;
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

    *params->ret_len = len;
    return ret;
}

static NTSTATUS wrap_ber_peek_tag( void *args )
{
    struct ber_peek_tag_params *params = args;
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_peek_tag( params->ber, &len )) == LBER_ERROR) return WLDAP32_LBER_ERROR;
    if (len > ~0u)
    {
        ERR( "len too large\n" );
        return WLDAP32_LBER_ERROR;
    }

    *params->ret_len = len;
    return ret;
}

static NTSTATUS wrap_ber_skip_tag( void *args )
{
    struct ber_skip_tag_params *params = args;
    ber_len_t len;
    ber_tag_t ret;

    if ((ret = ber_skip_tag( params->ber, &len )) == LBER_ERROR) return WLDAP32_LBER_ERROR;
    if (len > ~0u)
    {
        ERR( "len too large\n" );
        return WLDAP32_LBER_ERROR;
    }

    *params->ret_len = len;
    return ret;
}

static NTSTATUS wrap_ber_printf( void *args )
{
    struct ber_printf_params *params = args;

    assert( strlen(params->fmt) == 1 );

    return ber_printf( params->ber, params->fmt, params->arg1, params->arg2 );
}

static NTSTATUS wrap_ber_scanf( void *args )
{
    struct ber_scanf_params *params = args;

    assert( strlen(params->fmt) == 1 );

    return ber_scanf( params->ber, params->fmt, params->arg1, params->arg2 );
}

static NTSTATUS wrap_ldap_abandon_ext( void *args )
{
    struct ldap_abandon_ext_params *params = args;
    return ldap_abandon_ext( params->ld, params->msgid, (LDAPControl **)params->serverctrls,
                             (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_add_ext( void *args )
{
    struct ldap_add_ext_params *params = args;
    int dummy;
    return ldap_add_ext( params->ld, params->dn ? params->dn : "",
                         params->attrs ? (LDAPMod **)params->attrs : nullmods,
                         (LDAPControl **)params->serverctrls, (LDAPControl **)params->clientctrls,
                         params->msg ? (int *)params->msg : &dummy );
}

static NTSTATUS wrap_ldap_add_ext_s( void *args )
{
    struct ldap_add_ext_s_params *params = args;
    return ldap_add_ext_s( params->ld, params->dn ? params->dn : "",
                           params->attrs ? (LDAPMod **)params->attrs : nullmods,
                           (LDAPControl **)params->serverctrls, (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_compare_ext( void *args )
{
    struct ldap_compare_ext_params *params = args;
    int dummy;
    return ldap_compare_ext( params->ld, params->dn ? params->dn : "", params->attrs ? params->attrs : "",
                             (struct berval *)params->value, (LDAPControl **)params->serverctrls,
                             (LDAPControl **)params->clientctrls, params->msg ? (int *)params->msg : &dummy );
}

static NTSTATUS wrap_ldap_compare_ext_s( void *args )
{
    struct ldap_compare_ext_s_params *params = args;
    return ldap_compare_ext_s( params->ld, params->dn ? params->dn : "", params->attrs ? params->attrs : "",
                               (struct berval *)params->value, (LDAPControl **)params->serverctrls,
                               (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_control_free( void *args )
{
    ldap_control_free( args );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ldap_controls_free( void *args )
{
    ldap_controls_free( args );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ldap_count_entries( void *args )
{
    struct ldap_count_entries_params *params = args;
    return ldap_count_entries( params->ld, params->chain );
}

static NTSTATUS wrap_ldap_count_references( void *args )
{
    struct ldap_count_references_params *params = args;
    return ldap_count_references( params->ld, params->chain );
}

static NTSTATUS wrap_ldap_count_values_len( void *args )
{
    return ldap_count_values_len( args );
}

static NTSTATUS wrap_ldap_create_sort_control( void *args )
{
    struct ldap_create_sort_control_params *params = args;
    return ldap_create_sort_control( params->ld, (LDAPSortKey **)params->keylist, params->critical,
                                     (LDAPControl **)params->control );
}

static NTSTATUS wrap_ldap_create_vlv_control( void *args )
{
    struct ldap_create_vlv_control_params *params = args;
    return ldap_create_vlv_control( params->ld, (LDAPVLVInfo *)params->info,
                                    (LDAPControl **)params->control );
}

static NTSTATUS wrap_ldap_delete_ext( void *args )
{
    struct ldap_delete_ext_params *params = args;
    int dummy;
    return ldap_delete_ext( params->ld, params->dn ? params->dn : "", (LDAPControl **)params->serverctrls,
                            (LDAPControl **)params->clientctrls, params->msg ? (int *)params->msg : &dummy );
}

static NTSTATUS wrap_ldap_delete_ext_s( void *args )
{
    struct ldap_delete_ext_s_params *params = args;
    return ldap_delete_ext_s( params->ld, params->dn ? params->dn : "", (LDAPControl **)params->serverctrls,
                              (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_dn2ufn( void *args )
{
    struct ldap_dn2ufn_params *params = args;
    *params->ret = ldap_dn2ufn( params->dn );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_explode_dn( void *args )
{
    struct ldap_explode_dn_params *params = args;
    *params->ret = ldap_explode_dn( params->dn, params->notypes );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_extended_operation( void *args )
{
    struct ldap_extended_operation_params *params = args;
    int dummy;
    return ldap_extended_operation( params->ld, params->oid ? params->oid : "",
                                    (struct berval *)params->data, (LDAPControl **)params->serverctrls,
                                    (LDAPControl **)params->clientctrls,
                                    params->msg ? (int *)params->msg : &dummy );
}

static NTSTATUS wrap_ldap_extended_operation_s( void *args )
{
    struct ldap_extended_operation_s_params *params = args;
    return ldap_extended_operation_s( params->ld, params->oid ? params->oid : "",
                                      (struct berval *)params->data, (LDAPControl **)params->serverctrls,
                                      (LDAPControl **)params->clientctrls, params->retoid,
                                      (struct berval **)params->retdata );
}

static NTSTATUS wrap_ldap_get_dn( void *args )
{
    struct ldap_get_dn_params *params = args;
    *params->ret = ldap_get_dn( params->ld, params->entry );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_first_attribute( void *args )
{
    struct ldap_first_attribute_params *params = args;
    *params->ret = ldap_first_attribute( params->ld, params->entry, (BerElement **)params->ber );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_first_entry( void *args )
{
    struct ldap_first_entry_params *params = args;
    *params->ret = ldap_first_entry( params->ld, params->chain );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_first_reference( void *args )
{
    struct ldap_first_reference_params *params = args;
    *params->ret = ldap_first_reference( params->ld, params->chain );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_get_option( void *args )
{
    struct ldap_get_option_params *params = args;
    return ldap_get_option( params->ld, params->option, params->value );
}

static NTSTATUS wrap_ldap_get_values_len( void *args )
{
    struct ldap_get_values_len_params *params = args;
    *params->ret = (struct bervalU **)ldap_get_values_len( params->ld, params->entry, params->attr );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_initialize( void *args )
{
    struct ldap_initialize_params *params = args;
    return ldap_initialize( (LDAP **)params->ld, params->url );
}

static NTSTATUS wrap_ldap_memfree( void *args )
{
    ldap_memfree( args );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ldap_memvfree( void *args )
{
    ldap_memvfree( args );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_ldap_modify_ext( void *args )
{
    struct ldap_modify_ext_params *params = args;
    int dummy;
    return ldap_modify_ext( params->ld, params->dn ? params->dn : "",
                            params->mods ? (LDAPMod **)params->mods : nullmods,
                            (LDAPControl **)params->serverctrls, (LDAPControl **)params->clientctrls,
                            params->msg ? (int *)params->msg : &dummy );
}

static NTSTATUS wrap_ldap_modify_ext_s( void *args )
{
    struct ldap_modify_ext_s_params *params = args;
    return ldap_modify_ext_s( params->ld, params->dn ? params->dn : "",
                              params->mods ? (LDAPMod **)params->mods : nullmods,
                              (LDAPControl **)params->serverctrls, (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_msgfree( void *args )
{
    return ldap_msgfree( args );
}

static NTSTATUS wrap_ldap_next_attribute( void *args )
{
    struct ldap_next_attribute_params *params = args;
    *params->ret = ldap_next_attribute( params->ld, params->entry, params->ber );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_next_entry( void *args )
{
    struct ldap_next_entry_params *params = args;
    *params->ret = ldap_next_entry( params->ld, params->entry );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_next_reference( void *args )
{
    struct ldap_next_reference_params *params = args;
    *params->ret = ldap_next_reference( params->ld, params->entry );
    return *params->ret ? 0 : -1;
}

static NTSTATUS wrap_ldap_parse_extended_result( void *args )
{
    struct ldap_parse_extended_result_params *params = args;
    return ldap_parse_extended_result( params->ld, params->result, params->retoid,
                                       (struct berval **)params->retdata, params->free );
}

static NTSTATUS wrap_ldap_parse_reference( void *args )
{
    struct ldap_parse_reference_params *params = args;
    return ldap_parse_reference( params->ld, params->ref, params->referrals,
                                 (LDAPControl ***)params->serverctrls, params->free );
}

static NTSTATUS wrap_ldap_parse_result( void *args )
{
    struct ldap_parse_result_params *params = args;
    return ldap_parse_result( params->ld, params->res, params->errcode, params->matcheddn, params->errmsg,
                              params->referrals, (LDAPControl ***)params->serverctrls, params->free );
}

static NTSTATUS wrap_ldap_parse_sortresponse_control( void *args )
{
    struct ldap_parse_sortresponse_control_params *params = args;
    return ldap_parse_sortresponse_control( params->ld, (LDAPControl *)params->ctrl,
                                            params->result, params->attr );
}

static NTSTATUS wrap_ldap_parse_vlvresponse_control( void *args )
{
    struct ldap_parse_vlvresponse_control_params *params = args;
    return ldap_parse_vlvresponse_control( params->ld, (LDAPControl *)params->ctrls, params->target_pos,
                                           params->list_count, (struct berval **)params->ctx,
                                           params->errcode );
}

static NTSTATUS wrap_ldap_rename( void *args )
{
    struct ldap_rename_params *params = args;
    return ldap_rename( params->ld, params->dn ? params->dn : "", params->newrdn, params->newparent,
                        params->delete, (LDAPControl **)params->clientctrls,
                        (LDAPControl **)params->serverctrls, (int *)params->msg );
}

static NTSTATUS wrap_ldap_rename_s( void *args )
{
    struct ldap_rename_s_params *params = args;
    return ldap_rename_s( params->ld, params->dn ? params->dn : "", params->newrdn, params->newparent,
                          params->delete, (LDAPControl **)params->clientctrls,
                          (LDAPControl **)params->serverctrls );
}

static NTSTATUS wrap_ldap_result( void *args )
{
    struct ldap_result_params *params = args;
    struct timeval tv;
    return ldap_result( params->ld, params->msgid, params->all,
                        convert_timeval(params->timeout, &tv), (LDAPMessage **)params->result );
}

static NTSTATUS wrap_ldap_sasl_bind( void *args )
{
    struct ldap_sasl_bind_params *params = args;
    return ldap_sasl_bind( params->ld, params->dn, params->mech, (struct berval *)params->cred,
                           (LDAPControl **)params->serverctrls, (LDAPControl **)params->clientctrls,
                           params->msgid );
}

static NTSTATUS wrap_ldap_sasl_bind_s( void *args )
{
    struct ldap_sasl_bind_s_params *params = args;
    return ldap_sasl_bind_s( params->ld, params->dn, params->mech, (struct berval *)params->cred,
                             (LDAPControl **)params->serverctrls, (LDAPControl **)params->clientctrls,
                             (struct berval **)params->servercred );
}

static int wrap_sasl_interact( LDAP *ld, unsigned int flags, void *defaults, void *interact )
{
#ifdef HAVE_SASL_SASL_H
    struct sasl_interactive_bind_id *id = defaults;
    struct sasl_interact *sasl = interact;

    TRACE( "(%p, 0x%08x, %p, %p)\n", ld, flags, defaults, interact );

    while (sasl->id != SASL_CB_LIST_END)
    {
        TRACE( "sasl->id = %04lx\n", sasl->id );

        if (sasl->id == SASL_CB_GETREALM)
        {
            sasl->result = id->domain;
            sasl->len = id->domain_len;
        }
        else if (sasl->id == SASL_CB_USER)
        {
            sasl->result = id->user;
            sasl->len = id->user_len;
        }
        else if (sasl->id == SASL_CB_PASS)
        {
            sasl->result = id->password;
            sasl->len = id->password_len;
        }
        sasl++;
    }

    return LDAP_SUCCESS;
#endif
    return -1;
}

static NTSTATUS wrap_ldap_sasl_interactive_bind_s( void *args )
{
    struct ldap_sasl_interactive_bind_s_params *params = args;
    return ldap_sasl_interactive_bind_s( params->ld, params->dn, params->mech,
                                         (LDAPControl **)params->serverctrls,
                                         (LDAPControl **)params->clientctrls, params->flags,
                                         wrap_sasl_interact, params->defaults );
}

static NTSTATUS wrap_ldap_search_ext( void *args )
{
    struct ldap_search_ext_params *params = args;
    struct timeval tv;
    return ldap_search_ext( params->ld, params->base, params->scope, params->filter, params->attrs,
                            params->attrsonly, (LDAPControl **)params->serverctrls,
                            (LDAPControl **)params->clientctrls, convert_timeval(params->timeout, &tv),
                            params->sizelimit, (int *)params->msg );
}

static NTSTATUS wrap_ldap_search_ext_s( void *args )
{
    struct ldap_search_ext_s_params *params = args;
    struct timeval tv;
    return ldap_search_ext_s( params->ld, params->base, params->scope, params->filter, params->attrs,
                              params->attrsonly, (LDAPControl **)params->serverctrls,
                              (LDAPControl **)params->clientctrls, convert_timeval(params->timeout, &tv),
                              params->sizelimit, (LDAPMessage **)params->result );
}

static NTSTATUS wrap_ldap_set_option( void *args )
{
    struct ldap_set_option_params *params = args;
    return ldap_set_option( params->ld, params->option, params->value );
}

static NTSTATUS wrap_ldap_start_tls_s( void *args )
{
    struct ldap_start_tls_s_params *params = args;
    return ldap_start_tls_s( params->ld, (LDAPControl **)params->serverctrls,
                             (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_unbind_ext( void *args )
{
    struct ldap_unbind_ext_params *params = args;
    return ldap_unbind_ext( params->ld, (LDAPControl **)params->serverctrls,
                            (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_unbind_ext_s( void *args )
{
    struct ldap_unbind_ext_s_params *params = args;
    return ldap_unbind_ext_s( params->ld, (LDAPControl **)params->serverctrls,
                              (LDAPControl **)params->clientctrls );
}

static NTSTATUS wrap_ldap_value_free_len( void *args )
{
    ldap_value_free_len( args );
    return STATUS_SUCCESS;
}

unixlib_entry_t __wine_unix_call_funcs[] =
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
    wrap_ber_printf,
    wrap_ber_scanf,
    wrap_ber_skip_tag,
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
    wrap_ldap_first_attribute,
    wrap_ldap_first_entry,
    wrap_ldap_first_reference,
    wrap_ldap_get_dn,
    wrap_ldap_get_option,
    wrap_ldap_get_values_len,
    wrap_ldap_initialize,
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

#endif /* HAVE_LDAP */
