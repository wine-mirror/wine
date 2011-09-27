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

static BOOL get_driver_name(PROPVARIANT *pv)
{
    IMMDeviceEnumerator *devenum;
    IMMDevice *device;
    IPropertyStore *ps;
    HRESULT hr;

    static const WCHAR wine_info_deviceW[] = {'W','i','n','e',' ',
        'i','n','f','o',' ','d','e','v','i','c','e',0};

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&devenum);
    if(FAILED(hr))
        return FALSE;

    hr = IMMDeviceEnumerator_GetDevice(devenum, wine_info_deviceW, &device);
    IMMDeviceEnumerator_Release(devenum);
    if(FAILED(hr))
        return FALSE;

    hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &ps);
    if(FAILED(hr)){
        IMMDevice_Release(device);
        return FALSE;
    }

    hr = IPropertyStore_GetValue(ps,
            (const PROPERTYKEY *)&DEVPKEY_Device_Driver, pv);
    IPropertyStore_Release(ps);
    IMMDevice_Release(device);
    if(FAILED(hr))
        return FALSE;

    return TRUE;
}

static void initAudioDlg (HWND hDlg)
{
    WCHAR display_str[256], format_str[256], disabled_str[64];
    PROPVARIANT pv;

    WINE_TRACE("\n");

    LoadStringW(GetModuleHandle(NULL), IDS_AUDIO_DRIVER,
            format_str, sizeof(format_str) / sizeof(*format_str));
    LoadStringW(GetModuleHandle(NULL), IDS_AUDIO_DRIVER_NONE,
            disabled_str, sizeof(disabled_str) / sizeof(*disabled_str));

    PropVariantInit(&pv);
    if(get_driver_name(&pv) && pv.u.pwszVal[0] != '\0')
        wnsprintfW(display_str, sizeof(display_str) / sizeof(*display_str),
                format_str, pv.u.pwszVal);
    else
        wnsprintfW(display_str, sizeof(display_str) / sizeof(*display_str),
                format_str, disabled_str);
    PropVariantClear(&pv);

    SetDlgItemTextW(hDlg, IDC_AUDIO_DRIVER, display_str);
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
        }
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
