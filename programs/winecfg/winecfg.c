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
 */
int GetConfigValueSZ (HKEY hCurrent, LPSTR subkey, LPSTR valueName, LPSTR RetVal, 
                    int length, LPSTR DefRes)
{
    CHAR *buffer=NULL;
    DWORD dataLength=0;
    HKEY hSubKey=NULL;
    DWORD res;

    if( (res=RegOpenKeyEx( hCurrent, subkey, 0, KEY_ALL_ACCESS, &hSubKey ))
            !=ERROR_SUCCESS )
    {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            WINE_TRACE("Value not present - using default\n");
            strncpy( RetVal,DefRes,length);
            res=TRUE;
        }
        else
        {
            WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
            res=FALSE;
        }
        goto end;
    }
    res = RegQueryValueExA( hSubKey, valueName, NULL, NULL, NULL, &dataLength);
        if( res==ERROR_FILE_NOT_FOUND )
    {
        WINE_TRACE("Value not present - using default\n");
        strncpy( RetVal,DefRes,length);
        res=TRUE;
        goto end;
    }

    if( res!=ERROR_SUCCESS )
    {
        WINE_ERR("Couldn't query value's length (%ld)\n", res );
        res=FALSE;
        goto end;
    }

    buffer=malloc( dataLength );
    if( buffer==NULL )
    {
        WINE_ERR("Couldn't allocate %lu bytes for the value\n", dataLength );
        res=FALSE;
        goto end;
    }
    
    RegQueryValueEx( hSubKey, valueName, NULL, NULL, (LPBYTE)buffer, &dataLength);
    strncpy( RetVal,buffer,length);
    free(buffer);
    
end:
   if( hSubKey!=NULL )
        RegCloseKey( hSubKey );

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

    if( (res=RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config", 0, KEY_ALL_ACCESS, &hSession ))
            !=ERROR_SUCCESS )
    {
        if( res==ERROR_FILE_NOT_FOUND )
            WINE_ERR("Wine config key does not exist");
        else
            WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
        
        res=FALSE;
        return 1;
    }
    
    /* Windows and DOS versions */
    GetConfigValueSZ(hSession,"Version","Windows",pCfg->szWinVer,MAX_VERSION_LENGTH,"win95");
    GetConfigValueSZ(hSession,"Version","DOS",pCfg->szDOSVer,MAX_VERSION_LENGTH,"6.22");
    GetConfigValueSZ(hSession,"Tweak.Layout","WineLook",pCfg->szWinLook,MAX_VERSION_LENGTH,"win95");

    /* System Paths */
    GetConfigValueSZ(hSession,"Wine","Windows",pCfg->szWinDir,MAX_PATH,"c:\\Windows");
    GetConfigValueSZ(hSession,"Wine","System",pCfg->szWinSysDir,MAX_PATH,"c:\\Windows\\System");
    GetConfigValueSZ(hSession,"Wine","Temp",pCfg->szWinTmpDir,MAX_PATH,"c:\\Windows\\Temp");
    GetConfigValueSZ(hSession,"Wine","Profile",pCfg->szWinProfDir,MAX_PATH,"c:\\Windows\\Profiles\\Administrator");
    GetConfigValueSZ(hSession,"Wine","Path",pCfg->szWinPath,MAX_PATH,"c:\\Windows;c:\\Windows\\System");

    /* Graphics driver */
    GetConfigValueSZ(hSession,"Wine","GraphicsDriver",pCfg->szGraphDriver,MAX_NAME_LENGTH,"x11drv");
    
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
    pCfg->sX11Drv.nXShm = 1;
    pCfg->sX11Drv.nXVidMode = 1;
    pCfg->sX11Drv.nTakeFocus = 1;
    pCfg->sX11Drv.nDXGrab = 0;
    pCfg->sX11Drv.nDoubleBuffered = 0;
    pCfg->sX11Drv.nXVideoPort = 43;
    pCfg->sX11Drv.nSynchronous = 1;
    
    RegCloseKey( hSession );

    return 0;
}

/*****************************************************************************
 * Name: saveConfig
 * Description: Stores the configuration structure
 * Parameters : pCfg
 * Returns    : 0 on success, -1 otherwise
 *
 * FIXME: This is where we are to write the changes to the registry.
 *        This is not setup yet, so do nothing and say ok.
 */
int saveConfig (const WINECFG_DESC* pCfg)
{
    return 0;
}
