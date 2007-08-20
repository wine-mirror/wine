/*
 *	self-registerable dll functions for shdocvw.dll
 *
 * Copyright (C) 2003 John K. Hohm
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "shdocvw.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/*
 * Near the bottom of this file are the exported DllRegisterServer and
 * DllUnregisterServer, which make all this worthwhile.
 */

/***********************************************************************
 *		interface for self-registering
 */
struct regsvr_interface
{
    IID const *iid;		/* NULL for end of list */
    LPCSTR name;		/* can be NULL to omit */
    IID const *base_iid;	/* can be NULL to omit */
    int num_methods;		/* can be <0 to omit */
    CLSID const *ps_clsid;	/* can be NULL to omit */
    CLSID const *ps_clsid32;	/* can be NULL to omit */
};

static HRESULT register_interfaces(struct regsvr_interface const *list);
static HRESULT unregister_interfaces(struct regsvr_interface const *list);

struct regsvr_coclass
{
    CLSID const *clsid;		/* NULL for end of list */
    LPCSTR name;		/* can be NULL to omit */
    LPCSTR ips;			/* can be NULL to omit */
    LPCSTR ips32;		/* can be NULL to omit */
    LPCSTR ips32_tmodel;	/* can be NULL to omit */
    LPCSTR progid;		/* can be NULL to omit */
    LPCSTR viprogid;		/* can be NULL to omit */
    LPCSTR progid_extra;	/* can be NULL to omit */
    DWORD flags;
    DWORD dwAttributes;
    DWORD dwCallForAttributes;
};

/* flags for regsvr_coclass.flags */
#define SHELLEX_MAYCHANGEDEFAULTMENU  0x00000001
#define SHELLFOLDER_WANTSFORPARSING   0x00000002
#define SHELLFOLDER_ATTRIBUTES        0x00000004
#define SHELLFOLDER_CALLFORATTRIBUTES 0x00000008

static HRESULT register_coclasses(struct regsvr_coclass const *list);
static HRESULT unregister_coclasses(struct regsvr_coclass const *list);

/***********************************************************************
 *		static string constants
 */
static WCHAR const interface_keyname[10] = {
    'I', 'n', 't', 'e', 'r', 'f', 'a', 'c', 'e', 0 };
static WCHAR const base_ifa_keyname[14] = {
    'B', 'a', 's', 'e', 'I', 'n', 't', 'e', 'r', 'f', 'a', 'c',
    'e', 0 };
static WCHAR const num_methods_keyname[11] = {
    'N', 'u', 'm', 'M', 'e', 't', 'h', 'o', 'd', 's', 0 };
static WCHAR const ps_clsid_keyname[15] = {
    'P', 'r', 'o', 'x', 'y', 'S', 't', 'u', 'b', 'C', 'l', 's',
    'i', 'd', 0 };
static WCHAR const ps_clsid32_keyname[17] = {
    'P', 'r', 'o', 'x', 'y', 'S', 't', 'u', 'b', 'C', 'l', 's',
    'i', 'd', '3', '2', 0 };
static WCHAR const clsid_keyname[6] = {
    'C', 'L', 'S', 'I', 'D', 0 };
static WCHAR const curver_keyname[7] = {
    'C', 'u', 'r', 'V', 'e', 'r', 0 };
static WCHAR const ips_keyname[13] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    0 };
static WCHAR const ips32_keyname[15] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    '3', '2', 0 };
static WCHAR const progid_keyname[7] = {
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const viprogid_keyname[25] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 'I', 'n', 'd', 'e', 'p',
    'e', 'n', 'd', 'e', 'n', 't', 'P', 'r', 'o', 'g', 'I', 'D',
    0 };
static WCHAR const shellex_keyname[8] = {
    's', 'h', 'e', 'l', 'l', 'e', 'x', 0 };
