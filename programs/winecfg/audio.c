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

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

/* Select the correct entry in the combobox based on drivername */
void selectAudioDriver(HWND hDlg, char *drivername)
{
  int i;
  const AUDIO_DRIVER *pAudioDrv = NULL;

  if ((pAudioDrv = getAudioDrivers()))
  {
    for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++)
    {
      if (!strcmp (pAudioDrv->szDriver, drivername))
      {
	addTransaction("Winmm", "Drivers", ACTION_SET, pAudioDrv->szDriver);
	SendDlgItemMessage(hDlg, IDC_AUDIO_DRIVER, CB_SETCURSEL,
			   (WPARAM) i, 0);
      }
    }
  }
}

void
initAudioDlg (HWND hDlg)
{
  char *curAudioDriver = getConfigValue("Winmm", "Drivers", "winealsa.drv");
  const AUDIO_DRIVER *pAudioDrv = NULL;
  int i;

    if ((pAudioDrv = getAudioDrivers ()))
    {
	for (i = 0; *pAudioDrv->szName; i++, pAudioDrv++)
	{
	    SendDlgItemMessage (hDlg, IDC_AUDIO_DRIVER, CB_ADDSTRING,
				0, (LPARAM) pAudioDrv->szName);
	    if (!strcmp (pAudioDrv->szDriver, curAudioDriver))
	      selectAudioDriver(hDlg, (char*)pAudioDrv->szDriver);
	}
    }
}

char *audioAutoDetect(void)
{
  struct stat buf;
  const char *argv_new[4];
  int fd;

  char *driversFound[10];
  char *name[10];
  int numFound = 0;

  argv_new[0] = "/bin/sh";
  argv_new[1] = "-c";
  argv_new[3] = NULL;

  /* try to detect arts */
  argv_new[2] = "ps awx|grep artsd|grep -v grep|grep artsd > /dev/null";
  if(!spawnvp(_P_WAIT, "/bin/sh", argv_new))
  {
    driversFound[numFound] = "winearts.drv";
    name[numFound] = "aRts";
    numFound++;
  }

  /* try to detect jack */
  argv_new[2] = "ps awx|grep jackd|grep -v grep|grep jackd > /dev/null";
  if(!spawnvp(_P_WAIT, "/bin/sh", argv_new))
  {
    driversFound[numFound] = "winejack.drv";
    name[numFound] = "jack";
    numFound++;
  }

  /* try to detect nas */
  /* TODO */

  /* try to detect audioIO (solaris) */
  /* TODO */

  /* try to detect alsa */
  if(!stat("/proc/asound", &buf))
  {
    driversFound[numFound] = "winealsa.drv";
    name[numFound] = "Alsa";
    numFound++;
  }

  /* try to detect oss */
  fd = open("/dev/dsp", O_WRONLY | O_NONBLOCK);
  if(fd)
  {
    close(fd);
    driversFound[numFound] = "wineoss.drv";
    name[numFound] = "OSS";
    numFound++;
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
    snprintf(text, sizeof(text), "Found %s", name[0]);
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
	}
	break;
	
      case WM_NOTIFY:
	switch(((LPNMHDR)lParam)->code) {
	    case PSN_KILLACTIVE:
	      SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
	      break;
	    case PSN_APPLY:
	      SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
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
