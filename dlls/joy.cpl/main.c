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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>
#include <dinput.h>
#include <cpl.h>
#include "ole2.h"
#include "dbt.h"

#include "initguid.h"
#include "ddk/hidclass.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

struct device
{
    struct list entry;
    IDirectInputDevice8W *device;
};

static HMODULE hcpl;

static CRITICAL_SECTION joy_cs;
static CRITICAL_SECTION_DEBUG joy_cs_debug =
{
    0, 0, &joy_cs,
    { &joy_cs_debug.ProcessLocksList, &joy_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": joy_cs") }
};
static CRITICAL_SECTION joy_cs = { &joy_cs_debug, -1, 0, 0, 0, 0 };

static struct list devices = LIST_INIT( devices );
static HDEVNOTIFY devnotify;

/*********************************************************************
 *  DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %ld, %p)\n", hdll, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
            hcpl = hdll;
    }
    return TRUE;
}

static BOOL CALLBACK enum_devices( const DIDEVICEINSTANCEW *instance, void *context )
{
    DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
    IDirectInput8W *dinput = context;
    struct device *entry;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return DIENUM_STOP;

    IDirectInput8_CreateDevice( dinput, &instance->guidInstance, &entry->device, NULL );
    IDirectInputDevice8_SetDataFormat( entry->device, &c_dfDIJoystick );
    IDirectInputDevice8_GetCapabilities( entry->device, &caps );

    list_add_tail( &devices, &entry->entry );

    return DIENUM_CONTINUE;
}

static void clear_devices(void)
{
    struct device *entry, *next;

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &devices, struct device, entry )
    {
        list_remove( &entry->entry );
        IDirectInputDevice8_Unacquire( entry->device );
        IDirectInputDevice8_Release( entry->device );
        free( entry );
    }
}

/******************************************************************************
 * get_app_key [internal]
 * Get the default DirectInput key and the selected app config key.
 */
static BOOL get_app_key(HKEY *defkey, HKEY *appkey)
{
    *appkey = 0;

    /* Registry key can be found in HKCU\Software\Wine\DirectInput */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Wine\\DirectInput\\Joysticks", 0, NULL, 0,
                KEY_SET_VALUE | KEY_READ, NULL, defkey, NULL))
        *defkey = 0;

    return *defkey || *appkey;
}

static BOOL get_advanced_key(HKEY *key)
{
    /* Registry key can be found in HKLM\System\CurrentControlSet\Services\WineBus */
    return !RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\WineBus", 0, NULL, 0,
                KEY_SET_VALUE | KEY_READ, NULL, key, NULL);
}

/******************************************************************************
 * set_config_key [internal]
 * Writes a string value to a registry key, deletes the key if value == NULL
 */
static DWORD set_config_key(HKEY defkey, HKEY appkey, const WCHAR *name, const WCHAR *value)
{
    if (value == NULL)
    {
        if (appkey && !RegDeleteValueW(appkey, name))
            return 0;

        if (defkey && !RegDeleteValueW(defkey, name))
            return 0;
    }
    else
    {
        DWORD size = wcslen(value);

        if (appkey && !RegSetValueExW(appkey, name, 0, REG_SZ, (const BYTE*) value, (size + 1)*sizeof(WCHAR)))
            return 0;

        if (defkey && !RegSetValueExW(defkey, name, 0, REG_SZ, (const BYTE*) value, (size + 1)*sizeof(WCHAR)))
            return 0;
    }

    return ERROR_FILE_NOT_FOUND;
}

/******************************************************************************
 * enable_joystick [internal]
 * Writes to the DirectInput registry key that enables/disables a joystick
 * from being enumerated.
 */
static void enable_joystick(WCHAR *joy_name, BOOL enable)
{
    HKEY hkey, appkey;

    get_app_key(&hkey, &appkey);

    if (!enable)
        set_config_key(hkey, appkey, joy_name, L"disabled");
    else
        set_config_key(hkey, appkey, joy_name, NULL);

    if (hkey) RegCloseKey(hkey);
    if (appkey) RegCloseKey(appkey);
}

