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
#include "winldap.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_parse_extended_resultA     (WLDAP32.@)
 *
 * See ldap_parse_extended_resultW.
 */
ULONG CDECL ldap_parse_extended_resultA( LDAP *ld, LDAPMessage *result, char **oid, struct berval **data,
    BOOLEAN free )
{
    ULONG ret;
    WCHAR *oidW = NULL;

    TRACE( "(%p, %p, %p, %p, 0x%02x)\n", ld, result, oid, data, free );

    if (!ld) return LDAP_PARAM_ERROR;
    if (!result) return LDAP_NO_RESULTS_RETURNED;

    ret = ldap_parse_extended_resultW( ld, result, &oidW, data, free );
    if (oid && oidW)
    {
        char *str;
        if ((str = strWtoA( oidW ))) *oid = str;
        else ret = LDAP_NO_MEMORY;
        ldap_memfreeW( oidW );
    }
    return ret;
}

/***********************************************************************
 *      ldap_parse_extended_resultW     (WLDAP32.@)
 *
 * Parse the result of an extended operation.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  result  [I] Result message from an extended operation.
 *  oid     [O] OID of the extended operation.
 *  data    [O] Result data.
 *  free    [I] Free the result message?
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Free the OID and result data with ldap_memfree. Pass a nonzero
 *  value for 'free' or call ldap_msgfree to free the result message.
 */
ULONG CDECL ldap_parse_extended_resultW( LDAP *ld, LDAPMessage *result, WCHAR **oid, struct berval **data,
    BOOLEAN free )
{
    ULONG ret;
    char *oidU = NULL;
    struct bervalU *dataU = NULL;

    TRACE( "(%p, %p, %p, %p, 0x%02x)\n", ld, result, oid, data, free );

    if (!ld) return LDAP_PARAM_ERROR;
    if (!result) return LDAP_NO_RESULTS_RETURNED;

    ret = map_error( ldap_funcs->fn_ldap_parse_extended_result( CTX(ld), result, &oidU, &dataU, free ) );
    if (oid && oidU)
    {
        WCHAR *str;
        if ((str = strUtoW( oidU ))) *oid = str;
        else ret = LDAP_NO_MEMORY;
        ldap_funcs->fn_ldap_memfree( oidU );
    }
    if (data && dataU)
    {
        struct berval *bv;
        if ((bv = bervalUtoW( dataU ))) *data = bv;
        else ret = LDAP_NO_MEMORY;
        ldap_funcs->fn_ber_bvfree( dataU );
    }

    return ret;
}

/***********************************************************************
 *      ldap_parse_referenceA     (WLDAP32.@)
 *
 * See ldap_parse_referenceW.
 */
ULONG CDECL ldap_parse_referenceA( LDAP *ld, LDAPMessage *message, char ***referrals )
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
        else ret = LDAP_NO_MEMORY;
        ldap_value_freeW( referralsW );
    }
    return ret;
}

/***********************************************************************
 *      ldap_parse_referenceW     (WLDAP32.@)
 *
 * Return any referrals from a result message.
 *
 * PARAMS
 *  ld         [I] Pointer to an LDAP context.
 *  result     [I] Result message.
 *  referrals  [O] Array of referral URLs.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Free the referrals with ldap_value_free.
 */
