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
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "rpc.h"
#include "winldap.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_bindA     (WLDAP32.@)
 *
 * See ldap_bindW.
 */
ULONG CDECL ldap_bindA( LDAP *ld, char *dn, char *cred, ULONG method )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *credW = NULL;

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (cred && !(credW = strAtoW( cred ))) goto exit;

    ret = ldap_bindW( ld, dnW, credW, method );

exit:
    free( dnW );
    free( credW );
    return ret;
}

/***********************************************************************
 *      ldap_bindW     (WLDAP32.@)
 *
 * Authenticate with an LDAP server (asynchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of entry to bind as.
 *  cred    [I] Credentials (e.g. password string).
 *  method  [I] Authentication method.
 *
 * RETURNS
 *  Success: Message ID of the bind operation.
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Only LDAP_AUTH_SIMPLE is supported (just like native).
 */
ULONG CDECL ldap_bindW( LDAP *ld, WCHAR *dn, WCHAR *cred, ULONG method )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *credU = NULL;
    struct bervalU pwd = { 0, NULL };
    int msg;

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return ~0u;
    if (method != LDAP_AUTH_SIMPLE) return LDAP_PARAM_ERROR;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (cred)
    {
        if (!(credU = strWtoU( cred ))) goto exit;
        pwd.bv_len = strlen( credU );
        pwd.bv_val = credU;
    }

    {
        struct ldap_sasl_bind_params params = { CTX(ld), dnU, 0, &pwd, NULL, NULL, &msg };
        ret = map_error( LDAP_CALL( ldap_sasl_bind, &params ));
    }
    if (ret == LDAP_SUCCESS)
        ret = msg;
    else
        ret = ~0u;

exit:
    free( dnU );
    free( credU );
    return ret;
}

/***********************************************************************
 *      ldap_bind_sA     (WLDAP32.@)
 *
 * See ldap_bind_sW.
 */
ULONG CDECL ldap_bind_sA( LDAP *ld, char *dn, char *cred, ULONG method )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *credW = NULL;

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (cred)
    {
        if (method == LDAP_AUTH_SIMPLE)
        {
            if (!(credW = strAtoW( cred ))) goto exit;
        }
        else credW = (WCHAR *)cred /* SEC_WINNT_AUTH_IDENTITY_A */;
    }

    ret = ldap_bind_sW( ld, dnW, credW, method );

exit:
    free( dnW );
    if (credW != (WCHAR *)cred) free( credW );
    return ret;
}

