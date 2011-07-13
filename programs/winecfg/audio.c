/*
 * Audio management UI code
 *
 * Copyright 2004 Chris Morgan
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
 *
 */

#define WIN32_LEAN_AND_MEAN
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <wine/debug.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <mmddk.h>

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#define DRIVER_MASK 0x80000000
#define DEVICE_MASK 0x40000000
#define MAX_NAME_LENGTH 64

typedef DWORD (WINAPI * MessagePtr)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

static struct DSOUNDACCEL
{
  UINT displayID;
  UINT visible;
  const char* settingStr;
} DSound_HW_Accels[] = {
  {IDS_ACCEL_FULL,      1, "Full"},
  {IDS_ACCEL_STANDARD,  0, "Standard"},
  {IDS_ACCEL_BASIC,     0, "Basic"},
  {IDS_ACCEL_EMULATION, 1, "Emulation"},
  {0, 0, 0}
};

static const char* DSound_Rates[] = {
  "48000",
  "44100",
  "22050",
  "16000",
  "11025",
  "8000",
  NULL
};

static const char* DSound_Bits[] = {
  "8",
  "16",
  NULL
};

typedef struct
{
  UINT nameID;
  const char *szDriver;
  HDRVR hDriver;
} AUDIO_DRIVER;

static AUDIO_DRIVER sAudioDrivers[] = {
  {IDS_DRIVER_ALSA,      "alsa"},
  {IDS_DRIVER_OSS,       "oss"},
  {IDS_DRIVER_COREAUDIO, "coreaudio"},
  {IDS_DRIVER_JACK,      "jack"},
  {0, ""}
};

/* list of available drivers */
static AUDIO_DRIVER * loadedAudioDrv;

/* local copy of registry setting */
static char curAudioDriver[1024];

/* driver index to configure */
static int toConfigure;

/* display a driver specific configuration dialog */
static void configureAudioDriver(HWND hDlg)
{
    const AUDIO_DRIVER *pAudioDrv = &loadedAudioDrv[toConfigure];

    if (strlen(pAudioDrv->szDriver) != 0)
    {
        HDRVR hdrvr;
        char wine_driver[MAX_NAME_LENGTH + 9];
        sprintf(wine_driver, "wine%s.drv", pAudioDrv->szDriver);
        hdrvr = pAudioDrv->hDriver;
        if (hdrvr != 0)
        {
            if (SendDriverMessage(hdrvr, DRV_QUERYCONFIGURE, 0, 0) != 0)
            {
                DRVCONFIGINFO dci;
                dci.dwDCISize = sizeof (dci);
                dci.lpszDCISectionName = NULL;
                dci.lpszDCIAliasName = NULL;
                SendDriverMessage(hdrvr, DRV_CONFIGURE, 0, (LONG_PTR)&dci);
            }
        }
        else
        {
            WCHAR wine_driverW[MAX_NAME_LENGTH+9];
            WCHAR messageStr[256];
            WCHAR str[1024];
            
            MultiByteToWideChar (CP_ACP, 0, wine_driver, -1, wine_driverW, 
                sizeof (wine_driverW)/sizeof(wine_driverW[0])); 
            
            LoadStringW (GetModuleHandle (NULL), IDS_OPEN_DRIVER_ERROR, messageStr,
                sizeof(messageStr)/sizeof(messageStr[0]));
            wsprintfW (str, messageStr, wine_driverW);
            MessageBoxW (hDlg, str, NULL, MB_OK | MB_ICONERROR);
        }
    }
}

/* is driver in local copy of driver registry string */
static BOOL isDriverSet(const char * driver)
{
    WINE_TRACE("driver = %s, curAudioDriver = %s\n", driver, curAudioDriver);

    if (strstr(curAudioDriver, driver))
        return TRUE;

    return FALSE;
}

/* add driver to local copy of driver registry string */
static void addDriver(const char * driver)
{
    if (!isDriverSet(driver))
    {
        if (strlen(curAudioDriver))
            strcat(curAudioDriver, ",");
        strcat(curAudioDriver, driver);
    }
}

