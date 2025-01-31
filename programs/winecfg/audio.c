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

#include <assert.h>
#include <stdlib.h>
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
#include "propkey.h"
#include "initguid.h"
#include "propkeydef.h"
#include "devpkey.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "audiopolicy.h"

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

struct DeviceInfo {
    WCHAR *id;
    PROPVARIANT name;
    int speaker_config;
};

static WCHAR g_drv_keyW[256] = L"Software\\Wine\\Drivers\\";

static UINT num_render_devs, num_capture_devs;
static struct DeviceInfo *render_devs, *capture_devs;
static UINT num_midi_devs;
static WCHAR **midi_devs;

static const struct
{
    int text_id;
    DWORD speaker_mask;
} speaker_configs[] =
{
    { IDS_AUDIO_SPEAKER_5POINT1, KSAUDIO_SPEAKER_5POINT1 },
    { IDS_AUDIO_SPEAKER_QUAD, KSAUDIO_SPEAKER_QUAD },
    { IDS_AUDIO_SPEAKER_STEREO, KSAUDIO_SPEAKER_STEREO },
    { IDS_AUDIO_SPEAKER_MONO, KSAUDIO_SPEAKER_MONO },
    { 0, 0 }
};

static BOOL load_device(IMMDevice *dev, struct DeviceInfo *info)
{
    IPropertyStore *ps;
    HRESULT hr;
    PROPVARIANT pv;
    UINT i;

    hr = IMMDevice_GetId(dev, &info->id);
    if(FAILED(hr)){
        info->id = NULL;
        return FALSE;
    }

    hr = IMMDevice_OpenPropertyStore(dev, STGM_READ, &ps);
    if(FAILED(hr)){
        CoTaskMemFree(info->id);
        info->id = NULL;
        return FALSE;
    }

    PropVariantInit(&info->name);

    hr = IPropertyStore_GetValue(ps,
            (PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &info->name);
    if(FAILED(hr)){
        CoTaskMemFree(info->id);
        info->id = NULL;
        IPropertyStore_Release(ps);
        return FALSE;
    }

    PropVariantInit(&pv);

    hr = IPropertyStore_GetValue(ps,
            &PKEY_AudioEndpoint_PhysicalSpeakers, &pv);

    info->speaker_config = -1;
    if(SUCCEEDED(hr) && pv.vt == VT_UI4){
        i = 0;
        while (speaker_configs[i].text_id != 0) {
            if ((speaker_configs[i].speaker_mask & pv.ulVal) == speaker_configs[i].speaker_mask) {
                info->speaker_config = i;
                break;
            }
            i++;
        }
    }

    /* fallback to stereo */
    if(info->speaker_config == -1)
        info->speaker_config = 2;

    IPropertyStore_Release(ps);

    return TRUE;
}

static BOOL load_devices(IMMDeviceEnumerator *devenum, EDataFlow dataflow,
        UINT *ndevs, struct DeviceInfo **out)
{
    IMMDeviceCollection *coll;
    UINT i;
    HRESULT hr;

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(devenum, dataflow,
            DEVICE_STATE_ACTIVE, &coll);
    if(FAILED(hr))
        return FALSE;

    hr = IMMDeviceCollection_GetCount(coll, ndevs);
    if(FAILED(hr)){
        IMMDeviceCollection_Release(coll);
        return FALSE;
    }

    if(*ndevs > 0){
        *out = malloc(sizeof(struct DeviceInfo) * (*ndevs));
        if(!*out){
            IMMDeviceCollection_Release(coll);
            return FALSE;
        }

        for(i = 0; i < *ndevs; ++i){
            IMMDevice *dev;

            hr = IMMDeviceCollection_Item(coll, i, &dev);
            if(FAILED(hr)){
                (*out)[i].id = NULL;
                continue;
            }

            load_device(dev, &(*out)[i]);

            IMMDevice_Release(dev);
        }
    }else
        *out = NULL;

    IMMDeviceCollection_Release(coll);

    return TRUE;
}

static BOOL load_midi_devices(UINT *ndevs, WCHAR ***out)
{
    UINT i;

    *ndevs = midiOutGetNumDevs();

    if(*ndevs > 0){
        *out = malloc(sizeof(WCHAR *) * (*ndevs));
        if(!*out)
            return FALSE;

        for(i = 0; i < *ndevs; ++i){
            MIDIOUTCAPSW caps;
            if (MMSYSERR_NOERROR != midiOutGetDevCapsW(i, &caps, sizeof(caps))){
                (*out)[i] = NULL;
                continue;
            }
            (*out)[i] = StrDupW(caps.szPname);
        }
    }else
        *out = NULL;

    return TRUE;
}

static BOOL get_driver_name(IMMDeviceEnumerator *devenum, PROPVARIANT *pv)
{
    IMMDevice *device;
    IPropertyStore *ps;
    HRESULT hr;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, eRender, eConsole, &device);
    if(FAILED(hr)) {
        ERR("Could not get default audio endpoint.\n");
        return FALSE;
    }

    hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &ps);
    if(FAILED(hr)){
        ERR("Could not get property store for default audio endpoint (0x%p).\n", device);
        IMMDevice_Release(device);
        return FALSE;
    }

    hr = IPropertyStore_GetValue(ps,
            (const PROPERTYKEY *)&DEVPKEY_Device_Driver, pv);
    IPropertyStore_Release(ps);
    IMMDevice_Release(device);
    if(FAILED(hr) || pv == NULL || pv->pwszVal == NULL) {
        ERR("Could not get device driver property value.\n");
        return FALSE;
    }

    return TRUE;
}

