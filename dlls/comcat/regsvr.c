/*
 *	self-registerable dll helper functions
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "regsvr.h"

static HRESULT recursive_delete_key(HKEY key);

/***********************************************************************
 *		regsvr_register
 */
HRESULT regsvr_register(struct regsvr_entry const *entries, size_t count)
{
    HKEY keys[count];
    struct regsvr_entry const *e;
    int i;
    HRESULT res = S_OK;

    /* Create keys and set values. */
    for (i = 0, e = entries; i < count; ++i, ++e) {
	/* predefined HKEY_'s are all >= 0x80000000 */
	HKEY parent_key = e->parent < 0x80000000 ?
	    keys[e->parent] : (HKEY)e->parent;
	if (e->value == NULL) {
	    res = RegCreateKeyExW(parent_key, e->name, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL, &keys[i], NULL);
	} else {
	    res = RegSetValueExW(parent_key, e->name, 0, REG_SZ,
				 (CONST BYTE*)e->value,
				 (lstrlenW(e->value) + 1) * sizeof(WCHAR));
	}
	if (res != ERROR_SUCCESS) break;
    }

    /* Close keys. */
    for (--i, --e; 0 <= i; --i, --e) {
	if (e->value == NULL) RegCloseKey(keys[i]);
    }

    return res == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(res);
}

/***********************************************************************
 *		regsvr_unregister
 */
HRESULT regsvr_unregister(struct regsvr_entry const *entries, size_t count)
{
    HKEY keys[count];
    struct regsvr_entry const *e;
    int i;
    HRESULT res = S_OK;

    /* Open (and possibly delete) keys. */
    for (i = 0, e = entries; i < count; ++i, ++e) {
	/* predefined HKEY_'s are all >= 0x80000000 */
	HKEY parent_key = e->parent < 0x80000000 ?
	    keys[e->parent] : (HKEY)e->parent;
	if (e->value == NULL && parent_key) {
	    res = RegOpenKeyExW(parent_key, e->name, 0,
				KEY_READ | KEY_WRITE, &keys[i]);
	    if (res == ERROR_SUCCESS && e->unreg_del)
		res = recursive_delete_key(keys[i]);
	    if (res == ERROR_FILE_NOT_FOUND) continue;
	    if (res != ERROR_SUCCESS) break;
	} else keys[i] = 0;
    }

    /* Close keys. */
    for (--i; 0 <= i; --i) {
	if (keys[i]) RegCloseKey(keys[i]);
    }

    return res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND ?
	HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		recursive_delete_key
 */
static LONG recursive_delete_key(HKEY key)
{
    LONG res;
    DWORD index;
    WCHAR subkey_name[MAX_PATH];
    DWORD cName;
    HKEY subkey;

    for (index = 0; ; ++index) {
	cName = sizeof subkey_name / sizeof(WCHAR);
	res = RegEnumKeyExW(key, index, subkey_name, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) {
	    res = ERROR_SUCCESS; /* presumably we're done enumerating */
	    break;
	}
	res = RegOpenKeyExW(key, subkey_name, 0,
			    KEY_READ | KEY_WRITE, &subkey);
	if (res == ERROR_FILE_NOT_FOUND) continue;
	if (res != ERROR_SUCCESS) break;

	res = recursive_delete_key(subkey);
	RegCloseKey(subkey);
	if (res != ERROR_SUCCESS) break;
    }

    if (res == ERROR_SUCCESS) res = RegDeleteKeyW(key, 0);
    return res;
}
