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
 *      ldap_parse_extended_resultA     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_extended_resultA( LDAP *ld, WLDAP32_LDAPMessage *result, char **oid,
                                         struct WLDAP32_berval **data, BOOLEAN free )
{
    ULONG ret;
    WCHAR *oidW = NULL;

    TRACE( "(%p, %p, %p, %p, 0x%02x)\n", ld, result, oid, data, free );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!result) return WLDAP32_LDAP_NO_RESULTS_RETURNED;

    ret = ldap_parse_extended_resultW( ld, result, &oidW, data, free );
    if (oid && oidW)
    {
        char *str;
        if ((str = strWtoA( oidW ))) *oid = str;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfreeW( oidW );
    }
    return ret;
}

/***********************************************************************
 *      ldap_parse_extended_resultW     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_extended_resultW( LDAP *ld, WLDAP32_LDAPMessage *result, WCHAR **oid,
                                         struct WLDAP32_berval **data, BOOLEAN free )
{
    ULONG ret;
    char *oidU = NULL;
    struct berval *dataU = NULL;

    TRACE( "(%p, %p, %p, %p, 0x%02x)\n", ld, result, oid, data, free );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!result) return WLDAP32_LDAP_NO_RESULTS_RETURNED;
    else
    {
        ret = map_error( ldap_parse_extended_result( CTX(ld), result, &oidU, &dataU, free ) );
    }
    if (oid && oidU)
    {
        WCHAR *str;
        if ((str = strUtoW( oidU ))) *oid = str;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfree( oidU );
    }
    if (data && dataU)
    {
        struct WLDAP32_berval *bv;
        if ((bv = bervalUtoW( dataU ))) *data = bv;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ber_bvfree( dataU );
    }

    return ret;
}

/***********************************************************************
 *      ldap_parse_referenceA     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_referenceA( LDAP *ld, WLDAP32_LDAPMessage *message, char ***referrals )
{
    ULONG ret;
    WCHAR **referralsW = NULL;

    TRACE( "(%p, %p, %p)\n", ld, message, referrals );

    if (!ld) return ~0u;

    ret = ldap_parse_referenceW( ld, message, &referralsW );
    if (referralsW)
    {
        char **ref;
        if ((ref = strarrayWtoA( referralsW ))) *referrals = ref;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_value_freeW( referralsW );
    }
    return ret;
}

/***********************************************************************
 *      ldap_parse_referenceW     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_referenceW( LDAP *ld, WLDAP32_LDAPMessage *message, WCHAR ***referrals )
{
    ULONG ret = ~0u;
    char **referralsU = NULL;

    TRACE( "(%p, %p, %p)\n", ld, message, referrals );

    if (ld) ret = map_error( ldap_parse_reference( CTX(ld), message, &referralsU, NULL, 0 ) );

    if (referralsU)
    {
        WCHAR **ref;
        if ((ref = strarrayUtoW( referralsU ))) *referrals = ref;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfree( referralsU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_parse_resultA     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_resultA( LDAP *ld, WLDAP32_LDAPMessage *result, ULONG *retcode, char **matched, char **error,
    char ***referrals, LDAPControlA ***serverctrls, BOOLEAN free )
{
    ULONG ret;
    WCHAR *matchedW = NULL, *errorW = NULL, **referralsW = NULL;
    LDAPControlW **serverctrlsW = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p, %p, 0x%02x)\n", ld, result, retcode, matched, error, referrals, serverctrls,
           free );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!result) return WLDAP32_LDAP_NO_RESULTS_RETURNED;

    ret = ldap_parse_resultW( ld, result, retcode, &matchedW, &errorW, &referralsW, &serverctrlsW, free );

    if (matched) *matched = strWtoA( matchedW );
    if (error) *error = strWtoA( errorW );
    if (referrals) *referrals = strarrayWtoA( referralsW );
    if (serverctrls) *serverctrls = controlarrayWtoA( serverctrlsW );

    ldap_memfreeW( matchedW );
    ldap_memfreeW( errorW );
    ldap_value_freeW( referralsW );
    ldap_controls_freeW( serverctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_parse_resultW     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_resultW( LDAP *ld, WLDAP32_LDAPMessage *result, ULONG *retcode, WCHAR **matched, WCHAR **error,
    WCHAR ***referrals, LDAPControlW ***serverctrls, BOOLEAN free )
{
    ULONG ret;
    char *matchedU = NULL, *errorU = NULL, **referralsU = NULL;
    LDAPControl **serverctrlsU = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p, %p, 0x%02x)\n", ld, result, retcode, matched, error, referrals, serverctrls,
           free );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!result) return WLDAP32_LDAP_NO_RESULTS_RETURNED;

    ret = map_error( ldap_parse_result( CTX(ld), MSG(result), (int *)retcode, &matchedU, &errorU, &referralsU,
                                        &serverctrlsU, free ) );

    if (matched) *matched = strUtoW( matchedU );
    if (error) *error = strUtoW( errorU );
    if (referrals) *referrals = strarrayUtoW( referralsU );
    if (serverctrls) *serverctrls = controlarrayUtoW( serverctrlsU );

    if (free) ldap_msgfree( result );
    ldap_memfree( matchedU );
    ldap_memfree( errorU );
    ldap_memfree( referralsU );
    ldap_controls_free( serverctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_parse_sort_controlA     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_sort_controlA( LDAP *ld, LDAPControlA **control, ULONG *result, char **attr )
{
    ULONG ret;
    WCHAR *attrW = NULL;
    LDAPControlW **controlW = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, control, result, attr );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!control) return WLDAP32_LDAP_CONTROL_NOT_FOUND;
    if (!(controlW = controlarrayAtoW( control ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_parse_sort_controlW( ld, controlW, result, &attrW );

    *attr = strWtoA( attrW );
    controlarrayfreeW( controlW );
    return ret;
}

/***********************************************************************
 *      ldap_parse_sort_controlW     (WLDAP32.@)
 */