static void initAudioDlg (HWND hDlg)
{
    WCHAR display_str[256], format_str[256], sysdefault_str[256], disabled_str[64];
    IMMDeviceEnumerator *devenum;
    BOOL have_driver = FALSE;
    WCHAR *reg_midi_dev;
    HRESULT hr;
    UINT i;
    LVCOLUMNW lvcol;
    WCHAR colW[64], speaker_str[256];
    RECT rect;
    DWORD width;

    WINE_TRACE("\n");

    LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_DRIVER, format_str, ARRAY_SIZE(format_str));
    LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_DRIVER_NONE, disabled_str,
            ARRAY_SIZE(disabled_str));
    LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_SYSDEFAULT, sysdefault_str,
            ARRAY_SIZE(sysdefault_str));

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&devenum);
    if(SUCCEEDED(hr)){
        PROPVARIANT pv;

        load_devices(devenum, eRender, &num_render_devs, &render_devs);
        load_devices(devenum, eCapture, &num_capture_devs, &capture_devs);

        PropVariantInit(&pv);
        if(get_driver_name(devenum, &pv) && pv.pwszVal[0] != '\0'){
            have_driver = TRUE;
            swprintf(display_str, ARRAY_SIZE(display_str), format_str, pv.pwszVal);
            lstrcatW(g_drv_keyW, pv.pwszVal);
        }
        PropVariantClear(&pv);

        IMMDeviceEnumerator_Release(devenum);
    }

    load_midi_devices(&num_midi_devs, &midi_devs);

    SendDlgItemMessageW(hDlg, IDC_AUDIOOUT_DEVICE, CB_ADDSTRING,
            0, (LPARAM)sysdefault_str);
    SendDlgItemMessageW(hDlg, IDC_AUDIOOUT_DEVICE, CB_SETCURSEL, 0, 0);
    SendDlgItemMessageW(hDlg, IDC_VOICEOUT_DEVICE, CB_ADDSTRING,
            0, (LPARAM)sysdefault_str);
    SendDlgItemMessageW(hDlg, IDC_VOICEOUT_DEVICE, CB_SETCURSEL, 0, 0);

    SendDlgItemMessageW(hDlg, IDC_AUDIOIN_DEVICE, CB_ADDSTRING,
            0, (LPARAM)sysdefault_str);
    SendDlgItemMessageW(hDlg, IDC_AUDIOIN_DEVICE, CB_SETCURSEL, 0, 0);
    SendDlgItemMessageW(hDlg, IDC_VOICEIN_DEVICE, CB_ADDSTRING,
            0, (LPARAM)sysdefault_str);
    SendDlgItemMessageW(hDlg, IDC_VOICEIN_DEVICE, CB_SETCURSEL, 0, 0);
    SendDlgItemMessageW(hDlg, IDC_MIDI_DEVICE, CB_ADDSTRING,
            0, (LPARAM)sysdefault_str);
    SendDlgItemMessageW(hDlg, IDC_MIDI_DEVICE, CB_SETCURSEL, 0, 0);

    i = 0;
    while (speaker_configs[i].text_id != 0) {
        LoadStringW(GetModuleHandleW(NULL), speaker_configs[i].text_id, speaker_str,
                ARRAY_SIZE(speaker_str));

        SendDlgItemMessageW(hDlg, IDC_SPEAKERCONFIG_SPEAKERS, CB_ADDSTRING,
            0, (LPARAM)speaker_str);

        i++;
    }

    GetClientRect(GetDlgItem(hDlg, IDC_LIST_AUDIO_DEVICES), &rect);
    width = (rect.right - rect.left) * 3 / 5;

    LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_DEVICE, colW, ARRAY_SIZE(colW));
    lvcol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvcol.pszText = colW;
    lvcol.cchTextMax = lstrlenW(colW);
    lvcol.cx = width;
    SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcol);

    LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_SPEAKER_CONFIG, colW, ARRAY_SIZE(colW));
    lvcol.pszText = colW;
    lvcol.cchTextMax = lstrlenW(colW);
    lvcol.cx = rect.right - rect.left - width;
    SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvcol);

    EnableWindow(GetDlgItem(hDlg, IDC_SPEAKERCONFIG_SPEAKERS), 0);

    if(have_driver){
        WCHAR *reg_out_dev, *reg_vout_dev, *reg_in_dev, *reg_vin_dev;

        reg_out_dev = get_reg_key(HKEY_CURRENT_USER, g_drv_keyW, L"DefaultOutput", NULL);
        reg_vout_dev = get_reg_key(HKEY_CURRENT_USER, g_drv_keyW, L"DefaultVoiceOutput", NULL);
        reg_in_dev = get_reg_key(HKEY_CURRENT_USER, g_drv_keyW, L"DefaultInput", NULL);
        reg_vin_dev = get_reg_key(HKEY_CURRENT_USER, g_drv_keyW, L"DefaultVoiceInput", NULL);

        for(i = 0; i < num_render_devs; ++i){
            LVITEMW lvitem;

            if(!render_devs[i].id)
                continue;

            SendDlgItemMessageW(hDlg, IDC_AUDIOOUT_DEVICE, CB_ADDSTRING,
                    0, (LPARAM)render_devs[i].name.pwszVal);
            SendDlgItemMessageW(hDlg, IDC_AUDIOOUT_DEVICE, CB_SETITEMDATA,
                    i + 1, (LPARAM)&render_devs[i]);

            if(reg_out_dev && !wcscmp(render_devs[i].id, reg_out_dev)){
                SendDlgItemMessageW(hDlg, IDC_AUDIOOUT_DEVICE, CB_SETCURSEL, i + 1, 0);
                SendDlgItemMessageW(hDlg, IDC_SPEAKERCONFIG_SPEAKERS, CB_SETCURSEL, render_devs[i].speaker_config, 0);
            }

            SendDlgItemMessageW(hDlg, IDC_VOICEOUT_DEVICE, CB_ADDSTRING,
                    0, (LPARAM)render_devs[i].name.pwszVal);
            SendDlgItemMessageW(hDlg, IDC_VOICEOUT_DEVICE, CB_SETITEMDATA,
                    i + 1, (LPARAM)&render_devs[i]);
            if(reg_vout_dev && !wcscmp(render_devs[i].id, reg_vout_dev))
                SendDlgItemMessageW(hDlg, IDC_VOICEOUT_DEVICE, CB_SETCURSEL, i + 1, 0);

            lvitem.mask = LVIF_TEXT | LVIF_PARAM;
            lvitem.iItem = i;
            lvitem.iSubItem = 0;
            lvitem.pszText = render_devs[i].name.pwszVal;
            lvitem.cchTextMax = lstrlenW(lvitem.pszText);
            lvitem.lParam = (LPARAM)&render_devs[i];

            SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_INSERTITEMW, 0, (LPARAM)&lvitem);

            LoadStringW(GetModuleHandleW(NULL), speaker_configs[render_devs[i].speaker_config].text_id,
                    speaker_str, ARRAY_SIZE(speaker_str));

            lvitem.mask = LVIF_TEXT;
            lvitem.iItem = i;
            lvitem.iSubItem = 1;
            lvitem.pszText = speaker_str;
            lvitem.cchTextMax = lstrlenW(lvitem.pszText);

            SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_SETITEMW, 0, (LPARAM)&lvitem);
        }

        for(i = 0; i < num_capture_devs; ++i){
            if(!capture_devs[i].id)
                continue;

            SendDlgItemMessageW(hDlg, IDC_AUDIOIN_DEVICE, CB_ADDSTRING,
                    0, (LPARAM)capture_devs[i].name.pwszVal);
            SendDlgItemMessageW(hDlg, IDC_AUDIOIN_DEVICE, CB_SETITEMDATA,
                    i + 1, (LPARAM)&capture_devs[i]);
            if(reg_in_dev && !wcscmp(capture_devs[i].id, reg_in_dev))
                SendDlgItemMessageW(hDlg, IDC_AUDIOIN_DEVICE, CB_SETCURSEL, i + 1, 0);

            SendDlgItemMessageW(hDlg, IDC_VOICEIN_DEVICE, CB_ADDSTRING,
                    0, (LPARAM)capture_devs[i].name.pwszVal);
            SendDlgItemMessageW(hDlg, IDC_VOICEIN_DEVICE, CB_SETITEMDATA,
                    i + 1, (LPARAM)&capture_devs[i]);
            if(reg_vin_dev && !wcscmp(capture_devs[i].id, reg_vin_dev))
                SendDlgItemMessageW(hDlg, IDC_VOICEIN_DEVICE, CB_SETCURSEL, i + 1, 0);
        }

        free(reg_out_dev);
        free(reg_vout_dev);
        free(reg_in_dev);
        free(reg_vin_dev);
    }else
        swprintf(display_str, ARRAY_SIZE(display_str), format_str, disabled_str);

    reg_midi_dev = get_reg_key(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap", L"szPname", NULL);
    for(i = 0; i < num_midi_devs; ++i){
        if(!midi_devs[i])
            continue;

        SendDlgItemMessageW(hDlg, IDC_MIDI_DEVICE, CB_ADDSTRING,
                0, (LPARAM)midi_devs[i]);
        SendDlgItemMessageW(hDlg, IDC_MIDI_DEVICE, CB_SETITEMDATA,
                i + 1, (LPARAM)midi_devs[i]);
        if(reg_midi_dev && !wcscmp(midi_devs[i], reg_midi_dev))
            SendDlgItemMessageW(hDlg, IDC_MIDI_DEVICE, CB_SETCURSEL, i + 1, 0);
    }
    free(reg_midi_dev);

    SetDlgItemTextW(hDlg, IDC_AUDIO_DRIVER, display_str);
}

