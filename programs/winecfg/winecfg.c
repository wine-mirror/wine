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

#include <stdio.h>
#include <limits.h>
#include <windows.h>
#include <winreg.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#include "winecfg.h"

HKEY configKey = NULL;

/*****************************************************************************
 */
WINECFG_DESC* allocConfig(void)
{
    WINECFG_DESC* pWineCfg =  malloc (sizeof (WINECFG_DESC));

    if (!pWineCfg) goto fail;
    ZeroMemory(pWineCfg, sizeof(*pWineCfg));

    pWineCfg->pDlls = DPA_Create(100);
    if (!pWineCfg->pDlls) goto fail;
    pWineCfg->pApps = DPA_Create(100);
    if (!pWineCfg->pApps) goto fail;
    
    return pWineCfg;

fail:
    /* FIXME: do something nice */
    printf("Out of memory");
    exit(1);
}


/*****************************************************************************
 */
int freeConfig (WINECFG_DESC* pCfg)
{
    int i;

    for (i = 0; i < pCfg->pDlls->nItemCount; i++)
	free (DPA_GetPtr(pCfg->pDlls, i));
    DPA_Destroy(pCfg->pDlls);
    
    for (i = 0; i < pCfg->pApps->nItemCount; i++)
	free (DPA_GetPtr(pCfg->pApps, i));
    DPA_Destroy(pCfg->pApps);
    
    free (pCfg);

    return 0;
}

/*****************************************************************************
 * getConfigValue: Retrieves a configuration value from the registry
 *
 * HKEY  hCurrent : the registry key that the configuration is rooted at
 * char *subKey : the name of the config section
 * char *valueName : the name of the config value
 * char *retVal : pointer to the start of a buffer that has room for >= length chars
 * int   length : length of the buffer pointed to by retVal
 * char *defaultResult : if the key isn't found, return this value instead
 *
 * Returns 0 upon success, non-zero otherwise
 *
 */
int getConfigValue (HKEY hCurrent, char *subkey, char *valueName, char *retVal, int length, char *defaultResult)
{
    CHAR *buffer;
    DWORD dataLength;
    HKEY hSubKey = NULL;
    DWORD res = 1; /* assume failure */

    WINE_TRACE("subkey=%s, valueName=%s, defaultResult=%s\n", subkey, valueName, defaultResult);
    
    if( (res=RegOpenKeyEx( hCurrent, subkey, 0, KEY_ALL_ACCESS, &hSubKey ))
            !=ERROR_SUCCESS )
    {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            WINE_TRACE("Section key not present - using default\n");
            strncpy(retVal, defaultResult, length);
        }
        else
        {
            WINE_ERR("RegOpenKey failed on wine config key (res=%ld)\n", res);
        }
        goto end;
    }
    
    res = RegQueryValueExA( hSubKey, valueName, NULL, NULL, NULL, &dataLength);
    if( res == ERROR_FILE_NOT_FOUND )
    {
        WINE_TRACE("Value not present - using default\n");
        strncpy(retVal, defaultResult, length);
        goto end;
    }

    if( res!=ERROR_SUCCESS )
    {
        WINE_ERR("Couldn't query value's length (res=%ld)\n", res );
        goto end;
    }

    buffer=malloc( dataLength );
    if( buffer==NULL )
    {
        WINE_ERR("Couldn't allocate %lu bytes for the value\n", dataLength );
        goto end;
    }
    
    RegQueryValueEx(hSubKey, valueName, NULL, NULL, (LPBYTE)buffer, &dataLength);
    strncpy(retVal, buffer, length);
    free(buffer);
    res = 0;
    
end:
    if( hSubKey!=NULL )
        RegCloseKey( hSubKey );

    return res;
    
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
int setConfigValue (HKEY hCurrent, char *subkey, char *valueName, const char *value) {
    DWORD res = 1;
    HKEY key = NULL;

    WINE_TRACE("subkey=%s, valueName=%s, value=%s\n", subkey, valueName, value);
    
    res = RegCreateKey(hCurrent, subkey, &key);
    if (res != ERROR_SUCCESS) goto end;

    res = RegSetValueEx(key, valueName, 0, REG_SZ, value, strlen(value) + 1);
    if (res != ERROR_SUCCESS) goto end;

    res = 0;
end:
    if (key) RegCloseKey(key);
    if (res != 0) WINE_ERR("Unable to set configuration key %s in section %s to %s, res=%ld\n", valueName, subkey, value, res);
    return res;
}


/*****************************************************************************
 * Name       : loadConfig
 * Description: Loads and populates a configuration structure
 * Parameters : pCfg
 * Returns    : 0 on success, -1 otherwise
 *
 * FIXME: We are supposed to load these values from the registry.
 *        This is not active yet, so just setup some (hopefully) 
 *        sane defaults
 */
