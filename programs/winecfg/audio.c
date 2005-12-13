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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wine/debug.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <mmddk.h>

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

typedef DWORD (WINAPI * MessagePtr)(UINT, UINT, DWORD, DWORD, DWORD);

static const char* DSound_HW_Accels[] = {
  "Full",
  "Standard",
  "Basic",
  "Emulation",
  NULL
};

/* Select the correct entry in the combobox based on drivername */
static void selectAudioDriver(HWND hDlg, const char *drivername)
{
  int i;
  const AUDIO_DRIVER *pAudioDrv = NULL;

  if ((pAudioDrv = getAudioDrivers()))
  {
    for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++)
    {
      if (!strcmp (pAudioDrv->szDriver, drivername))
      {
	set_reg_key(config_key, "Drivers", "Audio", (char *) pAudioDrv->szDriver);
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0); /* enable apply button */
	SendDlgItemMessage(hDlg, IDC_AUDIO_DRIVER, CB_SETCURSEL,
			   (WPARAM) i, 0);
      }
    }
  }
}

static void configureAudioDriver(HWND hDlg, const char *drivername)
{
  int i;
  const AUDIO_DRIVER *pAudioDrv = NULL;

  if ((pAudioDrv = getAudioDrivers()))
  {
    for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++)
    {
      if (!strcmp (pAudioDrv->szDriver, drivername))
      {
        if (strlen(pAudioDrv->szDriver) != 0)
        {
          HDRVR hdrvr;
	  char wine_driver[MAX_NAME_LENGTH + 8];
	  sprintf(wine_driver, "wine%s.drv", pAudioDrv->szDriver);
          hdrvr = OpenDriverA(wine_driver, 0, 0);
	  if (hdrvr != 0)
	  {
	    if (SendDriverMessage(hdrvr, DRV_QUERYCONFIGURE, 0, 0) != 0)
	    {
              DRVCONFIGINFO dci;
              LONG lRes;
              dci.dwDCISize = sizeof (dci);
              dci.lpszDCISectionName = NULL;
              dci.lpszDCIAliasName = NULL;
              lRes = SendDriverMessage(hdrvr, DRV_CONFIGURE, 0, (LONG)&dci);
	    }
	    CloseDriver(hdrvr, 0, 0);
	  }
          else
          {
	    char str[1024];
	    sprintf(str, "Couldn't open %s!", wine_driver);
	    MessageBox(NULL, str, "Fixme", MB_OK | MB_ICONERROR);
          }
	}
	break;
      }
    }
  }
}