ULONG CDECL ldap_parse_referenceW( LDAP *ld, LDAPMessage *message, WCHAR ***referrals )
{
    ULONG ret;
    char **referralsU = NULL;

    TRACE( "(%p, %p, %p)\n", ld, message, referrals );

    if (!ld) return ~0u;

    ret = map_error( ldap_funcs->fn_ldap_parse_reference( CTX(ld), message, &referralsU, NULL, 0 ) );
    if (referralsU)
    {
        WCHAR **ref;
        if ((ref = strarrayUtoW( referralsU ))) *referrals = ref;
        else ret = LDAP_NO_MEMORY;
        ldap_funcs->fn_ldap_memfree( referralsU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_parse_resultA     (WLDAP32.@)
 *
 * See ldap_parse_resultW.
 */
ULONG CDECL ldap_parse_resultA( LDAP *ld, LDAPMessage *result, ULONG *retcode, char **matched, char **error,
    char ***referrals, LDAPControlA ***serverctrls, BOOLEAN free )
{
    ULONG ret;
    WCHAR *matchedW = NULL, *errorW = NULL, **referralsW = NULL;
    LDAPControlW **serverctrlsW = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p, %p, 0x%02x)\n", ld, result, retcode, matched, error, referrals, serverctrls,
           free );

    if (!ld) return LDAP_PARAM_ERROR;

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
 *
 * Parse a result message.
 *
 * PARAMS
 *  ld           [I] Pointer to an LDAP context.
 *  result       [I] Result message.
 *  retcode      [O] Return code for the server operation.
 *  matched      [O] DNs matched in the operation.
 *  error        [O] Error message for the operation.
 *  referrals    [O] Referrals found in the result message.
 *  serverctrls  [O] Controls used in the operation.
 *  free         [I] Free the result message?
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Free the DNs and error message with ldap_memfree. Free
 *  the referrals with ldap_value_free and the controls with
 *  ldap_controls_free. Pass a nonzero value for 'free' or call
 *  ldap_msgfree to free the result message.
 */
ULONG CDECL ldap_parse_resultW( LDAP *ld, LDAPMessage *result, ULONG *retcode, WCHAR **matched, WCHAR **error,
    WCHAR ***referrals, LDAPControlW ***serverctrls, BOOLEAN free )
{
    ULONG ret;
    char *matchedU = NULL, *errorU = NULL, **referralsU = NULL;
    LDAPControlU **serverctrlsU = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p, %p, 0x%02x)\n", ld, result, retcode, matched, error, referrals, serverctrls,
           free );

    if (!ld) return LDAP_PARAM_ERROR;

    ret = map_error( ldap_funcs->fn_ldap_parse_result( CTX(ld), MSG(result), (int *)retcode, &matchedU, &errorU,
                                                       &referralsU, &serverctrlsU, free ) );

    if (matched) *matched = strUtoW( matchedU );
    if (error) *error = strUtoW( errorU );
    if (referrals) *referrals = strarrayUtoW( referralsU );
    if (serverctrls) *serverctrls = controlarrayUtoW( serverctrlsU );

    ldap_funcs->fn_ldap_memfree( matchedU );
    ldap_funcs->fn_ldap_memfree( errorU );
    ldap_funcs->fn_ldap_memvfree( (void **)referralsU );
    ldap_funcs->fn_ldap_controls_free( serverctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_parse_sort_controlA     (WLDAP32.@)
 *
 * See ldap_parse_sort_controlW.
 */
ULONG CDECL ldap_parse_sort_controlA( LDAP *ld, LDAPControlA **control, ULONG *result, char **attr )
{
    ULONG ret;
    WCHAR *attrW = NULL;
    LDAPControlW **controlW = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, control, result, attr );

    if (!ld) return LDAP_PARAM_ERROR;
    if (!control) return LDAP_CONTROL_NOT_FOUND;
    if (!(controlW = controlarrayAtoW( control ))) return LDAP_NO_MEMORY;

    ret = ldap_parse_sort_controlW( ld, controlW, result, &attrW );

    *attr = strWtoA( attrW );
    controlarrayfreeW( controlW );
    return ret;
}

/***********************************************************************
 *      ldap_parse_sort_controlW     (WLDAP32.@)
 *
 * Parse a sort control.
 *
 * PARAMS
 *  ld       [I] Pointer to an LDAP context.
 *  control  [I] Control obtained from a result message.
 *  result   [O] Result code.
 *  attr     [O] Failing attribute.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  If the function fails, free the failing attribute with ldap_memfree.
 */
ULONG CDECL ldap_parse_sort_controlW( LDAP *ld, LDAPControlW **control, ULONG *result, WCHAR **attr )
{
    ULONG ret;
    char *attrU = NULL;
    LDAPControlU **controlU, *sortcontrol = NULL;
    int res;
    unsigned int i;

    TRACE( "(%p, %p, %p, %p)\n", ld, control, result, attr );

    if (!ld) return LDAP_PARAM_ERROR;
    if (!control) return LDAP_CONTROL_NOT_FOUND;
    if (!(controlU = controlarrayWtoU( control ))) return LDAP_NO_MEMORY;

    for (i = 0; controlU[i]; i++)
    {
        if (!strcmp( LDAP_SERVER_RESP_SORT_OID, controlU[i]->ldctl_oid ))
            sortcontrol = controlU[i];
    }
    if (!sortcontrol)
    {
        controlarrayfreeU( controlU );
        return LDAP_CONTROL_NOT_FOUND;
    }

    ret = map_error( ldap_funcs->fn_ldap_parse_sortresponse_control( CTX(ld), sortcontrol, &res, &attrU ) );
    if (ret == LDAP_SUCCESS)
    {
        WCHAR *str;
        if ((str = strUtoW( attrU )))
        {
            *attr = str;
            *result = res;
        }
        else ret = LDAP_NO_MEMORY;
        ldap_funcs->fn_ldap_memfree( attrU );
    }

    controlarrayfreeU( controlU );
    return ret;
}

/***********************************************************************
 *      ldap_parse_vlv_controlA     (WLDAP32.@)
 *
 * See ldap_parse_vlv_controlW.
 */
int CDECL ldap_parse_vlv_controlA( LDAP *ld, LDAPControlA **control, ULONG *targetpos, ULONG *listcount,
    struct berval **context, int *errcode )
{
    int ret;
    LDAPControlW **controlW = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p)\n", ld, control, targetpos, listcount, context, errcode );

    if (!ld) return ~0u;

    if (control && !(controlW = controlarrayAtoW( control ))) return LDAP_NO_MEMORY;
    ret = ldap_parse_vlv_controlW( ld, controlW, targetpos, listcount, context, errcode );
    controlarrayfreeW( controlW );
    return ret;
}

