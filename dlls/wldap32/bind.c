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

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_bindA     (WLDAP32.@)
 */
ULONG CDECL ldap_bindA( LDAP *ld, char *dn, char *cred, ULONG method )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
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
 */
ULONG CDECL ldap_bindW( LDAP *ld, WCHAR *dn, WCHAR *cred, ULONG method )
{
    ULONG ret;
    char *dnU = NULL, *credU = NULL;
    struct berval pwd = { 0, NULL };
    int msg;

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return ~0u;
    if (method != WLDAP32_LDAP_AUTH_SIMPLE) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (cred)
    {
        if (!(credU = strWtoU( cred ))) goto exit;
        pwd.bv_len = strlen( credU );
        pwd.bv_val = credU;
    }

    ret = map_error( ldap_sasl_bind( CTX(ld), dnU, 0, &pwd, NULL, NULL, &msg ) );
    if (ret == WLDAP32_LDAP_SUCCESS)
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
 */
ULONG CDECL ldap_bind_sA( LDAP *ld, char *dn, char *cred, ULONG method )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *credW = NULL;

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (cred)
    {
        if (method == WLDAP32_LDAP_AUTH_SIMPLE)
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

#define SASL_CB_LIST_END    0
#define SASL_CB_AUTHNAME    0x4002
#define SASL_CB_PASS        0x4004
#define SASL_CB_GETREALM    0x4008

struct sasl_interact
{
    unsigned long id;
    const char *challenge;
    const char *prompt;
    const char *defresult;
    const void *result;
    unsigned int len;
};

static int interact_callback( LDAP *ld, unsigned flags, void *defaults, void *sasl_interact )
{
    SEC_WINNT_AUTH_IDENTITY_W *id = defaults;
    struct sasl_interact *ptr = sasl_interact;

    TRACE( "%p, %08x, %p, %p\n", ld, flags, defaults, sasl_interact );

    if (!defaults) return 0;

    while (ptr && ptr->id != SASL_CB_LIST_END)
    {
        switch (ptr->id)
        {
        case SASL_CB_AUTHNAME:
            ptr->result = id->User;
            ptr->len    = id->UserLength;
            break;
        case SASL_CB_GETREALM:
            ptr->result = id->Domain;
            ptr->len    = id->DomainLength;
            break;
        case SASL_CB_PASS:
            ptr->result = id->Password;
            ptr->len    = id->PasswordLength;
            break;
        default:
            ERR( "unexpected callback %#lx\n", ptr->id );
            return -1;
        }
        ptr++;
    }

    return 0;
}

/***********************************************************************
 *      ldap_bind_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_bind_sW( LDAP *ld, WCHAR *dn, WCHAR *cred, ULONG method )
{
    ULONG ret;
    char *dnU = NULL, *credU = NULL;
    struct berval pwd = { 0, NULL };

    TRACE( "(%p, %s, %p, %#lx)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (method == WLDAP32_LDAP_AUTH_SIMPLE)
    {
        if (dn && !(dnU = strWtoU( dn ))) goto exit;
        if (cred)
        {
            if (!(credU = strWtoU( cred ))) goto exit;
            pwd.bv_len = strlen( credU );
            pwd.bv_val = credU;
        }

        ret = map_error( ldap_sasl_bind_s( CTX(ld), dnU, 0, &pwd, NULL, NULL, NULL ) );
    }
    else if (method == WLDAP32_LDAP_AUTH_NEGOTIATE)
    {
        SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)cred, idW;

        if (id && (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
        {
            idW.User = (unsigned short *)strnAtoW( (char *)id->User, id->UserLength, &idW.UserLength );
            idW.Domain = (unsigned short *)strnAtoW( (char *)id->Domain, id->DomainLength, &idW.DomainLength );
            idW.Password = (unsigned short *)strnAtoW( (char *)id->Password, id->PasswordLength, &idW.PasswordLength );
            idW.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            id = &idW;
        }

        ret = map_error( ldap_sasl_interactive_bind_s( CTX(ld), NULL, NULL, NULL, NULL, LDAP_SASL_QUIET,
                                                       interact_callback, id ) );
        if (id && (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
        {
            free( idW.User );
            free( idW.Domain );
            free( idW.Password );
        }
    }
    else
    {
        FIXME( "method %#lx not supported\n", method );
        return WLDAP32_LDAP_PARAM_ERROR;
    }

exit:
    free( dnU );
    free( credU );
    return ret;
}

/***********************************************************************
 *      ldap_sasl_bindA     (WLDAP32.@)
 */
ULONG CDECL ldap_sasl_bindA( LDAP *ld, const PCHAR dn, const PCHAR mechanism, const BERVAL *cred,
                             LDAPControlA **serverctrls, LDAPControlA **clientctrls, int *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message) return WLDAP32_LDAP_PARAM_ERROR;

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
 */
ULONG CDECL ldap_sasl_bindW( LDAP *ld, const PWCHAR dn, const PWCHAR mechanism, const BERVAL *cred,
                             LDAPControlW **serverctrls, LDAPControlW **clientctrls, int *message )
{
    ULONG ret;
    char *dnU, *mechanismU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval credU;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = strWtoU( dn ))) goto exit;
    if (!(mechanismU = strWtoU( mechanism ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        credU.bv_len = cred->bv_len;
        credU.bv_val = cred->bv_val;
        ret = map_error( ldap_sasl_bind( CTX(ld), dnU, mechanismU, &credU, serverctrlsU, clientctrlsU, message) );
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
 */
ULONG CDECL ldap_sasl_bind_sA( LDAP *ld, const PCHAR dn, const PCHAR mechanism, const BERVAL *cred,
                               LDAPControlA **serverctrls, LDAPControlA **clientctrls, BERVAL **serverdata )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata) return WLDAP32_LDAP_PARAM_ERROR;

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
 */
ULONG CDECL ldap_sasl_bind_sW( LDAP *ld, const PWCHAR dn, const PWCHAR mechanism, const BERVAL *cred,
                               LDAPControlW **serverctrls, LDAPControlW **clientctrls, BERVAL **serverdata )
{
    ULONG ret;
    char *dnU, *mechanismU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval *dataU, credU;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = strWtoU( dn ))) goto exit;
    if (!(mechanismU = strWtoU( mechanism ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;

    credU.bv_len = cred->bv_len;
    credU.bv_val = cred->bv_val;
    ret = map_error( ldap_sasl_bind_s( CTX(ld), dnU, mechanismU, &credU, serverctrlsU, clientctrlsU, &dataU ) );

    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        BERVAL *ptr;
        if (!(ptr = bervalUtoW( dataU ))) ret = WLDAP32_LDAP_NO_MEMORY;
        else *serverdata = ptr;
        ber_bvfree( dataU );
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
 */
ULONG CDECL ldap_simple_bindA( LDAP *ld, char *dn, char *passwd )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
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
 */
ULONG CDECL ldap_simple_bindW( LDAP *ld, WCHAR *dn, WCHAR *passwd )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    char *dnU = NULL, *passwdU = NULL;
    struct berval pwd = { 0, NULL };
    int msg;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld || WLDAP32_ldap_connect( ld, NULL ) != WLDAP32_LDAP_SUCCESS) return ~0u;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (passwd)
    {
        if (!(passwdU = strWtoU( passwd ))) goto exit;
        pwd.bv_len = strlen( passwdU );
        pwd.bv_val = passwdU;
    }

    ret = map_error( ldap_sasl_bind( CTX(ld), dnU, 0, &pwd, NULL, NULL, &msg ) );
    if (ret == WLDAP32_LDAP_SUCCESS)
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
 */
ULONG CDECL ldap_simple_bind_sA( LDAP *ld, char *dn, char *passwd )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *passwdW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

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
 */
ULONG CDECL ldap_simple_bind_sW( LDAP *ld, WCHAR *dn, WCHAR *passwd )
{
    ULONG ret;
    char *dnU = NULL, *passwdU = NULL;
    struct berval pwd = { 0, NULL };

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (passwd)
    {
        if (!(passwdU = strWtoU( passwd ))) goto exit;
        pwd.bv_len = strlen( passwdU );
        pwd.bv_val = passwdU;
    }

    ret = map_error( ldap_sasl_bind_s( CTX(ld), dnU, 0, &pwd, NULL, NULL, NULL ) );

exit:
    free( dnU );
    free( passwdU );
    return ret;
}

/***********************************************************************
 *      ldap_unbind     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_unbind( LDAP *ld )
{
    ULONG ret;

    TRACE( "(%p)\n", ld );

    if (ld) ret = map_error( ldap_unbind_ext( CTX(ld), NULL, NULL ) );
    else return WLDAP32_LDAP_PARAM_ERROR;

    if (SERVER_CTRLS(ld)) ldap_value_free_len( SERVER_CTRLS(ld) );

    free( ld->ld_host );
    free( ld );
    return ret;
}

/***********************************************************************
 *      ldap_unbind_s     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_unbind_s( LDAP *ld )
{
    ULONG ret;

    TRACE( "(%p)\n", ld );

    if (ld) ret = map_error( ldap_unbind_ext_s( CTX(ld), NULL, NULL ) );
    else return WLDAP32_LDAP_PARAM_ERROR;

    if (SERVER_CTRLS(ld)) ldap_value_free_len( SERVER_CTRLS(ld) );

    free( ld );
    return ret;
}
