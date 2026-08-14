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
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>

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
    ret = ldap_search_ext_sA( ld, (char *)"dc=debian,dc=org", LDAP_SCOPE_ONELEVEL, (char *)"(uid=*)", NULL, 0,
                              ctrls, NULL, &timeout, 10, &res );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_TIMEOUT)
    {
        skip("test server can't be reached\n");
        ldap_control_freeA( sort );
        return;
    }
    ok( !ret, "ldap_search_ext_sA failed %#lx\n", ret );
    ok( res != NULL, "expected res != NULL\n" );

    ret = ldap_parse_resultA( NULL, NULL, NULL, NULL, NULL, NULL, &server_ctrls, 0 );
    ok( ret == LDAP_PARAM_ERROR, "ldap_parse_resultA should fail, got %#lx\n", ret );
    ret = ldap_parse_resultA( NULL, res, NULL, NULL, NULL, NULL, &server_ctrls, 0 );
    ok( ret == LDAP_PARAM_ERROR, "ldap_parse_resultA should fail, got %#lx\n", ret );
    ret = ldap_parse_resultA( ld, NULL, NULL, NULL, NULL, NULL, &server_ctrls, 0 );
    ok( ret == LDAP_NO_RESULTS_RETURNED, "ldap_parse_resultA should fail, got %#lx\n", ret );
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

static void test_opt_referrals( LDAP *ld )
{
    ULONG ret, value;

    value = 0xdeadbeef;
    ret = ldap_get_optionW( ld, LDAP_OPT_REFERRALS, &value );
    ok( !ret, "ldap_get_optionW failed %#lx\n", ret );
    todo_wine ok( value == 1, "got %lu\n", value );

    value = 0;
    ret = ldap_set_optionW( ld, LDAP_OPT_REFERRALS, (void *)&value );
    ok( !ret, "ldap_set_optionW failed %#lx\n", ret );

    value = 0xdeadbeef;
    ret = ldap_get_optionW( ld, LDAP_OPT_REFERRALS, &value );
    ok( !ret, "ldap_get_optionW failed %#lx\n", ret );
    ok( !value, "got %lu\n", value );

    ret = ldap_set_optionW( ld, LDAP_OPT_REFERRALS, LDAP_OPT_ON );
    ok( !ret, "ldap_set_optionW failed %#lx\n", ret );

    value = 0xdeadbeef;
    ret = ldap_get_optionW( ld, LDAP_OPT_REFERRALS, &value );
    ok( !ret, "ldap_get_optionW failed %#lx\n", ret );
    todo_wine ok( value == 1, "got %lu\n", value );
}

static void test_opt_protocol_version( LDAP *ld )
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

    ld = ldap_sslinitA( (char *)"db.debian.org", 636, 1 );
    ok( ld != NULL, "ldap_sslinit failed\n" );

    version = LDAP_VERSION3;
    ret = ldap_set_optionW( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }

    ret = ldap_connect( ld, NULL );
    if (ret == LDAP_SERVER_DOWN)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }
    ok( !ret, "ldap_connect failed %#lx\n", ret );

    ret = ldap_bind_sA( ld, (char *)"uid=winetest,ou=users,dc=debian,dc=org", (char *)"winetest",
                        LDAP_AUTH_SIMPLE );
    ok( ret == LDAP_INVALID_CREDENTIALS, "ldap_bind_s returned %#lx\n", ret );
    ldap_unbind( ld );

    ld = ldap_sslinitA( (char *)"db.debian.org", 389, 0 );
    ok( ld != NULL, "ldap_sslinit failed\n" );

    ret = ldap_connect( ld, NULL );
    ok( !ret, "ldap_connect failed %#lx\n", ret );
    ldap_unbind( ld );
}