static void initAudioDeviceTree(HWND hDlg)
{
    const AUDIO_DRIVER *pAudioDrv = NULL;
    int i, j;
    TVINSERTSTRUCT insert;
    HTREEITEM root, driver[10];
    HWND tree = NULL;

    tree = GetDlgItem(hDlg, IDC_AUDIO_TREE);

    if (!tree)
        return;

    SetWindowLong(tree, GWL_STYLE, GetWindowLong(tree, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);

    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = TVI_LAST;
    insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
    insert.u.item.pszText = "Sound Drivers";
    insert.u.item.cChildren = 1;

    root = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

    pAudioDrv = getAudioDrivers();

    for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++) {
        HDRVR hdrv;
        char name[MAX_PATH];
        char text[MAX_PATH];

        if (strlen(pAudioDrv->szDriver) == 0)
            continue;

        sprintf(name, "wine%s.drv", pAudioDrv->szDriver);
        sprintf(text, "%s Driver", pAudioDrv->szName);

        hdrv = OpenDriverA(name, 0, 0);
        if (hdrv == 0) /* no driver loaded */
        {
            insert.hParent = root;
            insert.u.item.mask = TVIF_TEXT;
            insert.u.item.pszText = text;

            driver[i] = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
        }
        else
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
                    insert.u.item.mask = TVIF_TEXT;
                    insert.u.item.pszText = text;

                    driver[i] = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                }
                else
                {
                    HTREEITEM type;

                    insert.hParent = root;
                    insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                    insert.u.item.pszText = text;
                    insert.u.item.cChildren = 1;

                    driver[i] = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                    if (num_wod)
                    {
                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = "Wave Out Devices";
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                        for (j = 0; j < num_wod; j++)
                        {
                            WAVEOUTCAPSW caps;
                            char szPname[MAXPNAMELEN];

                            wodMessagePtr(j, WODM_GETDEVCAPS, 0, (DWORD)&caps, sizeof(caps)); 
                            WideCharToMultiByte(CP_ACP, 0, caps.szPname, -1, szPname, MAXPNAMELEN, 0, 0);

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT;
                            insert.u.item.pszText = szPname;

                            SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_wid)
                    {
                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = "Wave In Devices";
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                        for (j = 0; j < num_wid; j++)
                        {
                            WAVEINCAPSW caps;
                            char szPname[MAXPNAMELEN];

                            widMessagePtr(j, WIDM_GETDEVCAPS, 0, (DWORD)&caps, sizeof(caps)); 
                            WideCharToMultiByte(CP_ACP, 0, caps.szPname, -1, szPname, MAXPNAMELEN, 0, 0);

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT;
                            insert.u.item.pszText = szPname;

                            SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_mod)
                    {
                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = "MIDI Out Devices";
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                        for (j = 0; j < num_mod; j++)
                        {
                            MIDIOUTCAPSW caps;
                            char szPname[MAXPNAMELEN];

                            modMessagePtr(j, MODM_GETDEVCAPS, 0, (DWORD)&caps, sizeof(caps)); 
                            WideCharToMultiByte(CP_ACP, 0, caps.szPname, -1, szPname, MAXPNAMELEN, 0, 0);

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT;
                            insert.u.item.pszText = szPname;

                            SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_mid)
                    {
                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = "MIDI In Devices";
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                        for (j = 0; j < num_mid; j++)
                        {
                            MIDIINCAPSW caps;
                            char szPname[MAXPNAMELEN];

                            midMessagePtr(j, MIDM_GETDEVCAPS, 0, (DWORD)&caps, sizeof(caps)); 
                            WideCharToMultiByte(CP_ACP, 0, caps.szPname, -1, szPname, MAXPNAMELEN, 0, 0);

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT;
                            insert.u.item.pszText = szPname;

                            SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_aux)
                    {
                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = "Aux Devices";
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                        for (j = 0; j < num_aux; j++)
                        {
                            AUXCAPSW caps;
                            char szPname[MAXPNAMELEN];

                            auxMessagePtr(j, AUXDM_GETDEVCAPS, 0, (DWORD)&caps, sizeof(caps)); 
                            WideCharToMultiByte(CP_ACP, 0, caps.szPname, -1, szPname, MAXPNAMELEN, 0, 0);

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT;
                            insert.u.item.pszText = szPname;

                            SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                        }
                    }

                    if (num_mxd)
                    {
                        insert.hParent = driver[i];
                        insert.u.item.mask = TVIF_TEXT | TVIF_CHILDREN;
                        insert.u.item.pszText = "Mixer Devices";
                        insert.u.item.cChildren = 1;

                        type = (HTREEITEM)SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);

                        for (j = 0; j < num_mxd; j++)
                        {
                            MIXERCAPSW caps;
                            char szPname[MAXPNAMELEN];

                            mxdMessagePtr(j, MXDM_GETDEVCAPS, 0, (DWORD)&caps, sizeof(caps)); 
                            WideCharToMultiByte(CP_ACP, 0, caps.szPname, -1, szPname, MAXPNAMELEN, 0, 0);

                            insert.hParent = type;
                            insert.u.item.mask = TVIF_TEXT;
                            insert.u.item.pszText = szPname;

                            SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_INSERTITEM, 0, (LPARAM)&insert);
                        }
                    }
                }
            }
        }
    }

    SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_SELECTITEM, 0, 0);
    SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_EXPAND, TVE_EXPAND, (LPARAM)root);
    for (j = 0; j < i; j++)
        SendDlgItemMessage(hDlg, IDC_AUDIO_TREE, TVM_EXPAND, TVE_EXPAND, (LPARAM)driver[j]);
}

static void initAudioDlg (HWND hDlg)
{
    char *curAudioDriver = get_reg_key(config_key, "Drivers", "Audio", "alsa");
    const AUDIO_DRIVER *pAudioDrv = NULL;
    int i;
    char* buf = NULL;

    WINE_TRACE("\n");

    pAudioDrv = getAudioDrivers ();
    for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++) {
        SendDlgItemMessage (hDlg, IDC_AUDIO_DRIVER, CB_ADDSTRING, 
                0, (LPARAM) pAudioDrv->szName);
        if (!strcmp (pAudioDrv->szDriver, curAudioDriver)) {
            SendDlgItemMessage(hDlg, IDC_AUDIO_DRIVER, CB_SETCURSEL, i, 0);
        }
    }

    initAudioDeviceTree(hDlg);

    SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_RESETCONTENT, 0, 0);
    for (i = 0; NULL != DSound_HW_Accels[i]; ++i) {
      SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_ADDSTRING, 0, (LPARAM) DSound_HW_Accels[i]);
    }    
    buf = get_reg_key(config_key, keypath("DirectSound"), "HardwareAcceleration", "Full"); 
    for (i = 0; NULL != DSound_HW_Accels[i]; ++i) {
      if (strcmp(buf, DSound_HW_Accels[i]) == 0) {
	SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_SETCURSEL, i, 0);
	break ;
      }
    }
    if (NULL == DSound_HW_Accels[i]) {
      WINE_ERR("Invalid Direct Sound HW Accel read from registry (%s)\n", buf);
    }
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("DirectSound"), "EmulDriver", "N");
    if (IS_OPTION_TRUE(*buf))
      CheckDlgButton(hDlg, IDC_DSOUND_DRV_EMUL, BST_CHECKED);
    else
      CheckDlgButton(hDlg, IDC_DSOUND_DRV_EMUL, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

}

