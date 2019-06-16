/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

struct USKEY
{
    HKEY  HKCUstart; /* Start key in CU hive */
    HKEY  HKCUkey;   /* Opened key in CU hive */
    HKEY  HKLMstart; /* Start key in LM hive */
    HKEY  HKLMkey;   /* Opened key in LM hive */
    WCHAR path[MAX_PATH];
};

LONG WINAPI SHRegCreateUSKeyA(LPCSTR path, REGSAM samDesired, HUSKEY relative_key, PHUSKEY new_uskey, DWORD flags)
{
    WCHAR *pathW;
    LONG ret;

    TRACE("%s, %#x, %p, %p, %#x\n", debugstr_a(path), samDesired, relative_key, new_uskey, flags);

    if (path)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        pathW = heap_alloc(len * sizeof(WCHAR));
        if (!pathW)
            return ERROR_NOT_ENOUGH_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);
    }
    else
        pathW = NULL;

    ret = SHRegCreateUSKeyW(pathW, samDesired, relative_key, new_uskey, flags);
    HeapFree(GetProcessHeap(), 0, pathW);
    return ret;
}

static HKEY reg_duplicate_hkey(HKEY hKey)
{
    HKEY newKey = 0;

    RegOpenKeyExW(hKey, 0, 0, MAXIMUM_ALLOWED, &newKey);
    return newKey;
}

static HKEY reg_get_hkey_from_huskey(HUSKEY hUSKey, BOOL is_hkcu)
{
    struct USKEY *mihk = hUSKey;
    HKEY test = hUSKey;

    if (test == HKEY_CLASSES_ROOT
            || test == HKEY_CURRENT_CONFIG
            || test == HKEY_CURRENT_USER
            || test == HKEY_DYN_DATA
            || test == HKEY_LOCAL_MACHINE
            || test == HKEY_PERFORMANCE_DATA
            || test == HKEY_USERS)
/* FIXME:  need to define for Win2k, ME, XP
 *    (test == HKEY_PERFORMANCE_TEXT)    ||
 *    (test == HKEY_PERFORMANCE_NLSTEXT) ||
 */
    {
        return test;
    }

    return is_hkcu ? mihk->HKCUkey : mihk->HKLMkey;
}

LONG WINAPI SHRegCreateUSKeyW(const WCHAR *path, REGSAM samDesired, HUSKEY relative_key, PHUSKEY new_uskey, DWORD flags)
{
    LONG ret = ERROR_CALL_NOT_IMPLEMENTED;
    struct USKEY *ret_key;

    TRACE("%s, %#x, %p, %p, %#x\n", debugstr_w(path), samDesired, relative_key, new_uskey, flags);

    if (!new_uskey)
        return ERROR_INVALID_PARAMETER;

    *new_uskey = NULL;

    if (flags & ~SHREGSET_FORCE_HKCU)
    {
        FIXME("unsupported flags 0x%08x\n", flags);
        return ERROR_SUCCESS;
    }

    ret_key = heap_alloc_zero(sizeof(*ret_key));
    lstrcpynW(ret_key->path, path, ARRAY_SIZE(ret_key->path));

    if (relative_key)
    {
        ret_key->HKCUstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, TRUE));
        ret_key->HKLMstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, FALSE));
    }
    else
    {
        ret_key->HKCUstart = HKEY_CURRENT_USER;
        ret_key->HKLMstart = HKEY_LOCAL_MACHINE;
    }

    if (flags & SHREGSET_FORCE_HKCU)
    {
        ret = RegCreateKeyExW(ret_key->HKCUstart, path, 0, NULL, 0, samDesired, NULL, &ret_key->HKCUkey, NULL);
        if (ret == ERROR_SUCCESS)
            *new_uskey = ret_key;
        else
            heap_free(ret_key);
    }

    return ret;
}

LONG WINAPI SHRegCloseUSKey(HUSKEY hUSKey)
{
    struct USKEY *key = hUSKey;
    LONG ret = ERROR_SUCCESS;

    if (!key)
        return ERROR_INVALID_PARAMETER;

    if (key->HKCUkey)
        ret = RegCloseKey(key->HKCUkey);
    if (key->HKCUstart && key->HKCUstart != HKEY_CURRENT_USER)
        ret = RegCloseKey(key->HKCUstart);
    if (key->HKLMkey)
        ret = RegCloseKey(key->HKLMkey);
    if (key->HKLMstart && key->HKLMstart != HKEY_LOCAL_MACHINE)
        ret = RegCloseKey(key->HKLMstart);

    heap_free(key);
    return ret;
}

