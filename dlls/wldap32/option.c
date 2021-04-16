/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_get_optionA     (WLDAP32.@)
 *
 * See ldap_get_optionW.
 */
ULONG CDECL ldap_get_optionA( WLDAP32_LDAP *ld, int option, void *value )
{
    ULONG ret;

    TRACE( "(%p, 0x%08x, %p)\n", ld, option, value );

    if (!ld || !value) return WLDAP32_LDAP_PARAM_ERROR;

    switch (option)
    {
    case WLDAP32_LDAP_OPT_API_FEATURE_INFO:
    {
        LDAPAPIFeatureInfoW featureW;
        LDAPAPIFeatureInfoA *featureA = value;

        if (!featureA->ldapaif_name) return WLDAP32_LDAP_PARAM_ERROR;

        featureW.ldapaif_info_version = featureA->ldapaif_info_version;
        if (!(featureW.ldapaif_name = strAtoW( featureA->ldapaif_name ))) return WLDAP32_LDAP_NO_MEMORY;
        featureW.ldapaif_version = 0;

        ret = ldap_get_optionW( ld, option, &featureW );

        if (ret == WLDAP32_LDAP_SUCCESS) featureA->ldapaif_version = featureW.ldapaif_version;
        strfreeW( featureW.ldapaif_name );
        return ret;
    }
    case WLDAP32_LDAP_OPT_API_INFO:
    {
        LDAPAPIInfoW infoW;
        LDAPAPIInfoA *infoA = value;

        memset( &infoW, 0, sizeof(infoW) );
        infoW.ldapai_info_version = infoA->ldapai_info_version;

        ret = ldap_get_optionW( ld, option, &infoW );
        if (ret == WLDAP32_LDAP_SUCCESS)
        {
            infoA->ldapai_api_version = infoW.ldapai_api_version;
            infoA->ldapai_protocol_version = infoW.ldapai_protocol_version;

            if (infoW.ldapai_extensions && !(infoA->ldapai_extensions = strarrayWtoA( infoW.ldapai_extensions )))
                return WLDAP32_LDAP_NO_MEMORY;
            if (infoW.ldapai_vendor_name && !(infoA->ldapai_vendor_name = strWtoA( infoW.ldapai_vendor_name )))
            {
                ldap_value_freeW( infoW.ldapai_extensions );
                return WLDAP32_LDAP_NO_MEMORY;
            }
            infoA->ldapai_vendor_version = infoW.ldapai_vendor_version;

            ldap_value_freeW( infoW.ldapai_extensions );
            ldap_memfreeW( infoW.ldapai_vendor_name );
        }
        return ret;
    }

    case WLDAP32_LDAP_OPT_DEREF:
    case WLDAP32_LDAP_OPT_DESC:
    case WLDAP32_LDAP_OPT_ERROR_NUMBER:
    case WLDAP32_LDAP_OPT_PROTOCOL_VERSION:
    case WLDAP32_LDAP_OPT_REFERRALS:
    case WLDAP32_LDAP_OPT_SIZELIMIT:
    case WLDAP32_LDAP_OPT_TIMELIMIT:
        return ldap_get_optionW( ld, option, value );

    case WLDAP32_LDAP_OPT_CACHE_ENABLE:
    case WLDAP32_LDAP_OPT_CACHE_FN_PTRS:
    case WLDAP32_LDAP_OPT_CACHE_STRATEGY:
    case WLDAP32_LDAP_OPT_IO_FN_PTRS:
    case WLDAP32_LDAP_OPT_REBIND_ARG:
    case WLDAP32_LDAP_OPT_REBIND_FN:
    case WLDAP32_LDAP_OPT_RESTART:
    case WLDAP32_LDAP_OPT_THREAD_FN_PTRS:
        return WLDAP32_LDAP_LOCAL_ERROR;

    case WLDAP32_LDAP_OPT_AREC_EXCLUSIVE:
    case WLDAP32_LDAP_OPT_AUTO_RECONNECT:
    case WLDAP32_LDAP_OPT_CLIENT_CERTIFICATE:
    case WLDAP32_LDAP_OPT_DNSDOMAIN_NAME:
    case WLDAP32_LDAP_OPT_ENCRYPT:
    case WLDAP32_LDAP_OPT_ERROR_STRING:
    case WLDAP32_LDAP_OPT_FAST_CONCURRENT_BIND:
    case WLDAP32_LDAP_OPT_GETDSNAME_FLAGS:
    case WLDAP32_LDAP_OPT_HOST_NAME:
    case WLDAP32_LDAP_OPT_HOST_REACHABLE:
    case WLDAP32_LDAP_OPT_PING_KEEP_ALIVE:
    case WLDAP32_LDAP_OPT_PING_LIMIT:
    case WLDAP32_LDAP_OPT_PING_WAIT_TIME:
    case WLDAP32_LDAP_OPT_PROMPT_CREDENTIALS:
    case WLDAP32_LDAP_OPT_REF_DEREF_CONN_PER_MSG:
    case WLDAP32_LDAP_OPT_REFERRAL_CALLBACK:
    case WLDAP32_LDAP_OPT_REFERRAL_HOP_LIMIT:
    case WLDAP32_LDAP_OPT_ROOTDSE_CACHE:
    case WLDAP32_LDAP_OPT_SASL_METHOD:
    case WLDAP32_LDAP_OPT_SECURITY_CONTEXT:
    case WLDAP32_LDAP_OPT_SEND_TIMEOUT:
    case WLDAP32_LDAP_OPT_SERVER_CERTIFICATE:
    case WLDAP32_LDAP_OPT_SERVER_CONTROLS:
    case WLDAP32_LDAP_OPT_SERVER_ERROR:
    case WLDAP32_LDAP_OPT_SERVER_EXT_ERROR:
    case WLDAP32_LDAP_OPT_SIGN:
    case WLDAP32_LDAP_OPT_SSL:
    case WLDAP32_LDAP_OPT_SSL_INFO:
    case WLDAP32_LDAP_OPT_SSPI_FLAGS:
    case WLDAP32_LDAP_OPT_TCP_KEEPALIVE:
        FIXME( "Unsupported option: 0x%02x\n", option );
        return WLDAP32_LDAP_NOT_SUPPORTED;

    default:
        FIXME( "Unknown option: 0x%02x\n", option );
        return WLDAP32_LDAP_LOCAL_ERROR;
    }
}