static void set_advanced_option( const WCHAR *option, DWORD value )
{
    HKEY hkey;
    if (!get_advanced_key( &hkey )) return;
    RegSetValueExW( hkey, option, 0, REG_DWORD, (BYTE *)&value, sizeof(value) );
    RegCloseKey( hkey );
}

static DWORD get_advanced_option( const WCHAR *option, DWORD default_value )
{
    DWORD value, size = sizeof(value);
    HKEY hkey;
    if (!get_advanced_key( &hkey )) return default_value;
    if (RegGetValueW( hkey, NULL, option, RRF_RT_REG_DWORD, NULL, (BYTE *)&value, &size ))
        value = default_value;
    RegCloseKey( hkey );
    return value;
}

static void refresh_joystick_list( HWND hwnd )
{
    IDirectInput8W *dinput;
    struct device *entry;
    HKEY hkey, appkey;
    DWORD values = 0;
    LSTATUS status;
    DWORD i;

    clear_devices();

    DirectInput8Create( GetModuleHandleW( NULL ), DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void **)&dinput, NULL );
    IDirectInput8_EnumDevices( dinput, DI8DEVCLASS_GAMECTRL, enum_devices, dinput, DIEDFL_ATTACHEDONLY );
    IDirectInput8_Release( dinput );

    SendDlgItemMessageW(hwnd, IDC_DI_ENABLED_LIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessageW(hwnd, IDC_XI_ENABLED_LIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessageW(hwnd, IDC_DISABLED_LIST, LB_RESETCONTENT, 0, 0);

    LIST_FOR_EACH_ENTRY( entry, &devices, struct device, entry )
    {
        DIDEVICEINSTANCEW info = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
        DIPROPGUIDANDPATH prop =
        {
            .diph =
            {
                .dwSize = sizeof(DIPROPGUIDANDPATH),
                .dwHeaderSize = sizeof(DIPROPHEADER),
                .dwHow = DIPH_DEVICE,
            },
        };

        if (FAILED(IDirectInputDevice8_GetDeviceInfo( entry->device, &info ))) continue;
        if (FAILED(IDirectInputDevice8_GetProperty( entry->device, DIPROP_GUIDANDPATH, &prop.diph ))) continue;

        if (wcsstr( prop.wszPath, L"&ig_" )) SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_ADDSTRING, 0, (LPARAM)info.tszInstanceName );
        else SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_ADDSTRING, 0, (LPARAM)info.tszInstanceName );
    }

    /* Search for disabled joysticks */
    get_app_key(&hkey, &appkey);
    RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &values, NULL, NULL, NULL, NULL);

    for (i=0; i < values; i++)
    {
        DWORD name_len = MAX_PATH, data_len = MAX_PATH;
        WCHAR buf_name[MAX_PATH + 9], buf_data[MAX_PATH];

        status = RegEnumValueW(hkey, i, buf_name, &name_len, NULL, NULL, (BYTE*) buf_data, &data_len);

        if (status == ERROR_SUCCESS && !wcscmp(L"disabled", buf_data))
            SendDlgItemMessageW(hwnd, IDC_DISABLED_LIST, LB_ADDSTRING, 0, (LPARAM) buf_name);
    }

    if (hkey) RegCloseKey(hkey);
    if (appkey) RegCloseKey(appkey);
}

static void override_joystick(WCHAR *joy_name, BOOL override)
{
    HKEY hkey, appkey;

    get_app_key(&hkey, &appkey);

    if (override)
        set_config_key(hkey, appkey, joy_name, L"override");
    else
        set_config_key(hkey, appkey, joy_name, NULL);

    if (hkey) RegCloseKey(hkey);
    if (appkey) RegCloseKey(appkey);
}

/*********************************************************************
 * list_dlgproc [internal]
 *
 */