static void set_reg_device(HWND hDlg, int dlgitem, const WCHAR *key_name)
{
    UINT idx;
    struct DeviceInfo *info;

    idx = SendDlgItemMessageW(hDlg, dlgitem, CB_GETCURSEL, 0, 0);

    info = (struct DeviceInfo *)SendDlgItemMessageW(hDlg, dlgitem,
            CB_GETITEMDATA, idx, 0);

    if(!info || info == (void*)CB_ERR)
        set_reg_key(HKEY_CURRENT_USER, g_drv_keyW, key_name, NULL);
    else
        set_reg_key(HKEY_CURRENT_USER, g_drv_keyW, key_name, info->id);
}

static void set_reg_midi_device(HWND hDlg, int dlgitem)
{
    UINT idx;
    WCHAR *id;

    idx = SendDlgItemMessageW(hDlg, dlgitem, CB_GETCURSEL, 0, 0);

    id = (WCHAR *)SendDlgItemMessageW(hDlg, dlgitem, CB_GETITEMDATA, idx, 0);

    if(!id || id == (void*)CB_ERR)
        set_reg_key(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap", L"szPname", NULL);
    else
        set_reg_key(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap", L"szPname", id);
}

static void test_sound(void)
{
    if(!PlaySoundW(MAKEINTRESOURCEW(IDW_TESTSOUND), NULL, SND_RESOURCE | SND_ASYNC)){
        WCHAR error_str[256], title_str[256];

        LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_TEST_FAILED, error_str,
                ARRAY_SIZE(error_str));
        LoadStringW(GetModuleHandleW(NULL), IDS_AUDIO_TEST_FAILED_TITLE, title_str,
                ARRAY_SIZE(title_str));

        MessageBoxW(NULL, error_str, title_str, MB_OK | MB_ICONERROR);
    }
}

static void apply_speaker_configs(void)
{
    UINT i;
    IMMDeviceEnumerator *devenum;
    IMMDevice *dev;
    IPropertyStore *ps;
    PROPVARIANT pv;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&devenum);

    if(FAILED(hr)){
        ERR("Unable to create MMDeviceEnumerator: 0x%08lx\n", hr);
        return;
    }

    PropVariantInit(&pv);
    pv.vt = VT_UI4;

    for (i = 0; i < num_render_devs; i++) {
        hr = IMMDeviceEnumerator_GetDevice(devenum, render_devs[i].id, &dev);

        if(FAILED(hr)){
            WARN("Could not get MMDevice for %s: 0x%08lx\n", wine_dbgstr_w(render_devs[i].id), hr);
            continue;
        }

        hr = IMMDevice_OpenPropertyStore(dev, STGM_WRITE, &ps);

        if(FAILED(hr)){
            WARN("Could not open property store for %s: 0x%08lx\n", wine_dbgstr_w(render_devs[i].id), hr);
            IMMDevice_Release(dev);
            continue;
        }

        pv.ulVal = speaker_configs[render_devs[i].speaker_config].speaker_mask;

        hr = IPropertyStore_SetValue(ps, &PKEY_AudioEndpoint_PhysicalSpeakers, &pv);

        if (FAILED(hr))
            WARN("IPropertyStore_SetValue failed for %s: 0x%08lx\n", wine_dbgstr_w(render_devs[i].id), hr);

        IPropertyStore_Release(ps);
        IMMDevice_Release(dev);
    }

    IMMDeviceEnumerator_Release(devenum);
}

