/*
 * Copyright (c) 2011 Lucas Fialho Zawacki
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
 */

#include "wine/debug.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "dinput_private.h"
#include "device_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

typedef struct {
    int nobjects;
    IDirectInputDevice8W *lpdid;
    DIDEVICEINSTANCEW ddi;
    DIDEVICEOBJECTINSTANCEW ddo[256];
} DeviceData;

typedef struct {
    int ndevices;
    DeviceData *devices;
} DIDevicesData;

typedef struct {
    IDirectInput8W *lpDI;
    LPDIACTIONFORMATW lpdiaf;
    DIDevicesData devices_data;
} ConfigureDevicesData;

/*
 * Enumeration callback functions
 */
static BOOL CALLBACK count_devices(LPCDIDEVICEINSTANCEW lpddi, IDirectInputDevice8W *lpdid, DWORD dwFlags, DWORD dwRemaining, LPVOID pvRef)
{
    DIDevicesData *data = (DIDevicesData*) pvRef;

    data->ndevices++;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK collect_devices(LPCDIDEVICEINSTANCEW lpddi, IDirectInputDevice8W *lpdid, DWORD dwFlags, DWORD dwRemaining, LPVOID pvRef)
{
    DIDevicesData *data = (DIDevicesData*) pvRef;
    DeviceData *device = &data->devices[data->ndevices];
    device->lpdid = lpdid;
    device->ddi = *lpddi;

    IDirectInputDevice_AddRef(lpdid);

    data->ndevices++;
    return DIENUM_CONTINUE;
}

/*
 * Utility functions
 */
static void init_devices(HWND dialog, IDirectInput8W *lpDI, DIDevicesData *data, LPDIACTIONFORMATW lpdiaf)
{
    int i;

    /* Count devices */
    IDirectInput8_EnumDevicesBySemantics(lpDI, NULL, lpdiaf, count_devices, (LPVOID) data, 0);

    /* Allocate devices */
    data->devices = (DeviceData*) HeapAlloc(GetProcessHeap(), 0, sizeof(DeviceData) * data->ndevices);

    /* Collect and insert */
    data->ndevices = 0;
    IDirectInput8_EnumDevicesBySemantics(lpDI, NULL, lpdiaf, collect_devices, (LPVOID) data, 0);

    for (i=0; i < data->ndevices; i++)
        SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_ADDSTRING, 0, (LPARAM) data->devices[i].ddi.tszProductName );
}

static void destroy_devices(HWND dialog)
{
    int i;
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    DIDevicesData *devices_data = &data->devices_data;

    for (i=0; i < devices_data->ndevices; i++)
        IDirectInputDevice8_Release(devices_data->devices[i].lpdid);

    HeapFree(GetProcessHeap(), 0, devices_data->devices);
}

static INT_PTR CALLBACK ConfigureDevicesDlgProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            ConfigureDevicesData *data = (ConfigureDevicesData*) lParam;

            /* Initialize action format and enumerate devices */
            init_devices(dialog, data->lpDI, &data->devices_data, data->lpdiaf);

            /* Store information in the window */
            SetWindowLongPtrW(dialog, DWLP_USER, (LONG_PTR) data);

            break;
        }
        case WM_COMMAND:

            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(dialog, 0);
                    destroy_devices(dialog);
                    break;

                case IDCANCEL:
                    EndDialog(dialog, 0);
                    destroy_devices(dialog);
                    break;

                case IDC_RESET:
                    break;
            }
        break;
    }

    return FALSE;
}

HRESULT _configure_devices(IDirectInput8W *iface,
                           LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
                           LPDICONFIGUREDEVICESPARAMSW lpdiCDParams,
                           DWORD dwFlags,
                           LPVOID pvRefData
)
{
    ConfigureDevicesData data;
    data.lpDI = iface;
    data.lpdiaf = lpdiCDParams->lprgFormats;

    InitCommonControls();

    DialogBoxParamW(GetModuleHandleA("dinput.dll"), (LPCWSTR) MAKEINTRESOURCE(IDD_CONFIGUREDEVICES), lpdiCDParams->hwnd, ConfigureDevicesDlgProc, (LPARAM) &data);

    return DI_OK;
}
