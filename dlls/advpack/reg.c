/*
 * Advpack registry functions
 *
 * Copyright 2004 Huw D M Davies
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winuser.h"
#include "winternl.h"
#include "setupapi.h"
#include "advpub.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

static const WCHAR REGINST[] = {'R','E','G','I','N','S','T',0};
static const WCHAR Strings[] = {'S','t','r','i','n','g','s',0};
static const WCHAR MOD_PATH[] = {'_','M','O','D','_','P','A','T','H',0};
static const WCHAR SYS_MOD_PATH[] = {'_','S','Y','S','_','M','O','D','_','P','A','T','H',0};
static const WCHAR SystemRoot[] = {'S','y','s','t','e','m','R','o','o','t',0};
static const WCHAR escaped_SystemRoot[] = {'%','S','y','s','t','e','m','R','o','o','t','%',0};

static BOOL get_temp_ini_path(LPWSTR name)
{
    WCHAR tmp_dir[MAX_PATH];
    WCHAR prefix[] = {'a','v','p',0};

    if(!GetTempPathW(sizeof(tmp_dir)/sizeof(WCHAR), tmp_dir))
       return FALSE;

    if(!GetTempFileNameW(tmp_dir, prefix, 0, name))
        return FALSE;
    return TRUE;
}

static BOOL create_tmp_ini_file(HMODULE hm, WCHAR *ini_file)
{
    HRSRC hrsrc;
    HGLOBAL hmem = NULL;
    DWORD rsrc_size, bytes_written;
    VOID *rsrc_data;
    HANDLE hf = INVALID_HANDLE_VALUE;

    if(!get_temp_ini_path(ini_file)) {
        ERR("Can't get temp ini file path\n");
        goto error;
    }

    if(!(hrsrc = FindResourceW(hm, REGINST, REGINST))) {
        ERR("Can't find REGINST resource\n");
        goto error;
    }

    rsrc_size = SizeofResource(hm, hrsrc);
    hmem = LoadResource(hm, hrsrc);
    rsrc_data = LockResource(hmem);

    if(!rsrc_data || !rsrc_size) {
        ERR("Can't load REGINST resource\n");
        goto error;
    }       

    if((hf = CreateFileW(ini_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        ERR("Unable to create temp ini file\n");
        goto error;
    }
    if(!WriteFile(hf, rsrc_data, rsrc_size, &bytes_written, NULL) || rsrc_size != bytes_written) {
        ERR("Write failed\n");
        goto error;
    }
    FreeResource(hmem);
    CloseHandle(hf);
    return TRUE;
error:
    if(hmem) FreeResource(hmem);
    if(hf != INVALID_HANDLE_VALUE) CloseHandle(hf);
    return FALSE;
}

/***********************************************************************
 *          RegInstallA (advpack.@)
 *
 * Loads an INF from a string resource, adds entries to the string
 * substitution table, and executes the INF.
 *
 * PARAMS
 *   hm         [I] Module that contains the REGINST resouce.
 *   pszSection [I] The INF section to execute.
 *   pstTable   [I] Table of string substitutions.
 * 
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI RegInstallA(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable)
{
    int i;
    WCHAR tmp_ini_path[MAX_PATH];
    WCHAR mod_path[MAX_PATH + 2], sys_mod_path[MAX_PATH + 2], sys_root[MAX_PATH];
    HINF hinf;
    WCHAR quote[] = {'\"',0};
    UNICODE_STRING section;

    TRACE("(%p %s %p)\n", hm, pszSection, pstTable);

    if (pstTable) for(i = 0; i < pstTable->cEntries; i++)
        TRACE("%d: %s -> %s\n", i, pstTable->pse[i].pszName,
             pstTable->pse[i].pszValue);

    if(!create_tmp_ini_file(hm, tmp_ini_path))
        return E_FAIL;

    /* Write a couple of pre-defined strings */
    mod_path[0] = '\"';
    GetModuleFileNameW(hm, mod_path + 1, sizeof(mod_path)/sizeof(WCHAR) - 2);
    strcatW(mod_path, quote);
    WritePrivateProfileStringW(Strings, MOD_PATH, mod_path, tmp_ini_path);
    
    *sys_root = '\0';
    GetEnvironmentVariableW(SystemRoot, sys_root, sizeof(sys_root)/sizeof(WCHAR));
    if(!strncmpiW(sys_root, mod_path + 1, strlenW(sys_root))) {
        sys_mod_path[0] = '\"';
        strcpyW(sys_mod_path + 1, escaped_SystemRoot);
        strcatW(sys_mod_path, mod_path + 1 + strlenW(sys_root));
    } else {
        FIXME("SYS_MOD_PATH needs more work\n");
        strcpyW(sys_mod_path, mod_path);
    }
    WritePrivateProfileStringW(Strings, SYS_MOD_PATH, sys_mod_path, tmp_ini_path);

    /* Write the additional string table */
    if (pstTable) for(i = 0; i < pstTable->cEntries; i++) {
        char tmp_value[MAX_PATH + 2];
        UNICODE_STRING name, value;
        tmp_value[0] = '\"';
        strcpy(tmp_value + 1, pstTable->pse[i].pszValue);
        strcat(tmp_value, "\"");
        RtlCreateUnicodeStringFromAsciiz(&name, pstTable->pse[i].pszName);
        RtlCreateUnicodeStringFromAsciiz(&value, tmp_value);
        WritePrivateProfileStringW(Strings, name.Buffer, value.Buffer, tmp_ini_path);
        RtlFreeUnicodeString(&name);
        RtlFreeUnicodeString(&value);
    }
    /* flush cache */
    WritePrivateProfileStringW(NULL, NULL, NULL, tmp_ini_path);


    if((hinf = SetupOpenInfFileW(tmp_ini_path, NULL, INF_STYLE_WIN4, NULL)) ==
       INVALID_HANDLE_VALUE) {
        ERR("Setupapi can't open inf\n");
        return E_FAIL;
    }

    /* append any layout files */
    SetupOpenAppendInfFileW(NULL, hinf, NULL);

    /* Need to do a lot more here */
    RtlCreateUnicodeStringFromAsciiz(&section, pszSection);
    SetupInstallFromInfSectionW(NULL, hinf, section.Buffer,
                                SPINST_INIFILES | SPINST_REGISTRY | SPINST_PROFILEITEMS,
                                HKEY_LOCAL_MACHINE, NULL, 0, NULL, NULL, NULL, NULL);
    RtlFreeUnicodeString(&section);

    
    SetupCloseInfFile(hinf);
    DeleteFileW(tmp_ini_path);

    return S_OK;
}

