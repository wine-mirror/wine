/*
 *	MSXML6 self-registerable dll functions
 *
 * Copyright (C) 2003 John K. Hohm
 * Copyright (C) 2006 Robert Shearman
 * Copyright (C) 2008 Alistair Leslie-Hughes
 * Copyright (C) 2010 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "ole2.h"

#include "initguid.h"
#include "msxml2.h"
#include "msxml6.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml6);

/*
 * Near the bottom of this file are the exported DllRegisterServer and
 * DllUnregisterServer, which make all this worthwhile.
 */

struct regsvr_coclass
{
    CLSID const *clsid;		/* NULL for end of list */
    LPCSTR name;		/* can be NULL to omit */
    LPCSTR ips;			/* can be NULL to omit */
    LPCSTR ips32;		/* can be NULL to omit */
    LPCSTR ips32_tmodel;	/* can be NULL to omit */
    LPCSTR progid;		/* can be NULL to omit */
    LPCSTR version;		/* can be NULL to omit */
};

static HRESULT register_coclasses(struct regsvr_coclass const *list);
static HRESULT unregister_coclasses(struct regsvr_coclass const *list);

struct progid
{
    LPCSTR name;		/* NULL for end of list */
    LPCSTR description;		/* can be NULL to omit */
    CLSID const *clsid;
    LPCSTR curver;		/* can be NULL to omit */
};

static HRESULT register_progids(struct progid const *list);
static HRESULT unregister_progids(struct progid const *list);

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
static WCHAR const ips_keyname[13] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    0 };
static WCHAR const ips32_keyname[15] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    '3', '2', 0 };
static WCHAR const progid_keyname[7] = {
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const versionindependentprogid_keyname[] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n',
    'I', 'n', 'd', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 't',
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const version_keyname[] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 0 };
static WCHAR const curver_keyname[] = {
    'C', 'u', 'r', 'V', 'e', 'r', 0 };
static char const tmodel_valuename[] = "ThreadingModel";

/***********************************************************************
 *		static helper functions
 */
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
				   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
				   char const *value);

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

	if (list->progid) {
            char *buffer = NULL;
            LPCSTR progid;

	    if (list->version) {
		buffer = HeapAlloc(GetProcessHeap(), 0, strlen(list->progid) + strlen(list->version) + 2);
		if (!buffer) {
		    res = ERROR_OUTOFMEMORY;
		    goto error_close_clsid_key;
		}
		strcpy(buffer, list->progid);
		strcat(buffer, ".");
		strcat(buffer, list->version);
		progid = buffer;
	    } else
		progid = list->progid;
	    res = register_key_defvalueA(clsid_key, progid_keyname,
					 progid);
	    HeapFree(GetProcessHeap(), 0, buffer);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    if (list->version) {
	        res = register_key_defvalueA(clsid_key, versionindependentprogid_keyname,
					     list->progid);
		if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	    }
	}

	if (list->version) {
	    res = register_key_defvalueA(clsid_key, version_keyname,
					 list->version);
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
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_progids
 */
static HRESULT register_progids(struct progid const *list)
{
    LONG res = ERROR_SUCCESS;

    for (; res == ERROR_SUCCESS && list->name; ++list) {
	WCHAR buf[39];
	HKEY progid_key;

	res = RegCreateKeyExA(HKEY_CLASSES_ROOT, list->name, 0,
			  NULL, 0, KEY_READ | KEY_WRITE, NULL,
			  &progid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	res = RegSetValueExA(progid_key, NULL, 0, REG_SZ,
			 (CONST BYTE*)list->description,
			 strlen(list->description) + 1);
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	StringFromGUID2(list->clsid, buf, 39);

	res = register_key_defvalueW(progid_key, clsid_keyname, buf);
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	if (list->curver) {
	    res = register_key_defvalueA(progid_key, curver_keyname, list->curver);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

    error_close_clsid_key:
	RegCloseKey(progid_key);
    }

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_progids
 */
static HRESULT unregister_progids(struct progid const *list)
{
    LONG res = ERROR_SUCCESS;

    for (; res == ERROR_SUCCESS && list->name; ++list) {
	res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->name);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
    }

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
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
 *		coclass list
 */
static struct regsvr_coclass const coclass_list[] = {
    {   &CLSID_DOMDocument60,
	"XML DOM Document 6.0",
	NULL,
	"msxml6.dll",
	"Both",
	"Msxml2.DOMDocument",
	"6.0"
    },
    {   &CLSID_XMLHTTP60,
	"XML HTTP 6.0",
	NULL,
	"msxml6.dll",
	"Apartment",
	"Msxml2.XMLHTTP.6.0",
	"6.0"
    },
    {   &CLSID_XMLSchemaCache60,
    "XML Schema Cache 6.0",
    NULL,
    "msxml6.dll",
    "Both",
    "Msxml2.XMLSchemaCache",
    "6.0"
    },
    {   &CLSID_MXXMLWriter60,
    "IMXWriter interface 6.0",
    NULL,
    "msxml6.dll",
    "Both",
    "Msxml2.MXXMLWriter",
    "6.0"
    },
    {   &CLSID_SAXXMLReader60,
        "SAX XML Reader 6.0",
        NULL,
        "msxml6.dll",
        "Both",
        "Msxml2.SAXXMLReader.6.0",
        "6.0"
    },
    {   &CLSID_SAXAttributes60,
    "SAX Attribute 6.0",
    NULL,
    "msxml6.dll",
    "Both",
    "Msxml2.SAXAttributes",
    "6.0"
    },
    {   &CLSID_FreeThreadedDOMDocument60,
    "Free Threaded XML DOM Document 6.0",
    NULL,
    "msxml6.dll",
    "Both",
    "Microsoft.FreeThreadedDOMDocument6.0",
    "6.0"
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		progid list
 */
static struct progid const progid_list[] = {
    {   "Msxml2.DOMDocument.6.0",
	"XML DOM Document 6.0",
	&CLSID_DOMDocument60,
	NULL
    },
    {   "Msxml2.XMLHTTP.6.0",
	"XML XMLHTTP 6.0",
	&CLSID_XMLHTTP60,
	NULL
    },
    {   "Msxml2.XMLSchemaCache.6.0",
    "XML Schema Cache 6.0",
    &CLSID_XMLSchemaCache60,
    NULL
    },
    {   "Msxml2.MXXMLWriter.6.0",
    "MXXMLWriter 6.0",
    &CLSID_MXXMLWriter60,
    NULL
    },
    {   "Msxml2.SAXAttributes.6.0",
    "SAX Attribute 6.0",
    &CLSID_SAXAttributes60,
    NULL
    },
    {   "MSXML.FreeThreadedDOMDocument60",
    "Free threaded XML DOM Document 6.0",
    &CLSID_FreeThreadedDOMDocument60,
    NULL
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		DllRegisterServer (MSXML4.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = register_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = register_progids(progid_list);

    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (MSXML4.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = unregister_progids(progid_list);

    return hr;
}