/* remove driver from local copy of driver registry string */
static void removeDriver(const char * driver)
{
    char pattern[32], *p;
    int drvlen, listlen;

    strcpy(pattern, ",");
    strcat(pattern, driver);
    strcat(pattern, ",");
    drvlen = strlen(driver);
    listlen = strlen(curAudioDriver);

    p = strstr(curAudioDriver, pattern);
    if (p) /* somewhere in the middle */
        memmove(p, p+drvlen+1, strlen(p+drvlen+1)+1);
    else if (!strncmp(curAudioDriver, pattern+1, drvlen+1)) /* the head */
        memmove(curAudioDriver, curAudioDriver+drvlen+1, listlen-drvlen);
    else if (!strncmp(curAudioDriver+listlen-drvlen-1, pattern, drvlen+1)) /* the tail */
        curAudioDriver[listlen-drvlen-1] = 0;
    else if (!strcmp(curAudioDriver, driver)) /* only one entry (head&tail) */
        curAudioDriver[0] = 0;
    else
        WINE_FIXME("driver '%s' is not in the list, please report!\n", driver);
}

static void initAudioDeviceTree(HWND hDlg)
{
    AUDIO_DRIVER *pAudioDrv = NULL;
    int i, j;
    TVINSERTSTRUCTW insert;
    HTREEITEM root, driver[10];
    HWND tree = NULL;
    HIMAGELIST hImageList;
    HBITMAP hBitMap;
    HCURSOR old_cursor;
    WCHAR driver_type[64], dev_type[64];

    tree = GetDlgItem(hDlg, IDC_AUDIO_TREE);

    if (!tree)
        return;

    /* set tree view style */
    SetWindowLong(tree, GWL_STYLE, GetWindowLong(tree, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);

    /* state checkbox */
    hImageList = ImageList_Create(16, 16, FALSE, 3, 0);
    hBitMap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CHECKBOX));
    ImageList_Add(hImageList, hBitMap, NULL);
    DeleteObject(hBitMap);
    SendMessageW( tree, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)hImageList );

    /* root item */
    LoadStringW (GetModuleHandle (NULL), IDS_SOUNDDRIVERS, driver_type,
        sizeof(driver_type)/sizeof(driver_type[0]));
    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = TVI_LAST;
    insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
    insert.u.item.pszText = driver_type;
    insert.u.item.cChildren = 1;
    root = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

    /* change to the wait cursor because this can take a while if there is a
     * misbehaving driver that takes a long time to open
     */
    old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

    /* iterate over list of loaded drivers */
    for (pAudioDrv = loadedAudioDrv, i = 0; pAudioDrv->nameID; i++, pAudioDrv++) {
        HDRVR hdrv;
        char name[MAX_PATH];
        WCHAR text[MAX_PATH];

        sprintf(name, "wine%s.drv", pAudioDrv->szDriver);
        LoadStringW (GetModuleHandle (NULL), pAudioDrv->nameID, text,
            sizeof(text)/sizeof(text[0]));

        if ((hdrv = pAudioDrv->hDriver))
        {
            HMODULE lib;
            if ((lib = GetDriverModuleHandle(hdrv)))
            {
                int num_wod = 0, num_wid = 0, num_mod = 0, num_mid = 0, num_aux = 0, num_mxd = 0;
                MessagePtr wodMessagePtr = (MessagePtr)GetProcAddress(lib, "wodMessage");
                MessagePtr widMessagePtr = (MessagePtr)GetProcAddress(lib, "widMessage");
                MessagePtr modMessagePtr = (MessagePtr)GetProcAddress(lib, "modMessage");
                MessagePtr midMessagePtr = (MessagePtr)GetProcAddress(lib, "midMessage");
                MessagePtr auxMessagePtr = (MessagePtr)GetProcAddress(lib, "auxMessage");
                MessagePtr mxdMessagePtr = (MessagePtr)GetProcAddress(lib, "mxdMessage");

                if (wodMessagePtr)
                    num_wod = wodMessagePtr(0, WODM_GETNUMDEVS, 0, 0, 0);

                if (widMessagePtr)
                    num_wid = widMessagePtr(0, WIDM_GETNUMDEVS, 0, 0, 0);

                if (modMessagePtr)
                    num_mod = modMessagePtr(0, MODM_GETNUMDEVS, 0, 0, 0);

                if (midMessagePtr)
                    num_mid = midMessagePtr(0, MIDM_GETNUMDEVS, 0, 0, 0);

                if (auxMessagePtr)
                    num_aux = auxMessagePtr(0, AUXDM_GETNUMDEVS, 0, 0, 0);

                if (mxdMessagePtr)
                    num_mxd = mxdMessagePtr(0, MXDM_GETNUMDEVS, 0, 0, 0);

                if (num_wod == 0 && num_wid == 0 && num_mod == 0 && num_mid == 0 && num_aux == 0 && num_mxd == 0)
                {
                    insert.hParent = root;
                    insert.u.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
                    insert.u.item.pszText = text;
                    insert.u.item.stateMask = TVIS_STATEIMAGEMASK;
                    insert.u.item.lParam =  i + DRIVER_MASK;
                    if (isDriverSet(pAudioDrv->szDriver))
                        insert.u.item.state = INDEXTOSTATEIMAGEMASK(2);
                    else
                        insert.u.item.state = INDEXTOSTATEIMAGEMASK(1);

                    driver[i] = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                }
                else
                {
                    HTREEITEM type;

                    insert.hParent = root;
                    insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE | TVIF_PARAM;
                    insert.u.item.pszText = text;
                    insert.u.item.cChildren = 1;
                    insert.u.item.stateMask = TVIS_STATEIMAGEMASK;
                    insert.u.item.lParam =  i + DRIVER_MASK;

                    if (isDriverSet(pAudioDrv->szDriver))
                        insert.u.item.state = INDEXTOSTATEIMAGEMASK(2);
                    else
                        insert.u.item.state = INDEXTOSTATEIMAGEMASK(1);

                    driver[i] = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                    if (num_wod)
                    {
                        LoadStringW (GetModuleHandle (NULL), IDS_DEVICES_WAVEOUT, dev_type,
                            sizeof(dev_type)/sizeof(dev_type[0]));

                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = dev_type;
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                        for (j = 0; j < num_wod; j++)
                        {
                            WAVEOUTCAPSW caps;

                            wodMessagePtr(j, WODM_GETDEVCAPS, 0, (DWORD_PTR)&caps, sizeof(caps));

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                            insert.u.item.pszText = caps.szPname;
                            insert.u.item.lParam = j + DEVICE_MASK;

                            SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_wid)
                    {
                        LoadStringW (GetModuleHandle (NULL), IDS_DEVICES_WAVEIN, dev_type,
                            sizeof(dev_type)/sizeof(dev_type[0]));

                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = dev_type;
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                        for (j = 0; j < num_wid; j++)
                        {
                            WAVEINCAPSW caps;

                            widMessagePtr(j, WIDM_GETDEVCAPS, 0, (DWORD_PTR)&caps, sizeof(caps));

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                            insert.u.item.pszText = caps.szPname;
                            insert.u.item.lParam = j + DEVICE_MASK;

                            SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_mod)
                    {
                        LoadStringW (GetModuleHandle (NULL), IDS_DEVICES_MIDIOUT, dev_type,
                            sizeof(dev_type)/sizeof(dev_type[0]));

                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = dev_type;
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                        for (j = 0; j < num_mod; j++)
                        {
                            MIDIOUTCAPSW caps;

                            modMessagePtr(j, MODM_GETDEVCAPS, 0, (DWORD_PTR)&caps, sizeof(caps));

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                            insert.u.item.pszText = caps.szPname;
                            insert.u.item.lParam = j + DEVICE_MASK;

                            SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_mid)
                    {
                        LoadStringW (GetModuleHandle (NULL), IDS_DEVICES_MIDIIN, dev_type,
                            sizeof(dev_type)/sizeof(dev_type[0]));

                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = dev_type;
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                        for (j = 0; j < num_mid; j++)
                        {
                            MIDIINCAPSW caps;

                            midMessagePtr(j, MIDM_GETDEVCAPS, 0, (DWORD_PTR)&caps, sizeof(caps));

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                            insert.u.item.pszText = caps.szPname;
                            insert.u.item.lParam = j + DEVICE_MASK;

                            SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_aux)
                    {
                        LoadStringW (GetModuleHandle (NULL), IDS_DEVICES_AUX, dev_type,
                            sizeof(dev_type)/sizeof(dev_type[0]));

                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = dev_type;
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                        for (j = 0; j < num_aux; j++)
                        {
                            AUXCAPSW caps;

                            auxMessagePtr(j, AUXDM_GETDEVCAPS, 0, (DWORD_PTR)&caps, sizeof(caps));

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                            insert.u.item.pszText = caps.szPname;
                            insert.u.item.lParam = j + DEVICE_MASK;

                            SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_mxd)
                    {
                        LoadStringW (GetModuleHandle (NULL), IDS_DEVICES_MIXER, dev_type,
                            sizeof(dev_type)/sizeof(dev_type[0]));

                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = dev_type;
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);

                        for (j = 0; j < num_mxd; j++)
                        {
                            MIXERCAPSW caps;

                            mxdMessagePtr(j, MXDM_GETDEVCAPS, 0, (DWORD_PTR)&caps, sizeof(caps));

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                            insert.u.item.pszText = caps.szPname;
                            insert.u.item.lParam = j + DEVICE_MASK;

                            SendDlgItemMessageW (hDlg, IDC_AUDIO_TREE, TVM_INSERTITEMW, 0, (LPARAM)&insert);
                        }
                    }
                }
            }
        }
    }

    /* restore the original cursor */
    SetCursor(old_cursor);

    SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_SELECTITEM, 0, 0);
    SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_EXPAND, TVE_EXPAND, (LPARAM)root);
    for (j = 0; j < i; j++)
        SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_EXPAND, TVE_EXPAND, (LPARAM)driver[j]);
}

