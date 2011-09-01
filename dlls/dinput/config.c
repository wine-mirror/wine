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
static BOOL CALLBACK collect_objects(LPCDIDEVICEOBJECTINSTANCEW lpddo, LPVOID pvRef)
{
    DeviceData *data = (DeviceData*) pvRef;

    data->ddo[data->nobjects] = *lpddo;

    data->nobjects++;
    return DIENUM_CONTINUE;
}

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

    device->nobjects = 0;
    IDirectInputDevice_EnumObjects(lpdid, collect_objects, (LPVOID) device, DIDFT_ALL);

    data->ndevices++;
    return DIENUM_CONTINUE;
}

/*
 * Listview utility functions
 */
static void init_listview_columns(HWND dialog)
{
    HINSTANCE hinstance = (HINSTANCE) GetWindowLongPtrW(dialog, GWLP_HINSTANCE);
    LVCOLUMNW listColumn;
    RECT viewRect;
    int width;
    WCHAR column[MAX_PATH];

    GetClientRect(GetDlgItem(dialog, IDC_DEVICEOBJECTSLIST), &viewRect);
    width = (viewRect.right - viewRect.left)/2;

    LoadStringW(hinstance, IDS_OBJECTCOLUMN, column, sizeof(column)/sizeof(column[0]));
    listColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW(listColumn.pszText);
    listColumn.cx = width;

    SendDlgItemMessageW (dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTCOLUMNW, 0, (LPARAM) &listColumn);

    LoadStringW(hinstance, IDS_ACTIONCOLUMN, column, sizeof(column)/sizeof(column[0]));
    listColumn.cx = width;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW(listColumn.pszText);

    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTCOLUMNW, 1, (LPARAM) &listColumn);
}

static void lv_set_action(HWND dialog, int item, int action, LPDIACTIONFORMATW lpdiaf)
{
    static const WCHAR no_action[] = {'-','\0'};
    const WCHAR *action_text = no_action;
    LVITEMW lvItem;

    if (item < 0) return;

    if (action != -1)
        action_text = lpdiaf->rgoAction[action].lptszActionName;

    /* Keep the action and text in the listview item */
    lvItem.iItem = item;

    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.lParam = (LPARAM) action;

    /* Action index */
    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_SETITEMW, 0, (LPARAM) &lvItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iSubItem = 1;
    lvItem.pszText = (WCHAR *)action_text;
    lvItem.cchTextMax = lstrlenW(lvItem.pszText);

    /* Text */
    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_SETITEMW, 0, (LPARAM) &lvItem);
}

/*
 * Utility functions
 */
static DeviceData* get_cur_device(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    int sel = SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_GETCURSEL, 0, 0);
    return &data->devices_data.devices[sel];
}

static LPDIACTIONFORMATW get_cur_lpdiaf(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    return data->lpdiaf;
}

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

static void fill_device_object_list(HWND dialog)
{
    DeviceData *device = get_cur_device(dialog);
    LPDIACTIONFORMATW lpdiaf = get_cur_lpdiaf(dialog);
    LVITEMW item;
    int i, j;

    /* Clean the listview */
    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_DELETEALLITEMS, 0, 0);

    /* Add each object */
    for (i=0; i < device->nobjects; i++)
    {
        int action = -1;

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = device->ddo[i].tszName;
        item.cchTextMax = lstrlenW(item.pszText);

        /* Add the item */
        SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTITEMW, 0, (LPARAM) &item);

        /* Search for an assigned action  for this device */
        for (j=0; j < lpdiaf->dwNumActions; j++)
        {
            if (IsEqualGUID(&lpdiaf->rgoAction[j].guidInstance, &device->ddi.guidInstance) &&
                lpdiaf->rgoAction[j].dwObjID == device->ddo[i].dwType)
            {
                action = j;
                break;
            }
        }

        lv_set_action(dialog, i, action, lpdiaf);
    }
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

            init_listview_columns(dialog);

            break;
        }
        case WM_COMMAND:

            switch(LOWORD(wParam))
            {

                case IDC_CONTROLLERCOMBO:

                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            fill_device_object_list(dialog);
                            break;
                    }
                    break;

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