/***********************************************************************
 *      ldap_get_optionW     (WLDAP32.@)
 *
 * Retrieve option values for a given LDAP context.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  option  [I] Option to get values for.
 *  value   [O] Pointer to option values.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_get_optionW( WLDAP32_LDAP *ld, int option, void *value )
{
    ULONG ret;

    TRACE( "(%p, 0x%08x, %p)\n", ld, option, value );

    if (!ld || !value) return WLDAP32_LDAP_PARAM_ERROR;

    switch (option)
    {
    case WLDAP32_LDAP_OPT_API_FEATURE_INFO:
    {
        LDAPAPIFeatureInfoU featureU;
        LDAPAPIFeatureInfoW *featureW = value;

        if (!featureW->ldapaif_name) return WLDAP32_LDAP_PARAM_ERROR;

        featureU.ldapaif_info_version = featureW->ldapaif_info_version;
        if (!(featureU.ldapaif_name = strWtoU( featureW->ldapaif_name ))) return WLDAP32_LDAP_NO_MEMORY;
        featureU.ldapaif_version = 0;

        ret = map_error( ldap_funcs->ldap_get_option( ld->ld, option, &featureU ) );

        if (ret == WLDAP32_LDAP_SUCCESS) featureW->ldapaif_version = featureU.ldapaif_version;
        strfreeU( featureU.ldapaif_name );
        return ret;
    }
    case WLDAP32_LDAP_OPT_API_INFO:
    {
        LDAPAPIInfoU infoU;
        LDAPAPIInfoW *infoW = value;

        memset( &infoU, 0, sizeof(infoU) );
        infoU.ldapai_info_version = infoW->ldapai_info_version;

        ret = map_error( ldap_funcs->ldap_get_option( ld->ld, option, &infoU ) );
        if (ret == WLDAP32_LDAP_SUCCESS)
        {
            infoW->ldapai_api_version = infoU.ldapai_api_version;
            infoW->ldapai_protocol_version = infoU.ldapai_protocol_version;

            if (infoU.ldapai_extensions && !(infoW->ldapai_extensions = strarrayUtoW( infoU.ldapai_extensions )))
                return WLDAP32_LDAP_NO_MEMORY;
            if (infoU.ldapai_vendor_name && !(infoW->ldapai_vendor_name = strUtoW( infoU.ldapai_vendor_name )))
            {
                ldap_funcs->ldap_memvfree( (void **)infoU.ldapai_extensions );
                return WLDAP32_LDAP_NO_MEMORY;
            }
            infoW->ldapai_vendor_version = infoU.ldapai_vendor_version;

            ldap_funcs->ldap_memvfree( (void **)infoU.ldapai_extensions );
            ldap_funcs->ldap_memfree( infoU.ldapai_vendor_name );
        }
        return ret;
    }

    case WLDAP32_LDAP_OPT_DEREF:
    case WLDAP32_LDAP_OPT_DESC:
    case WLDAP32_LDAP_OPT_ERROR_NUMBER:
    case WLDAP32_LDAP_OPT_PROTOCOL_VERSION:
    case WLDAP32_LDAP_OPT_REFERRALS:
    case WLDAP32_LDAP_OPT_SIZELIMIT:
    case WLDAP32_LDAP_OPT_TIMELIMIT:
        return map_error( ldap_funcs->ldap_get_option( ld->ld, option, value ));

    case WLDAP32_LDAP_OPT_CACHE_ENABLE:
    case WLDAP32_LDAP_OPT_CACHE_FN_PTRS:
    case WLDAP32_LDAP_OPT_CACHE_STRATEGY:
    case WLDAP32_LDAP_OPT_IO_FN_PTRS:
    case WLDAP32_LDAP_OPT_REBIND_ARG:
    case WLDAP32_LDAP_OPT_REBIND_FN:
    case WLDAP32_LDAP_OPT_RESTART:
    case WLDAP32_LDAP_OPT_THREAD_FN_PTRS:
        return WLDAP32_LDAP_LOCAL_ERROR;

    case WLDAP32_LDAP_OPT_AREC_EXCLUSIVE:
    case WLDAP32_LDAP_OPT_AUTO_RECONNECT:
    case WLDAP32_LDAP_OPT_CLIENT_CERTIFICATE:
    case WLDAP32_LDAP_OPT_DNSDOMAIN_NAME:
    case WLDAP32_LDAP_OPT_ENCRYPT:
    case WLDAP32_LDAP_OPT_ERROR_STRING:
    case WLDAP32_LDAP_OPT_FAST_CONCURRENT_BIND:
    case WLDAP32_LDAP_OPT_GETDSNAME_FLAGS:
    case WLDAP32_LDAP_OPT_HOST_NAME:
    case WLDAP32_LDAP_OPT_HOST_REACHABLE:
    case WLDAP32_LDAP_OPT_PING_KEEP_ALIVE:
    case WLDAP32_LDAP_OPT_PING_LIMIT:
    case WLDAP32_LDAP_OPT_PING_WAIT_TIME:
    case WLDAP32_LDAP_OPT_PROMPT_CREDENTIALS:
    case WLDAP32_LDAP_OPT_REF_DEREF_CONN_PER_MSG:
    case WLDAP32_LDAP_OPT_REFERRAL_CALLBACK:
    case WLDAP32_LDAP_OPT_REFERRAL_HOP_LIMIT:
    case WLDAP32_LDAP_OPT_ROOTDSE_CACHE:
    case WLDAP32_LDAP_OPT_SASL_METHOD:
    case WLDAP32_LDAP_OPT_SECURITY_CONTEXT:
    case WLDAP32_LDAP_OPT_SEND_TIMEOUT:
    case WLDAP32_LDAP_OPT_SERVER_CERTIFICATE:
    case WLDAP32_LDAP_OPT_SERVER_CONTROLS:
    case WLDAP32_LDAP_OPT_SERVER_ERROR:
    case WLDAP32_LDAP_OPT_SERVER_EXT_ERROR:
    case WLDAP32_LDAP_OPT_SIGN:
    case WLDAP32_LDAP_OPT_SSL:
    case WLDAP32_LDAP_OPT_SSL_INFO:
    case WLDAP32_LDAP_OPT_SSPI_FLAGS:
    case WLDAP32_LDAP_OPT_TCP_KEEPALIVE:
        FIXME( "Unsupported option: 0x%02x\n", option );
        return WLDAP32_LDAP_NOT_SUPPORTED;

    default:
        FIXME( "Unknown option: 0x%02x\n", option );
        return WLDAP32_LDAP_LOCAL_ERROR;
    }
}

/***********************************************************************
 *      ldap_set_optionA     (WLDAP32.@)
 *
 * See ldap_set_optionW.
 */
