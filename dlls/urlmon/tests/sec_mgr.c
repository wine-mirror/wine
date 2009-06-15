/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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
#define CONST_VTABLE
#define NONAMELESSUNION

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"

#include "initguid.h"

static const WCHAR url1[] = {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',
        '/','b','l','a','n','k','.','h','t','m',0};
static const WCHAR url2[] = {'i','n','d','e','x','.','h','t','m',0};
static const WCHAR url3[] = {'f','i','l','e',':','/','/','c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url4[] = {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e',
        '%','2','e','j','p','g',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g',0};
static const WCHAR url6[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static const WCHAR url7[] = {'f','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g','/',
        'f','i','l','e','.','t','e','s','t',0};
static const WCHAR url8[] = {'t','e','s','t',':','1','2','3','a','b','c',0};
static const WCHAR url9[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',
     '/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR url10[] = {'f','i','l','e',':','/','/','s','o','m','e','%','2','0','f','i','l','e',
        '.','j','p','g',0};

static const WCHAR url4e[] = {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e',
        '.','j','p','g',0};


static const BYTE secid1[] = {'f','i','l','e',':',0,0,0,0};
static const BYTE secid5[] = {'h','t','t','p',':','w','w','w','.','w','i','n','e','h','q',
    '.','o','r','g',3,0,0,0};
static const BYTE secid6[] = {'a','b','o','u','t',':','b','l','a','n','k',3,0,0,0};
static const BYTE secid7[] = {'f','t','p',':','w','i','n','e','h','q','.','o','r','g',
                              3,0,0,0};
static const BYTE secid10[] =
    {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e','.','j','p','g',3,0,0,0};
static const BYTE secid10_2[] =
    {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e','.','j','p','g',3,0,0,0};

static struct secmgr_test {
    LPCWSTR url;
    DWORD zone;
    HRESULT zone_hres;
    DWORD secid_size;
    const BYTE *secid;
    HRESULT secid_hres;
} secmgr_tests[] = {
    {url1, 0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url2, 100, 0x80041001, 0, NULL, E_INVALIDARG},
    {url3, 0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url5, 3,   S_OK, sizeof(secid5), secid5, S_OK},
    {url6, 3,   S_OK, sizeof(secid6), secid6, S_OK},
    {url7, 3,   S_OK, sizeof(secid7), secid7, S_OK}
};

static void test_SecurityManager(void)
{
    int i;
    IInternetSecurityManager *secmgr = NULL;
    BYTE buf[512];
    DWORD zone, size, policy;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i < sizeof(secmgr_tests)/sizeof(secmgr_tests[0]); i++) {
        zone = 100;
        hres = IInternetSecurityManager_MapUrlToZone(secmgr, secmgr_tests[i].url,
                                                     &zone, 0);
        ok(hres == secmgr_tests[i].zone_hres /* IE <=6 */
           || (FAILED(secmgr_tests[i].zone_hres) && hres == E_INVALIDARG), /* IE7 */
           "[%d] MapUrlToZone failed: %08x, expected %08x\n",
                i, hres, secmgr_tests[i].zone_hres);
        if(SUCCEEDED(hres))
            ok(zone == secmgr_tests[i].zone, "[%d] zone=%d, expected %d\n", i, zone,
               secmgr_tests[i].zone);
        else
            ok(zone == secmgr_tests[i].zone || zone == -1, "[%d] zone=%d\n", i, zone);

        size = sizeof(buf);
        memset(buf, 0xf0, sizeof(buf));
        hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[i].url,
                buf, &size, 0);
        ok(hres == secmgr_tests[i].secid_hres,
           "[%d] GetSecurityId failed: %08x, expected %08x\n",
           i, hres, secmgr_tests[i].secid_hres);
        if(secmgr_tests[i].secid) {
            ok(size == secmgr_tests[i].secid_size, "[%d] size=%d, expected %d\n",
                    i, size, secmgr_tests[i].secid_size);
            ok(!memcmp(buf, secmgr_tests[i].secid, size), "[%d] wrong secid\n", i);
        }
    }

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, url10, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08x, expected S_OK\n", hres);
    ok(zone == 3, "zone=%d, expected 3\n", zone);

    /* win2k3 translates %20 into a space */
    size = sizeof(buf);
    memset(buf, 0xf0, sizeof(buf));
    hres = IInternetSecurityManager_GetSecurityId(secmgr, url10, buf, &size, 0);
    ok(hres == S_OK, "GetSecurityId failed: %08x, expected S_OK\n", hres);
    ok(size == sizeof(secid10) ||
       size == sizeof(secid10_2), /* win2k3 */
       "size=%d\n", size);
    ok(!memcmp(buf, secid10, size) ||
       !memcmp(buf, secid10_2, size), /* win2k3 */
       "wrong secid\n");

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, NULL, &zone, 0);
    ok(hres == E_INVALIDARG, "MapUrlToZone failed: %08x, expected E_INVALIDARG\n", hres);
    ok(zone == 100 || zone == -1, "zone=%d\n", zone);

    size = sizeof(buf);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, NULL, buf, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  NULL, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  buf, NULL, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, NULL, URLACTION_SCRIPT_RUN, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == E_INVALIDARG, "ProcessUrlAction failed: %08x, expected E_INVALIDARG\n", hres);

    IInternetSecurityManager_Release(secmgr);
}