/***********************************************************************
 *      ldap_bind_sW     (WLDAP32.@)
 *
 * Authenticate with an LDAP server (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of entry to bind as.
 *  cred    [I] Credentials (e.g. password string).
 *  method  [I] Authentication method.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_bind_sW( LDAP *ld, WCHAR *dn, WCHAR *cred, ULONG method )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *credU = NULL;
    struct bervalU pwd = { 0, NULL };

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return LDAP_PARAM_ERROR;

    if (method == LDAP_AUTH_SIMPLE)
    {
        if (dn && !(dnU = strWtoU( dn ))) goto exit;
        if (cred)
        {
            if (!(credU = strWtoU( cred ))) goto exit;
            pwd.bv_len = strlen( credU );
            pwd.bv_val = credU;
        }

        {
            struct ldap_sasl_bind_s_params params = { CTX(ld), dnU, 0, &pwd, NULL, NULL, NULL };
            ret = map_error( LDAP_CALL( ldap_sasl_bind_s, &params ));
        }
    }
    else if (method == LDAP_AUTH_NEGOTIATE)
    {
        SEC_WINNT_AUTH_IDENTITY_A idU;
        SEC_WINNT_AUTH_IDENTITY_W idW;
        SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)cred;

        memset( &idU, 0, sizeof(idU) );
        if (id)
        {
            if (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
            {
                idW.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                idW.Domain = (unsigned short *)strnAtoW( (char *)id->Domain, id->DomainLength, &idW.DomainLength );
                idW.User = (unsigned short *)strnAtoW( (char *)id->User, id->UserLength, &idW.UserLength );
                idW.Password = (unsigned short *)strnAtoW( (char *)id->Password, id->PasswordLength, &idW.PasswordLength );
                id = &idW;
            }
            idU.Domain = (unsigned char *)strnWtoU( id->Domain, id->DomainLength, &idU.DomainLength );
            idU.User = (unsigned char *)strnWtoU( id->User, id->UserLength, &idU.UserLength );
            idU.Password = (unsigned char *)strnWtoU( id->Password, id->PasswordLength, &idU.PasswordLength );
        }

        {
            struct ldap_sasl_interactive_bind_s_params params = { CTX(ld),
                    NULL /* server will ignore DN anyway */,
                    NULL /* query supportedSASLMechanisms */,
                    NULL, NULL, 2 /* LDAP_SASL_QUIET */, &idU };
            ret = map_error( LDAP_CALL( ldap_sasl_interactive_bind_s, &params ));
        }

        if (id && (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
        {
            free( (WCHAR *)idW.Domain );
            free( (WCHAR *)idW.User );
            free( (WCHAR *)idW.Password );
        }

        free( (char *)idU.Domain );
        free( (char *)idU.User );
        free( (char *)idU.Password );
    }
    else
    {
        FIXME( "method %#lx not supported\n", method );
        return LDAP_PARAM_ERROR;
    }

exit:
    free( dnU );
    free( credU );
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bindA     (WLDAP32.@)
 *
 * See ldap_sasl_bindW.
 */
ULONG CDECL ldap_sasl_bindA( LDAP *ld, const PCHAR dn, const PCHAR mechanism, const BERVAL *cred,
    LDAPControlA **serverctrls, LDAPControlA **clientctrls, int *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message) return LDAP_PARAM_ERROR;

    if (!(dnW = strAtoW( dn ))) goto exit;
    if (!(mechanismW = strAtoW( mechanism ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_sasl_bindW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, message );

exit:
    free( dnW );
    free( mechanismW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bindW     (WLDAP32.@)
 *
 * Authenticate with an LDAP server using SASL (asynchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of entry to bind as.
 *  mechanism   [I] Authentication method.
 *  cred        [I] Credentials.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  message     [O] Message ID of the bind operation.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  The serverctrls and clientctrls parameters are optional and should
 *  be set to NULL if not used.
 */
ULONG CDECL ldap_sasl_bindW( LDAP *ld, const PWCHAR dn, const PWCHAR mechanism, const BERVAL *cred,
    LDAPControlW **serverctrls, LDAPControlW **clientctrls, int *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU, *mechanismU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct bervalU credU;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message) return LDAP_PARAM_ERROR;

    if (!(dnU = strWtoU( dn ))) goto exit;
    if (!(mechanismU = strWtoU( mechanism ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        struct ldap_sasl_bind_params params = { CTX(ld), dnU, mechanismU, &credU, serverctrlsU, clientctrlsU, message };
        credU.bv_len = cred->bv_len;
        credU.bv_val = cred->bv_val;
        ret = map_error( LDAP_CALL( ldap_sasl_bind, &params ));
    }

exit:
    free( dnU );
    free( mechanismU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bind_sA     (WLDAP32.@)
 *
 * See ldap_sasl_bind_sW.
 */
ULONG CDECL ldap_sasl_bind_sA( LDAP *ld, const PCHAR dn, const PCHAR mechanism, const BERVAL *cred,
    LDAPControlA **serverctrls, LDAPControlA **clientctrls, BERVAL **serverdata )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata) return LDAP_PARAM_ERROR;

    if (!(dnW = strAtoW( dn ))) goto exit;
    if (!(mechanismW = strAtoW( mechanism ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_sasl_bind_sW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, serverdata );

exit:
    free( dnW );
    free( mechanismW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bind_sW     (WLDAP32.@)
 *
 * Authenticate with an LDAP server using SASL (synchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of entry to bind as.
 *  mechanism   [I] Authentication method.
 *  cred        [I] Credentials.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  serverdata  [O] Authentication response from the server.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  The serverctrls and clientctrls parameters are optional and should
 *  be set to NULL if not used.
 */
ULONG CDECL ldap_sasl_bind_sW( LDAP *ld, const PWCHAR dn, const PWCHAR mechanism, const BERVAL *cred,
    LDAPControlW **serverctrls, LDAPControlW **clientctrls, BERVAL **serverdata )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU, *mechanismU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct bervalU *dataU, credU;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata) return LDAP_PARAM_ERROR;

    if (!(dnU = strWtoU( dn ))) goto exit;
    if (!(mechanismU = strWtoU( mechanism ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;

    credU.bv_len = cred->bv_len;
    credU.bv_val = cred->bv_val;

    {
        struct ldap_sasl_bind_s_params params = { CTX(ld), dnU, mechanismU, &credU, serverctrlsU, clientctrlsU, &dataU };
        ret = map_error( LDAP_CALL( ldap_sasl_bind_s, &params ));
    }
    if (ret == LDAP_SUCCESS)
    {
        BERVAL *ptr;
        if (!(ptr = bervalUtoW( dataU ))) ret = LDAP_NO_MEMORY;
        else *serverdata = ptr;
        LDAP_CALL( ber_bvfree, dataU );
    }

exit:
    free( dnU );
    free( mechanismU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_simple_bindA     (WLDAP32.@)
 *
 * See ldap_simple_bindW.
 */
ULONG CDECL ldap_simple_bindA( LDAP *ld, char *dn, char *passwd )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *passwdW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (passwd && !(passwdW = strAtoW( passwd ))) goto exit;

    ret = ldap_simple_bindW( ld, dnW, passwdW );

exit:
    free( dnW );
    free( passwdW );
    return ret;
}

/***********************************************************************
 *      ldap_simple_bindW     (WLDAP32.@)
 *
 * Authenticate with an LDAP server (asynchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of entry to bind as.
 *  passwd  [I] Password string.
 *
 * RETURNS
 *  Success: Message ID of the bind operation.
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Set dn and passwd to NULL to bind as an anonymous user.
 */
ULONG CDECL ldap_simple_bindW( LDAP *ld, WCHAR *dn, WCHAR *passwd )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *passwdU = NULL;
    struct bervalU pwd = { 0, NULL };
    int msg;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return ~0u;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (passwd)
    {
        if (!(passwdU = strWtoU( passwd ))) goto exit;
        pwd.bv_len = strlen( passwdU );
        pwd.bv_val = passwdU;
    }

    {
        struct ldap_sasl_bind_params params = { CTX(ld), dnU, 0, &pwd, NULL, NULL, &msg };
        ret = map_error( LDAP_CALL( ldap_sasl_bind, &params ));
    }
    if (ret == LDAP_SUCCESS)
        ret = msg;
    else
        ret = ~0u;

exit:
    free( dnU );
    free( passwdU );
    return ret;
}

/***********************************************************************
 *      ldap_simple_bind_sA     (WLDAP32.@)
 *
 * See ldap_simple_bind_sW.
 */
ULONG CDECL ldap_simple_bind_sA( LDAP *ld, char *dn, char *passwd )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *passwdW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (passwd && !(passwdW = strAtoW( passwd ))) goto exit;

    ret = ldap_simple_bind_sW( ld, dnW, passwdW );

exit:
    free( dnW );
    free( passwdW );
    return ret;
}

/***********************************************************************
 *      ldap_simple_bind_sW     (WLDAP32.@)
 *
 * Authenticate with an LDAP server (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of entry to bind as.
 *  passwd  [I] Password string.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Set dn and passwd to NULL to bind as an anonymous user.
 */
ULONG CDECL ldap_simple_bind_sW( LDAP *ld, WCHAR *dn, WCHAR *passwd )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *passwdU = NULL;
    struct bervalU pwd = { 0, NULL };

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (passwd)
    {
        if (!(passwdU = strWtoU( passwd ))) goto exit;
        pwd.bv_len = strlen( passwdU );
        pwd.bv_val = passwdU;
    }

    {
        struct ldap_sasl_bind_s_params params = { CTX(ld), dnU, 0, &pwd, NULL, NULL, NULL };
        ret = map_error( LDAP_CALL( ldap_sasl_bind_s, &params ));
    }

exit:
    free( dnU );
    free( passwdU );
    return ret;
}

/***********************************************************************
 *      ldap_unbind     (WLDAP32.@)
 *
 * Close LDAP connection and free resources (asynchronous operation).
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_unbind( LDAP *ld )
{
    ULONG ret;

    TRACE( "(%p)\n", ld );

    if (ld)
    {
        struct ldap_unbind_ext_params params = { CTX(ld), NULL, NULL };
        ret = map_error( LDAP_CALL( ldap_unbind_ext, &params ));
    }
    else return LDAP_PARAM_ERROR;

    if (SERVER_CTRLS(ld)) LDAP_CALL( ldap_value_free_len, SERVER_CTRLS(ld) );

    free( ld );
    return ret;
}

/***********************************************************************
 *      ldap_unbind_s     (WLDAP32.@)
 *
 * Close LDAP connection and free resources (synchronous operation).
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_unbind_s( LDAP *ld )
{
    ULONG ret;

    TRACE( "(%p)\n", ld );

    if (ld)
    {
        struct ldap_unbind_ext_s_params params = { CTX(ld), NULL, NULL };
        ret = map_error( LDAP_CALL( ldap_unbind_ext_s, &params ));
    }
    else return LDAP_PARAM_ERROR;

    if (SERVER_CTRLS(ld)) LDAP_CALL( ldap_value_free_len, SERVER_CTRLS(ld) );

    free( ld );
    return ret;
}
