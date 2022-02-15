/*
 * test parsing functions
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
 * Copyright 2020 Dmitry Timoshkov
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
#include <windef.h>
#include <winbase.h>
#include <winldap.h>
#include <winber.h>

#include "wine/test.h"

#ifndef LDAP_AUTH_SIMPLE
#define LDAP_AUTH_SIMPLE 0x80
#endif

#ifndef LDAP_OPT_SERVER_CONTROLS
#define LDAP_OPT_SERVER_CONTROLS 0x0012
#endif

static void test_ldap_parse_sort_control( LDAP *ld )
{
    ULONG ret, result;
    LDAPSortKeyA *sortkeys[2], key;
    LDAPControlA *sort, *ctrls[2], **server_ctrls;
    LDAPMessage *res = NULL;
    struct l_timeval timeout;

    key.sk_attrtype = (char *)"ou";
    key.sk_matchruleoid = NULL;
    key.sk_reverseorder = FALSE;

    sortkeys[0] = &key;
    sortkeys[1] = NULL;
    ret = ldap_create_sort_controlA( ld, sortkeys, 0, &sort );
    ok( !ret, "ldap_create_sort_controlA failed %#lx\n", ret );

    ctrls[0] = sort;
    ctrls[1] = NULL;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;
    ret = ldap_search_ext_sA( ld, (char *)"", LDAP_SCOPE_ONELEVEL, (char *)"(ou=*)", NULL, 0, ctrls, NULL, &timeout, 10, &res );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_TIMEOUT)
    {
        skip("test server can't be reached\n");
        ldap_control_freeA( sort );
        return;
    }
    ok( !ret, "ldap_search_ext_sA failed %#lx\n", ret );
    ok( res != NULL, "expected res != NULL\n" );

    if (GetProcAddress(GetModuleHandleA("wldap32.dll"), "ber_init"))
    {
        ret = ldap_parse_resultA( NULL, res, &result, NULL, NULL, NULL, &server_ctrls, 1 );
        ok( ret == LDAP_PARAM_ERROR, "ldap_parse_resultA failed %#lx\n", ret );
    }
    else
        win_skip("Test would crash on older wldap32 versions\n");

    result = ~0u;
    ret = ldap_parse_resultA( ld, res, &result, NULL, NULL, NULL, &server_ctrls, 1 );
    ok( !ret, "ldap_parse_resultA failed %#lx\n", ret );
    ok( !result, "got %#lx expected 0\n", result );

    ret = ldap_parse_sort_controlA( NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_parse_sort_controlA failed %#lx\n", ret );

    ret = ldap_parse_sort_controlA( ld, NULL, NULL, NULL );
    ok( ret == LDAP_CONTROL_NOT_FOUND, "ldap_parse_sort_controlA failed %#lx\n", ret );

    ret = ldap_parse_sort_controlA( ld, NULL, &result, NULL );
    ok( ret == LDAP_CONTROL_NOT_FOUND, "ldap_parse_sort_controlA failed %#lx\n", ret );

    ret = ldap_parse_sort_controlA( ld, server_ctrls, &result, NULL );
    ok( ret == LDAP_CONTROL_NOT_FOUND, "ldap_parse_sort_controlA failed %#lx\n", ret );

    ldap_control_freeA( sort );
    ldap_controls_freeA( server_ctrls );
}

static void test_ldap_search_extW( LDAP *ld )
{
    ULONG ret, message, timelimit;
    WCHAR base[] = L"", filter[] = L"ou=*";

    timelimit = 20;
    ret = ldap_search_extW( ld, base, LDAP_SCOPE_SUBTREE, filter, NULL, 0, NULL, NULL, timelimit, 0, &message );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip("test server can't be reached\n");
        return;
    }
    ok( !ret, "ldap_search_extW failed %#lx\n", ret );

    timelimit = 0;
    ret = ldap_search_extW( ld, base, LDAP_SCOPE_SUBTREE, filter, NULL, 0, NULL, NULL, timelimit, 0, &message );
    ok( !ret, "ldap_search_extW failed %#lx\n", ret );
}

static void test_ldap_set_optionW( LDAP *ld )
{
    ULONG ret, oldvalue;

    ret = ldap_get_optionW( ld, LDAP_OPT_REFERRALS, &oldvalue );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip("test server can't be reached\n");
        return;
    }

    ret = ldap_set_optionW( ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF );
    ok( !ret || broken(ret == LDAP_PARAM_ERROR) /* nt4, win2k */, "ldap_set_optionW failed %#lx\n", ret );

    ret = ldap_set_optionW( ld, LDAP_OPT_REFERRALS, (void *)&oldvalue );
    ok( !ret, "ldap_set_optionW failed %#lx\n", ret );
}