/* find all drivers that can be loaded */
static void findAudioDrivers(void)
{
    int numFound = 0;
    AUDIO_DRIVER *pAudioDrv = NULL;
    HCURSOR old_cursor;

    /* delete an existing list */
    HeapFree(GetProcessHeap(), 0, loadedAudioDrv);
    loadedAudioDrv = 0;

    /* change to the wait cursor because this can take a while if there is a
     * misbehaving driver that takes a long time to open
     */
    old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

    for (pAudioDrv = sAudioDrivers; pAudioDrv->nameID; pAudioDrv++)
    {
        if (strlen(pAudioDrv->szDriver))
        {
            HDRVR hdrv;
            char driver[MAX_PATH];

            sprintf(driver, "wine%s.drv", pAudioDrv->szDriver);

            hdrv = pAudioDrv->hDriver;
            if (!pAudioDrv->hDriver && (hdrv = OpenDriverA(driver, 0, 0))) {
                HMODULE lib = GetDriverModuleHandle(hdrv);
                MessagePtr wodMessagePtr = (MessagePtr)GetProcAddress(lib, "wodMessage");
                MessagePtr widMessagePtr = (MessagePtr)GetProcAddress(lib, "widMessage");
                MessagePtr modMessagePtr = (MessagePtr)GetProcAddress(lib, "modMessage");
                MessagePtr midMessagePtr = (MessagePtr)GetProcAddress(lib, "midMessage");
                MessagePtr auxMessagePtr = (MessagePtr)GetProcAddress(lib, "auxMessage");
                MessagePtr mxdMessagePtr = (MessagePtr)GetProcAddress(lib, "mxdMessage");

                pAudioDrv->hDriver = hdrv;

                if (wodMessagePtr)
                    wodMessagePtr(0, DRVM_INIT, 0, 0, 0);

                if (widMessagePtr)
                    widMessagePtr(0, DRVM_INIT, 0, 0, 0);

                if (modMessagePtr)
                    modMessagePtr(0, DRVM_INIT, 0, 0, 0);

                if (midMessagePtr)
                    midMessagePtr(0, DRVM_INIT, 0, 0, 0);

                if (auxMessagePtr)
                    auxMessagePtr(0, DRVM_INIT, 0, 0, 0);

                if (mxdMessagePtr)
                    mxdMessagePtr(0, DRVM_INIT, 0, 0, 0);
            }
            if (hdrv)
            {
                if (loadedAudioDrv)
                    loadedAudioDrv = HeapReAlloc(GetProcessHeap(), 0, loadedAudioDrv, (numFound + 1) * sizeof(AUDIO_DRIVER));
                else
                    loadedAudioDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(AUDIO_DRIVER));

                CopyMemory(&loadedAudioDrv[numFound], pAudioDrv, sizeof(AUDIO_DRIVER));
                numFound++;
            }
        }
    }

    /* restore the original cursor */
    SetCursor(old_cursor);

    /* terminate list with empty driver */
    if (numFound) {
        loadedAudioDrv = HeapReAlloc(GetProcessHeap(), 0, loadedAudioDrv, (numFound + 1) * sizeof(AUDIO_DRIVER));
        CopyMemory(&loadedAudioDrv[numFound], pAudioDrv, sizeof(AUDIO_DRIVER));
    } else {
        loadedAudioDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(AUDIO_DRIVER));
        CopyMemory(&loadedAudioDrv[0], pAudioDrv, sizeof(AUDIO_DRIVER));
    }
}

