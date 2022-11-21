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
#include <limits.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

static struct WLDAP32_berval null_cookieW = { 0, NULL };

/***********************************************************************
 *      ldap_create_page_controlA     (WLDAP32.@)
 */
ULONG CDECL ldap_create_page_controlA( LDAP *ld, ULONG pagesize, struct WLDAP32_berval *cookie, UCHAR critical,
                                       LDAPControlA **control )
{
    ULONG ret;
    LDAPControlW *controlW = NULL;

    TRACE( "(%p, %#lx, %p, 0x%02x, %p)\n", ld, pagesize, cookie, critical, control );

    if (!ld || !control || pagesize > INT_MAX) return WLDAP32_LDAP_PARAM_ERROR;

    ret = ldap_create_page_controlW( ld, pagesize, cookie, critical, &controlW );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        *control = controlWtoA( controlW );
        ldap_control_freeW( controlW );
    }
    return ret;
}

/* create a page control by hand */
static ULONG create_page_control( ULONG pagesize, struct WLDAP32_berval *cookie, UCHAR critical,
                                  LDAPControlW **control )
{
    LDAPControlW *ctrl;
    WLDAP32_BerElement *ber;
    struct WLDAP32_berval *berval, *vec[2];
    int ret, len;
    char *val;

    if (!(ber = WLDAP32_ber_alloc_t( WLDAP32_LBER_USE_DER ))) return WLDAP32_LDAP_NO_MEMORY;

    vec[1] = NULL;
    if (cookie)
        vec[0] = cookie;
    else
        vec[0] = &null_cookieW;
    len = WLDAP32_ber_printf( ber, (char *)"{iV}", pagesize, vec );

    ret = WLDAP32_ber_flatten( ber, &berval );
    WLDAP32_ber_free( ber, 1 );

    if (len == WLDAP32_LBER_ERROR) return WLDAP32_LDAP_ENCODING_ERROR;
    if (ret == -1) return WLDAP32_LDAP_NO_MEMORY;

    /* copy the berval so it can be properly freed by the caller */
    if (!(val = malloc( berval->bv_len ))) return WLDAP32_LDAP_NO_MEMORY;

    len = berval->bv_len;
    memcpy( val, berval->bv_val, len );
    WLDAP32_ber_bvfree( berval );

    if (!(ctrl = malloc( sizeof(*ctrl) )))
    {
        free( val );
        return WLDAP32_LDAP_NO_MEMORY;
    }
    if (!(ctrl->ldctl_oid = strAtoW( LDAP_PAGED_RESULT_OID_STRING )))
    {
        free( val );
        free( ctrl );
        return WLDAP32_LDAP_NO_MEMORY;
    }
    ctrl->ldctl_value.bv_len = len;
    ctrl->ldctl_value.bv_val = val;
    ctrl->ldctl_iscritical   = critical;

    *control = ctrl;
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_create_page_controlW     (WLDAP32.@)
 */
ULONG CDECL ldap_create_page_controlW( LDAP *ld, ULONG pagesize, struct WLDAP32_berval *cookie, UCHAR critical,
                                       LDAPControlW **control )
{
    TRACE( "(%p, %#lx, %p, 0x%02x, %p)\n", ld, pagesize, cookie, critical, control );

    if (!ld || !control || pagesize > INT_MAX) return WLDAP32_LDAP_PARAM_ERROR;
    return create_page_control( pagesize, cookie, critical, control );
}

ULONG CDECL ldap_get_next_page( LDAP *ld, LDAPSearch *search, ULONG pagesize, ULONG *message )
{
    FIXME( "(%p, %p, %#lx, %p)\n", ld, search, pagesize, message );

    if (!ld) return ~0u;
    return WLDAP32_LDAP_NOT_SUPPORTED;
}

ULONG CDECL ldap_get_next_page_s( LDAP *ld, LDAPSearch *search, struct l_timeval *timeout, ULONG pagesize,
                                  ULONG *count, WLDAP32_LDAPMessage **results )
{
    ULONG ret;

    TRACE( "(%p, %p, %p, %lu, %p, %p)\n", ld, search, timeout, pagesize, count, results );

    if (!ld || !search || !count || !results) return ~0u;

    if (search->cookie && !search->cookie->bv_len)
    {
        /* end of paged results */
        *count = 0;
        *results = NULL;
        return WLDAP32_LDAP_NO_RESULTS_RETURNED;
    }

    if (search->serverctrls[0])
    {
        controlfreeW( search->serverctrls[0] );
        search->serverctrls[0] = NULL;
    }

    TRACE("search->cookie: %s\n", search->cookie ? debugstr_an(search->cookie->bv_val, search->cookie->bv_len) : "NULL");
    ret = ldap_create_page_controlW( ld, pagesize, search->cookie, 1, &search->serverctrls[0] );
    if (ret != WLDAP32_LDAP_SUCCESS) return ret;

    ret = ldap_search_ext_sW( ld, search->dn, search->scope, search->filter, search->attrs, search->attrsonly,
                              search->serverctrls, search->clientctrls,
                              search->timeout.tv_sec ? &search->timeout : NULL, search->sizelimit, results );
    if (ret != WLDAP32_LDAP_SUCCESS) return ret;

    return ldap_get_paged_count( ld, search, count, *results );
}

ULONG CDECL ldap_get_paged_count( LDAP *ld, LDAPSearch *search, ULONG *count, WLDAP32_LDAPMessage *results )
{
    ULONG ret;
    LDAPControlW **server_ctrls = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, search, count, results );

    if (!ld || !count || !results) return WLDAP32_LDAP_PARAM_ERROR;

    *count = 0;

    ret = ldap_parse_resultW( ld, results, NULL, NULL, NULL, NULL, &server_ctrls, 0 );
    if (ret != WLDAP32_LDAP_SUCCESS) return ret;

    if (!server_ctrls) /* assume end of paged results */
    {
        search->cookie = &null_cookieW;
        return WLDAP32_LDAP_SUCCESS;
    }

    free( search->cookie );
    search->cookie = NULL;

    ret = ldap_parse_page_controlW( ld, server_ctrls, count, &search->cookie );
    if (ret == WLDAP32_LDAP_SUCCESS)
        TRACE( "new search->cookie: %s, count %lu\n",
               debugstr_an(search->cookie->bv_val, search->cookie->bv_len), *count );

    ldap_controls_freeW( server_ctrls );
    return ret;
}