LONG WINAPI SHRegDeleteEmptyUSKeyA(HUSKEY hUSKey, const char *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_a(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegDeleteEmptyUSKeyW(HUSKEY hUSKey, const WCHAR *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_w(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegDeleteUSValueA(HUSKEY hUSKey, const char *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_a(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegDeleteUSValueW(HUSKEY hUSKey, const WCHAR *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_w(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegEnumUSValueA(HUSKEY hUSKey, DWORD index, char *value_name, DWORD *value_name_len, DWORD *type,
        void *data, DWORD *data_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %#x, %p, %p, %p, %p, %p, %#x\n", hUSKey, index, value_name, value_name_len, type, data, data_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumValueA(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumValueA(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegEnumUSValueW(HUSKEY hUSKey, DWORD index, WCHAR *value_name, DWORD *value_name_len, DWORD *type,
        void *data, DWORD *data_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %#x, %p, %p, %p, %p, %p, %#x\n", hUSKey, index, value_name, value_name_len, type, data, data_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumValueW(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumValueW(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegEnumUSKeyA(HUSKEY hUSKey, DWORD index, char *name, DWORD *name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %d, %p, %p(%d), %d\n", hUSKey, index, name, name_len, *name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumKeyExA(dokey, index, name, name_len, 0, 0, 0, 0);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumKeyExA(dokey, index, name, name_len, 0, 0, 0, 0);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegEnumUSKeyW(HUSKEY hUSKey, DWORD index, WCHAR *name, DWORD *name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %d, %p, %p(%d), %d\n", hUSKey, index, name, name_len, *name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumKeyExW(dokey, index, name, name_len, 0, 0, 0, 0);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumKeyExW(dokey, index, name, name_len, 0, 0, 0, 0);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegOpenUSKeyA(const char *path, REGSAM access_mask, HUSKEY relative_key, HUSKEY *uskey, BOOL ignore_hkcu)
{
    WCHAR pathW[MAX_PATH];

    if (path)
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, ARRAY_SIZE(pathW));

    return SHRegOpenUSKeyW(path ? pathW : NULL, access_mask, relative_key, uskey, ignore_hkcu);
}

LONG WINAPI SHRegOpenUSKeyW(const WCHAR *path, REGSAM access_mask, HUSKEY relative_key, HUSKEY *uskey, BOOL ignore_hkcu)
{
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    struct USKEY *key;

    TRACE("%s, %#x, %p, %p, %d\n", debugstr_w(path), access_mask, relative_key, uskey, ignore_hkcu);

    if (uskey)
        *uskey = NULL;

    /* Create internal HUSKEY */
    key = heap_alloc_zero(sizeof(*key));
    lstrcpynW(key->path, path, ARRAY_SIZE(key->path));

    if (relative_key)
    {
        key->HKCUstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, TRUE));
        key->HKLMstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, FALSE));

        /* FIXME: if either of these keys is NULL, create the start key from
         *        the relative keys start+path
         */
    }
    else
    {
        key->HKCUstart = HKEY_CURRENT_USER;
        key->HKLMstart = HKEY_LOCAL_MACHINE;
    }

    if (!ignore_hkcu)
    {
        ret1 = RegOpenKeyExW(key->HKCUstart, key->path, 0, access_mask, &key->HKCUkey);
        if (ret1)
            key->HKCUkey = 0;
    }

    ret2 = RegOpenKeyExW(key->HKLMstart, key->path, 0, access_mask, &key->HKLMkey);
    if (ret2)
        key->HKLMkey = 0;

    if (ret1 || ret2)
        TRACE("one or more opens failed: HKCU=%d HKLM=%d\n", ret1, ret2);

    if (ret1 && ret2)
    {
        /* Neither open succeeded: fail */
        SHRegCloseUSKey(key);
        return ret2;
    }

    TRACE("HUSKEY=%p\n", key);
    if (uskey)
        *uskey = key;

    return ERROR_SUCCESS;
}

LONG WINAPI SHRegWriteUSValueA(HUSKEY hUSKey, const char *value, DWORD type, void *data, DWORD data_len, DWORD flags)
{
    WCHAR valueW[MAX_PATH];

    if (value)
        MultiByteToWideChar(CP_ACP, 0, value, -1, valueW, ARRAY_SIZE(valueW));

    return SHRegWriteUSValueW(hUSKey, value ? valueW : NULL, type, data, data_len, flags);
}

LONG WINAPI SHRegWriteUSValueW(HUSKEY hUSKey, const WCHAR *value, DWORD type, void *data, DWORD data_len, DWORD flags)
{
    struct USKEY *hKey = hUSKey;
    LONG ret = ERROR_SUCCESS;
    DWORD dummy;

    TRACE("%p, %s, %d, %p, %d, %#x\n", hUSKey, debugstr_w(value), type, data, data_len, flags);

    if (!hUSKey || IsBadWritePtr(hUSKey, sizeof(struct USKEY)) || !(flags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)))
        return ERROR_INVALID_PARAMETER;

    if (flags & (SHREGSET_FORCE_HKCU | SHREGSET_HKCU))
    {
        if (!hKey->HKCUkey)
        {
            /* Create the key */
            ret = RegCreateKeyW(hKey->HKCUstart, hKey->path, &hKey->HKCUkey);
            TRACE("Creating HKCU key, ret = %d\n", ret);
            if (ret && (flags & SHREGSET_FORCE_HKCU))
            {
                hKey->HKCUkey = 0;
                return ret;
            }
        }

        if (!ret)
        {
            if ((flags & SHREGSET_FORCE_HKCU) || RegQueryValueExW(hKey->HKCUkey, value, NULL, NULL, NULL, &dummy))
            {
                /* Doesn't exist or we are forcing: Write value */
                ret = RegSetValueExW(hKey->HKCUkey, value, 0, type, data, data_len);
                TRACE("Writing HKCU value, ret = %d\n", ret);
            }
        }
    }

    if (flags & (SHREGSET_FORCE_HKLM | SHREGSET_HKLM))
    {
        if (!hKey->HKLMkey)
        {
            /* Create the key */
            ret = RegCreateKeyW(hKey->HKLMstart, hKey->path, &hKey->HKLMkey);
            TRACE("Creating HKLM key, ret = %d\n", ret);
            if (ret && (flags & (SHREGSET_FORCE_HKLM)))
            {
                hKey->HKLMkey = 0;
                return ret;
            }
        }

        if (!ret)
        {
            if ((flags & SHREGSET_FORCE_HKLM) || RegQueryValueExW(hKey->HKLMkey, value, NULL, NULL, NULL, &dummy))
            {
                /* Doesn't exist or we are forcing: Write value */
                ret = RegSetValueExW(hKey->HKLMkey, value, 0, type, data, data_len);
                TRACE("Writing HKLM value, ret = %d\n", ret);
            }
        }
    }

    return ret;
}

LONG WINAPI SHRegSetUSValueA(const char *subkey, const char *value, DWORD type, void *data, DWORD data_len,
        DWORD flags)
{
    BOOL ignore_hkcu;
    HUSKEY hkey;
    LONG ret;

    TRACE("%s, %s, %d, %p, %d, %#x\n", debugstr_a(subkey), debugstr_a(value), type, data, data_len, flags);

    if (!data)
        return ERROR_INVALID_FUNCTION;

    ignore_hkcu = !(flags & SHREGSET_HKCU || flags & SHREGSET_FORCE_HKCU);

    ret = SHRegOpenUSKeyA(subkey, KEY_ALL_ACCESS, 0, &hkey, ignore_hkcu);
    if (ret == ERROR_SUCCESS)
    {
        ret = SHRegWriteUSValueA(hkey, value, type, data, data_len, flags);
        SHRegCloseUSKey(hkey);
    }

    return ret;
}

LONG WINAPI SHRegSetUSValueW(const WCHAR *subkey, const WCHAR *value, DWORD type, void *data, DWORD data_len,
        DWORD flags)
{
    BOOL ignore_hkcu;
    HUSKEY hkey;
    LONG ret;

    TRACE("%s, %s, %d, %p, %d, %#x\n", debugstr_w(subkey), debugstr_w(value), type, data, data_len, flags);

    if (!data)
        return ERROR_INVALID_FUNCTION;

    ignore_hkcu = !(flags & SHREGSET_HKCU || flags & SHREGSET_FORCE_HKCU);

    ret = SHRegOpenUSKeyW(subkey, KEY_ALL_ACCESS, 0, &hkey, ignore_hkcu);
    if (ret == ERROR_SUCCESS)
    {
        ret = SHRegWriteUSValueW(hkey, value, type, data, data_len, flags);
        SHRegCloseUSKey(hkey);
    }

    return ret;
}

LONG WINAPI SHRegQueryInfoUSKeyA(HUSKEY hUSKey, DWORD *subkeys, DWORD *max_subkey_len, DWORD *values,
        DWORD *max_value_name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;
    LONG ret;

    TRACE("%p, %p, %p, %p, %p, %#x\n", hUSKey, subkeys, max_subkey_len, values, max_value_name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryInfoKeyA(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
        if (ret == ERROR_SUCCESS || flags == SHREGENUM_HKCU)
            return ret;
    }

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        return RegQueryInfoKeyA(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
    }

    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegQueryInfoUSKeyW(HUSKEY hUSKey, DWORD *subkeys, DWORD *max_subkey_len, DWORD *values,
        DWORD *max_value_name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;
    LONG ret;

    TRACE("%p, %p, %p, %p, %p, %#x\n", hUSKey, subkeys, max_subkey_len, values, max_value_name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryInfoKeyW(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
        if (ret == ERROR_SUCCESS || flags == SHREGENUM_HKCU)
            return ret;
    }

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        return RegQueryInfoKeyW(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
    }

    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegQueryUSValueA(HUSKEY hUSKey, const char *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    LONG ret = ~ERROR_SUCCESS;
    DWORD move_len;
    HKEY dokey;

    /* If user wants HKCU, and it exists, then try it */
    if (!ignore_hkcu && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryValueExA(dokey, value, 0, type, data, data_len);
        TRACE("HKCU RegQueryValue returned %d\n", ret);
    }

    /* If HKCU did not work and HKLM exists, then try it */
    if ((ret != ERROR_SUCCESS) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        ret = RegQueryValueExA(dokey, value, 0, type, data, data_len);
        TRACE("HKLM RegQueryValue returned %d\n", ret);
    }

    /* If neither worked, and default data exists, then use it */
    if (ret != ERROR_SUCCESS)
    {
        if (default_data && default_data_len)
        {
            move_len = default_data_len >= *data_len ? *data_len : default_data_len;
            memmove(data, default_data, move_len);
            *data_len = move_len;
            TRACE("setting default data\n");
            ret = ERROR_SUCCESS;
        }
    }

    return ret;
}

LONG WINAPI SHRegQueryUSValueW(HUSKEY hUSKey, const WCHAR *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    LONG ret = ~ERROR_SUCCESS;
    DWORD move_len;
    HKEY dokey;

    /* If user wants HKCU, and it exists, then try it */
    if (!ignore_hkcu && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryValueExW(dokey, value, 0, type, data, data_len);
        TRACE("HKCU RegQueryValue returned %d\n", ret);
    }

    /* If HKCU did not work and HKLM exists, then try it */
    if ((ret != ERROR_SUCCESS) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        ret = RegQueryValueExW(dokey, value, 0, type, data, data_len);
        TRACE("HKLM RegQueryValue returned %d\n", ret);
    }

    /* If neither worked, and default data exists, then use it */
    if (ret != ERROR_SUCCESS)
    {
        if (default_data && default_data_len)
        {
            move_len = default_data_len >= *data_len ? *data_len : default_data_len;
            memmove(data, default_data, move_len);
            *data_len = move_len;
            TRACE("setting default data\n");
            ret = ERROR_SUCCESS;
        }
    }

    return ret;
}

LONG WINAPI SHRegGetUSValueA(const char *subkey, const char *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    HUSKEY myhuskey;
    LONG ret;

    if (!data || !data_len)
        return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/

    TRACE("%s, %s, %d\n", debugstr_a(subkey), debugstr_a(value), *data_len);

    ret = SHRegOpenUSKeyA(subkey, KEY_QUERY_VALUE, 0, &myhuskey, ignore_hkcu);
    if (!ret)
    {
        ret = SHRegQueryUSValueA(myhuskey, value, type, data, data_len, ignore_hkcu, default_data, default_data_len);
        SHRegCloseUSKey(myhuskey);
    }

    return ret;
}

LONG WINAPI SHRegGetUSValueW(const WCHAR *subkey, const WCHAR *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    HUSKEY myhuskey;
    LONG ret;

    if (!data || !data_len)
        return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/

    TRACE("%s, %s, %d\n", debugstr_w(subkey), debugstr_w(value), *data_len);

    ret = SHRegOpenUSKeyW(subkey, KEY_QUERY_VALUE, 0, &myhuskey, ignore_hkcu);
    if (!ret)
    {
        ret = SHRegQueryUSValueW(myhuskey, value, type, data, data_len, ignore_hkcu, default_data, default_data_len);
        SHRegCloseUSKey(myhuskey);
    }

    return ret;
}

BOOL WINAPI SHRegGetBoolUSValueA(const char *subkey, const char *value, BOOL ignore_hkcu, BOOL default_value)
{
    BOOL ret = default_value;
    DWORD type, datalen;
    char data[10];

    TRACE("%s, %s, %d\n", debugstr_a(subkey), debugstr_a(value), ignore_hkcu);

    datalen = ARRAY_SIZE(data) - 1;
    if (!SHRegGetUSValueA(subkey, value, &type, data, &datalen, ignore_hkcu, 0, 0))
    {
        switch (type)
        {
            case REG_SZ:
                data[9] = '\0';
                if (!lstrcmpiA(data, "YES") || !lstrcmpiA(data, "TRUE"))
                    ret = TRUE;
                else if (!lstrcmpiA(data, "NO") || !lstrcmpiA(data, "FALSE"))
                    ret = FALSE;
                break;
            case REG_DWORD:
                ret = *(DWORD *)data != 0;
                break;
            case REG_BINARY:
                if (datalen == 1)
                {
                    ret = !!data[0];
                    break;
                }
            default:
                FIXME("Unsupported registry data type %d\n", type);
                ret = FALSE;
        }
        TRACE("got value (type=%d), returning %d\n", type, ret);
    }
    else
        TRACE("returning default value %d\n", ret);

    return ret;
}

BOOL WINAPI SHRegGetBoolUSValueW(const WCHAR *subkey, const WCHAR *value, BOOL ignore_hkcu, BOOL default_value)
{
    static const WCHAR yesW[]= {'Y','E','S',0};
    static const WCHAR trueW[] = {'T','R','U','E',0};
    static const WCHAR noW[] = {'N','O',0};
    static const WCHAR falseW[] = {'F','A','L','S','E',0};
    BOOL ret = default_value;
    DWORD type, datalen;
    WCHAR data[10];

    TRACE("%s, %s, %d\n", debugstr_w(subkey), debugstr_w(value), ignore_hkcu);

    datalen = (ARRAY_SIZE(data) - 1) * sizeof(WCHAR);
    if (!SHRegGetUSValueW(subkey, value, &type, data, &datalen, ignore_hkcu, 0, 0))
    {
        switch (type)
        {
            case REG_SZ:
                data[9] = '\0';
                if (!lstrcmpiW(data, yesW) || !lstrcmpiW(data, trueW))
                    ret = TRUE;
                else if (!lstrcmpiW(data, noW) || !lstrcmpiW(data, falseW))
                    ret = FALSE;
                break;
            case REG_DWORD:
                ret = *(DWORD *)data != 0;
                break;
            case REG_BINARY:
                if (datalen == 1)
                {
                    ret = !!data[0];
                    break;
                }
            default:
                FIXME("Unsupported registry data type %d\n", type);
                ret = FALSE;
        }
        TRACE("got value (type=%d), returning %d\n", type, ret);
    }
    else
        TRACE("returning default value %d\n", ret);

    return ret;
}
