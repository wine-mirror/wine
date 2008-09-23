/*
 * Copyright 2008 Hans Leidekker
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
#include "winhttp.h"

#include "wine/test.h"

static WCHAR empty[]    = {0};
static WCHAR ftp[]      = {'f','t','p',0};
static WCHAR http[]     = {'h','t','t','p',0};
static WCHAR winehq[]   = {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static WCHAR username[] = {'u','s','e','r','n','a','m','e',0};
static WCHAR password[] = {'p','a','s','s','w','o','r','d',0};
static WCHAR about[]    = {'/','s','i','t','e','/','a','b','o','u','t',0};
static WCHAR query[]    = {'?','q','u','e','r','y',0};
static WCHAR escape[]   = {' ','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^','_','`','{','|','}','~',0};

static const WCHAR url1[]  =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url2[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url3[] = {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':',0};
static const WCHAR url4[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/',0};
static const WCHAR url6[] =
    {'f','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','8','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url7[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','4','2','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url8[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t',
     '%','2','0','!','%','2','2','%','2','3','$','%','2','5','&','\'','(',')','*','+',',','-','.','/',':',';','%','3','C','=','%','3','E','?','@','%',
     '5','B','%','5','C','%','5','D','%','5','E','_','%','6','0','%','7','B','%','7','C','%','7','D','%','7','E',0};
static const WCHAR url9[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};

static void fill_url_components( URL_COMPONENTS *uc )
{
    uc->dwStructSize = sizeof(URL_COMPONENTS);
    uc->lpszScheme = http;
    uc->dwSchemeLength = lstrlenW( uc->lpszScheme );
    uc->nScheme = INTERNET_SCHEME_HTTP;
    uc->lpszHostName = winehq;
    uc->dwHostNameLength = lstrlenW( uc->lpszHostName );
    uc->nPort = 80;
    uc->lpszUserName = username;
    uc->dwUserNameLength = lstrlenW( uc->lpszUserName );
    uc->lpszPassword = password;
    uc->dwPasswordLength = lstrlenW( uc->lpszPassword );
    uc->lpszUrlPath = about;
    uc->dwUrlPathLength = lstrlenW( uc->lpszUrlPath );
    uc->lpszExtraInfo = query;
    uc->dwExtraInfoLength = lstrlenW( uc->lpszExtraInfo );
}

static void WinHttpCreateUrl_test( void )
{
    URL_COMPONENTS uc;
    WCHAR *url;
    DWORD len;
    BOOL ret;

    /* NULL components */
    len = ~0UL;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( NULL, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0UL, "expected len ~0UL got %u\n", len );

    /* zero'ed components */
    memset( &uc, 0, sizeof(URL_COMPONENTS) );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0UL, "expected len ~0UL got %u\n", len );

    /* valid components, NULL url, NULL length */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, NULL );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0UL, "expected len ~0UL got %u\n", len );

    /* valid components, NULL url */
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* correct size, NULL url */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* valid components, allocated url, short length */
    SetLastError( 0xdeadbeef );
    url = HeapAlloc( GetProcessHeap(), 0, 256 * sizeof(WCHAR) );
    url[0] = 0;
    len = 2;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* allocated url, NULL scheme */
    uc.lpszScheme = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url1 ), "url doesn't match\n" );

    /* allocated url, 0 scheme */
    fill_url_components( &uc );
    uc.nScheme = 0;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );

    /* valid components, allocated url */
    fill_url_components( &uc );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %d\n", len );
    ok( !lstrcmpW( url, url1 ), "url doesn't match\n" );

    /* valid username, NULL password */
    fill_url_components( &uc );
    uc.lpszPassword = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );

    /* valid username, empty password */
    fill_url_components( &uc );
    uc.lpszPassword = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url3 ), "url doesn't match\n" );

    /* valid password, NULL username */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    uc.lpszUserName = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );

    /* valid password, empty username */
    fill_url_components( &uc );
    uc.lpszUserName = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n");

    /* NULL username, NULL password */
    fill_url_components( &uc );
    uc.lpszUserName = NULL;
    uc.lpszPassword = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 38, "expected len 38 got %u\n", len );
    ok( !lstrcmpW( url, url4 ), "url doesn't match\n" );

    /* empty username, empty password */
    fill_url_components( &uc );
    uc.lpszUserName = empty;
    uc.lpszPassword = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url5 ), "url doesn't match\n" );

    /* nScheme has lower precedence than lpszScheme */
    fill_url_components( &uc );
    uc.lpszScheme = ftp;
    uc.dwSchemeLength = lstrlenW( uc.lpszScheme );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == lstrlenW( url6 ), "expected len %d got %u\n", lstrlenW( url6 ) + 1, len );
    ok( !lstrcmpW( url, url6 ), "url doesn't match\n" );

    /* non-standard port */
    uc.lpszScheme = http;
    uc.dwSchemeLength = lstrlenW( uc.lpszScheme );
    uc.nPort = 42;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 59, "expected len 59 got %u\n", len );
    ok( !lstrcmpW( url, url7 ), "url doesn't match\n" );

    /* escape extra info */
    fill_url_components( &uc );
    uc.lpszExtraInfo = escape;
    uc.dwExtraInfoLength = lstrlenW( uc.lpszExtraInfo );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 113, "expected len 113 got %u\n", len );
    ok( !lstrcmpW( url, url8 ), "url doesn't match\n" );

    /* NULL lpszScheme, 0 nScheme and nPort */
    fill_url_components( &uc );
    uc.lpszScheme = NULL;
    uc.dwSchemeLength = 0;
    uc.nScheme = 0;
    uc.nPort = 0;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 58, "expected len 58 got %u\n", len );
    ok( !lstrcmpW( url, url9 ), "url doesn't match\n" );

    HeapFree( GetProcessHeap(), 0, url );
}

START_TEST(url)
{
    WinHttpCreateUrl_test();
}
