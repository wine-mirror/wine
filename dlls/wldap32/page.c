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
#ifndef LDAP_MAXINT
#define LDAP_MAXINT  2147483647
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "winldap_private.h"
#include "wldap32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

#ifdef HAVE_LDAP
static struct berval null_cookieU = { 0, NULL };
static struct WLDAP32_berval null_cookieW = { 0, NULL };
#endif

/***********************************************************************
 *      ldap_create_page_controlA     (WLDAP32.@)
 *
 * See ldap_create_page_controlW.
 */
ULONG CDECL ldap_create_page_controlA( WLDAP32_LDAP *ld, ULONG pagesize,
    struct WLDAP32_berval *cookie, UCHAR critical, PLDAPControlA *control )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW *controlW = NULL;

    TRACE( "(%p, 0x%08x, %p, 0x%02x, %p)\n", ld, pagesize, cookie,
           critical, control );

    if (!ld || !control || pagesize > LDAP_MAXINT)
        return WLDAP32_LDAP_PARAM_ERROR;

    ret = ldap_create_page_controlW( ld, pagesize, cookie, critical, &controlW );
    if (ret == LDAP_SUCCESS)
    {
        *control = controlWtoA( controlW );
        ldap_control_freeW( controlW );
    }

#endif
    return ret;
}

#ifdef HAVE_LDAP

/* create a page control by hand */
static ULONG create_page_control( ULONG pagesize, struct berval *cookie,
    UCHAR critical, PLDAPControlW *control )
{
    LDAPControlW *ctrl;
    BerElement *ber;
    ber_tag_t tag;
    struct berval *berval;
    INT ret, len;
    char *val;

    ber = ber_alloc_t( LBER_USE_DER );
    if (!ber) return WLDAP32_LDAP_NO_MEMORY;

    if (cookie)
        tag = ber_printf( ber, "{iO}", (ber_int_t)pagesize, cookie );
    else
        tag = ber_printf( ber, "{iO}", (ber_int_t)pagesize, &null_cookieU );

    ret = ber_flatten( ber, &berval );
    ber_free( ber, 1 );

    if (tag == LBER_ERROR)
        return WLDAP32_LDAP_ENCODING_ERROR;

    if (ret == -1)
        return WLDAP32_LDAP_NO_MEMORY;

    /* copy the berval so it can be properly freed by the caller */
    if (!(val = heap_alloc( berval->bv_len ))) return WLDAP32_LDAP_NO_MEMORY;

    len = berval->bv_len;
    memcpy( val, berval->bv_val, len );
    ber_bvfree( berval );

    if (!(ctrl = heap_alloc( sizeof(LDAPControlW) )))
    {
        heap_free( val );
        return WLDAP32_LDAP_NO_MEMORY;
    }

    ctrl->ldctl_oid = strAtoW( LDAP_PAGED_RESULT_OID_STRING );
    ctrl->ldctl_value.bv_len = len;
    ctrl->ldctl_value.bv_val = val;
    ctrl->ldctl_iscritical = critical;

    *control = ctrl;

    return WLDAP32_LDAP_SUCCESS;
}

#endif /* HAVE_LDAP */