/* check local copy of registry string for unloadable drivers */
static void checkRegistrySetting(HWND hDlg)
{
    const AUDIO_DRIVER *pAudioDrv;
    char * token, * tokens;

    tokens = HeapAlloc(GetProcessHeap(), 0, strlen(curAudioDriver)+1);
    strcpy(tokens, curAudioDriver);

start_over:
    token = strtok(tokens, ",");
    while (token != NULL)
    {
        BOOL found = FALSE;
        for (pAudioDrv = loadedAudioDrv; pAudioDrv->nameID; pAudioDrv++)
        {
            if (strcmp(token, pAudioDrv->szDriver) == 0)
            {
                found = TRUE;
                break;
            }
        }
        if (found == FALSE)
        {
            WCHAR tokenW[MAX_NAME_LENGTH+1];
            WCHAR messageStr[256];
            WCHAR str[1024];
            WCHAR caption[64];
            
            MultiByteToWideChar (CP_ACP, 0, token, -1, tokenW, sizeof(tokenW)/sizeof(tokenW[0]));
            
            LoadStringW (GetModuleHandle (NULL), IDS_UNAVAILABLE_DRIVER, messageStr,
                sizeof(messageStr)/sizeof(messageStr[0]));
            wsprintfW (str, messageStr, tokenW);
            LoadStringW (GetModuleHandle (NULL), IDS_WARNING, caption,
                sizeof(caption)/sizeof(caption[0]));
            if (MessageBoxW (hDlg, str, caption, MB_ICONWARNING | MB_YESNOCANCEL) == IDYES)
            {
                removeDriver(token);
                strcpy(tokens, curAudioDriver);
                goto start_over;
            }
        }
        token = strtok(NULL, ",");
    }
    HeapFree(GetProcessHeap(), 0, tokens);
}

