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

#include <stdarg.h>
#ifdef HAVE_LDAP_H
#include <ldap.h>
#endif
#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "rpc.h"

#include "winldap_private.h"
#include "wldap32.h"
#include "wine/debug.h"

#ifdef HAVE_LDAP
WINE_DEFAULT_DEBUG_CHANNEL(wldap32);
#endif

/***********************************************************************
 *      ldap_bindA     (WLDAP32.@)
 *
 * See ldap_bindW.
 */
ULONG CDECL ldap_bindA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR cred, ULONG method )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *credW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08x)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return ~0u;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (cred) {
        credW = strAtoW( cred );
        if (!credW) goto exit;
    }

    ret = ldap_bindW( ld, dnW, credW, method );

exit:
    strfreeW( dnW );
    strfreeW( credW );

#endif
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
ULONG CDECL ldap_bindW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *credU = NULL;
    struct berval pwd = { 0, NULL };
    int msg;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08x)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return ~0u;
    if (method != LDAP_AUTH_SIMPLE) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (cred) {
        credU = strWtoU( cred );
        if (!credU) goto exit;

        pwd.bv_len = strlen( credU );
        pwd.bv_val = credU;
    }

    ret = ldap_sasl_bind( ld->ld, dnU, LDAP_SASL_SIMPLE, &pwd, NULL, NULL, &msg );

    if (ret == LDAP_SUCCESS)
        ret = msg;
    else
        ret = ~0u;

exit:
    strfreeU( dnU );
    strfreeU( credU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_bind_sA     (WLDAP32.@)
 *
 * See ldap_bind_sW.
 */
ULONG CDECL ldap_bind_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR cred, ULONG method )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *credW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08x)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (cred) {
        if (method == LDAP_AUTH_SIMPLE)
        {
            credW = strAtoW( cred );
            if (!credW) goto exit;
        }
        else
            credW = (WCHAR *)cred /* SEC_WINNT_AUTH_IDENTITY_A */;
    }

    ret = ldap_bind_sW( ld, dnW, credW, method );

exit:
    strfreeW( dnW );
    if (credW != (WCHAR *)cred) strfreeW( credW );

#endif
    return ret;
}

#ifdef HAVE_LDAP

static int sasl_interact( LDAP *ld, unsigned flags, void *defaults, void *interact )
{
#ifdef HAVE_SASL_SASL_H
    SEC_WINNT_AUTH_IDENTITY_A *id = defaults;
    sasl_interact_t *sasl = interact;

    TRACE( "%p,%08x,%p,%p\n", ld, flags, defaults, interact );

    while (sasl->id != SASL_CB_LIST_END)
    {
        TRACE("sasl->id = %04lx\n", sasl->id);

        if (sasl->id == SASL_CB_GETREALM)
        {
            sasl->result = id->Domain;
            sasl->len = id->DomainLength;
        }
        else if (sasl->id == SASL_CB_USER)
        {
            sasl->result = id->User;
            sasl->len = id->UserLength;
        }
        else if (sasl->id == SASL_CB_PASS)
        {
            sasl->result = id->Password;
            sasl->len = id->PasswordLength;
        }

        sasl++;
    }

    return LDAP_SUCCESS;
#else
    FIXME( "%p,%08x,%p,%p: stub\n", ld, flags, defaults, interact );
    return LDAP_SUCCESS;
#endif /* HAVE_SASL_SASL_H */
}

