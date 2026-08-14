/*
 * Copyright 2024-2026 Zhiyi Zhang for CodeWeavers
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
#define COBJMACROS
#include <stdarg.h>
#include "initguid.h"
#include "roapi.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "wine/test.h"

#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"

static void test_IUriRuntimeClassFactory(void)
{
    static const WCHAR *class_name = L"Windows.Foundation.Uri";
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IUriRuntimeClassFactory *uri_factory = NULL;
    IActivationFactory *factory = NULL;
    IUriRuntimeClass *uri_class = NULL;
    HSTRING str, uri;
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = WindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);
    WindowsDeleteString(str);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        RoUninitialize();
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IUriRuntimeClassFactory, (void **)&uri_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IUriRuntimeClassFactory_QueryInterface(uri_factory, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tmp_inspectable == inspectable, "QueryInterface IID_IInspectable returned %p, expected %p.\n",
       tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);
    IInspectable_Release(inspectable);

    hr = IUriRuntimeClassFactory_QueryInterface(uri_factory, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tmp_agile_object == agile_object, "QueryInterface IID_IAgileObject returned %p, expected %p.\n",
       tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);
    IAgileObject_Release(agile_object);

    /* Test IUriRuntimeClassFactory_CreateUri() */
    hr = WindowsCreateString(L"https://www.winehq.org", wcslen(L"https://www.winehq.org"), &uri);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IUriRuntimeClassFactory_CreateUri(uri_factory, NULL, &uri_class);
    ok(hr == E_POINTER, "Got unexpected hr %#lx.\n", hr);

    hr = IUriRuntimeClassFactory_CreateUri(uri_factory, uri, &uri_class);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUriRuntimeClass_Release(uri_class);

    WindowsDeleteString(uri);

    IUriRuntimeClassFactory_Release(uri_factory);
    IActivationFactory_Release(factory);
    RoUninitialize();
}

