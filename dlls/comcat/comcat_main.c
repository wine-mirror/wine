/*
 *	exported dll functions for comcat.dll
 *
 * Copyright (C) 2002 John K. Hohm
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "comcat_private.h"
#include "regsvr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

DWORD dll_ref = 0;
HINSTANCE COMCAT_hInstance = 0;

/***********************************************************************
 *		Global string constant definitions
 */
const WCHAR clsid_keyname[6] = { 'C', 'L', 'S', 'I', 'D', 0 };

/***********************************************************************
 *		Registration entries
 */
static const WCHAR class_keyname[39] = {
    '{', '0', '0', '0', '2', 'E', '0', '0', '5', '-', '0', '0',
    '0', '0', '-', '0', '0', '0', '0', '-', 'C', '0', '0', '0',
    '-', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '4',
    '6', '}', 0 };
static const WCHAR class_name[26] = {
    'S', 't', 'd', 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n', 't',
    'C', 'a', 't', 'e', 'g', 'o', 'r', 'i', 'e', 's', 'M', 'g',
    'r', 0 };
static const WCHAR ips32_keyname[15] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    '3', '2', 0 };
static const WCHAR tm_valname[15] = {
    'T', 'h', 'r', 'e', 'a', 'd', 'i', 'n', 'g', 'M', 'o', 'd',
    'e', 'l', 0 };
static const WCHAR tm_value[5] = { 'B', 'o', 't', 'h', 0 };
static struct regsvr_entry regsvr_entries[6] = {
    { (int)HKEY_CLASSES_ROOT, 0, clsid_keyname },
    { 0,                 1, class_keyname },
    { 1,                 1, NULL, class_name },
    { 1,                 1, ips32_keyname },
    { 3,                 1, NULL, /*dynamic, path to dll module*/ },
    { 3,                 1, tm_valname, tm_value }
};

/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        COMCAT_hInstance = hinstDLL;
	break;

    case DLL_PROCESS_DETACH:
        COMCAT_hInstance = 0;
	break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject (COMCAT.@)
 */
HRESULT WINAPI COMCAT_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualGUID(rclsid, &CLSID_StdComponentCategoriesMgr)) {
	return IClassFactory_QueryInterface((LPCLASSFACTORY)&COMCAT_ClassFactory, iid, ppv);
    }
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (COMCAT.@)
 */
HRESULT WINAPI COMCAT_DllCanUnloadNow()
{
    return dll_ref != 0 ? S_FALSE : S_OK;
}

/***********************************************************************
 *		DllRegisterServer (COMCAT.@)
 */
HRESULT WINAPI COMCAT_DllRegisterServer()
{
    WCHAR dll_module[MAX_PATH];

    TRACE("\n");

    if (!GetModuleFileNameW(COMCAT_hInstance, dll_module,
			    sizeof dll_module / sizeof(WCHAR)))
	return HRESULT_FROM_WIN32(GetLastError());

    regsvr_entries[4].value = dll_module;
    return regsvr_register(regsvr_entries, 6);
}

/***********************************************************************
 *		DllUnregisterServer (COMCAT.@)
 */
HRESULT WINAPI COMCAT_DllUnregisterServer()
{
    WCHAR dll_module[MAX_PATH];

    TRACE("\n");

    if (!GetModuleFileNameW(COMCAT_hInstance, dll_module,
			    sizeof dll_module / sizeof(WCHAR)))
	return HRESULT_FROM_WIN32(GetLastError());

    regsvr_entries[4].value = dll_module;
    return regsvr_unregister(regsvr_entries, 6);
}
