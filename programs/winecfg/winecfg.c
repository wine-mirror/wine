/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Mike Hearn
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
 *
 */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <windows.h>
#include <winreg.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#include "winecfg.h"

HKEY configKey = NULL;


int initialize(void) {
    DWORD res = RegCreateKey(HKEY_LOCAL_MACHINE, WINE_KEY_ROOT, &configKey);
    if (res != ERROR_SUCCESS) {
	WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
	return 1;
    }
    return 0;
}


/*****************************************************************************
 * getConfigValue: Retrieves a configuration value from the registry
 *
 * char *subKey : the name of the config section
 * char *valueName : the name of the config value
 * char *defaultResult : if the key isn't found, return this value instead
 *
 * Returns a buffer holding the value if successful, NULL if not. Caller is responsible for freeing the result.
 *
 */
char *getConfigValue (char *subkey, char *valueName, char *defaultResult)
{
    char *buffer = NULL;
    DWORD dataLength;
    HKEY hSubKey = NULL;
    DWORD res;

    WINE_TRACE("subkey=%s, valueName=%s, defaultResult=%s\n", subkey, valueName, defaultResult);

    res = RegOpenKeyEx( configKey, subkey, 0, KEY_ALL_ACCESS, &hSubKey );
    if(res != ERROR_SUCCESS)  {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            WINE_TRACE("Section key not present - using default\n");
            return strdup(defaultResult);
        }
        else
        {
            WINE_ERR("RegOpenKey failed on wine config key (res=%ld)\n", res);
        }
        goto end;
    }
    
    res = RegQueryValueExA( hSubKey, valueName, NULL, NULL, NULL, &dataLength);
    if( res == ERROR_FILE_NOT_FOUND ) {
        WINE_TRACE("Value not present - using default\n");
        buffer = strdup(defaultResult);
	goto end;
    } else if( res!=ERROR_SUCCESS )  {
        WINE_ERR("Couldn't query value's length (res=%ld)\n", res );
        goto end;
    }

    buffer = malloc(dataLength);
    if( buffer==NULL )
    {
        WINE_ERR("Couldn't allocate %lu bytes for the value\n", dataLength );
        goto end;
    }
    
    RegQueryValueEx(hSubKey, valueName, NULL, NULL, (LPBYTE)buffer, &dataLength);
    
end:
    if( hSubKey!=NULL )
        RegCloseKey( hSubKey );

    return buffer;
    
}

/*****************************************************************************
 * setConfigValue : Sets a configuration key in the registry
 *
 * HKEY  hCurrent : the registry key that the configuration is rooted at
 * char *subKey : the name of the config section
 * char *valueName : the name of the config value
 * char *value : the value to set the configuration key to
 *
 * Returns 0 on success, non-zero otherwise
 *
 */
int setConfigValue (char *subkey, char *valueName, const char *value) {
    DWORD res = 1;
    HKEY key = NULL;

    WINE_TRACE("subkey=%s, valueName=%s, value=%s\n", subkey, valueName, value);

    assert( subkey != NULL );
    assert( valueName != NULL );
    assert( value != NULL );
    
    res = RegCreateKey(configKey, subkey, &key);
    if (res != ERROR_SUCCESS) goto end;

    res = RegSetValueEx(key, valueName, 0, REG_SZ, value, strlen(value) + 1);
    if (res != ERROR_SUCCESS) goto end;

    res = 0;
end:
    if (key) RegCloseKey(key);
    if (res != 0) WINE_ERR("Unable to set configuration key %s in section %s to %s, res=%ld\n", valueName, subkey, value, res);
    return res;
}

/* returns 0 on success, an HRESULT from the registry funtions otherwise */
HRESULT doesConfigValueExist(char *subkey, char *valueName) {
    HRESULT hr;
    HKEY key;

    WINE_TRACE("subkey=%s, valueName=%s - ", subkey, valueName);
    
    hr = RegOpenKeyEx(configKey, subkey, 0, KEY_READ, &key);
    if (hr != S_OK) {
	WINE_TRACE("no: subkey does not exist\n");
	return hr;
    }

    hr = RegQueryValueEx(key, valueName, NULL, NULL, NULL, NULL);
    if (hr != S_OK) {
	WINE_TRACE("no: key does not exist\n");
	return hr;
    }

    RegCloseKey(key);
    WINE_TRACE("yes\n");
    return S_OK;
}

/* removes the requested value from the registry, however, does not remove the section if empty. Returns S_OK (0) on success. */
HRESULT removeConfigValue(char *subkey, char *valueName) {
    HRESULT hr;
    HKEY key;
    WINE_TRACE("subkey=%s, valueName=%s\n", subkey, valueName);
    
    hr = RegOpenKeyEx(configKey, subkey, 0, KEY_READ, &key);
    if (hr != S_OK) return hr;

    hr = RegDeleteValue(key, valueName);
    if (hr != ERROR_SUCCESS) return hr;

    return S_OK;
}