static void test_ldap_add( LDAP *ld )
{
    char *one_empty_string[] = { (char *)"", NULL };
    LDAPModA empty_equals_empty = { 0, (char *)"", { one_empty_string } };
    LDAPModA *attrs[] = { &empty_equals_empty, NULL };
    ULONG ret, num;

    ret = ldap_addA( NULL, NULL, NULL );
    ok( ret == (ULONG)-1, "ldap_addA should fail, got %#lx\n", ret );
    ret = ldap_addA( NULL, (char *)"", attrs );
    ok( ret == (ULONG)-1, "ldap_addA should fail, got %#lx\n", ret );
    ret = ldap_addA( ld, NULL, attrs );
    ok( ret != (ULONG)-1, "ldap_addA should succeed, got %#lx\n", ret );
    ret = ldap_addA( ld, (char *)"", NULL );
    ok( ret != (ULONG)-1, "ldap_addA should succeed, got %#lx\n", ret );
    ret = ldap_addA( ld, (char *)"", attrs );
    ok( ret != (ULONG)-1, "ldap_addA should succeed, got %#lx\n", ret );

    ret = ldap_add_sA( NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_sA should fail, got %#lx\n", ret );
    ret = ldap_add_sA( NULL, (char *)"", attrs );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_sA should fail, got %#lx\n", ret );
    ret = ldap_add_sA( ld, NULL, attrs );
    ok( ret == LDAP_ALREADY_EXISTS, "ldap_add_sA should fail, got %#lx\n", ret );
    ret = ldap_add_sA( ld, (char *)"", NULL );
    ok( ret == LDAP_PROTOCOL_ERROR, "ldap_add_sA should fail, got %#lx\n", ret );
    ret = ldap_add_sA( ld, (char *)"", attrs );
    ok( ret == LDAP_ALREADY_EXISTS, "ldap_add_sA should fail, got %#lx\n", ret );

    ret = ldap_add_extA( NULL, NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_extA should fail, got %#lx\n", ret );
    ret = ldap_add_extA( NULL, (char *)"", attrs, NULL, NULL, &num );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_extA should fail, got %#lx\n", ret );
    ret = ldap_add_extA( ld, NULL, attrs, NULL, NULL, &num );
    ok( !ret, "ldap_add_extA should succeed, got %#lx\n", ret );
    ret = ldap_add_extA( ld, (char *)"", NULL, NULL, NULL, &num );
    ok( !ret, "ldap_add_extA should succeed, got %#lx\n", ret );
    ret = ldap_add_extA( ld, (char *)"", attrs, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_extA should fail, got %#lx\n", ret );
    ret = ldap_add_extA( ld, (char *)"", attrs, NULL, NULL, &num );
    ok( !ret, "ldap_add_extA should succeed, got %#lx\n", ret );

    ret = ldap_add_ext_sA( NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_add_ext_sA( NULL, (char *)"", attrs, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_add_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_add_ext_sA( ld, NULL, attrs, NULL, NULL );
    ok( ret == LDAP_ALREADY_EXISTS, "ldap_add_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_add_ext_sA( ld, (char *)"", NULL, NULL, NULL );
    ok( ret == LDAP_PROTOCOL_ERROR, "ldap_add_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_add_ext_sA( ld, (char *)"", attrs, NULL, NULL );
    ok( ret == LDAP_ALREADY_EXISTS, "ldap_add_ext_sA should fail, got %#lx\n", ret );
}

static void test_ldap_modify( LDAP *ld )
{
    char *one_empty_string[] = { (char *)"", NULL };
    LDAPModA empty_equals_empty = { 0, (char *)"", { one_empty_string } };
    LDAPModA *attrs[] = { &empty_equals_empty, NULL };
    ULONG ret, num;

    ret = ldap_modifyA( NULL, NULL, NULL );
    ok( ret == (ULONG)-1, "ldap_modifyA should fail, got %#lx\n", ret );
    ret = ldap_modifyA( NULL, (char *)"", attrs );
    ok( ret == (ULONG)-1, "ldap_modifyA should fail, got %#lx\n", ret );
    ret = ldap_modifyA( ld, NULL, attrs );
    ok( ret != (ULONG)-1, "ldap_modifyA should succeed, got %#lx\n", ret );
    ret = ldap_modifyA( ld, (char *)"", NULL );
    ok( ret != (ULONG)-1, "ldap_modifyA should succeed, got %#lx\n", ret );
    ret = ldap_modifyA( ld, (char *)"", attrs );
    ok( ret != (ULONG)-1, "ldap_modifyA should succeed, got %#lx\n", ret );

    ret = ldap_modify_sA( NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_sA( NULL, (char *)"", attrs );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_sA( ld, NULL, attrs );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_modify_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_sA( ld, (char *)"", NULL );
    ok( ret == LDAP_UNWILLING_TO_PERFORM, "ldap_modify_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_sA( ld, (char *)"", attrs );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_modify_sA should fail, got %#lx\n", ret );

    ret = ldap_modify_extA( NULL, NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_extA should fail, got %#lx\n", ret );
    ret = ldap_modify_extA( NULL, (char *)"", attrs, NULL, NULL, &num );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_extA should fail, got %#lx\n", ret );
    ret = ldap_modify_extA( ld, NULL, attrs, NULL, NULL, &num );
    ok( !ret, "ldap_modify_extA should succeed, got %#lx\n", ret );
    ret = ldap_modify_extA( ld, (char *)"", NULL, NULL, NULL, &num );
    ok( !ret, "ldap_modify_extA should succeed, got %#lx\n", ret );
    ret = ldap_modify_extA( ld, (char *)"", attrs, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_extA should fail, got %#lx\n", ret );
    ret = ldap_modify_extA( ld, (char *)"", attrs, NULL, NULL, &num );
    ok( !ret, "ldap_modify_extA should succeed, got %#lx\n", ret );

    ret = ldap_modify_ext_sA( NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_ext_sA( NULL, (char *)"", attrs, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_modify_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_ext_sA( ld, NULL, attrs, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_modify_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_ext_sA( ld, (char *)"", NULL, NULL, NULL );
    ok( ret == LDAP_UNWILLING_TO_PERFORM, "ldap_modify_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_modify_ext_sA( ld, (char *)"", attrs, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_modify_ext_sA should fail, got %#lx\n", ret );
}

static void test_ldap_compare( LDAP *ld )
{
    struct berval empty_value = { 0 };
    ULONG ret, num;

    ret = ldap_compareA( NULL, NULL, NULL, NULL );
    ok( ret == (ULONG)-1, "ldap_compareA should fail, got %#lx\n", ret );
    ret = ldap_compareA( NULL, (char *)"", (char *)"", (char *)"" );
    ok( ret == (ULONG)-1, "ldap_compareA should fail, got %#lx\n", ret );
    ret = ldap_compareA( ld, NULL, (char *)"", (char *)"" );
    ok( ret != (ULONG)-1, "ldap_compareA should succeed, got %#lx\n", ret );
    ret = ldap_compareA( ld, (char *)"", NULL, (char *)"" );
    ok( ret == (ULONG)-1, "ldap_compareA should fail, got %#lx\n", ret );
    ret = ldap_compareA( ld, (char *)"", (char *)"", NULL );
    ok( ret != (ULONG)-1, "ldap_compareA should succeed, got %#lx\n", ret );
    ret = ldap_compareA( ld, (char *)"", (char *)"", (char *)"" );
    ok( ret != (ULONG)-1, "ldap_compareA should succeed, got %#lx\n", ret );

    ret = ldap_compare_sA( NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_sA( NULL, (char *)"", (char *)"", (char *)"" );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_sA( ld, NULL, (char *)"", (char *)"" );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_sA( ld, (char *)"", NULL, (char *)"" );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_sA( ld, (char *)"", (char *)"", NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_sA( ld, (char *)"", (char *)"", (char *)"" );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_sA should fail, got %#lx\n", ret );

    ret = ldap_compare_extA( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_extA should fail, got %#lx\n", ret );
    ret = ldap_compare_extA( NULL, (char *)"", (char *)"", (char *)"", &empty_value, NULL, NULL, &num );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_extA should fail, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, NULL, (char *)"", (char *)"", &empty_value, NULL, NULL, &num );
    ok( !ret, "ldap_compare_extA should succeed, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, (char *)"", NULL, (char *)"", &empty_value, NULL, NULL, &num );
    ok( ret == LDAP_NO_MEMORY, "ldap_compare_extA should fail, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, (char *)"", (char *)"", NULL, &empty_value, NULL, NULL, &num );
    ok( !ret, "ldap_compare_extA should succeed, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, (char *)"", (char *)"", (char *)"", NULL, NULL, NULL, &num );
    ok( !ret, "ldap_compare_extA should succeed, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, (char *)"", (char *)"", (char *)"", &empty_value, NULL, NULL, &num );
    ok( !ret, "ldap_compare_extA should succeed, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, (char *)"", (char *)"", (char *)"", &empty_value, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_extA should fail, got %#lx\n", ret );
    ret = ldap_compare_extA( ld, (char *)"", (char *)"", (char *)"", &empty_value, NULL, NULL, &num );
    ok( !ret, "ldap_compare_extA should succeed, got %#lx\n", ret );

    ret = ldap_compare_ext_sA( NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_ext_sA( NULL, (char *)"", (char *)"", (char *)"", &empty_value, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_ext_sA( ld, NULL, (char *)"", (char *)"", &empty_value, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_ext_sA( ld, (char *)"", NULL, (char *)"", &empty_value, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_ext_sA( ld, (char *)"", (char *)"", NULL, &empty_value, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_ext_sA( ld, (char *)"", (char *)"", (char *)"", NULL, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_compare_ext_sA( ld, (char *)"", (char *)"", (char *)"", &empty_value, NULL, NULL );
    ok( ret == LDAP_UNDEFINED_TYPE, "ldap_compare_ext_sA should fail, got %#lx\n", ret );
}

static void test_ldap_delete( LDAP *ld )
{
    ULONG ret, num;

    ret = ldap_deleteA( NULL, NULL );
    ok( ret == (ULONG)-1, "ldap_deleteA should fail, got %#lx\n", ret );
    ret = ldap_deleteA( NULL, (char *)"" );
    ok( ret == (ULONG)-1, "ldap_deleteA should fail, got %#lx\n", ret );

    ret = ldap_delete_sA( NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_sA should fail, got %#lx\n", ret );
    ret = ldap_delete_sA( NULL, (char *)"" );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_sA should fail, got %#lx\n", ret );

    ret = ldap_delete_extA( NULL, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_extA should fail, got %#lx\n", ret );
    ret = ldap_delete_extA( NULL, (char *)"", NULL, NULL, &num );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_extA should fail, got %#lx\n", ret );
    ret = ldap_delete_extA( ld, (char *)"", NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_extA should fail, got %#lx\n", ret );

    ret = ldap_delete_ext_sA( NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_ext_sA should fail, got %#lx\n", ret );
    ret = ldap_delete_ext_sA( NULL, (char *)"", NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_delete_ext_sA should fail, got %#lx\n", ret );
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

    ld = ldap_initA( (char *)"db.debian.org", 389 );
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
    if (ret == LDAP_SERVER_DOWN)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }
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
    ret = ldap_search_sA( ld, (char *)"dc=debian,dc=org", LDAP_SCOPE_BASE, (char *)"(objectclass=*)", NULL, FALSE, &res );
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

    ld = ldap_initA( (char *)"db.debian.org", 389 );
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
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }
    ok( !ret, "ldap_get_next_page_s failed %#lx\n", ret );
    ok( res != NULL, "expected res != NULL\n" );
    ok( count == 0, "got %lu\n", count );

    count = ldap_count_entries( NULL, NULL );
    ok( count == 0, "got %lu\n", count );
    count = ldap_count_entries( ld, NULL );
    ok( count == 0, "got %lu\n", count );
    count = ldap_count_entries( NULL, res );
    todo_wine ok( count == 1, "got %lu\n", count );
    count = ldap_count_entries( ld, res );
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

static void test_opt_ssl(void)
{
    LDAP *ld;
    ULONG ret, version = LDAP_VERSION3, value;

    /* Turning on LDAP_OPT_SSL without setting the protocol version does not work */
    ld = ldap_initA( (char *)"db.debian.org", 636 );
    ok( ld != NULL, "ldap_init failed\n" );
    ret = ldap_set_optionA( ld, LDAP_OPT_SSL, LDAP_OPT_ON );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_simple_bind_sA( ld, NULL, NULL );
    if (ret == LDAP_SERVER_DOWN)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }
    todo_wine ok( ret == LDAP_PROTOCOL_ERROR, "ldap_simple_bind_sA should fail, got %#lx\n", ret );
    ldap_unbind( ld );

    /* Setting the protocol version to 3 automatically enables SSL when using port 636 */
    ld = ldap_initA( (char *)"db.debian.org", 636 );
    ok( ld != NULL, "ldap_init failed\n" );
    ret = ldap_set_optionA( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_set_optionA( ld, LDAP_OPT_SSL, LDAP_OPT_OFF );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_simple_bind_sA( ld, NULL, NULL );
    todo_wine ok( !ret, "ldap_simple_bind_sA should succeed, got %#lx\n", ret );
    ldap_unbind( ld );

    /* Turning on SSL when the server does not expect it does not work */
    ld = ldap_initA( (char *)"db.debian.org", 389 );
    ok( ld != NULL, "ldap_init failed\n" );
    ret = ldap_set_optionA( ld, LDAP_OPT_SSL, LDAP_OPT_ON );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_simple_bind_sA( ld, NULL, NULL );
    ok( ret == LDAP_SERVER_DOWN, "ldap_simple_bind_sA should fail, got %#lx\n", ret );
    ldap_unbind( ld );

    /* SSL can also be turned on by passing a pointer to a variable with value 1 */
    ld = ldap_initA( (char *)"db.debian.org", 389 );
    ok( ld != NULL, "ldap_init failed\n" );
    value = 1;
    ret = ldap_set_optionA( ld, LDAP_OPT_SSL, &value );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_simple_bind_sA( ld, NULL, NULL );
    ok( ret == LDAP_SERVER_DOWN, "ldap_simple_bind_sA should fail, got %#lx\n", ret );
    ldap_unbind( ld );

    /* SSL cannot be turned on by passing a pointer to a variable with a bogus value */
    ld = ldap_initA( (char *)"db.debian.org", 389 );
    ok( ld != NULL, "ldap_init failed\n" );
    value = 2;
    ret = ldap_set_optionA( ld, LDAP_OPT_SSL, &value );
    ok( ret == LDAP_PARAM_ERROR, "ldap_set_optionA should fail, got %#lx\n", ret );
    ldap_unbind( ld );
}

static BOOLEAN CDECL verify_certificate( LDAP *ld, const CERT_CONTEXT **cert )
{
    CertFreeCertificateContext(*cert);
    return FALSE;
}

static void test_opt_server_certificate(void)
{
    LDAP *ld;
    ULONG ret, version = LDAP_VERSION3;

    ld = ldap_initA( (char *)"db.debian.org", 636 );
    ok( ld != NULL, "ldap_init failed\n" );
    ret = ldap_set_optionA( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_set_optionA( ld, LDAP_OPT_SSL, LDAP_OPT_ON );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_set_optionA( ld, LDAP_OPT_SERVER_CERTIFICATE, &verify_certificate );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_connect( ld, NULL );
    ok( ret == LDAP_SERVER_DOWN, "ldap_connect should fail, got %#lx\n", ret );
    ret = ldap_simple_bindA( ld, NULL, NULL );
    ok( ret == (ULONG)-1, "ldap_simple_bindA should fail, got %#lx\n", ret );
    ret = ldap_simple_bind_sA( ld, NULL, NULL );
    ok( ret == LDAP_SERVER_DOWN, "ldap_simple_bind_sA should fail, got %#lx\n", ret );
    ldap_unbind( ld );

    ld = ldap_initA( (char *)"db.debian.org", 389 );
    ok( ld != NULL, "ldap_init failed\n" );
    ret = ldap_set_optionA( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_set_optionA( ld, LDAP_OPT_SERVER_CERTIFICATE, &verify_certificate );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_connect( ld, NULL );
    ok( !ret, "ldap_connect should succeed, got %#lx\n", ret );
    ret = ldap_start_tls_sA( ld, NULL, NULL, NULL, NULL );
    ok( ret == LDAP_LOCAL_ERROR, "ldap_start_tls_sA should fail, got %#lx\n", ret );
    ldap_unbind( ld );
}

static void test_opt_auto_reconnect(void)
{
    LDAP *ld;
    ULONG ret, value;

    ld = ldap_initA( (char *)"db.debian.org", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    ret = ldap_set_optionA( ld, LDAP_OPT_AUTO_RECONNECT, LDAP_OPT_ON );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_get_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( !ret, "ldap_get_optionA should succeed, got %#lx\n", ret );
    ok( value == 1, "got %lu\n", ret );

    ret = ldap_set_optionA( ld, LDAP_OPT_AUTO_RECONNECT, LDAP_OPT_OFF );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_get_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( !ret, "ldap_get_optionA should succeed, got %#lx\n", ret );
    ok( value == 0, "got %lu\n", ret );

    value = 1;
    ret = ldap_set_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_get_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( !ret, "ldap_get_optionA should succeed, got %#lx\n", ret );
    ok( value == 1, "got %lu\n", ret );

    value = 0;
    ret = ldap_set_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( !ret, "ldap_set_optionA should succeed, got %#lx\n", ret );
    ret = ldap_get_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( !ret, "ldap_get_optionA should succeed, got %#lx\n", ret );
    ok( value == 0, "got %lu\n", ret );

    value = 2;
    ret = ldap_set_optionA( ld, LDAP_OPT_AUTO_RECONNECT, &value );
    ok( ret == LDAP_PARAM_ERROR, "ldap_set_optionA should fail, got %#lx\n", ret );
}

static void test_ldap_host_name(void)
{
    LDAP *ld;
    char *value;
    ULONG ret;

    ld = ldap_initA( NULL, 389 );
    ok(ld != NULL, "ldap_init failed\n");

    value = (char *)"deadbeef";
    ret = ldap_get_optionA( ld, LDAP_OPT_HOST_NAME, &value );
    ok( !ret, "ldap_get_option error %#lx\n", ret );
    todo_wine
    ok( !value, "got %s\n", value );

    value = (char *)"deadbeef";
    ret = ldap_set_optionA( ld, LDAP_OPT_HOST_NAME, &value );
    ok( !ret, "ldap_set_option error %#lx\n", ret );

    value = (char *)"";
    ret = ldap_get_optionA( ld, LDAP_OPT_HOST_NAME, &value );
    ok( !ret, "ldap_get_option error %#lx\n", ret );
    ok( !strcmp(value, "deadbeef") || !strcmp(value, "deadbeef:389"), "got %s\n", value );

    ldap_unbind( ld );
}

static void test_ldap_search_dc(void)
{
    DOMAIN_CONTROLLER_INFOW *info;
    char *attrs[] = { (char *)"defaultNamingContext", NULL };
    LDAP *ld;
    int version;
    LDAPMessage *res, *entry;
    char **values;
    ULONG ret;

    ret = DsGetDcNameW( NULL, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &info );
    if (ret != ERROR_SUCCESS)
    {
        skip( "Computer is not part of an Active Directory Domain\n" );
        return;
    }
    trace( "Computer is part of domain %s\n", wine_dbgstr_w(info->DomainName) );
    NetApiBufferFree( info );

    ld = ldap_initA( NULL, 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    version = LDAP_VERSION3;
    ret = ldap_set_optionW( ld, LDAP_OPT_PROTOCOL_VERSION, &version );
    if (ret == LDAP_SERVER_DOWN || ret == LDAP_UNAVAILABLE)
    {
        skip( "test server can't be reached\n" );
        ldap_unbind( ld );
        return;
    }

    ret = ldap_search_sA( ld, NULL, LDAP_SCOPE_BASE, (char *)"(objectclass=*)", attrs, FALSE, &res );
    ok( !ret, "ldap_search_s error %#lx\n", ret );
    if (ret)
    {
        ldap_unbind( ld );
        return;
    }

    entry = ldap_first_entry( ld, res );
    ok( entry != NULL, "expected entry != NULL\n" );

    values = ldap_get_valuesA( ld, entry, attrs[0] );
    ok( entry != NULL, "expected values != NULL\n" );
    trace( "%s[0]: %s\n", attrs[0], values[0] );

    ldap_value_freeA( values );
    ldap_msgfree( res );

    ldap_unbind( ld );
}

START_TEST (parse)
{
    LDAP *ld;

    test_ldap_search_dc();
    test_ldap_host_name();
    test_ldap_paged_search();
    test_ldap_server_control();
    test_ldap_bind_sA();
    test_opt_ssl();
    test_opt_server_certificate();
    test_opt_auto_reconnect();

    ld = ldap_initA( (char *)"db.debian.org", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    test_ldap_add( ld );
    test_ldap_modify( ld );
    test_ldap_compare( ld );
    test_ldap_delete( ld );
    test_ldap_parse_sort_control( ld );
    test_ldap_search_extW( ld );
    test_opt_referrals( ld );
    test_opt_protocol_version( ld );
    ldap_unbind( ld );
}