/* Check if Internet Explorer is configured to run in "Enhanced Security Configuration" (aka hardened mode) */
/* Note: this code is duplicated in dlls/mshtml/tests/dom.c, dlls/mshtml/tests/script.c and dlls/urlmon/tests/sec_mgr.c */
static BOOL is_ie_hardened(void)
{
    HKEY zone_map;
    DWORD ie_harden, type, size;

    ie_harden = 0;
    if(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap",
                    0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueExA(zone_map, "IEHarden", NULL, &type, (LPBYTE) &ie_harden, &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            ie_harden = 0;
        }
        RegCloseKey(zone_map);
    }

    return ie_harden != 0;
}

static void test_url_action(IInternetSecurityManager *secmgr, IInternetZoneManager *zonemgr, DWORD action)
{
    DWORD res, size, policy, reg_policy;
    char buf[10];
    HKEY hkey;
    HRESULT hres;

    /* FIXME: HKEY_CURRENT_USER is most of the time the default but this can be changed on a system.
     * The test should be changed to cope with that, if need be.
     */
    res = RegOpenKeyA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3", &hkey);
    if(res != ERROR_SUCCESS) {
        ok(0, "Could not open zone key\n");
        return;
    }

    wsprintf(buf, "%X", action);
    size = sizeof(DWORD);
    res = RegQueryValueExA(hkey, buf, NULL, NULL, (BYTE*)&reg_policy, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || size != sizeof(DWORD)) {
        policy = 0xdeadbeef;
        hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                sizeof(WCHAR), NULL, 0, 0, 0);
        ok(hres == E_FAIL, "ProcessUrlAction(%x) failed: %08x, expected E_FAIL\n", action, hres);
        ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);

        policy = 0xdeadbeef;
        hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
                sizeof(DWORD), URLZONEREG_DEFAULT);
        ok(hres == E_FAIL, "GetZoneActionPolicy failed: %08x, expected E_FAIL\n", hres);
        ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);
        return;
    }

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(policy == reg_policy, "(%x) policy=%x, expected %x\n", action, policy, reg_policy);

    if(policy != URLPOLICY_QUERY) {
        if(winetest_interactive || ! is_ie_hardened()) {
            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(WCHAR), NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%x) failed: %08x\n", action, hres);
            ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);
        }else {
            skip("IE running in Enhanced Security Configuration\n");
        }
    }
}

static void test_special_url_action(IInternetSecurityManager *secmgr, IInternetZoneManager *zonemgr, DWORD action)
{
    DWORD policy;
    HRESULT hres;

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(policy == URLPOLICY_DISALLOW, "(%x) policy=%x, expected URLPOLICY_DISALLOW\n", action, policy);

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, action, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
}

static void test_polices(void)
{
    IInternetZoneManager *zonemgr = NULL;
    IInternetSecurityManager *secmgr = NULL;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08x\n", hres);
    hres = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08x\n", hres);

    test_url_action(secmgr, zonemgr, URLACTION_SCRIPT_RUN);
    test_url_action(secmgr, zonemgr, URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY);
    test_url_action(secmgr, zonemgr, URLACTION_CHANNEL_SOFTDIST_PERMISSIONS);
    test_url_action(secmgr, zonemgr, 0xdeadbeef);

    test_special_url_action(secmgr, zonemgr, URLACTION_SCRIPT_OVERRIDE_SAFETY);

    IInternetSecurityManager_Release(secmgr);
    IInternetZoneManager_Release(zonemgr);
}

static void test_ZoneManager(void)
{
    IInternetZoneManager *zonemgr = NULL;
    BYTE buf[32];
    HRESULT hres;
    DWORD action = URLACTION_CREDENTIALS_USE; /* Implemented on all IE versions */

    hres = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(*(DWORD*)buf == URLPOLICY_CREDENTIALS_SILENT_LOGON_OK ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_MUST_PROMPT_USER ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_CONDITIONAL_PROMPT ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_ANONYMOUS_ONLY,
            "unexpected policy=%d\n", *(DWORD*)buf);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, NULL,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, buf,
            2, URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1fff, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_FAIL, "GetZoneActionPolicy failed: %08x, expected E_FAIL\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 13, action, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    IInternetZoneManager_Release(zonemgr);
}



START_TEST(sec_mgr)
{
    OleInitialize(NULL);

    test_SecurityManager();
    test_polices();
    test_ZoneManager();

    OleUninitialize();
}
