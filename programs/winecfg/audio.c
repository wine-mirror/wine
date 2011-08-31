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

#define COBJMACROS
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

#include "ole2.h"
#include "initguid.h"
#include "devpkey.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "audiopolicy.h"

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

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

static void initAudioDlg (HWND hDlg)
{
    int i, j, found;
    char* buf = NULL;

    WINE_TRACE("\n");

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
          case IDC_AUDIO_TEST:
              if(!PlaySound(MAKEINTRESOURCE(IDW_TESTSOUND), NULL, SND_RESOURCE | SND_SYNC))
                  MessageBox(NULL, "Audio test failed!", "Error", MB_OK | MB_ICONERROR);
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