/*****************************************************************************
 * Name       : loadConfig
 * Description: Loads and populates a configuration structure
 * Parameters : pCfg
 * Returns    : 0 on success, -1 otherwise
 *
 */
int loadConfig (WINECFG_DESC* pCfg)
{
    const DLL_DESC *pDllDefaults;
    char buffer[MAX_PATH];
    char subKeyName[51];
    DWORD res, i, sizeOfSubKeyName = 50;

    WINE_TRACE("\n");

    return_if_fail( initialize() == 0 , 1 );

    /* System Paths */
/*  getConfigValue(configKey, "Wine", "Windows", pCfg->szWinDir, MAX_PATH, "c:\\Windows");
    getConfigValue(configKey, "Wine", "System", pCfg->szWinSysDir, MAX_PATH, "c:\\Windows\\System");
    getConfigValue(configKey, "Wine", "Temp", pCfg->szWinTmpDir, MAX_PATH, "c:\\Windows\\Temp");
    getConfigValue(configKey, "Wine", "Profile", pCfg->szWinProfDir, MAX_PATH, "c:\\Windows\\Profiles\\Administrator");
    getConfigValue(configKey, "Wine", "Path", pCfg->szWinPath, MAX_PATH, "c:\\Windows;c:\\Windows\\System");
*/
    /* Graphics driver */
/*  getConfigValue(configKey, "Wine", "GraphicsDriver", pCfg->szGraphDriver, MAX_NAME_LENGTH, "x11drv");
 */    
    /*
     * DLL defaults for all applications is built using
     * the default DLL structure
     */
/*    for (pDllDefaults = getDLLDefaults (); *pDllDefaults->szName; pDllDefaults++)
    {
        DLL_DESC *pDll = malloc(sizeof(DLL_DESC));
	memcpy (pDll, pDllDefaults, sizeof(DLL_DESC));
        DPA_InsertPtr(pCfg->pDlls, INT_MAX, pDll);
    }
*/

    pCfg->sX11Drv.nTextCP = 0;
    pCfg->sX11Drv.nXVideoPort = 43;
    pCfg->sX11Drv.nDepth = 16;
    pCfg->sX11Drv.nManaged = 1;
    pCfg->sX11Drv.nDesktopSizeX = 640;
    pCfg->sX11Drv.nDesktopSizeY = 480;
    pCfg->sX11Drv.nDGA = 1;
    pCfg->sX11Drv.nXVidMode = 1;
    pCfg->sX11Drv.nTakeFocus = 1;
    pCfg->sX11Drv.nDXGrab = 0;
    pCfg->sX11Drv.nDoubleBuffered = 0;
    pCfg->sX11Drv.nSynchronous = 1;


    /*
     * Drive mappings
     */
/*  pCfg->pDrives = DPA_Create(26);
    for (i = 0;
	 RegEnumKeyExA(configKey, i, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, NULL ) != ERROR_NO_MORE_ITEMS;
         ++i, sizeOfSubKeyName=50) {
	HKEY hkDrive;
	DWORD returnType;
	char returnBuffer[MAX_NAME_LENGTH];
	DWORD sizeOfReturnBuffer = sizeof(returnBuffer);
	LONG r;

	if (!strncmp("Drive ", subKeyName, 5)) {
	    DRIVE_DESC *pDrive = malloc(sizeof(DRIVE_DESC));

	    WINE_TRACE("Loading %s\n", subKeyName);
	    
	    ZeroMemory(pDrive, sizeof(*pDrive));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    if (RegOpenKeyExA (configKey, subKeyName, 0, KEY_READ, &hkDrive) != ERROR_SUCCESS)	{
		WINE_ERR("unable to open drive registry key");
		RegCloseKey(configKey);
		return 1;
	    }
	   
	    strncpy(pDrive->szName, &subKeyName[strlen(subKeyName)-1],1);
	    if(pDrive->szName) {
		pDrive->szName[0] = toupper(pDrive->szName[0]);
	    }
	    
	    ZeroMemory(returnBuffer, sizeof(*returnBuffer));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    r = RegQueryValueExA(hkDrive, "Label", NULL, &returnType, returnBuffer, &sizeOfReturnBuffer);
	    if (r == ERROR_SUCCESS) {
		strncpy(pDrive->szLabel, returnBuffer, sizeOfReturnBuffer);
	    } else {
		WINE_WARN("pDrive->szLabel not loaded: %ld\n", r);
	    }
	    
	    ZeroMemory(returnBuffer, sizeof(*returnBuffer));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    r = RegQueryValueExA(hkDrive, "Serial", NULL, &returnType, returnBuffer, &sizeOfReturnBuffer);
	    if (r == ERROR_SUCCESS) {
		strncpy(pDrive->szSerial, returnBuffer, sizeOfReturnBuffer);
	    } else {
		WINE_WARN("pDrive->szSerial not loaded: %ld\n", r);
	    }
	    
	    
	    ZeroMemory(returnBuffer, sizeof(*returnBuffer));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    r = RegQueryValueExA(hkDrive, "Type", NULL, &returnType, returnBuffer, &sizeOfReturnBuffer);
	    if (r == ERROR_SUCCESS) {
		strncpy(pDrive->szType, returnBuffer, sizeOfReturnBuffer);
	    } else {
		WINE_WARN("pDrive->szType not loaded: %ld", r);
	    }
	    
	    ZeroMemory(returnBuffer, sizeof(*returnBuffer));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    if (RegQueryValueExA(hkDrive, "Path", NULL, &returnType, returnBuffer, &sizeOfReturnBuffer) == ERROR_SUCCESS) {
		strncpy(pDrive->szPath, returnBuffer, sizeOfReturnBuffer);
	    } else {
		WINE_WARN("pDrive->szPath not loaded: %ld\n", GetLastError());
	    }
	    
	    ZeroMemory(returnBuffer, sizeof(*returnBuffer));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    if (RegQueryValueExA(hkDrive, "FileSystem", NULL, &returnType, returnBuffer, &sizeOfReturnBuffer) == ERROR_SUCCESS) {
		strncpy(pDrive->szFS, returnBuffer, sizeOfReturnBuffer);
	    } else {
		WINE_WARN("pDrive->szFS not loaded: %ld\n", GetLastError());
	    }

	    ZeroMemory(returnBuffer, sizeof(*returnBuffer));
	    sizeOfReturnBuffer = sizeof(returnBuffer);
	    if (RegQueryValueExA(hkDrive, "Device", NULL, &returnType, returnBuffer, &sizeOfReturnBuffer) == ERROR_SUCCESS) {
		strncpy(pDrive->szDevice, returnBuffer, sizeOfReturnBuffer);
	    } else {
		WINE_WARN("pDrive->szDevice not found: %ld\n", GetLastError());
	    }

	    if (DPA_InsertPtr(pCfg->pDrives, pCfg->driveCount, pDrive) == -1)
		WINE_ERR("Failed to insert pDrive into DPA\n");
	    else
		pCfg->driveCount++;
	    
	}
    }
    WINE_TRACE("loaded %d drives\n", pCfg->driveCount);
 */
    return 0;
}



