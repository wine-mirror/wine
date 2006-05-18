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

#include "config.h"

#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#else
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

ULONG ldap_parse_extended_resultA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *result,
    PCHAR *oid, struct WLDAP32_berval **data, BOOLEAN free )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *oidW = NULL;

    TRACE( "(%p, %p, %p, %p, 0x%02x)\n", ld, result, oid, data, free );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!result) return WLDAP32_LDAP_NO_RESULTS_RETURNED;

    ret = ldap_parse_extended_resultW( ld, result, &oidW, data, free );

    if (oid) {
        *oid = strWtoA( oidW );
        if (!*oid) ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfreeW( oidW );
    }

#endif
    return ret;
}

ULONG ldap_parse_extended_resultW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *result,
    PWCHAR *oid, struct WLDAP32_berval **data, BOOLEAN free )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *oidU = NULL;

    TRACE( "(%p, %p, %p, %p, 0x%02x)\n", ld, result, oid, data, free );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!result) return WLDAP32_LDAP_NO_RESULTS_RETURNED;

    ret = ldap_parse_extended_result( ld, result, &oidU, (struct berval **)data, free );

    if (oid) {
        *oid = strUtoW( oidU );
        if (!*oid) ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfree( oidU );
    }

#endif
    return ret;
}

ULONG ldap_parse_referenceA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *message,
    PCHAR **referrals )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR **referralsW = NULL;

    TRACE( "(%p, %p, %p)\n", ld, message, referrals );

    if (!ld) return ~0UL;

    ret = ldap_parse_referenceW( ld, message, &referralsW );

    *referrals = strarrayWtoA( referralsW );
    ldap_value_freeW( referralsW );

#endif 
    return ret;
}

ULONG ldap_parse_referenceW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *message,
    PWCHAR **referrals )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP_PARSE_REFERENCE
    char **referralsU = NULL;

    TRACE( "(%p, %p, %p)\n", ld, message, referrals );

    if (!ld) return ~0UL;
    
    ret = ldap_parse_reference( ld, message, &referralsU, NULL, 0 );

    *referrals = strarrayUtoW( referralsU );
    ldap_memfree( referralsU );

#endif
    return ret;
}

ULONG ldap_parse_resultA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *result,
    ULONG *retcode, PCHAR *matched, PCHAR *error, PCHAR **referrals,
    PLDAPControlA **serverctrls, BOOLEAN free )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR **matchedW = NULL, **errorW = NULL, **referralsW = NULL;
    LDAPControlW **serverctrlsW = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p, %p, 0x%02x)\n", ld, result, retcode,
           matched, error, referrals, serverctrls, free );

    if (!ld) return ~0UL;

    ret = ldap_parse_resultW( ld, result, retcode, matchedW, errorW,
                              &referralsW, &serverctrlsW, free );

    matched = strarrayWtoA( matchedW );
    error = strarrayWtoA( errorW );

    *referrals = strarrayWtoA( referralsW );
    *serverctrls = controlarrayWtoA( serverctrlsW );

    ldap_value_freeW( matchedW );
    ldap_value_freeW( errorW );
    ldap_value_freeW( referralsW );
    ldap_controls_freeW( serverctrlsW );

#endif
    return ret;
}

ULONG ldap_parse_resultW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *result,
    ULONG *retcode, PWCHAR *matched, PWCHAR *error, PWCHAR **referrals,
    PLDAPControlW **serverctrls, BOOLEAN free )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char **matchedU = NULL, **errorU = NULL, **referralsU = NULL;
    LDAPControl **serverctrlsU = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p, %p, 0x%02x)\n", ld, result, retcode,
           matched, error, referrals, serverctrls, free );

    if (!ld) return ~0UL;

    ret = ldap_parse_result( ld, result, (int *)retcode, matchedU, errorU,
                             &referralsU, &serverctrlsU, free );

    matched = strarrayUtoW( matchedU );
    error = strarrayUtoW( errorU );

    *referrals = strarrayUtoW( referralsU );
    *serverctrls = controlarrayUtoW( serverctrlsU );

    ldap_memfree( matchedU );
    ldap_memfree( errorU );
    ldap_memfree( referralsU );
    ldap_controls_free( serverctrlsU );

#endif
    return ret;
}

ULONG ldap_parse_sort_controlA( WLDAP32_LDAP *ld, PLDAPControlA *control,
    ULONG *result, PCHAR *attr )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *attrW = NULL;
    LDAPControlW **controlW = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, control, result, attr );

    if (!ld) return ~0UL;

    if (control) {
        controlW = controlarrayAtoW( control );
        if (!controlW) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_parse_sort_controlW( ld, controlW, result, &attrW );

    *attr = strWtoA( attrW );
    controlarrayfreeW( controlW );

#endif
    return ret;
}

ULONG ldap_parse_sort_controlW( WLDAP32_LDAP *ld, PLDAPControlW *control,
    ULONG *result, PWCHAR *attr )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *attrU = NULL;
    LDAPControl **controlU = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, control, result, attr );

    if (!ld) return ~0UL;

    if (control) {
        controlU = controlarrayWtoU( control );
        if (!controlU) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_parse_sort_control( ld, controlU, result, &attrU );

    *attr = strUtoW( attrU );
    controlarrayfreeU( controlU );

#endif
    return ret;
}

INT ldap_parse_vlv_controlA( WLDAP32_LDAP *ld, PLDAPControlA *control,
    PULONG targetpos, PULONG listcount,
    struct WLDAP32_berval **context, PINT errcode )
{
    int ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW **controlW = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p)\n", ld, control, targetpos,
           listcount, context, errcode );

    if (!ld) return ~0UL;

    if (control) {
        controlW = controlarrayAtoW( control );
        if (!controlW) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_parse_vlv_controlW( ld, controlW, targetpos, listcount,
                                   context, errcode );

    controlarrayfreeW( controlW );

#endif
    return ret;
}

INT ldap_parse_vlv_controlW( WLDAP32_LDAP *ld, PLDAPControlW *control,
    PULONG targetpos, PULONG listcount,
    struct WLDAP32_berval **context, PINT errcode )
{
    int ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControl **controlU = NULL;

    TRACE( "(%p, %p, %p, %p, %p, %p)\n", ld, control, targetpos,
           listcount, context, errcode );

    if (!ld) return ~0UL;

    if (control) {
        controlU = controlarrayWtoU( control );
        if (!controlU) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_parse_vlv_control( ld, controlU, targetpos, listcount,
                                  (struct berval **)context, errcode );

    controlarrayfreeU( controlU );

#endif
    return ret;
}