static INT_PTR CALLBACK list_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    WCHAR instance_name[MAX_PATH] = {0};
    DEV_BROADCAST_DEVICEINTERFACE_W filter =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_W),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
        .dbcc_classguid = GUID_DEVINTERFACE_HID,
    };
    int sel;

    TRACE("(%p, 0x%08x/%d, 0x%Ix)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        BOOL enable_sdl, disable_hidraw;

        refresh_joystick_list( hwnd );

        EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_ENABLE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_DISABLE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_RESET ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_XI_OVERRIDE ), FALSE );

        enable_sdl = get_advanced_option( L"Enable SDL", TRUE );
        SendMessageW( GetDlgItem( hwnd, IDC_ENABLE_SDL ), BM_SETCHECK, enable_sdl, 0 );
        disable_hidraw = get_advanced_option( L"DisableHidraw", FALSE );
        SendMessageW( GetDlgItem( hwnd, IDC_DISABLE_HIDRAW ), BM_SETCHECK, disable_hidraw, 0 );

        devnotify = RegisterDeviceNotificationW( hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE );
        return TRUE;
    }

    case WM_DEVICECHANGE:
        refresh_joystick_list( hwnd );
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_BUTTON_DI_DISABLE:
            if ((sel = SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_GETCURSEL, 0, 0 )) >= 0)
                SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_GETTEXT, sel, (LPARAM)instance_name );
            if ((sel = SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_GETCURSEL, 0, 0 )) >= 0)
                SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_GETTEXT, sel, (LPARAM)instance_name );

            if (instance_name[0])
            {
                enable_joystick( instance_name, FALSE );
                refresh_joystick_list( hwnd );
            }
            break;

        case IDC_BUTTON_ENABLE:
            if ((sel = SendDlgItemMessageW( hwnd, IDC_DISABLED_LIST, LB_GETCURSEL, 0, 0 )) >= 0)
                SendDlgItemMessageW( hwnd, IDC_DISABLED_LIST, LB_GETTEXT, sel, (LPARAM)instance_name );

            if (instance_name[0])
            {
                enable_joystick( instance_name, TRUE );
                refresh_joystick_list( hwnd );
            }
            break;

        case IDC_BUTTON_DI_RESET:
            if ((sel = SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_GETCURSEL, 0, 0 )) >= 0)
            {
                SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_GETTEXT, sel, (LPARAM)instance_name );
                override_joystick( instance_name, FALSE );
                refresh_joystick_list( hwnd );
            }
            break;

        case IDC_BUTTON_XI_OVERRIDE:
            if ((sel = SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_GETCURSEL, 0, 0 )) >= 0)
            {
                SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_GETTEXT, sel, (LPARAM)instance_name );
                override_joystick( instance_name, TRUE );
                refresh_joystick_list( hwnd );
            }
            break;

        case IDC_DI_ENABLED_LIST:
            SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_SETCURSEL, -1, 0 );
            SendDlgItemMessageW( hwnd, IDC_DISABLED_LIST, LB_SETCURSEL, -1, 0 );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_ENABLE ), FALSE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_DISABLE ), TRUE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_RESET ), TRUE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_XI_OVERRIDE ), FALSE );
            break;

        case IDC_XI_ENABLED_LIST:
            SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_SETCURSEL, -1, 0 );
            SendDlgItemMessageW( hwnd, IDC_DISABLED_LIST, LB_SETCURSEL, -1, 0 );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_ENABLE ), FALSE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_DISABLE ), TRUE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_RESET ), FALSE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_XI_OVERRIDE ), TRUE );
            break;

        case IDC_DISABLED_LIST:
            SendDlgItemMessageW( hwnd, IDC_DI_ENABLED_LIST, LB_SETCURSEL, -1, 0 );
            SendDlgItemMessageW( hwnd, IDC_XI_ENABLED_LIST, LB_SETCURSEL, -1, 0 );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_ENABLE ), TRUE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_DISABLE ), FALSE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_DI_RESET ), FALSE );
            EnableWindow( GetDlgItem( hwnd, IDC_BUTTON_XI_OVERRIDE ), FALSE );
            break;

        case IDC_ENABLE_SDL:
            sel = SendMessageW( GetDlgItem( hwnd, IDC_ENABLE_SDL ), BM_GETCHECK, 0, 0 );
            set_advanced_option( L"Enable SDL", sel );
            break;

        case IDC_DISABLE_HIDRAW:
            sel = SendMessageW( GetDlgItem( hwnd, IDC_DISABLE_HIDRAW ), BM_GETCHECK, 0, 0 );
            set_advanced_option( L"DisableHidraw", sel );
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
    TRACE("(%p, 0x%08x/%d, 0x%Ix)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case PSCB_INITIALIZED:
            break;
    }
    return 0;
}

