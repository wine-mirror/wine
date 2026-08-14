/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include <stdio.h>

#define COBJMACROS

#include <windows.h>
#include <winsxs.h>
#include <corerror.h>

#include "wine/test.h"

static void test_CreateAssemblyNameObject( void )
{
    static const WCHAR emptyW[] = {0};
    IAssemblyName *name;
    HRESULT hr;

    hr = CreateAssemblyNameObject( NULL, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr);

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    hr = CreateAssemblyNameObject( NULL, L"wine", CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine", CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    hr = CreateAssemblyNameObject( NULL, L"wine", 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine", 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    hr = CreateAssemblyNameObject( NULL, L"wine", CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine", CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine,version=\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version=1.2.3.4", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine, version=\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME ),
        "expected ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version =\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME ),
        "expected ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version= \"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine ,version=\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine,type=\"\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,type=\"win32", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );
}

static void test_display_name( void )
{
    static const struct { const WCHAR *input, *expect; } tests[] =
    {
        { L"wine", L"wine" },
        { L"wine,version=\"1.2.3.4\",type=\"foo\"", L"wine,type=\"foo\",version=\"1.2.3.4\"" },
        { L"wine,version=\"1.2.3.4\",language=\"en-US\",type=\"win32\",processorArchitecture=\"amd64\",publicKeyToken=\"foo\"",
          L"wine,language=\"en-US\",processorArchitecture=\"amd64\",publicKeyToken=\"foo\",type=\"win32\",version=\"1.2.3.4\"" },
        { L"wine ,version=\"1 . 2 . 3 . 4\"",
          L"wine&#x20;,version=\"1&#x20;.&#x20;2&#x20;.&#x20;3&#x20;.&#x20;4\"" },
        { L"wine&#x0020;&#x3c;&#x3e;&#X2c;&#3;&#010;&#x123456;A&#0;B,version=\"&#x26;&#x27;3\"",
          L"wine&#x20;&lt;&gt;&#x2c;&#x3;&#xa;&#x3456;A,version=\"&amp;&apos;3\"" },
        { L"wine\x1234\x2345zz,version=\"1.2.3.4\"", L"wine&#x1234;&#x2345;zz,version=\"1.2.3.4\"" },
        { L"wine\a\b\t\r\n!\"#$%'*+-./:;<=>?@[\\]^_`{|}~\x7f",
          L"wine&#x7;&#x8;&#x9;&#xd;&#xa;&#x21;&quot;&#x23;&#x24;&#x25;&apos;&#x2a;&#x2b;-.&#x2f;&#x3a;&#x3b;&lt;&#x3d;&gt;&#x3f;&#x40;&#x5b;&#x5c;&#x5d;&#x5e;_&#x60;&#x7b;&#x7c;&#x7d;&#x7e;&#x7f;" },
        { L"wine&*;A&B;C&;D&", L"wineACD" },
        { L"wine&**;A", NULL },
    };
    unsigned int i;
    WCHAR buffer[1024];
    DWORD size;
    HRESULT hr;
    IAssemblyName *name;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context( "%u", i );
        name = NULL;
        hr = CreateAssemblyNameObject( &name, tests[i].input, CANOF_PARSE_DISPLAY_NAME, NULL );
        if (tests[i].expect)
        {
            ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
            size = ARRAY_SIZE(buffer);
            IAssemblyName_GetDisplayName( name, buffer, &size, 0 );
            ok( !wcscmp( buffer, tests[i].expect ), "wrong result %s\n", debugstr_w(buffer) );
        }
        else ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
        if (!FAILED( hr )) IAssemblyName_Release( name );
        winetest_pop_context();
    }
}

