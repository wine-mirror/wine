/*
 * Joystick testing control panel applet
 *
 * Copyright 2012 Lucas Fialho Zawacki
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

#define NONAMELESSUNION
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>
#include <cpl.h>
#include "ole2.h"

#include "wine/debug.h"
#include "joy.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

DECLSPEC_HIDDEN HMODULE hcpl;

/*********************************************************************
 *  DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", hdll, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
            hcpl = hdll;
    }
    return TRUE;
}

/***********************************************************************
 *  enum_callback [internal]
 *   Enumerates, creates and sets the common data format for all the joystick devices.
 *   First time it checks if space for the joysticks was already reserved
 *   and if not, just counts how many there are.
 */
static BOOL CALLBACK enum_callback(const DIDEVICEINSTANCEW *instance, void *context)
{
    struct JoystickData *data = context;
    struct Joystick *joystick;

    if (data->joysticks == NULL)
    {
        data->num_joysticks += 1;
        return DIENUM_CONTINUE;
    }

    joystick = &data->joysticks[data->cur_joystick];
    data->cur_joystick += 1;

    IDirectInput8_CreateDevice(data->di, &instance->guidInstance, &joystick->device, NULL);
    IDirectInputDevice8_SetDataFormat(joystick->device, &c_dfDIJoystick);

    joystick->instance = *instance;

    return DIENUM_CONTINUE;
}

/***********************************************************************
 *  initialize_joysticks [internal]
 */
static void initialize_joysticks(struct JoystickData *data)
{
    data->num_joysticks = 0;
    data->cur_joystick = 0;
    IDirectInput8_EnumDevices(data->di, DI8DEVCLASS_GAMECTRL, enum_callback, data, DIEDFL_ATTACHEDONLY);
    data->joysticks = HeapAlloc(GetProcessHeap(), 0, sizeof(struct Joystick) * data->num_joysticks);

    /* Get all the joysticks */
    IDirectInput8_EnumDevices(data->di, DI8DEVCLASS_GAMECTRL, enum_callback, data, DIEDFL_ATTACHEDONLY);
}

/***********************************************************************
 *  destroy_joysticks [internal]
 */
static void destroy_joysticks(struct JoystickData *data)
{
    int i;

    for (i = 0; i < data->num_joysticks; i++)
    {
        IDirectInputDevice8_Unacquire(data->joysticks[i].device);
        IDirectInputDevice8_Release(data->joysticks[i].device);
    }

    HeapFree(GetProcessHeap(), 0, data->joysticks);
}

/*********************************************************************
 * list_dlgproc [internal]
 *
 */
INT_PTR CALLBACK list_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    TRACE("(%p, 0x%08x/%d, 0x%lx)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            int i;
            struct JoystickData *data = (struct JoystickData*) ((PROPSHEETPAGEW*)lparam)->lParam;

            /* Set dialog information */
            for (i = 0; i < data->num_joysticks; i++)
            {
                struct Joystick *joy = &data->joysticks[i];
                SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_ADDSTRING, 0, (LPARAM) joy->instance.tszInstanceName);
            }

            return TRUE;
        }

        case WM_COMMAND:

            switch (LOWORD(wparam))
            {
                case IDC_BUTTONDISABLE:
                    FIXME("Disable selected joystick from being enumerated\n");
                    break;

                case IDC_BUTTONENABLE:
                    FIXME("Re-Enable selected joystick\n");
                    break;
            }

            return TRUE;

        case WM_NOTIFY:
            return TRUE;

        default:
            break;
    }
    return FALSE;
}

/******************************************************************************
 * propsheet_callback [internal]
 */
static int CALLBACK propsheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    TRACE("(%p, 0x%08x/%d, 0x%lx)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case PSCB_INITIALIZED:
            break;
    }
    return 0;
}

/******************************************************************************
 * display_cpl_sheets [internal]
 *
 * Build and display the dialog with all control panel propertysheets
 *
 */
static void display_cpl_sheets(HWND parent, struct JoystickData *data)
{
    INITCOMMONCONTROLSEX icex;
    PROPSHEETPAGEW psp[NUM_PROPERTY_PAGES];
    PROPSHEETHEADERW psh;
    DWORD id = 0;

    OleInitialize(NULL);
    /* Initialize common controls */
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    ZeroMemory(&psh, sizeof(psh));
    ZeroMemory(psp, sizeof(psp));

    /* Fill out all PROPSHEETPAGE */
    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].u.pszTemplate = MAKEINTRESOURCEW(IDD_LIST);
    psp[id].pfnDlgProc = list_dlgproc;
    psp[id].lParam = (INT_PTR) data;
    id++;

    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].u.pszTemplate = MAKEINTRESOURCEW(IDD_TEST);
    psp[id].pfnDlgProc = NULL;
    psp[id].lParam = (INT_PTR) data;
    id++;

    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].u.pszTemplate = MAKEINTRESOURCEW(IDD_FORCEFEEDBACK);
    psp[id].pfnDlgProc = NULL;
    psp[id].lParam = (INT_PTR) data;
    id++;

    /* Fill out the PROPSHEETHEADER */
    psh.dwSize = sizeof (PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = parent;
    psh.hInstance = hcpl;
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPL_NAME);
    psh.nPages = id;
    psh.u3.ppsp = psp;
    psh.pfnCallback = propsheet_callback;

    /* display the dialog */
    PropertySheetW(&psh);

    OleUninitialize();
}

/*********************************************************************
 * CPlApplet (joy.cpl.@)
 *
 * Control Panel entry point
 *
 * PARAMS
 *  hWnd    [I] Handle for the Control Panel Window
 *  command [I] CPL_* Command
 *  lParam1 [I] first extra Parameter
 *  lParam2 [I] second extra Parameter
 *
 * RETURNS
 *  Depends on the command
 *
 */
LONG CALLBACK CPlApplet(HWND hwnd, UINT command, LPARAM lParam1, LPARAM lParam2)
{
    static struct JoystickData data;
    TRACE("(%p, %u, 0x%lx, 0x%lx)\n", hwnd, command, lParam1, lParam2);

    switch (command)
    {
        case CPL_INIT:
        {
            HRESULT hr;

            /* Initialize dinput */
            hr = DirectInput8Create(GetModuleHandleW(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void**)&data.di, NULL);

            if (FAILED(hr))
            {
                ERR("Failed to initialize DirectInput: 0x%08x\n", hr);
                return FALSE;
            }

            /* Then get all the connected joysticks */
            initialize_joysticks(&data);

            return TRUE;
        }
        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idName = IDS_CPL_NAME;
            appletInfo->idInfo = IDS_CPL_INFO;
            appletInfo->lData = 0;
            return TRUE;
        }

        case CPL_DBLCLK:
            display_cpl_sheets(hwnd, &data);
            break;

        case CPL_STOP:
            destroy_joysticks(&data);

            /* And destroy dinput too */
            IDirectInput8_Release(data.di);
            break;
    }

    return FALSE;
}