static void display_cpl_sheets( HWND parent )
{
    INITCOMMONCONTROLSEX init =
    {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES,
    };
    PROPSHEETPAGEW pages[] =
    {
        {
            .dwSize = sizeof(PROPSHEETPAGEW),
            .hInstance = hcpl,
            .pszTemplate = MAKEINTRESOURCEW( IDD_LIST ),
            .pfnDlgProc = list_dlgproc,
        },
        {
            .dwSize = sizeof(PROPSHEETPAGEW),
            .hInstance = hcpl,
            .pszTemplate = MAKEINTRESOURCEW( IDD_TEST_DI ),
            .pfnDlgProc = test_di_dialog_proc,
        },
        {
            .dwSize = sizeof(PROPSHEETPAGEW),
            .hInstance = hcpl,
            .pszTemplate = MAKEINTRESOURCEW( IDD_TEST_XI ),
            .pfnDlgProc = test_xi_dialog_proc,
        },
    };
    PROPSHEETHEADERW header =
    {
        .dwSize = sizeof(PROPSHEETHEADERW),
        .dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK,
        .hwndParent = parent,
        .hInstance = hcpl,
        .pszCaption = MAKEINTRESOURCEW( IDS_CPL_NAME ),
        .nPages = ARRAY_SIZE(pages),
        .ppsp = pages,
        .pfnCallback = propsheet_callback,
    };
    ACTCTXW context_desc =
    {
        .cbSize = sizeof(ACTCTXW),
        .hModule = hcpl,
        .lpResourceName = MAKEINTRESOURCEW( 124 ),
        .dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID,
    };
    ULONG_PTR cookie;
    HANDLE context;
    BOOL activated;

    OleInitialize( NULL );

    context = CreateActCtxW( &context_desc );
    if (context == INVALID_HANDLE_VALUE) activated = FALSE;
    else activated = ActivateActCtx( context, &cookie );

    InitCommonControlsEx( &init );
    PropertySheetW( &header );

    if (activated) DeactivateActCtx( 0, cookie );
    ReleaseActCtx( context );
    OleUninitialize();
}

static void register_window_class(void)
{
    WNDCLASSW xi_class =
    {
        .hInstance = hcpl,
        .lpfnWndProc = &test_xi_window_proc,
        .lpszClassName = L"JoyCplXInput",
    };
    WNDCLASSW di_axes_class =
    {
        .hInstance = hcpl,
        .lpfnWndProc = &test_di_axes_window_proc,
        .lpszClassName = L"JoyCplDInputAxes",
    };
    WNDCLASSW di_povs_class =
    {
        .hInstance = hcpl,
        .lpfnWndProc = &test_di_povs_window_proc,
        .lpszClassName = L"JoyCplDInputPOVs",
    };
    WNDCLASSW di_buttons_class =
    {
        .hInstance = hcpl,
        .lpfnWndProc = &test_di_buttons_window_proc,
        .lpszClassName = L"JoyCplDInputButtons",
    };

    RegisterClassW( &xi_class );
    RegisterClassW( &di_axes_class );
    RegisterClassW( &di_povs_class );
    RegisterClassW( &di_buttons_class );
}

static void unregister_window_class(void)
{
    UnregisterClassW( L"JoyCplDInputAxes", hcpl );
    UnregisterClassW( L"JoyCplDInputPOVs", hcpl );
    UnregisterClassW( L"JoyCplDInputButtons", hcpl );
    UnregisterClassW( L"JoyCplXInput", hcpl );
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
    TRACE("(%p, %u, 0x%Ix, 0x%Ix)\n", hwnd, command, lParam1, lParam2);

    switch (command)
    {
        case CPL_INIT:
            register_window_class();
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idIcon = ICO_MAIN;
            appletInfo->idName = IDS_CPL_NAME;
            appletInfo->idInfo = IDS_CPL_INFO;
            appletInfo->lData = 0;
            return TRUE;
        }

        case CPL_DBLCLK:
            display_cpl_sheets( hwnd );
            break;

        case CPL_STOP:
            clear_devices();
            unregister_window_class();
            break;
    }

    return FALSE;
}