/***********************************************************************
 *      ldap_parse_page_controlA      (WLDAP32.@)
 */
ULONG CDECL ldap_parse_page_controlA( LDAP *ld, LDAPControlA **ctrls, ULONG *count, struct WLDAP32_berval **cookie )
{
    ULONG ret;
    LDAPControlW **ctrlsW = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, ctrls, count, cookie );

    if (!ld || !ctrls || !count || !cookie) return WLDAP32_LDAP_PARAM_ERROR;

    if (!(ctrlsW = controlarrayAtoW( ctrls ))) return WLDAP32_LDAP_NO_MEMORY;
    ret = ldap_parse_page_controlW( ld, ctrlsW, count, cookie );
    controlarrayfreeW( ctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_parse_page_controlW      (WLDAP32.@)
 */
ULONG CDECL ldap_parse_page_controlW( LDAP *ld, LDAPControlW **ctrls, ULONG *ret_count,
                                      struct WLDAP32_berval **ret_cookie )
{
    ULONG ret, count;
    LDAPControlW *control = NULL;
    WLDAP32_BerElement *ber;
    struct WLDAP32_berval *cookie = NULL;
    int tag;
    ULONG i;

    TRACE( "(%p, %p, %p, %p)\n", ld, ctrls, ret_count, ret_cookie );

    if (!ld || !ctrls || !ret_count || !ret_cookie) return WLDAP32_LDAP_PARAM_ERROR;

    for (i = 0; ctrls[i]; i++)
    {
        if (!wcscmp( LDAP_PAGED_RESULT_OID_STRING_W, ctrls[i]->ldctl_oid ))
            control = ctrls[i];
    }
    if (!control) return WLDAP32_LDAP_CONTROL_NOT_FOUND;

    if (!(ber = WLDAP32_ber_init( &control->ldctl_value ))) return WLDAP32_LDAP_NO_MEMORY;

    tag = WLDAP32_ber_scanf( ber, (char *)"{iO}", &count, &cookie );
    if (tag == WLDAP32_LBER_ERROR) ret = WLDAP32_LDAP_DECODING_ERROR;
    else
    {
        *ret_count = count;
        *ret_cookie = cookie;
        ret = WLDAP32_LDAP_SUCCESS;
    }

    WLDAP32_ber_free( ber, 1 );
    return ret;
}

ULONG CDECL ldap_search_abandon_page( LDAP *ld, LDAPSearch *search )
{
    LDAPControlW **ctrls;

    TRACE( "(%p, %p)\n", ld, search );

    if (!ld || !search) return ~0u;

    free( search->dn );
    free( search->filter );
    strarrayfreeW( search->attrs );
    ctrls = search->serverctrls;
    controlfreeW( ctrls[0] ); /* page control */
    ctrls++;
    while (*ctrls) controlfreeW( *ctrls++ );
    free( search->serverctrls );
    controlarrayfreeW( search->clientctrls );
    if (search->cookie && search->cookie != &null_cookieW) free( search->cookie );
    free( search );

    return WLDAP32_LDAP_SUCCESS;
}

LDAPSearch * CDECL ldap_search_init_pageA( LDAP *ld, char *dn, ULONG scope, char *filter, char **attrs,
                                           ULONG attrsonly, LDAPControlA **serverctrls, LDAPControlA **clientctrls,
                                           ULONG timelimit, ULONG sizelimit, LDAPSortKeyA **sortkeys )
{
    FIXME( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p, %#lx, %#lx, %p)\n", ld, debugstr_a(dn), scope,
           debugstr_a(filter), attrs, attrsonly, serverctrls, clientctrls, timelimit, sizelimit, sortkeys );
    return NULL;
}

LDAPSearch * CDECL ldap_search_init_pageW( LDAP *ld, WCHAR *dn, ULONG scope, WCHAR *filter, WCHAR **attrs,
                                           ULONG attrsonly, LDAPControlW **serverctrls, LDAPControlW **clientctrls,
                                           ULONG timelimit, ULONG sizelimit, LDAPSortKeyW **sortkeys )
{
    LDAPSearch *search;
    DWORD i, len;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p, %#lx, %#lx, %p)\n", ld, debugstr_w(dn), scope,
           debugstr_w(filter), attrs, attrsonly, serverctrls, clientctrls, timelimit, sizelimit, sortkeys );

    if (!(search = calloc( 1, sizeof(*search) )))
    {
        ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;
        return NULL;
    }

    if (dn && !(search->dn = wcsdup( dn ))) goto fail;
    if (filter && !(search->filter = wcsdup( filter ))) goto fail;
    if (attrs && !(search->attrs = strarraydupW( attrs ))) goto fail;

    len = serverctrls ? controlarraylenW( serverctrls ) : 0;
    if (!(search->serverctrls = malloc( sizeof(LDAPControlW *) * (len + 2) ))) goto fail;
    search->serverctrls[0] = NULL; /* reserve 0 for page control */
    for (i = 0; i < len; i++)
    {
        if (!(search->serverctrls[i + 1] = controldupW( serverctrls[i] )))
        {
            for (; i > 0; i--) controlfreeW( search->serverctrls[i] );
            goto fail;
        }
    }
    search->serverctrls[len + 1] = NULL;

    if (clientctrls && !(search->clientctrls = controlarraydupW( clientctrls )))
    {
        for (i = 0; i < len; i++) controlfreeW( search->serverctrls[i] );
        goto fail;
    }

    search->scope           = scope;
    search->attrsonly       = attrsonly;
    search->timeout.tv_sec  = timelimit;
    search->timeout.tv_usec = 0;
    search->sizelimit       = sizelimit;
    search->cookie          = NULL;
    return search;

fail:
    ldap_search_abandon_page( ld, search );
    ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;
    return NULL;
}