ULONG CDECL ldap_parse_sort_controlW( LDAP *ld, LDAPControlW **control, ULONG *result, WCHAR **attr )
{
    ULONG ret;
    char *attrU = NULL;
    LDAPControl **controlU, *sortcontrol = NULL;
    int res;
    unsigned int i;

    TRACE( "(%p, %p, %p, %p)\n", ld, control, result, attr );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!control) return WLDAP32_LDAP_CONTROL_NOT_FOUND;
    if (!(controlU = controlarrayWtoU( control ))) return WLDAP32_LDAP_NO_MEMORY;

    for (i = 0; controlU[i]; i++)
    {
        if (!strcmp( LDAP_SERVER_RESP_SORT_OID, controlU[i]->ldctl_oid ))
            sortcontrol = controlU[i];
    }
    if (!sortcontrol)
    {
        controlarrayfreeU( controlU );
        return WLDAP32_LDAP_CONTROL_NOT_FOUND;
    }

    ret = map_error( ldap_parse_sortresponse_control( CTX(ld), sortcontrol, &res, &attrU ) );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        WCHAR *str;
        if ((str = strUtoW( attrU )))
        {
            *attr = str;
            *result = res;
        }
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfree( attrU );
    }

    controlarrayfreeU( controlU );
    return ret;
}

/***********************************************************************
 *      ldap_parse_vlv_controlA     (WLDAP32.@)
 */
int CDECL ldap_parse_vlv_controlA( LDAP *ld, LDAPControlA **control, ULONG *targetpos, ULONG *listcount,
                                   struct WLDAP32_berval **context, int *errcode )
{
    int ret;
    LDAPControlW **controlW = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p)\n", ld, control, targetpos, listcount, context, errcode );

    if (!ld) return ~0u;

    if (control && !(controlW = controlarrayAtoW( control ))) return WLDAP32_LDAP_NO_MEMORY;
    ret = ldap_parse_vlv_controlW( ld, controlW, targetpos, listcount, context, errcode );
    controlarrayfreeW( controlW );
    return ret;
}

/***********************************************************************
 *      ldap_parse_vlv_controlW     (WLDAP32.@)
 */
int CDECL ldap_parse_vlv_controlW( LDAP *ld, LDAPControlW **control, ULONG *targetpos, ULONG *listcount,
                                   struct WLDAP32_berval **context, int *errcode )
{
    int ret, pos, count;
    LDAPControl **controlU, *vlvcontrolU = NULL;
    struct berval *ctxU;
    unsigned int i;

    TRACE( "(%p, %p, %p, %p, %p, %p)\n", ld, control, targetpos, listcount, context, errcode );

    if (!ld || !control) return ~0u;

    if (!(controlU = controlarrayWtoU( control ))) return WLDAP32_LDAP_NO_MEMORY;

    for (i = 0; controlU[i]; i++)
    {
        if (!strcmp( LDAP_CONTROL_VLVRESPONSE, controlU[i]->ldctl_oid ))
            vlvcontrolU = controlU[i];
    }
    if (!vlvcontrolU)
    {
        controlarrayfreeU( controlU );
        return WLDAP32_LDAP_CONTROL_NOT_FOUND;
    }

    ret = map_error( ldap_parse_vlvresponse_control( CTX(ld), vlvcontrolU, &pos, &count, &ctxU, errcode ) );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        struct WLDAP32_berval *bv;
        if ((bv = bervalUtoW( ctxU )))
        {
            *context = bv;
            *targetpos = pos;
            *listcount = count;
        }
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ber_bvfree( ctxU );
    }

    controlarrayfreeU( controlU );
    return ret;
}