static WCHAR const shellfolder_keyname[12] = {
    'S', 'h', 'e', 'l', 'l', 'F', 'o', 'l', 'd', 'e', 'r', 0 };
static WCHAR const mcdm_keyname[21] = {
    'M', 'a', 'y', 'C', 'h', 'a', 'n', 'g', 'e', 'D', 'e', 'f',
    'a', 'u', 'l', 't', 'M', 'e', 'n', 'u', 0 };
static char const tmodel_valuename[] = "ThreadingModel";
static char const wfparsing_valuename[] = "WantsFORPARSING";
static char const attributes_valuename[] = "Attributes";
static char const cfattributes_valuename[] = "CallForAttributes";
static WCHAR const lcs32_keyname[] = {
    'L','o','c','a','l','S','e','r','v','e','r','3','2',0 };
static const WCHAR szIERelPath[] = {
    'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
    'i','e','x','p','l','o','r','e','.','e','x','e',0 };

/***********************************************************************
 *		static helper functions
 */
static LONG register_key_guid(HKEY base, WCHAR const *name, GUID const *guid);
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
				   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
				   char const *value);
static LONG register_progid(WCHAR const *clsid,
			    char const *progid, char const *curver_progid,
			    char const *name, char const *extra);

/***********************************************************************
 *		register_interfaces
 */