static void selectDriver(HWND hDlg, const char * driver)
{
    WCHAR text[1024];
    WCHAR caption[64];

    strcpy(curAudioDriver, driver);
    set_reg_key(config_key, "Drivers", "Audio", curAudioDriver);

    if (LoadStringW(GetModuleHandle(NULL), IDS_AUDIO_MISSING, text, sizeof(text)/sizeof(text[0])))
    {
        if (LoadStringW(GetModuleHandle(NULL), IDS_WINECFG_TITLE, caption, sizeof(caption)/sizeof(caption[0])))
            MessageBoxW(hDlg, text, caption, MB_OK | MB_ICONINFORMATION);
    }
   
    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0); /* enable apply button */
}

static void initAudioDlg (HWND hDlg)
{
    int i, j, found;
    char* buf = NULL;

    WINE_TRACE("\n");

    /* make a list of all drivers that can be loaded */
    findAudioDrivers();

    /* get current registry setting if available */
    buf = get_reg_key(config_key, "Drivers", "Audio", NULL);

    /* check for first time install and set a default driver
     * select first available driver, and if that fails: none
     */
    if (buf == NULL)
    {
        /* select first available driver */
        if (*loadedAudioDrv->szDriver)
            selectDriver(hDlg, loadedAudioDrv->szDriver);
    }
    else /* make a local copy of the current registry setting */
        strcpy(curAudioDriver, buf);

    WINE_TRACE("curAudioDriver = %s\n", curAudioDriver);

    /* check for drivers that can't be loaded */
    checkRegistrySetting(hDlg);

    initAudioDeviceTree(hDlg);

    SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_RESETCONTENT, 0, 0);
    buf = get_reg_key(config_key, keypath("DirectSound"), "HardwareAcceleration", "Full");

    j = found = 0;
    for (i = 0; 0 != DSound_HW_Accels[i].displayID; ++i) {
      WCHAR accelStr[64];
      int match;

      match = (strcmp(buf, DSound_HW_Accels[i].settingStr) == 0);
      if (match)
      {
        DSound_HW_Accels[i].visible = 1;
        found = 1;
      }

      if (DSound_HW_Accels[i].visible)
      {
        LoadStringW (GetModuleHandle (NULL), DSound_HW_Accels[i].displayID,
                     accelStr, sizeof(accelStr)/sizeof(accelStr[0]));
        SendDlgItemMessageW (hDlg, IDC_DSOUND_HW_ACCEL, CB_ADDSTRING, 0, (LPARAM)accelStr);
        if (match)
          SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_SETCURSEL, j, 0);
        j++;
      }
    }
    if (!found) {
      WINE_ERR("Invalid Direct Sound HW Accel read from registry (%s)\n", buf);
    }
    HeapFree(GetProcessHeap(), 0, buf);

    SendDlgItemMessage(hDlg, IDC_DSOUND_RATES, CB_RESETCONTENT, 0, 0);
    for (i = 0; NULL != DSound_Rates[i]; ++i) {
      SendDlgItemMessage(hDlg, IDC_DSOUND_RATES, CB_ADDSTRING, 0, (LPARAM) DSound_Rates[i]);
    }
    buf = get_reg_key(config_key, keypath("DirectSound"), "DefaultSampleRate", "44100");
    for (i = 0; NULL != DSound_Rates[i]; ++i) {
      if (strcmp(buf, DSound_Rates[i]) == 0) {
	SendDlgItemMessage(hDlg, IDC_DSOUND_RATES, CB_SETCURSEL, i, 0);
	break ;
      }
    }

    SendDlgItemMessage(hDlg, IDC_DSOUND_BITS, CB_RESETCONTENT, 0, 0);
    for (i = 0; NULL != DSound_Bits[i]; ++i) {
      SendDlgItemMessage(hDlg, IDC_DSOUND_BITS, CB_ADDSTRING, 0, (LPARAM) DSound_Bits[i]);
    }
    buf = get_reg_key(config_key, keypath("DirectSound"), "DefaultBitsPerSample", "16");
    for (i = 0; NULL != DSound_Bits[i]; ++i) {
      if (strcmp(buf, DSound_Bits[i]) == 0) {
	SendDlgItemMessage(hDlg, IDC_DSOUND_BITS, CB_SETCURSEL, i, 0);
	break ;
      }
    }
    HeapFree(GetProcessHeap(), 0, buf);
}

