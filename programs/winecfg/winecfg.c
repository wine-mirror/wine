/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
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
    
    HKEY hSession=NULL;
    DWORD res;

    WINE_TRACE("\n");

    res = RegCreateKey(HKEY_LOCAL_MACHINE, WINE_KEY_ROOT, &hSession);
    if (res != ERROR_SUCCESS)
    {
        WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
        return -1;
    }

    /* Windows and DOS versions */
    getConfigValue(hSession, "Version", "Windows", pCfg->szWinVer, MAX_VERSION_LENGTH, "win95");
    getConfigValue(hSession, "Version", "DOS", pCfg->szDOSVer, MAX_VERSION_LENGTH, "6.22");
    getConfigValue(hSession, "Tweak.Layout", "WineLook", pCfg->szWinLook, MAX_VERSION_LENGTH, "win95");

    /* System Paths */
    getConfigValue(hSession, "Wine", "Windows", pCfg->szWinDir, MAX_PATH, "c:\\Windows");
    getConfigValue(hSession, "Wine", "System", pCfg->szWinSysDir, MAX_PATH, "c:\\Windows\\System");
    getConfigValue(hSession, "Wine", "Temp", pCfg->szWinTmpDir, MAX_PATH, "c:\\Windows\\Temp");
    getConfigValue(hSession, "Wine", "Profile", pCfg->szWinProfDir, MAX_PATH, "c:\\Windows\\Profiles\\Administrator");
    getConfigValue(hSession, "Wine", "Path", pCfg->szWinPath, MAX_PATH, "c:\\Windows;c:\\Windows\\System");

    /* Graphics driver */
    getConfigValue(hSession, "Wine", "GraphicsDriver", pCfg->szGraphDriver, MAX_NAME_LENGTH, "x11drv");
    
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
    /* FIXME: TODO */

    /*
     * X11Drv defaults
     */
    strcpy(pCfg->sX11Drv.szX11Display, ":0.0");
    pCfg->sX11Drv.nSysColors = 100;
    pCfg->sX11Drv.nPrivateMap = 0;
    pCfg->sX11Drv.nPerfect = 0;
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
    
    RegCloseKey( hSession );

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