ULONG CDECL ldap_set_optionA( WLDAP32_LDAP *ld, int option, void *value )
{
    ULONG ret;

    TRACE( "(%p, 0x%08x, %p)\n", ld, option, value );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    switch (option)
    {
    case WLDAP32_LDAP_OPT_SERVER_CONTROLS:
    {
        LDAPControlW **ctrlsW;
        if (!(ctrlsW = controlarrayAtoW( value ))) return WLDAP32_LDAP_NO_MEMORY;
        ret = ldap_set_optionW( ld, option, ctrlsW );
        controlarrayfreeW( ctrlsW );
        return ret;
    }
    case WLDAP32_LDAP_OPT_DEREF:
    case WLDAP32_LDAP_OPT_DESC:
    case WLDAP32_LDAP_OPT_ERROR_NUMBER:
    case WLDAP32_LDAP_OPT_PROTOCOL_VERSION:
    case WLDAP32_LDAP_OPT_REFERRALS:
    case WLDAP32_LDAP_OPT_SIZELIMIT:
    case WLDAP32_LDAP_OPT_TIMELIMIT:
        return ldap_set_optionW( ld, option, value );

    case WLDAP32_LDAP_OPT_CACHE_ENABLE:
    case WLDAP32_LDAP_OPT_CACHE_FN_PTRS:
    case WLDAP32_LDAP_OPT_CACHE_STRATEGY:
    case WLDAP32_LDAP_OPT_IO_FN_PTRS:
    case WLDAP32_LDAP_OPT_REBIND_ARG:
    case WLDAP32_LDAP_OPT_REBIND_FN:
    case WLDAP32_LDAP_OPT_RESTART:
    case WLDAP32_LDAP_OPT_THREAD_FN_PTRS:
        return WLDAP32_LDAP_LOCAL_ERROR;

    case WLDAP32_LDAP_OPT_API_FEATURE_INFO:
    case WLDAP32_LDAP_OPT_API_INFO:
        return WLDAP32_LDAP_UNWILLING_TO_PERFORM;

    case WLDAP32_LDAP_OPT_AREC_EXCLUSIVE:
    case WLDAP32_LDAP_OPT_AUTO_RECONNECT:
    case WLDAP32_LDAP_OPT_CLIENT_CERTIFICATE:
    case WLDAP32_LDAP_OPT_DNSDOMAIN_NAME:
    case WLDAP32_LDAP_OPT_ENCRYPT:
    case WLDAP32_LDAP_OPT_ERROR_STRING:
    case WLDAP32_LDAP_OPT_FAST_CONCURRENT_BIND:
    case WLDAP32_LDAP_OPT_GETDSNAME_FLAGS:
    case WLDAP32_LDAP_OPT_HOST_NAME:
    case WLDAP32_LDAP_OPT_HOST_REACHABLE:
    case WLDAP32_LDAP_OPT_PING_KEEP_ALIVE:
    case WLDAP32_LDAP_OPT_PING_LIMIT:
    case WLDAP32_LDAP_OPT_PING_WAIT_TIME:
    case WLDAP32_LDAP_OPT_PROMPT_CREDENTIALS:
    case WLDAP32_LDAP_OPT_REF_DEREF_CONN_PER_MSG:
    case WLDAP32_LDAP_OPT_REFERRAL_CALLBACK:
    case WLDAP32_LDAP_OPT_REFERRAL_HOP_LIMIT:
    case WLDAP32_LDAP_OPT_ROOTDSE_CACHE:
    case WLDAP32_LDAP_OPT_SASL_METHOD:
    case WLDAP32_LDAP_OPT_SECURITY_CONTEXT:
    case WLDAP32_LDAP_OPT_SEND_TIMEOUT:
    case WLDAP32_LDAP_OPT_SERVER_CERTIFICATE:
    case WLDAP32_LDAP_OPT_SERVER_ERROR:
    case WLDAP32_LDAP_OPT_SERVER_EXT_ERROR:
    case WLDAP32_LDAP_OPT_SIGN:
    case WLDAP32_LDAP_OPT_SSL:
    case WLDAP32_LDAP_OPT_SSL_INFO:
    case WLDAP32_LDAP_OPT_SSPI_FLAGS:
    case WLDAP32_LDAP_OPT_TCP_KEEPALIVE:
        FIXME( "Unsupported option: 0x%02x\n", option );
        return WLDAP32_LDAP_NOT_SUPPORTED;

    default:
        FIXME( "Unknown option: 0x%02x\n", option );
        return WLDAP32_LDAP_LOCAL_ERROR;
    }
}