/***********************************************************************
 *      ldap_parse_vlv_controlW     (WLDAP32.@)
 *
 * Parse a virtual list view control.
 *
 * PARAMS
 *  ld         [I] Pointer to an LDAP context.
 *  control    [I] Controls obtained from a result message.
 *  targetpos  [O] Position of the target in the result list.
 *  listcount  [O] Estimate of the number of results in the list.
 *  context    [O] Server side context.
 *  errcode    [O] Error code from the listview operation.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Free the server context with ber_bvfree.
 */
int CDECL ldap_parse_vlv_controlW( LDAP *ld, LDAPControlW **control, ULONG *targetpos, ULONG *listcount,
    struct berval **context, int *errcode )
{
    int ret, pos, count;
    LDAPControlU **controlU, *vlvcontrolU = NULL;
    struct bervalU *ctxU;
    unsigned int i;

    TRACE( "(%p, %p, %p, %p, %p, %p)\n", ld, control, targetpos, listcount, context, errcode );

    if (!ld || !control) return ~0u;

    if (!(controlU = controlarrayWtoU( control ))) return LDAP_NO_MEMORY;

    for (i = 0; controlU[i]; i++)
    {
        if (!strcmp( LDAP_CONTROL_VLVRESPONSE, controlU[i]->ldctl_oid ))
            vlvcontrolU = controlU[i];
    }
    if (!vlvcontrolU)
    {
        controlarrayfreeU( controlU );
        return LDAP_CONTROL_NOT_FOUND;
    }

    ret = map_error( ldap_funcs->fn_ldap_parse_vlvresponse_control( CTX(ld), vlvcontrolU, &pos, &count, &ctxU,
                                                                    errcode ) );
    if (ret == LDAP_SUCCESS)
    {
        struct berval *bv;
        if ((bv = bervalUtoW( ctxU )))
        {
            *context = bv;
            *targetpos = pos;
            *listcount = count;
        }
        else ret = LDAP_NO_MEMORY;
        ldap_funcs->fn_ber_bvfree( ctxU );
    }

    controlarrayfreeU( controlU );
    return ret;
}