static void listview_changed(HWND hDlg)
{
    int idx;

    idx = SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
    if(idx < 0) {
        EnableWindow(GetDlgItem(hDlg, IDC_SPEAKERCONFIG_SPEAKERS), 0);
        return;
    }

    SendDlgItemMessageW(hDlg, IDC_SPEAKERCONFIG_SPEAKERS, CB_SETCURSEL,
            render_devs[idx].speaker_config, 0);

    EnableWindow(GetDlgItem(hDlg, IDC_SPEAKERCONFIG_SPEAKERS), 1);
}

INT_PTR CALLBACK
AudioDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
      case WM_COMMAND:
        switch (LOWORD(wParam)) {
          case IDC_AUDIO_TEST:
              test_sound();
              break;
          case IDC_AUDIOOUT_DEVICE:
              if(HIWORD(wParam) == CBN_SELCHANGE){
                  set_reg_device(hDlg, IDC_AUDIOOUT_DEVICE, L"DefaultOutput");
                  SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
              }
              break;
          case IDC_VOICEOUT_DEVICE:
              if(HIWORD(wParam) == CBN_SELCHANGE){
                  set_reg_device(hDlg, IDC_VOICEOUT_DEVICE, L"DefaultVoiceOutput");
                  SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
              }
              break;
          case IDC_AUDIOIN_DEVICE:
              if(HIWORD(wParam) == CBN_SELCHANGE){
                  set_reg_device(hDlg, IDC_AUDIOIN_DEVICE, L"DefaultInput");
                  SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
              }
              break;
          case IDC_VOICEIN_DEVICE:
              if(HIWORD(wParam) == CBN_SELCHANGE){
                  set_reg_device(hDlg, IDC_VOICEIN_DEVICE, L"DefaultVoiceInput");
                  SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
              }
              break;
          case IDC_MIDI_DEVICE:
              if(HIWORD(wParam) == CBN_SELCHANGE){
                  set_reg_midi_device(hDlg, IDC_MIDI_DEVICE);
                  SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
              }
              break;
          case IDC_SPEAKERCONFIG_SPEAKERS:
              if(HIWORD(wParam) == CBN_SELCHANGE){
                  UINT dev, idx;

                  idx = SendDlgItemMessageW(hDlg, IDC_SPEAKERCONFIG_SPEAKERS, CB_GETCURSEL, 0, 0);
                  dev = SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

                  if(dev < num_render_devs){
                      WCHAR speaker_str[256];
                      LVITEMW lvitem;

                      render_devs[dev].speaker_config = idx;

                      LoadStringW(GetModuleHandleW(NULL), speaker_configs[idx].text_id,
                          speaker_str, ARRAY_SIZE(speaker_str));

                      lvitem.mask = LVIF_TEXT;
                      lvitem.iItem = dev;
                      lvitem.iSubItem = 1;
                      lvitem.pszText = speaker_str;
                      lvitem.cchTextMax = lstrlenW(lvitem.pszText);

                      SendDlgItemMessageW(hDlg, IDC_LIST_AUDIO_DEVICES, LVM_SETITEMW, 0, (LPARAM)&lvitem);

                      SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                  }
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
              SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, FALSE);
              break;
            case PSN_APPLY:
              apply_speaker_configs();
              apply();
              SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
              break;
            case PSN_SETACTIVE:
              break;
            case LVN_ITEMCHANGED:
              listview_changed(hDlg);
              break;
        }
        break;
      case WM_INITDIALOG:
        initAudioDlg(hDlg);
        break;
  }

  return FALSE;
}