/***********************************************************************
 *      ldap_create_page_controlW     (WLDAP32.@)
 *
 * Create a control for paged search results.
 *
 * PARAMS
 *  ld        [I] Pointer to an LDAP context.
 *  pagesize  [I] Number of entries to return per page.
 *  cookie    [I] Used by the server to track its location in the
 *                search results.
 *  critical  [I] Tells the server this control is critical to the
 *                search operation.
 *  control   [O] LDAPControl created.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_create_page_controlW( WLDAP32_LDAP *ld, ULONG pagesize,
    struct WLDAP32_berval *cookie, UCHAR critical, PLDAPControlW *control )
{
#ifdef HAVE_LDAP
    struct berval *cookieU = NULL;
    ULONG ret;

    TRACE( "(%p, 0x%08x, %p, 0x%02x, %p)\n", ld, pagesize, cookie,
           critical, control );

    if (!ld || !control || pagesize > LDAP_MAXINT)
        return WLDAP32_LDAP_PARAM_ERROR;

    if (cookie && !(cookieU = bervalWtoU( cookie ))) return WLDAP32_LDAP_NO_MEMORY;
    ret = create_page_control( pagesize, cookieU, critical, control );
    heap_free( cookieU );
    return ret;

#else
    return WLDAP32_LDAP_NOT_SUPPORTED;
#endif
}

ULONG CDECL ldap_get_next_page( WLDAP32_LDAP *ld, PLDAPSearch search, ULONG pagesize,
    ULONG *message )
{
    FIXME( "(%p, %p, 0x%08x, %p)\n", ld, search, pagesize, message );

    if (!ld) return ~0u;
    return WLDAP32_LDAP_NOT_SUPPORTED;
}

ULONG CDECL ldap_get_next_page_s( WLDAP32_LDAP *ld, PLDAPSearch search,
    struct l_timeval *timeout, ULONG pagesize, ULONG *count,
    WLDAP32_LDAPMessage **results )
{
#ifdef HAVE_LDAP
    ULONG ret;

    TRACE( "(%p, %p, %p, %u, %p, %p)\n", ld, search, timeout,
           pagesize, count, results );
    if (!ld || !search || !count || !results) return ~0u;

    if (search->cookie && search->cookie->bv_len == 0)
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

    ret = ldap_search_ext_sW( ld, search->dn, search->scope,
                              search->filter, search->attrs, search->attrsonly,
                              search->serverctrls, search->clientctrls,
                              search->timeout.tv_sec ? &search->timeout : NULL, search->sizelimit, results );
    if (ret != WLDAP32_LDAP_SUCCESS) return ret;

    return ldap_get_paged_count( ld, search, count, *results );

#endif
    return WLDAP32_LDAP_NOT_SUPPORTED;
}

ULONG CDECL ldap_get_paged_count( WLDAP32_LDAP *ld, PLDAPSearch search,
    ULONG *count, WLDAP32_LDAPMessage *results )
{
#ifdef HAVE_LDAP
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

    heap_free( search->cookie );
    search->cookie = NULL;

    ret = ldap_parse_page_controlW( ld, server_ctrls, count, &search->cookie );
    if (ret == WLDAP32_LDAP_SUCCESS)
        TRACE("new search->cookie: %s, count %u\n", debugstr_an(search->cookie->bv_val, search->cookie->bv_len), *count);

    ldap_controls_freeW( server_ctrls );

    return ret;

#endif
    return WLDAP32_LDAP_NOT_SUPPORTED;
}

/***********************************************************************
 *      ldap_parse_page_controlA      (WLDAP32.@)
 */
ULONG CDECL ldap_parse_page_controlA( WLDAP32_LDAP *ld, PLDAPControlA *ctrls,
    ULONG *count, struct WLDAP32_berval **cookie )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW **ctrlsW = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, ctrls, count, cookie );

    if (!ld || !ctrls || !count || !cookie)
        return WLDAP32_LDAP_PARAM_ERROR;

    ctrlsW = controlarrayAtoW( ctrls );
    if (!ctrlsW) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_parse_page_controlW( ld, ctrlsW, count, cookie );
    controlarrayfreeW( ctrlsW );
 
#endif
    return ret;
}

/***********************************************************************
 *      ldap_parse_page_controlW      (WLDAP32.@)
 */
ULONG CDECL ldap_parse_page_controlW( WLDAP32_LDAP *ld, PLDAPControlW *ctrls,
    ULONG *count, struct WLDAP32_berval **cookie )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW *control = NULL;
    struct berval *cookieU = NULL, *valueU;
    BerElement *ber;
    ber_tag_t tag;
    ULONG i;

    TRACE( "(%p, %p, %p, %p)\n", ld, ctrls, count, cookie );

    if (!ld || !ctrls || !count || !cookie)
        return WLDAP32_LDAP_PARAM_ERROR;

    for (i = 0; ctrls[i]; i++)
    {
        if (!lstrcmpW( LDAP_PAGED_RESULT_OID_STRING_W, ctrls[i]->ldctl_oid ))
            control = ctrls[i];
    }

    if (!control)
        return WLDAP32_LDAP_CONTROL_NOT_FOUND;

    if (cookie && !(cookieU = bervalWtoU( *cookie )))
        return WLDAP32_LDAP_NO_MEMORY;

    if (!(valueU = bervalWtoU( &control->ldctl_value )))
    {
        heap_free( cookieU );
        return WLDAP32_LDAP_NO_MEMORY;
    }

    ber = ber_init( valueU );
    heap_free( valueU );
    if (!ber)
    {
        heap_free( cookieU );
        return WLDAP32_LDAP_NO_MEMORY;
    }

    tag = ber_scanf( ber, "{iO}", count, &cookieU );
    if (tag == LBER_ERROR)
        ret = WLDAP32_LDAP_DECODING_ERROR;
    else
        ret = WLDAP32_LDAP_SUCCESS;

    heap_free( cookieU );
    ber_free( ber, 1 );