static void test_manifest_path(void)
{
    static const struct { const WCHAR *input, *expect; DWORD err; const WCHAR *broken; } tests[] =
    {
        { L"wine", NULL, ERROR_INVALID_PARAMETER },
        { L"version=\"1.2.3.4\"", NULL, ERROR_INVALID_PARAMETER },
        { L"wine,version=\"1.2.3.4\"",
          L"none_wine_none_1.2.3.4_none_84773ee2583267fe" },
        { L"wine,version=\"1.2.3.4\",type=\"foo\"",
          L"none_wine_none_1.2.3.4_none_809e0f845afa3452" },
        { L"wine,version=\"1.2.3.4\",type=\"win32\"",
          L"none_wine_none_1.2.3.4_none_e70f7e4e47ba9a81" },
        { L"wine,version=\"1.2.3.4\",publicKeyToken=\"31bf3856ad364e35\"",
          L"none_wine_31bf3856ad364e35_1.2.3.4_none_71afa6b5b61c3bc4" },
        { L"wine,processorArchitecture=\"amd64\",publicKeyToken=\"31bf3856ad364e35\",type=\"win32\",version=\"1.2.3.4\"",
          L"amd64_wine_31bf3856ad364e35_1.2.3.4_none_fdf9237c42e26d18" },
        { L"wine,language=\"en-US\",publicKeyToken=\"31bf3856ad364e35\",type=\"win32\",version=\"1.2.3.4\"",
          L"none_wine_31bf3856ad364e35_1.2.3.4_en-us_39407a45109afc33" },
        { L"wine,language=\"en-US\",processorArchitecture=\"amd64\",version=\"1.2.3.4\"",
          L"amd64_wine_none_1.2.3.4_en-us_a496a08d77174661" },
        { L"wine,language=\"en-US\",processorArchitecture=\"amd64\",publicKeyToken=\"31bf3856ad364e35\",type=\"win32\",version=\"1.2.3.4\"",
          L"amd64_wine_31bf3856ad364e35_1.2.3.4_en-us_2bda3d954a55adbe" },
        { L"wine,language=\"neutral\",version=\"1.2.3.4\"",
          L"none_wine_none_1.2.3.4_neutral_f9ff6dcba258cd73" },
        { L"wine,language=\"*\",version=\"1.2.3.4\"",
          L"none_wine_none_1.2.3.4__16e8bafd97447162" },
        { L"WI !\"#$%'()*+-./:;<=>?@[\\]`{|}~\u00a0\u2014NE,language=\"\",processorArchitecture=\"\",version=\"1.2.3.4\"",
          L"wi-.ne_none_1.2.3.4_479b515b7cd6b072" },
        { L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789,version=\"1.2.3.4\"",
          L"none_abcdefghijklmnopqrs..rstuvwxyz0123456789_none_1.2.3.4_none_039d813ec5d31e1e" },
        { L"wine,processorArchitecture=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\",version=\"1.2.3.4\"",
          L"abcdefg..3456789_wine_none_1.2.3.4_none_4cf3b364ce50d048",
          0, L"abc..789_wine_none_1.2.3.4_none_4cf3b364ce50d048" /* win8 */ },
        { L"wine,publicKeyToken=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijkl\",version=\"1.2.3.4\"",
          L"none_wine_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijkl_1.2.3.4_none_23520ded75d26b1d" },
        { L"wine,publicKeyToken=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%'()*+-./:;<=>?@[\\]`{|}~\u00a0\u2014\",version=\"1.2.3.4\"",
          L"none_wine_ABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%'()*+-./:;<=>?@[\\]`{|}~\u00a0\u2014_1.2.3.4_none_36e94b00900cd8e5" },
        { L"wine,language=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\",version=\"1.2.3.4\"",
          L"none_wine_none_1.2.3.4_abc..789_9608dbda04ebd12f" },
        { L"wine,version=\"00000000000000000000000001.0000000000000000000000002.0000000000000000000000003.000000000000000000000004\"",
          L"none_wine_none_00000000000000000000000001.0000000000000000000000002.0000000000000000000000003.000000000000000000000004_none_c96f37b6c34189f6" },
        { L"wine,publicKeyToken=\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklm\",version=\"1.2.3.4\"", NULL, ERROR_INSUFFICIENT_BUFFER },
        { L"wine,version=\"00000000000000000000000001.0000000000000000000000002.0000000000000000000000003.0000000000000000000000004\"", NULL, ERROR_INSUFFICIENT_BUFFER },
    };
    unsigned int i;
    WCHAR buffer[1024];
    DWORD len;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    len = 10;
    ret = SxspGenerateManifestPathOnAssemblyIdentity( L"wine,version=\"1.2.3.4\"", buffer, &len, NULL );
    ok( !ret, "SxspGenerateManifestPathOnAssemblyIdentity succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );
    ok( len > 10, "wrong len %lu\n", len );
    ret = SxspGenerateManifestPathOnAssemblyIdentity( L"wine,version=\"1.2.3.4\"", buffer, &len, NULL );
    ok( ret, "SxspGenerateManifestPathOnAssemblyIdentity failed err %lu\n", GetLastError() );
    ok( wcslen( buffer ) == len - 1, "wrong len %Iu\n", wcslen(buffer) );

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context( "%u", i );
        SetLastError( 0xdeadbeef );
        len = ARRAY_SIZE( buffer );
        ret = SxspGenerateManifestPathOnAssemblyIdentity( tests[i].input, buffer, &len, NULL );
        ok( len == ARRAY_SIZE( buffer ), "wrong len %lu\n", len );
        if (tests[i].expect)
        {
            ok( ret, "SxspGenerateManifestPathOnAssemblyIdentity failed err %lu\n", GetLastError() );
            if (ret)
            {
                /* FIXME: hash is different on Wine, so compare without it first */
                const WCHAR *p = wcsrchr( buffer, '_' );
                ok( (p && !wcsncmp( buffer, tests[i].expect, p + 1 - buffer )) ||
                     broken( tests[i].broken && !wcsncmp( buffer, tests[i].broken, p + 1 - buffer )),
                     "wrong result %s / %s\n", debugstr_w(buffer), debugstr_w(tests[i].expect) );
                todo_wine
                ok( !wcscmp( buffer, tests[i].expect ) ||
                    broken( tests[i].broken && !wcscmp( buffer, tests[i].broken )),
                    "wrong result %s / %s\n", debugstr_w(buffer), debugstr_w(tests[i].expect) );
            }
        }
        else
        {
            ok( !ret, "SxspGenerateManifestPathOnAssemblyIdentity succeeded\n" );
            ok( GetLastError() == tests[i].err, "wrong error %lu\n", GetLastError() );
        }
        winetest_pop_context();
    }
}

START_TEST(name)
{
    test_CreateAssemblyNameObject();
    test_display_name();
    test_manifest_path();
}