#endif /* HAVE_LDAP */

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
ULONG CDECL ldap_bind_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *credU = NULL;
    struct berval pwd = { 0, NULL };

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08x)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (method == LDAP_AUTH_SIMPLE)
    {
        if (dn)
        {
            dnU = strWtoU( dn );
            if (!dnU) goto exit;
        }
        if (cred)
        {
            credU = strWtoU( cred );
            if (!credU) goto exit;

            pwd.bv_len = strlen( credU );
            pwd.bv_val = credU;
        }

        ret = map_error( ldap_sasl_bind_s( ld->ld, dnU, LDAP_SASL_SIMPLE, &pwd, NULL, NULL, NULL ));
    }
    else if (method == WLDAP32_LDAP_AUTH_NEGOTIATE)
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

        ret = map_error( ldap_sasl_interactive_bind_s( ld->ld,
                             NULL /* server will ignore DN anyway */,
                             NULL /* query supportedSASLMechanisms */,
                             NULL, NULL, LDAP_SASL_QUIET, sasl_interact, &idU ));

        if (id && (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
        {
            strfreeW( (WCHAR *)idW.Domain );
            strfreeW( (WCHAR *)idW.User );
            strfreeW( (WCHAR *)idW.Password );
        }

        strfreeU( (char *)idU.Domain );
        strfreeU( (char *)idU.User );
        strfreeU( (char *)idU.Password );
    }
    else
    {
        FIXME( "method %#x not supported\n", method );
        return WLDAP32_LDAP_PARAM_ERROR;
    }

exit:
    strfreeU( dnU );
    strfreeU( credU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bindA     (WLDAP32.@)
 *
 * See ldap_sasl_bindW.
 */
ULONG CDECL ldap_sasl_bindA( WLDAP32_LDAP *ld, const PCHAR dn,
    const PCHAR mechanism, const BERVAL *cred, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, int *message )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnW = strAtoW( dn );
    if (!dnW) goto exit;

    mechanismW = strAtoW( mechanism );
    if (!mechanismW) goto exit;

    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_sasl_bindW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, message );

exit:
    strfreeW( dnW );
    strfreeW( mechanismW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
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
ULONG CDECL ldap_sasl_bindW( WLDAP32_LDAP *ld, const PWCHAR dn,
    const PWCHAR mechanism, const BERVAL *cred, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, int *message )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *mechanismU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval credU;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnU = strWtoU( dn );
    if (!dnU) goto exit;

    mechanismU = strWtoU( mechanism );
    if (!mechanismU) goto exit;

    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    credU.bv_len = cred->bv_len;
    credU.bv_val = cred->bv_val;

    ret = map_error( ldap_sasl_bind( ld->ld, dnU, mechanismU, &credU,
                                     serverctrlsU, clientctrlsU, message ));

exit:
    strfreeU( dnU );
    strfreeU( mechanismU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bind_sA     (WLDAP32.@)
 *
 * See ldap_sasl_bind_sW.
 */
ULONG CDECL ldap_sasl_bind_sA( WLDAP32_LDAP *ld, const PCHAR dn,
    const PCHAR mechanism, const BERVAL *cred, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, PBERVAL *serverdata )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnW = strAtoW( dn );
    if (!dnW) goto exit;

    mechanismW = strAtoW( mechanism );
    if (!mechanismW) goto exit;

    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_sasl_bind_sW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, serverdata );

exit:
    strfreeW( dnW );
    strfreeW( mechanismW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
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
ULONG CDECL ldap_sasl_bind_sW( WLDAP32_LDAP *ld, const PWCHAR dn,
    const PWCHAR mechanism, const BERVAL *cred, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, PBERVAL *serverdata )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *mechanismU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval credU;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnU = strWtoU( dn );
    if (!dnU) goto exit;

    mechanismU = strWtoU( mechanism );
    if (!mechanismU) goto exit;

    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    credU.bv_len = cred->bv_len;
    credU.bv_val = cred->bv_val;

    ret = map_error( ldap_sasl_bind_s( ld->ld, dnU, mechanismU, &credU,
                                       serverctrlsU, clientctrlsU, (struct berval **)serverdata ));

exit:
    strfreeU( dnU );
    strfreeU( mechanismU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_simple_bindA     (WLDAP32.@)
 *
 * See ldap_simple_bindW.
 */
ULONG CDECL ldap_simple_bindA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR passwd )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *passwdW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return ~0u;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (passwd) {
        passwdW = strAtoW( passwd );
        if (!passwdW) goto exit;
    }

    ret = ldap_simple_bindW( ld, dnW, passwdW );

exit:
    strfreeW( dnW );
    strfreeW( passwdW );

#endif
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
ULONG CDECL ldap_simple_bindW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR passwd )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *passwdU = NULL;
    struct berval pwd = { 0, NULL };
    int msg;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return ~0u;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (passwd) {
        passwdU = strWtoU( passwd );
        if (!passwdU) goto exit;

        pwd.bv_len = strlen( passwdU );
        pwd.bv_val = passwdU;
    }

    ret = ldap_sasl_bind( ld->ld, dnU, LDAP_SASL_SIMPLE, &pwd, NULL, NULL, &msg );

    if (ret == LDAP_SUCCESS)
        ret = msg;
    else
        ret = ~0u;

exit:
    strfreeU( dnU );
    strfreeU( passwdU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_simple_bind_sA     (WLDAP32.@)
 *
 * See ldap_simple_bind_sW.
 */
ULONG CDECL ldap_simple_bind_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR passwd )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *passwdW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (passwd) {
        passwdW = strAtoW( passwd );
        if (!passwdW) goto exit;
    }

    ret = ldap_simple_bind_sW( ld, dnW, passwdW );

exit:
    strfreeW( dnW );
    strfreeW( passwdW );

#endif
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
ULONG CDECL ldap_simple_bind_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR passwd )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *passwdU = NULL;
    struct berval pwd = { 0, NULL };

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (passwd) {
        passwdU = strWtoU( passwd );
        if (!passwdU) goto exit;

        pwd.bv_len = strlen( passwdU );
        pwd.bv_val = passwdU;
    }

    ret = map_error( ldap_sasl_bind_s( ld->ld, dnU, LDAP_SASL_SIMPLE, &pwd, NULL, NULL, NULL ));

exit:
    strfreeU( dnU );
    strfreeU( passwdU );

#endif
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
ULONG CDECL WLDAP32_ldap_unbind( WLDAP32_LDAP *ld )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", ld );

    if (ld)
    {
        ret = map_error( ldap_unbind_ext( ld->ld, NULL, NULL ));
        if ( ld->ld_server_ctrls )
            ldap_value_free_len( ld->ld_server_ctrls );
        heap_free( ld );
    }
    else
        ret = WLDAP32_LDAP_PARAM_ERROR;

#endif
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
ULONG CDECL WLDAP32_ldap_unbind_s( WLDAP32_LDAP *ld )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", ld );

    if (ld)
    {
        ret = map_error( ldap_unbind_ext_s( ld->ld, NULL, NULL ));
        if ( ld->ld_server_ctrls )
            ldap_value_free_len( ld->ld_server_ctrls );
        heap_free( ld );
    }
    else
        ret = WLDAP32_LDAP_PARAM_ERROR;

#endif
    return ret;
}