int loadConfig (WINECFG_DESC* pCfg)
{
    const DLL_DESC *pDllDefaults;
    char buffer[MAX_PATH];
    char subKeyName[51];
    DWORD res, i, sizeOfSubKeyName = 50;

    WINE_TRACE("\n");

    res = RegCreateKey(HKEY_LOCAL_MACHINE, WINE_KEY_ROOT, &configKey);
    if (res != ERROR_SUCCESS)
    {
        WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
        return -1;
    }

    /* Windows and DOS versions */
    getConfigValue(configKey, "Version", "Windows", pCfg->szWinVer, MAX_VERSION_LENGTH, "win95");
    getConfigValue(configKey, "Version", "DOS", pCfg->szDOSVer, MAX_VERSION_LENGTH, "6.22");
    getConfigValue(configKey, "Tweak.Layout", "WineLook", pCfg->szWinLook, MAX_VERSION_LENGTH, "win95");

    /* System Paths */
    getConfigValue(configKey, "Wine", "Windows", pCfg->szWinDir, MAX_PATH, "c:\\Windows");
    getConfigValue(configKey, "Wine", "System", pCfg->szWinSysDir, MAX_PATH, "c:\\Windows\\System");
    getConfigValue(configKey, "Wine", "Temp", pCfg->szWinTmpDir, MAX_PATH, "c:\\Windows\\Temp");
    getConfigValue(configKey, "Wine", "Profile", pCfg->szWinProfDir, MAX_PATH, "c:\\Windows\\Profiles\\Administrator");
    getConfigValue(configKey, "Wine", "Path", pCfg->szWinPath, MAX_PATH, "c:\\Windows;c:\\Windows\\System");

    /* Graphics driver */
    getConfigValue(configKey, "Wine", "GraphicsDriver", pCfg->szGraphDriver, MAX_NAME_LENGTH, "x11drv");
    
    /*
     * DLL defaults for all applications is built using
     * the default DLL structure
     */
    for (pDllDefaults = getDLLDefaults (); *pDllDefaults->szName; pDllDefaults++)
    {
        DLL_DESC *pDll = malloc(sizeof(DLL_DESC));
	memcpy (pDll, pDllDefaults, sizeof(DLL_DESC));
        DPA_InsertPtr(pCfg->pDlls, INT_MAX, pDll);
    }

    /*
     * Application defaults on a per application
     * level (if not set, this defaults to what 
     * is already there)
     */
    
    /* FIXME: Finish these off. Do we actually need GUI for all of them? */

    /*
     * X11Drv defaults
     */
    getConfigValue(configKey, "x11drv", "Display", pCfg->sX11Drv.szX11Display, sizeof(pCfg->sX11Drv.szX11Display), ":0.0");
    
    getConfigValue(configKey, "x11drv", "AllocSystemColors", buffer, sizeof(buffer), "100");
    pCfg->sX11Drv.nSysColors = atoi(buffer);
    
    getConfigValue(configKey, "x11drv", "PrivateColorMap", buffer, sizeof(buffer), "N");
    pCfg->sX11Drv.nPrivateMap = IS_OPTION_TRUE(buffer[0]);
    
    getConfigValue(configKey, "x11drv", "PerfectGraphics", buffer, sizeof(buffer), "N");
    pCfg->sX11Drv.nPerfect = IS_OPTION_TRUE(buffer[0]);
    
    getConfigValue(configKey, "x11drv", "Desktop", buffer, sizeof(buffer), "640x480");
    sscanf(buffer, "%dx%d", &pCfg->sX11Drv.nDesktopSizeX, &pCfg->sX11Drv.nDesktopSizeY);

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
    pCfg->pDrives = DPA_Create(26);
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

    RegCloseKey( configKey );
    return 0;
}

/*****************************************************************************
 * saveConfig : Stores the configuration structure
 *
 * Returns    : 0 on success, -1 otherwise
 *
 * FIXME: This is where we are to write the changes to the registry.
 *        This is not setup yet, so do nothing and say ok.
 */
int saveConfig (const WINECFG_DESC* pCfg)
{
    HKEY key;
    DWORD res;

    WINE_TRACE("\n");
    
    res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINE_KEY_ROOT, 0, KEY_ALL_ACCESS, &key);
    if (res != ERROR_SUCCESS) {
	WINE_ERR("Failed to open Wine config registry branch, res=%ld\n", res);
	return -1;
    }

    /* Windows and DOS versions */
    setConfigValue(key, "Version", "Windows", pCfg->szWinVer);
    
    WINE_FIXME("We don't write out the entire configuration yet\n");
    return 0;
}