/* ========================================================================= */
/* Transaction management code */

struct transaction *tqhead, *tqtail;
int instantApply = 1;

void destroyTransaction(struct transaction *trans) {
    assert( trans != NULL );
    
    WINE_TRACE("destroying %p\n", trans);
    
    free(trans->section);
    free(trans->key);
    if (trans->newValue) free(trans->newValue);
    
    if (trans->next) trans->next->prev = trans->prev;
    if (trans->prev) trans->prev->next = trans->next;
    if (trans == tqhead) tqhead = NULL;
    if (trans == tqtail) tqtail = NULL;
    
    free(trans);
}

void addTransaction(char *section, char *key, enum transaction_action action, char *newValue) {
    struct transaction *trans = malloc(sizeof(struct transaction));
    
    assert( section != NULL );
    assert( key != NULL );
    if (action == ACTION_SET) assert( newValue != NULL );

    trans->section = section;
    trans->key = key;
    trans->newValue = newValue;
    trans->action = action;
    trans->next = NULL;
    trans->prev = NULL;
    
    if (tqtail == NULL) {
	tqtail = trans;
	tqhead = tqtail;
    } else {
	tqhead->next = trans;
	trans->prev = tqhead;
	tqhead = trans;
    }

    if (instantApply) processTransaction(trans);
}

void processTransaction(struct transaction *trans) {
    if (trans->action == ACTION_SET) {
	WINE_TRACE("Setting %s\\%s to '%s'\n", trans->section, trans->key, trans->newValue);
	setConfigValue(trans->section, trans->key, trans->newValue);
    } else if (trans->action == ACTION_REMOVE) {
	WINE_TRACE("Removing %s\\%s", trans->section, trans->key);
	removeConfigValue(trans->section, trans->key);
    }
    /* TODO: implement notifications here */
}

void processTransQueue() {
    WINE_TRACE("\n");
    while (tqhead != NULL) {
	processTransaction(tqhead);
	tqhead = tqhead->next;
	destroyTransaction(tqhead->prev);
    }
}