static void test_ldap_get_optionW( LDAP *ld )
{
    ULONG ret, version;

    ret = ldap_get_optionW( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    ok( !ret, "ldap_get_optionW failed %#lx\n", ret );
    ok( version == LDAP_VERSION3, "got %lu\n", version );
}

static void test_ldap_bind_sA( void )
{
    LDAP *ld;
    ULONG ret;
    int version;

    ld = ldap_initA( (char *)"ldap.forumsys.com", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    version = LDAP_VERSION3;
    ret = ldap_set_optionW( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }

    ret = ldap_connect( ld, NULL );
    ok( !ret, "ldap_connect failed %#lx\n", ret );

    ret = ldap_bind_sA( ld, (char *)"CN=read-only-admin,DC=example,DC=com", (char *)"password", LDAP_AUTH_SIMPLE );
    ok( !ret, "ldap_bind_s failed %#lx\n", ret );

    ldap_unbind( ld );
}

static void test_ldap_server_control( void )
{
    /* SEQUENCE  { INTEGER :: 0x07 } */
    static char mask_ber[5] = {0x30,0x03,0x02,0x01,0x07};
    LDAP *ld;
    ULONG ret;
    int version;
    LDAPControlW *ctrls[2], mask;
    LDAPMessage *res;

    ld = ldap_initA( (char *)"ldap.forumsys.com", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    version = LDAP_VERSION3;
    ret = ldap_set_optionW( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }

    ret = ldap_connect( ld, NULL );
    ok( !ret, "ldap_connect failed %#lx\n", ret );

    /* test setting a not supported server control */
    mask.ldctl_oid = (WCHAR *)L"1.2.840.113556.1.4.801";
    mask.ldctl_iscritical = TRUE;
    mask.ldctl_value.bv_val = mask_ber;
    mask.ldctl_value.bv_len = sizeof(mask_ber);
    ctrls[0] = &mask;
    ctrls[1] = NULL;
    ret = ldap_set_optionW(ld, LDAP_OPT_SERVER_CONTROLS, ctrls);
    ok( ret == LDAP_PARAM_ERROR, "ldap_set_optionW should fail: %#lx\n", ret );

    res = NULL;
    ret = ldap_search_sA( ld, (char *)"OU=scientists,DC=example,DC=com", LDAP_SCOPE_BASE, (char *)"(objectclass=*)", NULL, FALSE, &res );
    ok( !ret, "ldap_search_sA failed %#lx\n", ret );
    ok( res != NULL, "expected res != NULL\n" );

    ldap_msgfree( res );
    ldap_unbind( ld );
}

static void test_ldap_paged_search(void)
{
    LDAP *ld;
    ULONG ret, count;
    int version;
    LDAPSearch *search;
    LDAPMessage *res, *entry;
    BerElement *ber;
    WCHAR *attr;

    ld = ldap_initA( (char *)"ldap.forumsys.com", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    version = LDAP_VERSION3;
    ret = ldap_set_optionW( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }

    search = ldap_search_init_pageW( ld, (WCHAR *)L"", LDAP_SCOPE_BASE, (WCHAR *)L"(objectclass=*)",
                                     NULL, FALSE, NULL, NULL, 0, 0, NULL);
    ok( search != NULL, "ldap_search_init_page failed\n" );

    count = 0xdeadbeef;
    res = NULL;
    ret = ldap_get_next_page_s( ld, search, NULL, 1, &count, &res );
    ok( !ret, "ldap_get_next_page_s failed %#lx\n", ret );
    ok( res != NULL, "expected res != NULL\n" );
    ok( count == 0, "got %lu\n", count );

    count = ldap_count_entries( ld, res);
    ok( count == 1, "got %lu\n", count );

    entry = ldap_first_entry( ld, res);
    ok( res != NULL, "expected entry != NULL\n" );

    attr = ldap_first_attributeW( ld, entry, &ber );
    ok( !wcscmp( attr, L"objectClass" ), "got %s\n", wine_dbgstr_w( attr ) );
    ldap_memfreeW( attr );
    attr = ldap_next_attributeW( ld, entry, ber );
    ok( !attr, "got %s\n", wine_dbgstr_w( attr ));

    ber_free(ber, 0);
    ldap_msgfree( res );

    count = 0xdeadbeef;
    res = (void *)0xdeadbeef;
    ret = ldap_get_next_page_s( ld, search, NULL, 1, &count, &res );
    ok( ret == LDAP_NO_RESULTS_RETURNED, "got %#lx\n", ret );
    ok( !res, "expected res == NULL\n" );
    ok( count == 0, "got %lu\n", count );

    ldap_search_abandon_page( ld, search );
    ldap_unbind( ld );
}

START_TEST (parse)
{
    LDAP *ld;

    test_ldap_paged_search();
    test_ldap_server_control();
    test_ldap_bind_sA();

    ld = ldap_initA((char *)"ldap.itd.umich.edu", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    test_ldap_parse_sort_control( ld );
    test_ldap_search_extW( ld );
    test_ldap_get_optionW( ld );
    test_ldap_set_optionW( ld );
    ldap_unbind( ld );
}