static void test_IUriRuntimeClass(void)
{
    static const struct
    {
        const WCHAR *src;
        const WCHAR *RawUri;
        const WCHAR *AbsoluteUri;
        const WCHAR *DisplayUri;
        const WCHAR *Domain;
        const WCHAR *Extension;
        const WCHAR *Fragment;
        const WCHAR *Host;
        const WCHAR *Password;
        const WCHAR *Path;
        const WCHAR *Query;
        const WCHAR *SchemeName;
        const WCHAR *UserName;
        INT32 Port;
        BOOL invalid;
    }
    tests[] =
    {
        /* Invalid URIs */
        {L"://example.com", .invalid = TRUE},
        {L"//", .invalid = TRUE},
        {L"//a", .invalid = TRUE},
        {L"a", .invalid = TRUE},
        {L"///a", .invalid = TRUE},
        {L"http://www.host.com:port", .invalid = TRUE},
        {L"http://example.com:+80/", .invalid = TRUE},
        {L"http://example.com:-80/", .invalid = TRUE},
        {L"http_s://example.com", .invalid = TRUE},
        {L"/a/b/c/./../../g", .invalid = TRUE},
        {L"mid/content=5/../6", .invalid = TRUE},
        {L"#frag", .invalid = TRUE},
        {L"http://example.com/a%2z", .invalid = TRUE},
        {L"http://example.com/a%z", .invalid = TRUE},
        {L"http://u:p?a@", .invalid = TRUE},
        {L"http://[1]/", .invalid = TRUE},
        {L"http://[fe80::1ff:fe23:4567:890a%eth0]/", .invalid = TRUE},
        {L"http://[fe80::1ff:fe23:4567:890a%25eth0]/", .invalid = TRUE},
        /* Valid URIs */
        /* URI syntax tests */
        {L"http:", L"http:", NULL, L"http:", NULL, NULL, NULL, NULL, NULL, NULL, NULL, L"http", NULL, 80},
        {L"http:/", L"http:/", NULL, L"http:/", NULL, NULL, NULL, NULL, NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://u:p@", L"http://u:p@", L"http://u:p@/", L"http:///", NULL, NULL, NULL, NULL, L"p", L"/", NULL, L"http", L"u", 80},
        {L"http://u:p;a@", L"http://u:p;a@", L"http://u:p;a@/", L"http:///", NULL, NULL, NULL, NULL, L"p;a", L"/", NULL, L"http", L"u", 80},
        {L"http://u;a:p@", L"http://u;a:p@", L"http://u;a:p@/", L"http:///", NULL, NULL, NULL, NULL, L"p", L"/", NULL, L"http", L"u;a", 80},
        {L"http://u:p:a@", L"http://u:p:a@", L"http://u:p:a@/", L"http:///", NULL, NULL, NULL, NULL, L"p:a", L"/", NULL, L"http", L"u", 80},
        {L"http://example.com", L"http://example.com", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://example.com/", L"http://example.com/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://example.com:/", L"http://example.com:/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"dummy://example.com:/", L"dummy://example.com:/", L"dummy://example.com:/", L"dummy://example.com:/", L"example.com:", NULL, NULL, L"example.com:", NULL, L"/", NULL, L"dummy", NULL, -1},
        {L"http://example.com:80/", L"http://example.com:80/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"https://example.com:443/", L"https://example.com:443/", L"https://example.com/", L"https://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"https", NULL, 443},
        {L"http://example.com/data", L"http://example.com/data", L"http://example.com/data", L"http://example.com/data", L"example.com", NULL, NULL, L"example.com", NULL, L"/data", NULL, L"http", NULL, 80},
        {L"http://example.com/data/", L"http://example.com/data/", L"http://example.com/data/", L"http://example.com/data/", L"example.com", NULL, NULL, L"example.com", NULL, L"/data/", NULL, L"http", NULL, 80},
        {L"http://u:p@example.com/", L"http://u:p@example.com/", L"http://u:p@example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", L"p", L"/", NULL, L"http", L"u", 80},
        {L"http://u@example.com/", L"http://u@example.com/", L"http://u@example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", L"u", 80},
        {L"http://user:@example.com", L"http://user:@example.com", L"http://user:@example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", L"user", 80},
        {L"http://:pass@example.com", L"http://:pass@example.com", L"http://:pass@example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", L"pass", L"/", NULL, L"http", NULL, 80},
        {L"http://:@example.com", L"http://:@example.com", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"1http://example.com", L"1http://example.com", L"1http://example.com/", L"1http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"1http", NULL, -1},
        {L"ahttp://example.com", L"ahttp://example.com", L"ahttp://example.com/", L"ahttp://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"ahttp", NULL, -1},
        {L"http://example.com?q=a?b", L"http://example.com?q=a?b", L"http://example.com/?q=a?b", L"http://example.com/?q=a?b", L"example.com", NULL, NULL, L"example.com", NULL, L"/", L"?q=a?b", L"http", NULL, 80},
        {L"http://example.com?q=a b", L"http://example.com?q=a b", L"http://example.com/?q=a b", L"http://example.com/?q=a b", L"example.com", NULL, NULL, L"example.com", NULL, L"/", L"?q=a b", L"http", NULL, 80},
        {L"http://example.com#a b", L"http://example.com#a b", L"http://example.com/#a b", L"http://example.com/#a b", L"example.com", NULL, L"#a b", L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://a/b/c?d#e", L"http://a/b/c?d#e", L"http://a/b/c?d#e", L"http://a/b/c?d#e", NULL, NULL, L"#e", L"a", NULL, L"/b/c", L"?d", L"http", NULL, 80},
        {L"mailto:John.Doe@example.com", L"mailto:John.Doe@example.com", L"mailto:John.Doe@example.com", L"mailto:John.Doe@example.com", NULL, L".com", NULL, NULL, NULL, L"John.Doe@example.com", NULL, L"mailto", NULL, -1},
        {L"news:comp.infosystems.www.servers.unix", L"news:comp.infosystems.www.servers.unix", L"news:comp.infosystems.www.servers.unix", L"news:comp.infosystems.www.servers.unix", NULL, L".unix", NULL, NULL, NULL, L"comp.infosystems.www.servers.unix", NULL, L"news", NULL, -1},
        {L"tel:+1-816-555-1212", L"tel:+1-816-555-1212", L"tel:+1-816-555-1212", L"tel:+1-816-555-1212", NULL, NULL, NULL, NULL, NULL, L"+1-816-555-1212", NULL, L"tel", NULL, -1},
        {L"telnet://192.0.2.16:80/", L"telnet://192.0.2.16:80/", L"telnet://192.0.2.16:80/", L"telnet://192.0.2.16:80/", NULL, NULL, NULL, L"192.0.2.16", NULL, L"/", NULL, L"telnet", NULL, 80},
        {L"urn:oasis:names:specification:docbook:dtd:xml:4.1.2", L"urn:oasis:names:specification:docbook:dtd:xml:4.1.2", L"urn:oasis:names:specification:docbook:dtd:xml:4.1.2", L"urn:oasis:names:specification:docbook:dtd:xml:4.1.2", NULL, L".2", NULL, NULL, NULL, L"oasis:names:specification:docbook:dtd:xml:4.1.2", NULL, L"urn", NULL, -1},
        {L"urn:ietf:rfc:2648", L"urn:ietf:rfc:2648", L"urn:ietf:rfc:2648", L"urn:ietf:rfc:2648", NULL, NULL, NULL, NULL, NULL, L"ietf:rfc:2648", NULL, L"urn", NULL, -1},
        /* IPv4 hosts */
        {L"http://1/", L"http://1/", L"http://0.0.0.1/", L"http://0.0.0.1/", NULL, NULL, NULL, L"0.0.0.1", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://1.2/", L"http://1.2/", L"http://1.0.0.2/", L"http://1.0.0.2/", NULL, NULL, NULL, L"1.0.0.2", NULL, L"/", NULL, L"http", NULL, 80},
        {L"dummy://1.2/", L"dummy://1.2/", L"dummy://1.2/", L"dummy://1.2/", NULL, NULL, NULL, L"1.2", NULL, L"/", NULL, L"dummy", NULL, -1},
        {L"http://1.2.3/", L"http://1.2.3/", L"http://1.2.0.3/", L"http://1.2.0.3/", NULL, NULL, NULL, L"1.2.0.3", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://1.2.3.4/", L"http://1.2.3.4/", L"http://1.2.3.4/", L"http://1.2.3.4/", NULL, NULL, NULL, L"1.2.3.4", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://4294967296/", L"http://4294967296/", L"http://4294967296/", L"http://4294967296/", NULL, NULL, NULL, L"4294967296", NULL, L"/", NULL, L"http", NULL, 80},
        /* IPv6 hosts */
        {L"http://[::1]/", L"http://[::1]/", L"http://[::1]/", L"http://[::1]/", NULL, NULL, NULL, L"::1", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://[::1]:8080/", L"http://[::1]:8080/", L"http://[::1]:8080/", L"http://[::1]:8080/", NULL, NULL, NULL, L"::1", NULL, L"/", NULL, L"http", NULL, 8080},
        {L"http://[::ffff:127.0.0.1]/", L"http://[::ffff:127.0.0.1]/", L"http://[::ffff:127.0.0.1]/", L"http://[::ffff:127.0.0.1]/", NULL, NULL, NULL, L"::ffff:127.0.0.1", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://[2001:db8::7]/", L"http://[2001:db8::7]/", L"http://[2001:db8::7]/", L"http://[2001:db8::7]/", NULL, NULL, NULL, L"2001:db8::7", NULL, L"/", NULL, L"http", NULL, 80},
        /* Domain. Domain is the registrable domain name, which is the effective top-level domain plus one */
        {L"http://www.example.com", L"http://www.example.com", L"http://www.example.com/", L"http://www.example.com/", L"example.com", NULL, NULL, L"www.example.com", NULL, L"/", NULL, L"http", NULL, 80},
        /* Default ports */
        {L"ftp://", L"ftp://", L"ftp:///", L"ftp:///", NULL, NULL, NULL, NULL, NULL, L"/", NULL, L"ftp", NULL, 21},
        {L"telnet://", L"telnet://", L"telnet:///", L"telnet:///", NULL, NULL, NULL, NULL, NULL, L"/", NULL, L"telnet", NULL, 23},
        {L"http://", L"http://", L"http:///", L"http:///", NULL, NULL, NULL, NULL, NULL, L"/", NULL, L"http", NULL, 80},
        {L"https://", L"https://", L"https:///", L"https:///", NULL, NULL, NULL, NULL, NULL, L"/", NULL, L"https", NULL, 443},
        {L"https://example.com:0", L"https://example.com:0", L"https://example.com:0/", L"https://example.com:0/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"https", NULL, 0},
        {L"dummy://example.com:0", L"dummy://example.com:0", L"dummy://example.com:0/", L"dummy://example.com:0/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"dummy", NULL, 0},
        /* Extension */
        {L"http://example.com/.txt", L"http://example.com/.txt", L"http://example.com/.txt", L"http://example.com/.txt", L"example.com", L".txt", NULL, L"example.com", NULL, L"/.txt", NULL, L"http", NULL, 80},
        {L"http://example.com/1.txt", L"http://example.com/1.txt", L"http://example.com/1.txt", L"http://example.com/1.txt", L"example.com", L".txt", NULL, L"example.com", NULL, L"/1.txt", NULL, L"http", NULL, 80},
        {L"http://example.com/1..txt", L"http://example.com/1..txt", L"http://example.com/1..txt", L"http://example.com/1..txt", L"example.com", L".txt", NULL, L"example.com", NULL, L"/1..txt", NULL, L"http", NULL, 80},
        {L"http://example.com/1.2.txt", L"http://example.com/1.2.txt", L"http://example.com/1.2.txt", L"http://example.com/1.2.txt", L"example.com", L".txt", NULL, L"example.com", NULL, L"/1.2.txt", NULL, L"http", NULL, 80},
        /* Backslash normalization */
        {L"dummy://a\\b//c\\d?e\\f#g\\h", L"dummy://a\\b//c\\d?e\\f#g\\h", L"dummy://a\\b//c\\d?e\\f#g\\h", L"dummy://a\\b//c\\d?e\\f#g\\h", NULL, NULL, L"#g\\h", L"a\\b", NULL, L"//c\\d", L"?e\\f", L"dummy", NULL, -1},
        {L"http://a\\b//c\\d?e\\f#g\\h", L"http://a\\b//c\\d?e\\f#g\\h", L"http://a/b//c/d?e\\f#g\\h", L"http://a/b//c/d?e\\f#g\\h", NULL, NULL, L"#g\\h", L"a", NULL, L"/b//c/d", L"?e\\f", L"http", NULL, 80},
        {L"file://\\sample\\sample.bundle", L"file://\\sample\\sample.bundle", L"file:///sample/sample.bundle", L"file:///sample/sample.bundle", NULL, L".bundle", NULL, NULL, NULL, L"/sample/sample.bundle", NULL, L"file", NULL, -1},
        /* Dot segments removal */
        {L"http://example.com/.", L"http://example.com/.", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://example.com/..", L"http://example.com/..", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://example.com/...", L"http://example.com/...", L"http://example.com/...", L"http://example.com/...", L"example.com", L".", NULL, L"example.com", NULL, L"/...", NULL, L"http", NULL, 80},
        {L"http://example.com/a/.", L"http://example.com/a/.", L"http://example.com/a/", L"http://example.com/a/", L"example.com", NULL, NULL, L"example.com", NULL, L"/a/", NULL, L"http", NULL, 80},
        {L"http://example.com/a/..", L"http://example.com/a/..", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://example.com/a/b/../../c", L"http://example.com/a/b/../../c", L"http://example.com/c", L"http://example.com/c", L"example.com", NULL, NULL, L"example.com", NULL, L"/c", NULL, L"http", NULL, 80},
        {L"http://example.com/a/b/.././c", L"http://example.com/a/b/.././c", L"http://example.com/a/c", L"http://example.com/a/c", L"example.com", NULL, NULL, L"example.com", NULL, L"/a/c", NULL, L"http", NULL, 80},
        {L"http://example.com/a/b/..", L"http://example.com/a/b/..", L"http://example.com/a/", L"http://example.com/a/", L"example.com", NULL, NULL, L"example.com", NULL, L"/a/", NULL, L"http", NULL, 80},
        {L"http://example.com/a/b/.", L"http://example.com/a/b/.", L"http://example.com/a/b/", L"http://example.com/a/b/", L"example.com", NULL, NULL, L"example.com", NULL, L"/a/b/", NULL, L"http", NULL, 80},
        /* Implicit file scheme */
        {L"a:", L"a:", L"file:///a:", L"file:///a:", NULL, NULL, NULL, NULL, NULL, L"/a:", NULL, L"file", NULL, -1},
        {L"A:", L"A:", L"file:///A:", L"file:///A:", NULL, NULL, NULL, NULL, NULL, L"/A:", NULL, L"file", NULL, -1},
        {L"a:/", L"a:/", L"file:///a:/", L"file:///a:/", NULL, NULL, NULL, NULL, NULL, L"/a:/", NULL, L"file", NULL, -1},
        {L"z:/", L"z:/", L"file:///z:/", L"file:///z:/", NULL, NULL, NULL, NULL, NULL, L"/z:/", NULL, L"file", NULL, -1},
        {L"a://", L"a://", L"file:///a://", L"file:///a://", NULL, NULL, NULL, NULL, NULL, L"/a://", NULL, L"file", NULL, -1},
        {L"a:b/c", L"a:b/c", L"file:///a:b/c", L"file:///a:b/c", NULL, NULL, NULL, NULL, NULL, L"/a:b/c", NULL, L"file", NULL, -1},
        {L"ab:c/d", L"ab:c/d", L"ab:c/d", L"ab:c/d", NULL, NULL, NULL, NULL, NULL, L"c/d", NULL, L"ab", NULL, -1},
        /* White spaces */
        {L"http:// example.com/", L"http:// example.com/", L"http://%20example.com/", L"http://%20example.com/", L"%20example.com", NULL, NULL, L"%20example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://\texample.com/", L"http://example.com/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://\rexample.com/", L"http://example.com/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"http://\nexample.com/", L"http://example.com/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        /* Percent encoding */
        {L"http://example.com/a%20b", L"http://example.com/a%20b", L"http://example.com/a%20b", L"http://example.com/a%20b", L"example.com", NULL, NULL, L"example.com", NULL, L"/a%20b", NULL, L"http", NULL, 80},
        {L"http://example.com/a%2f/b", L"http://example.com/a%2f/b", L"http://example.com/a%2f/b", L"http://example.com/a%2f/b", L"example.com", NULL, NULL, L"example.com", NULL, L"/a%2f/b", NULL, L"http", NULL, 80},
        {L"http://example.com/a%2F/b", L"http://example.com/a%2F/b", L"http://example.com/a%2F/b", L"http://example.com/a%2F/b", L"example.com", NULL, NULL, L"example.com", NULL, L"/a%2F/b", NULL, L"http", NULL, 80},
        {L"http://%FF%FF%FF.com", L"http://%FF%FF%FF.com", L"http://%ff%ff%ff.com/", L"http://%ff%ff%ff.com/", L"%ff%ff%ff.com", NULL, NULL, L"%ff%ff%ff.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"dummy://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"dummy://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"dummy://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"dummy://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"exam ple.com", NULL, L"#frag ment", L"exam ple.com", L"pass word", L"/pa th", L"?qu ery=val ue", L"dummy", L"us er", -1},
        {L"http://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"http://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"http://us%20er:pass%20word@exam%20ple.com/pa%20th?qu ery=val ue#frag ment", L"http://exam%20ple.com/pa%20th?qu ery=val ue#frag ment", L"exam%20ple.com", NULL, L"#frag ment", L"exam%20ple.com", L"pass%20word", L"/pa%20th", L"?qu ery=val ue", L"http", L"us%20er", 80},
        {L"https://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"https://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"https://us%20er:pass%20word@exam%20ple.com/pa%20th?qu ery=val ue#frag ment", L"https://exam%20ple.com/pa%20th?qu ery=val ue#frag ment", L"exam%20ple.com", NULL, L"#frag ment", L"exam%20ple.com", L"pass%20word", L"/pa%20th", L"?qu ery=val ue", L"https", L"us%20er", 443},
        {L"ftp://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"ftp://us er:pass word@exam ple.com/pa th?qu ery=val ue#frag ment", L"ftp://us%20er:pass%20word@exam%20ple.com/pa%20th?qu ery=val ue#frag ment", L"ftp://exam%20ple.com/pa%20th?qu ery=val ue#frag ment", L"exam%20ple.com", NULL, L"#frag ment", L"exam%20ple.com", L"pass%20word", L"/pa%20th", L"?qu ery=val ue", L"ftp", L"us%20er", 21},
        {L"file://a b/c d", L"file://a b/c d", L"file://a%20b/c%20d", L"file://a%20b/c%20d", NULL, NULL, NULL, L"a%20b", NULL, L"/c%20d", NULL, L"file", NULL, -1},
        {L"http://example.com/path/to /resource", L"http://example.com/path/to /resource", L"http://example.com/path/to%20/resource", L"http://example.com/path/to%20/resource", L"example.com", NULL, NULL, L"example.com", NULL, L"/path/to%20/resource", NULL, L"http", NULL, 80},
        /* Unicode */
        {L"http://example.com/\u00e4\u00f6\u00fc", L"http://example.com/\u00e4\u00f6\u00fc", L"http://example.com/\u00e4\u00f6\u00fc", L"http://example.com/\u00e4\u00f6\u00fc", L"example.com", NULL, NULL, L"example.com", NULL, L"/\u00e4\u00f6\u00fc", NULL, L"http", NULL, 80},
        /* Miscellaneous tests */
        {L"HTTP://EXAMPLE.COM/", L"HTTP://EXAMPLE.COM/", L"http://example.com/", L"http://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"http", NULL, 80},
        {L"dummy:", L"dummy:", L"dummy:", L"dummy:", NULL, NULL, NULL, NULL, NULL, NULL, NULL, L"dummy", NULL, -1},
        {L"dummy://example.com", L"dummy://example.com", L"dummy://example.com/", L"dummy://example.com/", L"example.com", NULL, NULL, L"example.com", NULL, L"/", NULL, L"dummy", NULL, -1},
    };
    static const WCHAR *class_name = L"Windows.Foundation.Uri";
    IActivationFactory *activation_factory = NULL;
    IUriRuntimeClassFactory *uri_factory = NULL;
    IUriRuntimeClass *uri_class = NULL;
    IInspectable *inspectable = NULL;
    IPropertyValue *prop_value;
    const WCHAR *buffer;
    INT32 int32_value;
    HSTRING str, uri;
    unsigned int i;
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = WindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&activation_factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);
    WindowsDeleteString(str);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        RoUninitialize();
        return;
    }

    hr = IActivationFactory_QueryInterface(activation_factory, &IID_IUriRuntimeClassFactory, (void **)&uri_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("%s", wine_dbgstr_w(tests[i].src));

        hr = WindowsCreateString(tests[i].src, wcslen(tests[i].src), &uri);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IUriRuntimeClassFactory_CreateUri(uri_factory, uri, &uri_class);
        if (tests[i].invalid)
        {
            ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
            WindowsDeleteString(uri);
            winetest_pop_context();
            continue;
        }
        else
        {
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        if (i == 0)
        {
            hr = IUriRuntimeClass_QueryInterface(uri_class, &IID_IInspectable, (void **)&inspectable);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(uri_class == (void *)inspectable, "QueryInterface IID_IInspectable returned %p, expected %p.\n",
               inspectable, uri_factory);
            IInspectable_Release(inspectable);

            hr = IUriRuntimeClass_QueryInterface(uri_class, &IID_IPropertyValue, (void **)&prop_value);
            ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
        }

#define TEST_HSTRING(prop)                                                                         \
    winetest_push_context(#prop);                                                                  \
    str = NULL;                                                                                    \
    hr = IUriRuntimeClass_get_##prop(uri_class, &str);                                             \
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);                                               \
    ok(str != uri, "Expected a different pointer.\n");                                             \
    if (tests[i].prop)                                                                             \
    {                                                                                              \
        ok(!!str, "Expected a valid pointer.\n");                                                  \
        buffer = WindowsGetStringRawBuffer(str, NULL);                                             \
        ok(!wcscmp(buffer, tests[i].prop), "Expected %s, got %s.\n", wine_dbgstr_w(tests[i].prop), \
           debugstr_hstring(str));                                                                 \
        WindowsDeleteString(str);                                                                  \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        ok(!str, "Expected a NULL pointer.\n");                                                    \
    }                                                                                              \
    winetest_pop_context();

#define TEST_INT32(prop)                                                                    \
    winetest_push_context(#prop);                                                           \
    hr = IUriRuntimeClass_get_##prop(uri_class, &int32_value);                              \
    if (tests[i].prop == -1)                                                                \
    {                                                                                       \
        ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);                                 \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);                                    \
        ok(int32_value == tests[i].prop, "Expected " #prop " %d, got %d.\n", tests[i].prop, \
           int32_value);                                                                    \
    }                                                                                       \
    winetest_pop_context();

        TEST_HSTRING(AbsoluteUri)
        TEST_HSTRING(DisplayUri)
        TEST_HSTRING(Domain)
        TEST_HSTRING(Extension)
        TEST_HSTRING(Fragment)
        TEST_HSTRING(Host)
        TEST_HSTRING(Password)
        TEST_HSTRING(Path)
        TEST_HSTRING(Query)
        TEST_HSTRING(RawUri)
        TEST_HSTRING(SchemeName)
        TEST_HSTRING(UserName)
        TEST_INT32(Port)

#undef TEST_INT32
#undef TEST_HSTRING

        IUriRuntimeClass_Release(uri_class);
        WindowsDeleteString(uri);
        winetest_pop_context();
    }

    IUriRuntimeClassFactory_Release(uri_factory);
    IActivationFactory_Release(activation_factory);
    RoUninitialize();
}


START_TEST(iertutil)
{
    test_IUriRuntimeClassFactory();
    test_IUriRuntimeClass();
}