static BOOL query_supported_server_ctrls( WLDAP32_LDAP *ld )
{
    char *attrs[] = { (char *)"supportedControl", NULL };
    void *res, *entry;
    ULONG ret;

    if (ld->ld_server_ctrls) return TRUE;

    ret = map_error( ldap_funcs->ldap_search_ext_s( ld->ld, (char *)"", WLDAP32_LDAP_SCOPE_BASE,
                                                    (char *)"(objectClass=*)", attrs, FALSE, NULL, NULL, NULL, 0, &res ) );
    if (ret != WLDAP32_LDAP_SUCCESS) return FALSE;

    entry = ldap_funcs->ldap_first_entry( ld->ld, res );
    if (entry)
    {
        ULONG count, i;

        ld->ld_server_ctrls = ldap_funcs->ldap_get_values_len( ld->ld, entry, attrs[0] );
        count = ldap_funcs->ldap_count_values_len( ld->ld_server_ctrls );
        for (i = 0; i < count; i++)
            TRACE("%u: %s\n", i, debugstr_an( ld->ld_server_ctrls[i]->bv_val, ld->ld_server_ctrls[i]->bv_len ));
    }

    ldap_funcs->ldap_msgfree( res );

    return ld->ld_server_ctrls != NULL;
}

static BOOL is_supported_server_ctrls( WLDAP32_LDAP *ld, LDAPControlU **ctrls )
{
    ULONG user_count, server_count, i, n, supported = 0;

    if (!query_supported_server_ctrls( ld ))
        return TRUE; /* can't verify, let the server handle it on next query */

    user_count = controlarraylenU( ctrls );
    server_count = ldap_funcs->ldap_count_values_len( ld->ld_server_ctrls );

    for (n = 0; n < user_count; n++)
    {
        TRACE("looking for %s\n", debugstr_a(ctrls[n]->ldctl_oid));

        for (i = 0; i < server_count; i++)
        {
            if (!strncmp( ctrls[n]->ldctl_oid, ld->ld_server_ctrls[i]->bv_val, ld->ld_server_ctrls[i]->bv_len))
            {
                supported++;
                break;
            }
        }
    }

    return supported == user_count;
}