#endif
    return ret;
}

ULONG CDECL ldap_search_abandon_page( WLDAP32_LDAP *ld, PLDAPSearch search )
{
#ifdef HAVE_LDAP
    LDAPControlW **ctrls;

    TRACE( "(%p, %p)\n", ld, search );

    if (!ld || !search) return ~0u;

    strfreeW( search->dn );
    strfreeW( search->filter );
    strarrayfreeW( search->attrs );
    ctrls = search->serverctrls;
    controlfreeW( ctrls[0] ); /* page control */
    ctrls++;
    while (*ctrls) controlfreeW( *ctrls++ );
    heap_free( search->serverctrls );
    controlarrayfreeW( search->clientctrls );
    if (search->cookie && search->cookie != &null_cookieW)
        heap_free( search->cookie );
    heap_free( search );

    return WLDAP32_LDAP_SUCCESS;

#else
    return WLDAP32_LDAP_NOT_SUPPORTED;
#endif
}

PLDAPSearch CDECL ldap_search_init_pageA( WLDAP32_LDAP *ld, PCHAR dn, ULONG scope,
    PCHAR filter, PCHAR attrs[], ULONG attrsonly, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, ULONG timelimit, ULONG sizelimit, PLDAPSortKeyA *sortkeys )
{
    FIXME( "(%p, %s, 0x%08x, %s, %p, 0x%08x)\n", ld, debugstr_a(dn),
           scope, debugstr_a(filter), attrs, attrsonly );
    return NULL;
}

PLDAPSearch CDECL ldap_search_init_pageW( WLDAP32_LDAP *ld, PWCHAR dn, ULONG scope,
    PWCHAR filter, PWCHAR attrs[], ULONG attrsonly, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, ULONG timelimit, ULONG sizelimit, PLDAPSortKeyW *sortkeys )
{
#ifdef HAVE_LDAP
    LDAPSearch *search;
    DWORD i, len;

    TRACE( "(%p, %s, 0x%08x, %s, %p, 0x%08x, %p, %p, 0x%08x, 0x%08x, %p)\n",
           ld, debugstr_w(dn), scope, debugstr_w(filter), attrs, attrsonly,
           serverctrls, clientctrls, timelimit, sizelimit, sortkeys );

    search = heap_alloc_zero( sizeof(*search) );
    if (!search)
    {
        ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;
        return NULL;
    }

    if (dn)
    {
        search->dn = strdupW( dn );
        if (!search->dn) goto fail;
    }
    if (filter)
    {
        search->filter = strdupW( filter );
        if (!search->filter) goto fail;
    }
    if (attrs)
    {
        search->attrs = strarraydupW( attrs );
        if (!search->attrs) goto fail;
    }

    len = serverctrls ? controlarraylenW( serverctrls ) : 0;
    search->serverctrls = heap_alloc( sizeof(LDAPControl *) * (len + 2) );
    if (!search->serverctrls) goto fail;
    search->serverctrls[0] = NULL; /* reserve 0 for page control */
    for (i = 0; i < len; i++)
    {
        search->serverctrls[i + 1] = controldupW( serverctrls[i] );
        if (!search->serverctrls[i + 1]) goto fail;
    }
    search->serverctrls[len + 1] = NULL;

    if (clientctrls)
    {
        search->clientctrls = controlarraydupW( clientctrls );
        if (!search->clientctrls) goto fail;
    }

    search->scope = scope;
    search->attrsonly = attrsonly;
    search->timeout.tv_sec = timelimit;
    search->timeout.tv_usec = 0;
    search->sizelimit = sizelimit;
    search->cookie = NULL;

    return search;

fail:
    ldap_search_abandon_page( ld, search );
    ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;

#endif
    return NULL;
}