static HRESULT register_interfaces(struct regsvr_interface const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY interface_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, interface_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &interface_key, NULL);
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->iid; ++list) {
	WCHAR buf[39];
	HKEY iid_key;

	StringFromGUID2(list->iid, buf, 39);
	res = RegCreateKeyExW(interface_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &iid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_interface_key;

	if (list->name) {
	    res = RegSetValueExA(iid_key, NULL, 0, REG_SZ,
				 (CONST BYTE*)(list->name),
				 strlen(list->name) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->base_iid) {
	    res = register_key_guid(iid_key, base_ifa_keyname, list->base_iid);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (0 <= list->num_methods) {
	    static WCHAR const fmt[3] = { '%', 'd', 0 };
	    HKEY key;

	    res = RegCreateKeyExW(iid_key, num_methods_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL, &key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;

	    wsprintfW(buf, fmt, list->num_methods);
	    res = RegSetValueExW(key, NULL, 0, REG_SZ,
				 (CONST BYTE*)buf,
				 (lstrlenW(buf) + 1) * sizeof(WCHAR));
	    RegCloseKey(key);

	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->ps_clsid) {
	    res = register_key_guid(iid_key, ps_clsid_keyname, list->ps_clsid);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->ps_clsid32) {
	    res = register_key_guid(iid_key, ps_clsid32_keyname, list->ps_clsid32);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

    error_close_iid_key:
	RegCloseKey(iid_key);
    }

error_close_interface_key:
    RegCloseKey(interface_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_interfaces
 */
static HRESULT unregister_interfaces(struct regsvr_interface const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY interface_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, interface_keyname, 0,
			KEY_READ | KEY_WRITE, &interface_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->iid; ++list) {
	WCHAR buf[39];

	StringFromGUID2(list->iid, buf, 39);
	res = RegDeleteTreeW(interface_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
    }

    RegCloseKey(interface_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_coclasses
 */
static HRESULT register_coclasses(struct regsvr_coclass const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	WCHAR buf[39];
	HKEY clsid_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	if (list->name) {
	    res = RegSetValueExA(clsid_key, NULL, 0, REG_SZ,
				 (CONST BYTE*)(list->name),
				 strlen(list->name) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->ips) {
	    res = register_key_defvalueA(clsid_key, ips_keyname, list->ips);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->ips32) {
	    HKEY ips32_key;

	    res = RegCreateKeyExW(clsid_key, ips32_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL,
				  &ips32_key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    res = RegSetValueExA(ips32_key, NULL, 0, REG_SZ,
				 (CONST BYTE*)list->ips32,
				 lstrlenA(list->ips32) + 1);
	    if (res == ERROR_SUCCESS && list->ips32_tmodel)
		res = RegSetValueExA(ips32_key, tmodel_valuename, 0, REG_SZ,
				     (CONST BYTE*)list->ips32_tmodel,
				     strlen(list->ips32_tmodel) + 1);
	    RegCloseKey(ips32_key);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->flags & SHELLEX_MAYCHANGEDEFAULTMENU) {
	    HKEY shellex_key, mcdm_key;

	    res = RegCreateKeyExW(clsid_key, shellex_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL,
				  &shellex_key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	    res = RegCreateKeyExW(shellex_key, mcdm_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL,
				  &mcdm_key, NULL);
	    RegCloseKey(shellex_key);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	    RegCloseKey(mcdm_key);
	}

	if (list->flags & 
		(SHELLFOLDER_WANTSFORPARSING|SHELLFOLDER_ATTRIBUTES|SHELLFOLDER_CALLFORATTRIBUTES))
	{
	    HKEY shellfolder_key;

	    res = RegCreateKeyExW(clsid_key, shellfolder_keyname, 0, NULL, 0,
			     	  KEY_READ | KEY_WRITE, NULL,
				  &shellfolder_key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	    if (list->flags & SHELLFOLDER_WANTSFORPARSING)
		res = RegSetValueExA(shellfolder_key, wfparsing_valuename, 0, REG_SZ, (const BYTE *)"", 1);
	    if (list->flags & SHELLFOLDER_ATTRIBUTES) 
		res = RegSetValueExA(shellfolder_key, attributes_valuename, 0, REG_DWORD, 
				     (const BYTE *)&list->dwAttributes, sizeof(DWORD));
	    if (list->flags & SHELLFOLDER_CALLFORATTRIBUTES) 
		res = RegSetValueExA(shellfolder_key, cfattributes_valuename, 0, REG_DWORD,
				     (const BYTE *)&list->dwCallForAttributes, sizeof(DWORD));
	    RegCloseKey(shellfolder_key);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->progid) {
	    res = register_key_defvalueA(clsid_key, progid_keyname,
					 list->progid);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    res = register_progid(buf, list->progid, NULL,
				  list->name, list->progid_extra);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->viprogid) {
	    res = register_key_defvalueA(clsid_key, viprogid_keyname,
					 list->viprogid);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    res = register_progid(buf, list->viprogid, list->progid,
				  list->name, list->progid_extra);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

    error_close_clsid_key:
	RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_coclasses
 */
static HRESULT unregister_coclasses(struct regsvr_coclass const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
			KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	WCHAR buf[39];

	StringFromGUID2(list->clsid, buf, 39);
	res = RegDeleteTreeW(coclass_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	if (list->progid) {
	    res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->progid);
	    if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	    if (res != ERROR_SUCCESS) goto error_close_coclass_key;
	}

	if (list->viprogid) {
	    res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->viprogid);
	    if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	    if (res != ERROR_SUCCESS) goto error_close_coclass_key;
	}
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		regsvr_key_guid
 */
static LONG register_key_guid(HKEY base, WCHAR const *name, GUID const *guid)
{
    WCHAR buf[39];

    StringFromGUID2(guid, buf, 39);
    return register_key_defvalueW(base, name, buf);
}

/***********************************************************************
 *		regsvr_key_defvalueW
 */
static LONG register_key_defvalueW(
    HKEY base,
    WCHAR const *name,
    WCHAR const *value)
{
    LONG res;
    HKEY key;

    res = RegCreateKeyExW(base, name, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &key, NULL);
    if (res != ERROR_SUCCESS) return res;
    res = RegSetValueExW(key, NULL, 0, REG_SZ, (CONST BYTE*)value,
			 (lstrlenW(value) + 1) * sizeof(WCHAR));
    RegCloseKey(key);
    return res;
}

/***********************************************************************
 *		regsvr_key_defvalueA
 */
static LONG register_key_defvalueA(
    HKEY base,
    WCHAR const *name,
    char const *value)
{
    LONG res;
    HKEY key;

    res = RegCreateKeyExW(base, name, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &key, NULL);
    if (res != ERROR_SUCCESS) return res;
    res = RegSetValueExA(key, NULL, 0, REG_SZ, (CONST BYTE*)value,
			 lstrlenA(value) + 1);
    RegCloseKey(key);
    return res;
}

/***********************************************************************
 *		regsvr_progid
 */
static LONG register_progid(
    WCHAR const *clsid,
    char const *progid,
    char const *curver_progid,
    char const *name,
    char const *extra)
{
    LONG res;
    HKEY progid_key;

    res = RegCreateKeyExA(HKEY_CLASSES_ROOT, progid, 0,
			  NULL, 0, KEY_READ | KEY_WRITE, NULL,
			  &progid_key, NULL);
    if (res != ERROR_SUCCESS) return res;

    if (name) {
	res = RegSetValueExA(progid_key, NULL, 0, REG_SZ,
			     (CONST BYTE*)name, strlen(name) + 1);
	if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (clsid) {
	res = register_key_defvalueW(progid_key, clsid_keyname, clsid);
	if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (curver_progid) {
	res = register_key_defvalueA(progid_key, curver_keyname,
				     curver_progid);
	if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (extra) {
	HKEY extra_key;

	res = RegCreateKeyExA(progid_key, extra, 0,
			      NULL, 0, KEY_READ | KEY_WRITE, NULL,
			      &extra_key, NULL);
	if (res == ERROR_SUCCESS)
	    RegCloseKey(extra_key);
    }

error_close_progid_key:
    RegCloseKey(progid_key);
    return res;
}

static const GUID CLSID_MicrosoftBrowserArchitecture =
{0xa5e46e3a, 0x8849, 0x11d1, {0x9d, 0x8c, 0x00, 0xc0, 0x4f, 0xc9, 0x9d, 0x61}};
static const GUID CLSID_MruLongList =
{0x53bd6b4e, 0x3780, 0x4693, {0xaf, 0xc3, 0x71, 0x61, 0xc2, 0xf3, 0xee, 0x9c}};

/***********************************************************************
 *		coclass list
 */
static struct regsvr_coclass const coclass_list[] = {
    {   &CLSID_WebBrowser_V1,
        "Microsoft Web Browser Version 1",
        NULL,
        "shdocvw.dll",
        "Apartment",
        "Shell.Explorer.1",
        "Shell.Explorer"
    },
    {   &CLSID_WebBrowser,
	"Microsoft Web Browser",
	NULL,
	"shdocvw.dll",
	"Apartment",
	"Shell.Explorer.2",
	"Shell.Explorer"
    },
    {   &CLSID_InternetExplorer,
        "Internet Explorer(Ver 1.0)",
        NULL,
        NULL,
        NULL,
        "InternetExplorer.Application.1",
        "InternetExplorer.Application"
    },
    {   &CLSID_ShellSearchAssistantOC,
        "SearchAssistantOC",
        NULL,
        "shdocvw.dll",
        "Apartment",
        "SearchAssistantOC.SearchAssistantOC.1",
        "SearchAssistantOC.SearchAssistantOC"
    },
    {   &CLSID_ShellShellNameSpace,
        "Shell Name Space",
        NULL,
        "shdocvw.dll",
        "Apartment",
        "ShellNameSpace.ShellNameSpace.1",
        "ShellNameSpace.ShellNameSpace"
    },
    {   &CLSID_ShellNameSpace,
        "Shell Name Space",
        NULL,
        "shdocvw.dll",
        "Apartment",
        "ShellNameSpace.ShellNameSpace.1",
        "ShellNameSpace.ShellNameSpace"
    },
    {   &CLSID_ShellUIHelper,
        "Microsoft Shell UI Helper",
        NULL,
        "shdocvw.dll",
        "Apartment",
        "Shell.UIHelper.1",
        NULL
    },
    {   &CLSID_ShellWindows,
        "ShellWindows",
        NULL,
        "shdocvw.dll",
        "Apartment",
        NULL,
        NULL
    },
    {   &CLSID_SearchAssistantOC,
        "SearchAssistantOC",
        NULL,
        "shdocvw.dll",
        "Apartment",
        "SearchAssistantOC.SearchAssistantOC.1",
        "SearchAssistantOC.SearchAssistantOC"
    },
    {
        &CLSID_MicrosoftBrowserArchitecture,
        "Microsoft Browser Architecture",
        NULL,
        "shdocvw.dll",
        "Apartment",
        NULL,
        NULL
    },
    {
        &CLSID_MruLongList,
        "MruLongList",
        NULL,
        "shdocvw.dll",
        "Apartment",
        NULL,
        NULL
    },
    {
        &CLSID_CUrlHistory,
        "Microsoft Url History Service",
        NULL,
        "shdocvw.dll",
        "Apartment",
        NULL,
        NULL
    },
    {
        &CLSID_Internet,
        "Internet Explorer",
        NULL,
        "shdocvw.dll",
        "Apartment",
        NULL,
        NULL,
        NULL,
        SHELLFOLDER_ATTRIBUTES,
        SFGAO_CANDELETE|SFGAO_CANLINK
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		interface list
 */

static struct regsvr_interface const interface_list[] = {
    { NULL }			/* list terminator */
};

static HRESULT register_localserver(void)
{
    HKEY coclass_key = 0, clsid_key = 0;
    WCHAR buf[39], path[MAX_PATH];
    LONG res;
    UINT len;

    res = SHGetFolderPathW(NULL, CSIDL_FLAG_CREATE|CSIDL_PROGRAM_FILES,
                           NULL, SHGFP_TYPE_CURRENT, path);
    if (FAILED(res))
        return res;

    len = lstrlenW(path);
    if ((len + (sizeof szIERelPath/sizeof(WCHAR)) + 1) > MAX_PATH)
        return E_FAIL;

    path[len] = '\\';
    lstrcpyW(&path[len+1], szIERelPath);

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res != ERROR_SUCCESS) goto err;

    StringFromGUID2(&CLSID_InternetExplorer, buf, 39);
    res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
    if (res != ERROR_SUCCESS) goto err;

    res = register_key_defvalueW(clsid_key, lcs32_keyname, path);
    if (res != ERROR_SUCCESS) goto err;

err:
    if (clsid_key) RegCloseKey(clsid_key);
    if (coclass_key) RegCloseKey(coclass_key);
    return (res == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

static HRESULT register_typelib(void)
{
    ITypeLib *typelib;
    HRESULT hres;

    static const WCHAR wszSHDocVw[] = {'s','h','d','o','c','v','w','.','d','l','l',0};

    hres = LoadTypeLibEx(wszSHDocVw, REGKIND_REGISTER, &typelib);
    if(FAILED(hres)) {
        ERR("Could not load typelib: %08x\n", hres);
        return hres;
    }

    ITypeLib_Release(typelib);

    return hres;
}

static HRESULT unregister_typelib(void)
{
    return UnRegisterTypeLib(&LIBID_SHDocVw, 1, 1, LOCALE_SYSTEM_DEFAULT, SYS_WIN32);
}

/***********************************************************************
 *		DllRegisterServer (SHDOCVW.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = register_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = register_interfaces(interface_list);
    if (SUCCEEDED(hr))
        hr = register_localserver();
    if(SUCCEEDED(hr))
        hr = register_typelib();

    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (SHDOCVW.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = unregister_interfaces(interface_list);
    if(SUCCEEDED(hr))
        hr = unregister_typelib();

    return hr;
}