static const char *audioAutoDetect(void)
{
    const char *driversFound[10];
    const char *name[10];
    int numFound = 0;
    const AUDIO_DRIVER *pAudioDrv = NULL;
    int i;

    pAudioDrv = getAudioDrivers();

    for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++)
    {
        if (strlen(pAudioDrv->szDriver))
        {
            HDRVR hdrv;
            char driver[MAX_PATH];

            sprintf(driver, "wine%s.drv", pAudioDrv->szDriver);

            if ((hdrv = OpenDriverA(driver, 0, 0)))
            {
                CloseDriver(hdrv, 0, 0);
                driversFound[numFound] = pAudioDrv->szDriver;
                name[numFound] = pAudioDrv->szName;
                numFound++;
            }
        }
    }

  if(numFound == 0)
  {
    MessageBox(NULL, "Could not detect any audio devices/servers", "Failed", MB_OK);
    return "";
  }
  else
  {
    /* TODO: possibly smarter handling of multiple drivers? */
    char text[128];
    sprintf(text, "Found ");
    for(i=0;i<numFound;i++)
    {
      strcat(text, name[i]);
      if(i != numFound-1)
        strcat(text,", ");
    }
    MessageBox(NULL, (LPCTSTR)text, "Successful", MB_OK);
    return driversFound[0];
  }
}


INT_PTR CALLBACK
AudioDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	   case IDC_AUDIO_AUTODETECT:
	      selectAudioDriver(hDlg, audioAutoDetect());
	      break;
	   case IDC_AUDIO_DRIVER:
	     if ((HIWORD(wParam) == CBN_SELCHANGE) ||
		 (HIWORD(wParam) == CBN_SELCHANGE))
	     {
		const AUDIO_DRIVER *pAudioDrv = getAudioDrivers();
		int selected_driver = SendDlgItemMessage(hDlg, IDC_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);
		selectAudioDriver(hDlg, (char*)pAudioDrv[selected_driver].szDriver);
	     }
	     break;
          case IDC_AUDIO_CONFIGURE:
	     {
		const AUDIO_DRIVER *pAudioDrv = getAudioDrivers();
		int selected_driver = SendDlgItemMessage(hDlg, IDC_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);
		configureAudioDriver(hDlg, (char*)pAudioDrv[selected_driver].szDriver);
	     }
	     break;
          case IDC_AUDIO_CONTROL_PANEL:
	     MessageBox(NULL, "Launching audio control panel not implemented yet!", "Fixme", MB_OK | MB_ICONERROR);
             break;
          case IDC_DSOUND_HW_ACCEL:
	    if (HIWORD(wParam) == CBN_SELCHANGE) {
	      int selected_dsound_accel;
	      SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
	      selected_dsound_accel = SendDlgItemMessage(hDlg, IDC_DSOUND_HW_ACCEL, CB_GETCURSEL, 0, 0);
	      set_reg_key(config_key, keypath("DirectSound"), "HardwareAcceleration", DSound_HW_Accels[selected_dsound_accel]); 
	    }
	    break;
          case IDC_DSOUND_DRV_EMUL:
	    if (HIWORD(wParam) == BN_CLICKED) {
	      SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0); 
	      if (IsDlgButtonChecked(hDlg, IDC_DSOUND_DRV_EMUL) == BST_CHECKED)
		set_reg_key(config_key, keypath("DirectSound"), "EmulDriver", "Y");
	      else
		set_reg_key(config_key, keypath("DirectSound"), "EmulDriver", "N");
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
              apply();
	      SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
	      break;
	    case PSN_SETACTIVE:
	      break;
	}
	break;

  case WM_INITDIALOG:
    initAudioDlg(hDlg);
    break;
  }

  return FALSE;
}