/***********************************************************************
 *      ldap_set_optionW     (WLDAP32.@)
 *
 * Set option values for a given LDAP context.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  option  [I] Option to set values for.
 *  value   [I] Pointer to option values.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Set value to LDAP_OPT_ON or LDAP_OPT_OFF for on/off options.
 */
ULONG CDECL ldap_set_optionW( WLDAP32_LDAP *ld, int option, void *value )
{
    ULONG ret;

    TRACE( "(%p, 0x%08x, %p)\n", ld, option, value );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    switch (option)
    {
    case WLDAP32_LDAP_OPT_SERVER_CONTROLS:
    {
        LDAPControlU **ctrlsU;

        if (!(ctrlsU = controlarrayWtoU( value ))) return WLDAP32_LDAP_NO_MEMORY;

        if (!is_supported_server_ctrls( ld, ctrlsU ))
            ret = WLDAP32_LDAP_PARAM_ERROR;
        else
            ret = map_error( ldap_funcs->ldap_set_option( ld->ld, option, ctrlsU ) );
        controlarrayfreeU( ctrlsU );
        return ret;
    }
    case WLDAP32_LDAP_OPT_REFERRALS:
    {
        void *openldap_referral = WLDAP32_LDAP_OPT_ON;
        if (value == WLDAP32_LDAP_OPT_OFF)
            openldap_referral = WLDAP32_LDAP_OPT_OFF;
        else
            FIXME("upgrading referral value %p to LDAP_OPT_ON (OpenLDAP lacks sufficient granularity)\n", value);
        return map_error( ldap_funcs->ldap_set_option( ld->ld, option, openldap_referral ) );
        break;
    }
    case WLDAP32_LDAP_OPT_DEREF:
    case WLDAP32_LDAP_OPT_DESC:
    case WLDAP32_LDAP_OPT_ERROR_NUMBER:
    case WLDAP32_LDAP_OPT_PROTOCOL_VERSION:
    case WLDAP32_LDAP_OPT_SIZELIMIT:
    case WLDAP32_LDAP_OPT_TIMELIMIT:
        return map_error( ldap_funcs->ldap_set_option( ld->ld, option, value ));

    case WLDAP32_LDAP_OPT_CACHE_ENABLE:
    case WLDAP32_LDAP_OPT_CACHE_FN_PTRS:
    case WLDAP32_LDAP_OPT_CACHE_STRATEGY:
    case WLDAP32_LDAP_OPT_IO_FN_PTRS:
    case WLDAP32_LDAP_OPT_REBIND_ARG:
    case WLDAP32_LDAP_OPT_REBIND_FN:
    case WLDAP32_LDAP_OPT_RESTART:
    case WLDAP32_LDAP_OPT_THREAD_FN_PTRS:
        return WLDAP32_LDAP_LOCAL_ERROR;

    case WLDAP32_LDAP_OPT_API_FEATURE_INFO:
    case WLDAP32_LDAP_OPT_API_INFO:
        return WLDAP32_LDAP_UNWILLING_TO_PERFORM;

    case WLDAP32_LDAP_OPT_AREC_EXCLUSIVE:
    case WLDAP32_LDAP_OPT_AUTO_RECONNECT:
    case WLDAP32_LDAP_OPT_CLIENT_CERTIFICATE:
    case WLDAP32_LDAP_OPT_DNSDOMAIN_NAME:
    case WLDAP32_LDAP_OPT_ENCRYPT:
    case WLDAP32_LDAP_OPT_ERROR_STRING:
    case WLDAP32_LDAP_OPT_FAST_CONCURRENT_BIND:
    case WLDAP32_LDAP_OPT_GETDSNAME_FLAGS:
    case WLDAP32_LDAP_OPT_HOST_NAME:
    case WLDAP32_LDAP_OPT_HOST_REACHABLE:
    case WLDAP32_LDAP_OPT_PING_KEEP_ALIVE:
    case WLDAP32_LDAP_OPT_PING_LIMIT:
    case WLDAP32_LDAP_OPT_PING_WAIT_TIME:
    case WLDAP32_LDAP_OPT_PROMPT_CREDENTIALS:
    case WLDAP32_LDAP_OPT_REF_DEREF_CONN_PER_MSG:
    case WLDAP32_LDAP_OPT_REFERRAL_CALLBACK:
    case WLDAP32_LDAP_OPT_REFERRAL_HOP_LIMIT:
    case WLDAP32_LDAP_OPT_ROOTDSE_CACHE:
    case WLDAP32_LDAP_OPT_SASL_METHOD:
    case WLDAP32_LDAP_OPT_SECURITY_CONTEXT:
    case WLDAP32_LDAP_OPT_SEND_TIMEOUT:
    case WLDAP32_LDAP_OPT_SERVER_CERTIFICATE:
    case WLDAP32_LDAP_OPT_SERVER_ERROR:
    case WLDAP32_LDAP_OPT_SERVER_EXT_ERROR:
    case WLDAP32_LDAP_OPT_SIGN:
    case WLDAP32_LDAP_OPT_SSL:
    case WLDAP32_LDAP_OPT_SSL_INFO:
    case WLDAP32_LDAP_OPT_SSPI_FLAGS:
    case WLDAP32_LDAP_OPT_TCP_KEEPALIVE:
        FIXME( "Unsupported option: 0x%02x\n", option );
        return WLDAP32_LDAP_NOT_SUPPORTED;

    default:
        FIXME( "Unknown option: 0x%02x\n", option );
        return WLDAP32_LDAP_LOCAL_ERROR;
    }
}
