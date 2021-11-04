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
 *      ldap_modrdnA     (WLDAP32.@)
 *
 * See ldap_modrdnW.
 */
ULONG CDECL ldap_modrdnA( LDAP *ld, char *dn, char *newdn )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %s)\n", ld, debugstr_a(dn), debugstr_a(newdn) );

    if (!ld || !newdn) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdnW( ld, dnW, newdnW );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdnW     (WLDAP32.@)
 *
 * Change the RDN of a directory entry (asynchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to change.
 *  newdn   [I] New DN for the entry.
 *
 * RETURNS
 *  Success: Message ID of the modrdn operation.
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Call ldap_result with the message ID to get the result of
 *  the operation. Cancel the operation by calling ldap_abandon
 *  with the message ID.
 */
ULONG CDECL ldap_modrdnW( LDAP *ld, WCHAR *dn, WCHAR *newdn )
{
    TRACE( "(%p, %s, %s)\n", ld, debugstr_w(dn), debugstr_w(newdn) );
    return ldap_modrdn2W( ld, dn, newdn, 1 );
}

/***********************************************************************
 *      ldap_modrdn2A     (WLDAP32.@)
 *
 * See ldap_modrdn2W.
 */
ULONG CDECL ldap_modrdn2A( LDAP *ld, char *dn, char *newdn, int delete )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_a(dn), newdn, delete );

    if (!ld || !newdn) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdn2W( ld, dnW, newdnW, delete );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn2W     (WLDAP32.@)
 *
 * Change the RDN of a directory entry (asynchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to change.
 *  newdn   [I] New DN for the entry.
 *  delete  [I] Delete old DN?
 *
 * RETURNS
 *  Success: Message ID of the modrdn operation.
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Call ldap_result with the message ID to get the result of
 *  the operation. Cancel the operation by calling ldap_abandon
 *  with the message ID.
 */
ULONG CDECL ldap_modrdn2W( LDAP *ld, WCHAR *dn, WCHAR *newdn, int delete )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *newdnU = NULL;
    ULONG msg;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_w(dn), newdn, delete );

    if (!ld || !newdn) return ~0u;

    if (dn && !(dnU = strWtoU( dn ))) return LDAP_NO_MEMORY;

    if ((newdnU = strWtoU( newdn )))
    {
        struct ldap_rename_params params = { CTX(ld), dnU, newdnU, NULL, delete, NULL, NULL, &msg };
        ret = LDAP_CALL( ldap_rename, &params );
        if (ret == LDAP_SUCCESS)
            ret = msg;
        else
            ret = ~0u;
        free( newdnU );
    }
    free( dnU );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn2_sA     (WLDAP32.@)
 *
 * See ldap_modrdn2_sW.
 */
ULONG CDECL ldap_modrdn2_sA( LDAP *ld, char *dn, char *newdn, int delete )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_a(dn), newdn, delete );

    if (!ld || !newdn) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdn2_sW( ld, dnW, newdnW, delete );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn2_sW     (WLDAP32.@)
 *
 * Change the RDN of a directory entry (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to change.
 *  newdn   [I] New DN for the entry.
 *  delete  [I] Delete old DN?
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_modrdn2_sW( LDAP *ld, WCHAR *dn, WCHAR *newdn, int delete )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *newdnU = NULL;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_w(dn), newdn, delete );

    if (!ld || !newdn) return LDAP_PARAM_ERROR;

    if (dn && !(dnU = strWtoU( dn ))) return LDAP_NO_MEMORY;

    if ((newdnU = strWtoU( newdn )))
    {
        struct ldap_rename_s_params params = { CTX(ld), dnU, newdnU, NULL, delete, NULL, NULL };
        ret = map_error( LDAP_CALL( ldap_rename_s, &params ));
        free( newdnU );
    }
    free( dnU );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn_sA     (WLDAP32.@)
 *
 * See ldap_modrdn_sW.
 */
ULONG CDECL ldap_modrdn_sA( LDAP *ld, char *dn, char *newdn )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), newdn );

    if (!ld || !newdn) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdn_sW( ld, dnW, newdnW );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn_sW     (WLDAP32.@)
 *
 * Change the RDN of a directory entry (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to change.
 *  newdn   [I] New DN for the entry.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_modrdn_sW( LDAP *ld, WCHAR *dn, WCHAR *newdn )
{
    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), newdn );
    return ldap_modrdn2_sW( ld, dn, newdn, 1 );
}