/***********************************************************************
 *          RegRestoreAllA (advpack.@)
 *
 * Restores all saved registry entries.
 *
 * PARAMS
 *   hWnd           [I] Handle to the window used for the display.
 *   pszTitleString [I] Title of the window.
 *   hkBackupKey    [I] Handle to the backup key.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RegRestoreAllA(HWND hWnd, LPSTR pszTitleString, HKEY hkBackupKey)
{
    FIXME("(%p, %p, %p) stub\n", hWnd, pszTitleString, hkBackupKey);
    
    return E_FAIL;   
}

/***********************************************************************
 *          RegSaveRestoreA (advpack.@)
 *
 * Saves or restores the specified registry value.
 *
 * PARAMS
 *   hWnd           [I] Handle to the window used for the display.
 *   pszTitleString [I] Title of the window.
 *   hkBackupKey    [I] Key used to store the backup data.
 *   pcszRootKey    [I] Root key of the registry value
 *   pcszSubKey     [I] Sub key of the registry value.
 *   pcszValueName  [I] Value to save or restore. 
 *   dwFlags        [I] See advpub.h.
 * 
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RegSaveRestoreA(HWND hWnd, LPCSTR pszTitleString, HKEY hkBackupKey,
                              LPCSTR pcszRootKey, LPCSTR pcszSubKey,
                              LPCSTR pcszValueName, DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %ld) stub\n", hWnd, pszTitleString,
          hkBackupKey, pcszRootKey, pcszSubKey, pcszValueName, dwFlags);

    return E_FAIL;   
}

/***********************************************************************
 *          RegSaveRestoreOnINFA (advpack.@)
 *
 * Saves or restores the specified INF Reg section.
 *
 * PARAMS
 *   hWnd         [I] Handle to the window used for the display.
 *   pszTitle     [I] Title of the window.
 *   pszINF       [I] Filename of the INF.
 *   pszSection   [I] Section to save or restore.
 *   hHKLMBackKey [I] Opened key in HKLM to store data.
 *   hHKCUBackKey [I] Opened key in HKCU to store data.
 *   dwFlags      [I] See advpub.h
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RegSaveRestoreOnINFA(HWND hWnd, LPCSTR pszTitle, LPCSTR pszINF,
                                   LPCSTR pszSection, HKEY hHKLMBackKey,
                                   HKEY hHKCUBackKey, DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %ld) stub\n", hWnd, pszTitle, pszINF,
          pszSection, hHKLMBackKey, hHKCUBackKey, dwFlags);

    return E_FAIL;   
}