INT_PTR CALLBACK
AudioDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
          case IDC_AUDIO_CONFIGURE:
	     configureAudioDriver(hDlg);
	     break;
          case IDC_AUDIO_TEST:
	     if(!PlaySound(MAKEINTRESOURCE(IDW_TESTSOUND), NULL, SND_RESOURCE | SND_SYNC))
                MessageBox(NULL, "Audio test failed!", "Error", MB_OK | MB_ICONERROR);
             break;
          case IDC_AUDIO_CONTROL_PANEL:
	     MessageBox(NULL, "Launching audio control panel not implemented yet!", "Fixme", MB_OK | MB_ICONERROR);
             break;
          case IDC_DSOUND_HW_ACCEL:
	    if (HIWORD(wParam) == CBN_SELCHANGE) {
	      int selected_dsound_accel;
              int i, j = 0;

	      SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
	      selected_dsound_accel = SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_GETCURSEL, 0, 0);
              for (i = 0; DSound_HW_Accels[i].settingStr; ++i)
              {
                if (DSound_HW_Accels[i].visible)
                {
                  if (j == selected_dsound_accel)
                  {
                    set_reg_key(config_key, keypath("DirectSound"), "HardwareAcceleration",
                      DSound_HW_Accels[i].settingStr);
                    break;
                  }
                  j++;
                }
              }
	    }
	    break;
          case IDC_DSOUND_RATES:
	    if (HIWORD(wParam) == CBN_SELCHANGE) {
	      int selected_dsound_rate;
	      SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
	      selected_dsound_rate = SendDlgItemMessage(hDlg, IDC_DSOUND_RATES, CB_GETCURSEL, 0, 0);
	      set_reg_key(config_key, keypath("DirectSound"), "DefaultSampleRate", DSound_Rates[selected_dsound_rate]);
	    }
	    break;
          case IDC_DSOUND_BITS:
	    if (HIWORD(wParam) == CBN_SELCHANGE) {
	      int selected_dsound_bits;
	      SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
	      selected_dsound_bits = SendDlgItemMessage(hDlg, IDC_DSOUND_BITS, CB_GETCURSEL, 0, 0);
	      set_reg_key(config_key, keypath("DirectSound"), "DefaultBitsPerSample", DSound_Bits[selected_dsound_bits]);
	    }
	    break;
	}
	break;

      case WM_SHOWWINDOW:
        set_window_title(hDlg);
        break;

      case WM_NOTIFY:
	switch(((LPNMHDR)lParam)->code) {
	    case PSN_KILLACTIVE:
	      SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
	      break;
	    case PSN_APPLY:
	      set_reg_key(config_key, "Drivers", "Audio", curAudioDriver);
              apply();
	      SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
	      break;
	    case PSN_SETACTIVE:
	      break;
            case NM_CLICK:
              if (((LPNMHDR)lParam)->idFrom == IDC_AUDIO_TREE)
              {
                  TVHITTESTINFO ht;
                  DWORD dwPos = GetMessagePos();
                  HWND tree = ((LPNMHDR)lParam)->hwndFrom;
                  ZeroMemory(&ht, sizeof(ht));
                  ht.pt.x = (short)LOWORD(dwPos);
                  ht.pt.y = (short)HIWORD(dwPos);
                  MapWindowPoints(HWND_DESKTOP, tree, &ht.pt, 1);
                  SendMessageW( tree, TVM_HITTEST, 0, (LPARAM)&ht );
                  if (TVHT_ONITEMSTATEICON & ht.flags)
                  {
                      TVITEM tvItem;
                      int index;
                      ZeroMemory(&tvItem, sizeof(tvItem));
                      tvItem.hItem = ht.hItem;
                      SendMessageW( tree, TVM_GETITEMW, 0, (LPARAM) &tvItem );

                      index = TreeView_GetItemState(tree, ht.hItem, TVIS_STATEIMAGEMASK);
                      if (index == INDEXTOSTATEIMAGEMASK(1))
                      {
                          TreeView_SetItemState(tree, ht.hItem, INDEXTOSTATEIMAGEMASK(2), TVIS_STATEIMAGEMASK);
                          addDriver(loadedAudioDrv[tvItem.lParam & 0xff].szDriver);
                          SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0); /* enable apply button */
                      }
                      else if (index == INDEXTOSTATEIMAGEMASK(2))
                      {
                          TreeView_SetItemState(tree, ht.hItem, INDEXTOSTATEIMAGEMASK(1), TVIS_STATEIMAGEMASK);
                          removeDriver(loadedAudioDrv[tvItem.lParam & 0xff].szDriver);
                          SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0); /* enable apply button */
                      }
                  }
              }
              break;
            case NM_RCLICK:
              if (((LPNMHDR)lParam)->idFrom == IDC_AUDIO_TREE)
              {
                  TVHITTESTINFO ht;
                  DWORD dwPos = GetMessagePos();
                  HWND tree = ((LPNMHDR)lParam)->hwndFrom;
                  POINT pt;
                  ZeroMemory(&ht, sizeof(ht));
                  pt.x = (short)LOWORD(dwPos);
                  pt.y = (short)HIWORD(dwPos);
                  ht.pt = pt;
                  MapWindowPoints(HWND_DESKTOP, tree, &ht.pt, 1);
                  SendMessageW( tree, TVM_HITTEST, 0, (LPARAM)&ht );
                  if (TVHT_ONITEMLABEL & ht.flags)
                  {
                      TVITEM tvItem;
                      ZeroMemory(&tvItem, sizeof(tvItem));
                      tvItem.hItem = ht.hItem;
                      tvItem.mask = TVIF_PARAM;
                      tvItem.lParam = -1;
                      if (TreeView_GetItem(tree, &tvItem))
                      {
                          if (tvItem.lParam & DRIVER_MASK)
                          {
                              if (hPopupMenus)
                              {
                                  TrackPopupMenu(GetSubMenu(hPopupMenus, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, tree, NULL);
                                  toConfigure = tvItem.lParam & ~DRIVER_MASK;
                              }
                          }
                          else if (tvItem.lParam & DEVICE_MASK)
                          {
                              /* FIXME TBD */
                          }

                      }
                  }
              }
	}
	break;

  case WM_INITDIALOG:
    initAudioDlg(hDlg);
    break;
  }

  return FALSE;
}
