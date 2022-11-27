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

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

#define TEST_MAX_BUTTONS    32
#define TEST_POLL_TIME      100

#define FF_PLAY_TIME        2*DI_SECONDS
#define FF_PERIOD_TIME      FF_PLAY_TIME/4

struct effect
{
    struct list entry;
    IDirectInputEffect *effect;
};

struct device
{
    struct list entry;
    IDirectInputDevice8W *device;
};

struct Graphics
{
    HWND hwnd;
};

struct JoystickData
{
    IDirectInput8W *di;
    struct Graphics graphics;
    HWND di_dialog_hwnd;
    BOOL stop;
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

static struct list effects = LIST_INIT( effects );
static IDirectInputEffect *effect_selected;

static struct list devices = LIST_INIT( devices );
static IDirectInputDevice8W *device_selected;

static HANDLE device_state_event;

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

static BOOL CALLBACK enum_effects( const DIEFFECTINFOW *info, void *context )
{
    IDirectInputDevice8W *device = context;
    DWORD axes[2] = {DIJOFS_X, DIJOFS_Y};
    LONG direction[2] = {0};
    DIEFFECT params =
    {
        .dwSize = sizeof(DIEFFECT),
        .dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS,
        .dwDuration = FF_PLAY_TIME,
        .dwGain = DI_FFNOMINALMAX,
        .rglDirection = direction,
        .rgdwAxes = axes,
        .cAxes = 2,
    };
    DICONSTANTFORCE constant =
    {
        .lMagnitude = DI_FFNOMINALMAX,
    };
    DIPERIODIC periodic =
    {
        .dwMagnitude = DI_FFNOMINALMAX,
        .dwPeriod = FF_PERIOD_TIME,
    };
    DICONDITION condition =
    {
        .dwPositiveSaturation = 10000,
        .dwNegativeSaturation = 10000,
        .lPositiveCoefficient = 10000,
        .lNegativeCoefficient = 10000,
    };
    DIRAMPFORCE ramp =
    {
        .lEnd = DI_FFNOMINALMAX,
    };
    IDirectInputEffect *effect;
    struct effect *entry;
    HRESULT hr;

    hr = IDirectInputDevice8_Acquire( device );
    if (FAILED(hr)) return DIENUM_CONTINUE;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return DIENUM_STOP;

    if (IsEqualGUID( &info->guid, &GUID_RampForce ))
    {
        params.cbTypeSpecificParams = sizeof(ramp);
        params.lpvTypeSpecificParams = &ramp;
        params.dwFlags |= DIEP_TYPESPECIFICPARAMS;
    }
    else if (IsEqualGUID( &info->guid, &GUID_ConstantForce ))
    {
        params.cbTypeSpecificParams = sizeof(constant);
        params.lpvTypeSpecificParams = &constant;
        params.dwFlags |= DIEP_TYPESPECIFICPARAMS;
    }
    else if (IsEqualGUID( &info->guid, &GUID_Sine ) ||
             IsEqualGUID( &info->guid, &GUID_Square ) ||
             IsEqualGUID( &info->guid, &GUID_Triangle ) ||
             IsEqualGUID( &info->guid, &GUID_SawtoothUp ) ||
             IsEqualGUID( &info->guid, &GUID_SawtoothDown ))
    {
        params.cbTypeSpecificParams = sizeof(periodic);
        params.lpvTypeSpecificParams = &periodic;
        params.dwFlags |= DIEP_TYPESPECIFICPARAMS;
    }
    else if (IsEqualGUID( &info->guid, &GUID_Spring ) ||
             IsEqualGUID( &info->guid, &GUID_Damper ) ||
             IsEqualGUID( &info->guid, &GUID_Inertia ) ||
             IsEqualGUID( &info->guid, &GUID_Friction ))
    {
        params.cbTypeSpecificParams = sizeof(condition);
        params.lpvTypeSpecificParams = &condition;
        params.dwFlags |= DIEP_TYPESPECIFICPARAMS;
    }

    do hr = IDirectInputDevice2_CreateEffect( device, &info->guid, &params, &effect, NULL );
    while (FAILED(hr) && --params.cAxes);

    if (FAILED(hr))
    {
        FIXME( "Failed to create effect with type %s, hr %#lx\n", debugstr_guid( &info->guid ), hr );
        free( entry );
        return DIENUM_CONTINUE;
    }

    entry->effect = effect;
    list_add_tail( &effects, &entry->entry );

    return DIENUM_CONTINUE;
}

static void set_selected_effect( IDirectInputEffect *effect )
{
    IDirectInputEffect *previous;

    EnterCriticalSection( &joy_cs );
    if ((previous = effect_selected)) IDirectInputEffect_Release( previous );
    if ((effect_selected = effect)) IDirectInput_AddRef( effect );
    LeaveCriticalSection( &joy_cs );
}

static IDirectInputEffect *get_selected_effect(void)
{
    IDirectInputEffect *effect;

    EnterCriticalSection( &joy_cs );
    if ((effect = effect_selected)) IDirectInputEffect_AddRef( effect );
    LeaveCriticalSection( &joy_cs );

    return effect;
}

static void clear_effects(void)
{
    struct effect *effect, *next;

    set_selected_effect( NULL );

    LIST_FOR_EACH_ENTRY_SAFE( effect, next, &effects, struct effect, entry )
    {
        list_remove( &effect->entry );
        IDirectInputEffect_Release( effect->effect );
        free( effect );
    }
}

static BOOL CALLBACK enum_devices( const DIDEVICEINSTANCEW *instance, void *context )
{
    DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
    struct JoystickData *data = context;
    struct device *entry;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return DIENUM_STOP;

    IDirectInput8_CreateDevice( data->di, &instance->guidInstance, &entry->device, NULL );
    IDirectInputDevice8_SetDataFormat( entry->device, &c_dfDIJoystick );
    IDirectInputDevice8_GetCapabilities( entry->device, &caps );

    list_add_tail( &devices, &entry->entry );

    return DIENUM_CONTINUE;
}

static void set_selected_device( IDirectInputDevice8W *device )
{
    IDirectInputDevice8W *previous;

    EnterCriticalSection( &joy_cs );

    set_selected_effect( NULL );

    if ((previous = device_selected))
    {
        IDirectInputDevice8_SetEventNotification( previous, NULL );
        IDirectInputDevice8_Release( previous );
    }
    if ((device_selected = device))
    {
        IDirectInputDevice8_AddRef( device );
        IDirectInputDevice8_SetEventNotification( device, device_state_event );
        IDirectInputDevice8_Acquire( device );
    }

    LeaveCriticalSection( &joy_cs );
}

static IDirectInputDevice8W *get_selected_device(void)
{
    IDirectInputDevice8W *device;

    EnterCriticalSection( &joy_cs );
    device = device_selected;
    if (device) IDirectInputDevice8_AddRef( device );
    LeaveCriticalSection( &joy_cs );

    return device;
}

static void clear_devices(void)
{
    struct device *entry, *next;

    set_selected_device( NULL );

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

/******************************************************************************
 * set_config_key [internal]
 * Writes a string value to a registry key, deletes the key if value == NULL
 */
static DWORD set_config_key(HKEY defkey, HKEY appkey, const WCHAR *name, const WCHAR *value, DWORD size)
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
        set_config_key(hkey, appkey, joy_name, L"disabled", wcslen(L"disabled"));
    else
        set_config_key(hkey, appkey, joy_name, NULL, 0);

    if (hkey) RegCloseKey(hkey);
    if (appkey) RegCloseKey(appkey);
}

static void refresh_joystick_list(HWND hwnd, struct JoystickData *data)
{
    struct device *entry;
    HKEY hkey, appkey;
    DWORD values = 0;
    LSTATUS status;
    DWORD i;

    clear_effects();
    clear_devices();

    IDirectInput8_EnumDevices( data->di, DI8DEVCLASS_GAMECTRL, enum_devices, data, DIEDFL_ATTACHEDONLY );

    SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessageW(hwnd, IDC_DISABLEDLIST, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_RESETCONTENT, 0, 0);

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

        if (wcsstr( prop.wszPath, L"&ig_" )) SendDlgItemMessageW( hwnd, IDC_XINPUTLIST, LB_ADDSTRING, 0, (LPARAM)info.tszInstanceName );
        else SendDlgItemMessageW( hwnd, IDC_JOYSTICKLIST, LB_ADDSTRING, 0, (LPARAM)info.tszInstanceName );
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
            SendDlgItemMessageW(hwnd, IDC_DISABLEDLIST, LB_ADDSTRING, 0, (LPARAM) buf_name);
    }

    if (hkey) RegCloseKey(hkey);
    if (appkey) RegCloseKey(appkey);
}

static void override_joystick(WCHAR *joy_name, BOOL override)
{
    HKEY hkey, appkey;

    get_app_key(&hkey, &appkey);

    if (override)
        set_config_key(hkey, appkey, joy_name, L"override", wcslen(L"override"));
    else
        set_config_key(hkey, appkey, joy_name, NULL, 0);

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
    static struct JoystickData *data;
    int sel;

    TRACE("(%p, 0x%08x/%d, 0x%Ix)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            data = (struct JoystickData*) ((PROPSHEETPAGEW*)lparam)->lParam;

            refresh_joystick_list(hwnd, data);

            EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_BUTTONRESET), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_BUTTONOVERRIDE), FALSE);

            /* Store the hwnd to be used with MapDialogRect for unit conversions */
            data->graphics.hwnd = hwnd;

            return TRUE;
        }

        case WM_COMMAND:

            switch (LOWORD(wparam))
            {
                case IDC_BUTTONDISABLE:
                {
                    if ((sel = SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_GETCURSEL, 0, 0)) >= 0)
                        SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_GETTEXT, sel, (LPARAM)instance_name);
                    if ((sel = SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_GETCURSEL, 0, 0)) >= 0)
                        SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_GETTEXT, sel, (LPARAM)instance_name);

                    if (instance_name[0])
                    {
                        enable_joystick(instance_name, FALSE);
                        refresh_joystick_list(hwnd, data);
                    }
                }
                break;

                case IDC_BUTTONENABLE:
                {
                    if ((sel = SendDlgItemMessageW(hwnd, IDC_DISABLEDLIST, LB_GETCURSEL, 0, 0)) >= 0)
                        SendDlgItemMessageW(hwnd, IDC_DISABLEDLIST, LB_GETTEXT, sel, (LPARAM)instance_name);

                    if (instance_name[0])
                    {
                        enable_joystick(instance_name, TRUE);
                        refresh_joystick_list(hwnd, data);
                    }
                }
                break;

                case IDC_BUTTONRESET:
                {
                    if ((sel = SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_GETCURSEL, 0, 0)) >= 0)
                    {
                        SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_GETTEXT, sel, (LPARAM)instance_name);
                        override_joystick(instance_name, FALSE);
                        refresh_joystick_list(hwnd, data);
                    }
                }
                break;

                case IDC_BUTTONOVERRIDE:
                {
                    if ((sel = SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_GETCURSEL, 0, 0)) >= 0)
                    {
                        SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_GETTEXT, sel, (LPARAM)instance_name);
                        override_joystick(instance_name, TRUE);
                        refresh_joystick_list(hwnd, data);
                    }
                }
                break;

                case IDC_JOYSTICKLIST:
                    SendDlgItemMessageW(hwnd, IDC_DISABLEDLIST, LB_SETCURSEL, -1, 0);
                    SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_SETCURSEL, -1, 0);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONOVERRIDE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONRESET), TRUE);
                break;

                case IDC_XINPUTLIST:
                    SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_SETCURSEL, -1, 0);
                    SendDlgItemMessageW(hwnd, IDC_DISABLEDLIST, LB_SETCURSEL, -1, 0);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONOVERRIDE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONRESET), FALSE);
                break;

                case IDC_DISABLEDLIST:
                    SendDlgItemMessageW(hwnd, IDC_JOYSTICKLIST, LB_SETCURSEL, -1, 0);
                    SendDlgItemMessageW(hwnd, IDC_XINPUTLIST, LB_SETCURSEL, -1, 0);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONOVERRIDE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONRESET), FALSE);
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

/*********************************************************************
 * Joystick testing functions
 *
 */
static void dump_joy_state(DIJOYSTATE* st)
{
    int i;
    TRACE("Ax (% 5ld,% 5ld,% 5ld)\n", st->lX,st->lY, st->lZ);
    TRACE("RAx (% 5ld,% 5ld,% 5ld)\n", st->lRx, st->lRy, st->lRz);
    TRACE("Slider (% 5ld,% 5ld)\n", st->rglSlider[0], st->rglSlider[1]);
    TRACE("Pov (% 5ld,% 5ld,% 5ld,% 5ld)\n", st->rgdwPOV[0], st->rgdwPOV[1], st->rgdwPOV[2], st->rgdwPOV[3]);

    TRACE("Buttons ");
    for(i=0; i < TEST_MAX_BUTTONS; i++)
        TRACE("  %c",st->rgbButtons[i] ? 'x' : 'o');
    TRACE("\n");
}

static DWORD WINAPI input_thread(void *param)
{
    struct JoystickData *data = param;

    while (!data->stop)
    {
        IDirectInputDevice8W *device;
        IDirectInputEffect *effect;
        DIJOYSTATE state = {0};
        unsigned int i;

        if (WaitForSingleObject( device_state_event, TEST_POLL_TIME ) == WAIT_TIMEOUT) continue;

        if ((device = get_selected_device()))
        {
            DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};

            IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
            IDirectInputDevice8_GetCapabilities( device, &caps );
            IDirectInputDevice8_Release( device );

            dump_joy_state(&state);
            set_di_device_state( data->di_dialog_hwnd, &state, &caps );
        }

        if ((effect = get_selected_effect()))
        {
            DWORD flags = DIEP_AXES | DIEP_DIRECTION | DIEP_NORESTART;
            LONG direction[3] = {0};
            DWORD axes[3] = {0};
            DIEFFECT params =
            {
                .dwSize = sizeof(DIEFFECT),
                .dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS,
                .rglDirection = direction,
                .rgdwAxes = axes,
                .cAxes = 3,
            };

            IDirectInputEffect_GetParameters( effect, &params, flags );
            params.rgdwAxes[0] = state.lX;
            params.rgdwAxes[1] = state.lY;

            for (i=0; i < TEST_MAX_BUTTONS; i++)
            {
                if (state.rgbButtons[i])
                {
                    IDirectInputEffect_SetParameters( effect, &params, flags );
                    IDirectInputEffect_Start( effect, 1, 0 );
                    break;
                }
            }

            IDirectInputEffect_Release( effect );
        }
    }

    return 0;
}

static void initialize_effects_list( HWND hwnd, IDirectInputDevice8W *device )
{
    struct effect *effect;

    clear_effects();

    IDirectInputDevice8_EnumEffects( device, enum_effects, device, 0 );

    SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_RESETCONTENT, 0, 0 );
    SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_ADDSTRING, 0, (LPARAM)L"None" );

    LIST_FOR_EACH_ENTRY( effect, &effects, struct effect, entry )
    {
        DIEFFECTINFOW info = {.dwSize = sizeof(DIEFFECTINFOW)};
        GUID guid;

        if (FAILED(IDirectInputEffect_GetEffectGuid( effect->effect, &guid ))) continue;
        if (FAILED(IDirectInputDevice8_GetEffectInfo( device, &info, &guid ))) continue;
        SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_ADDSTRING, 0, (LPARAM)( info.tszName + 5 ) );
    }
}

static void test_handle_joychange(HWND hwnd, struct JoystickData *data)
{
    DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
    IDirectInputDevice8W *device;
    DIJOYSTATE state = {0};
    struct list *entry;
    int i;

    set_selected_device( NULL );

    i = SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_GETCURSEL, 0, 0 );
    if (i < 0) return;

    entry = list_head( &devices );
    while (i-- && entry) entry = list_next( &devices, entry );
    if (!entry) return;

    device = LIST_ENTRY( entry, struct device, entry )->device;
    if (FAILED(IDirectInputDevice8_GetCapabilities( device, &caps ))) return;

    set_di_device_state( data->di_dialog_hwnd, &state, &caps );
    set_selected_device( device );
    initialize_effects_list( hwnd, device );
}

static void ff_handle_effectchange( HWND hwnd )
{
    IDirectInputDevice8W *device;
    struct list *entry;
    int sel;

    set_selected_effect( NULL );

    sel = SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_GETCURSEL, 0, 0 ) - 1;
    if (sel < 0) return;

    entry = list_head( &effects );
    while (sel-- && entry) entry = list_next( &effects, entry );
    if (!entry) return;

    set_selected_effect( LIST_ENTRY( entry, struct effect, entry )->effect );

    if ((device = get_selected_device()))
    {
        IDirectInputDevice8_Unacquire( device );
        IDirectInputDevice8_SetCooperativeLevel( device, GetAncestor( hwnd, GA_ROOT ), DISCL_BACKGROUND | DISCL_EXCLUSIVE );
        IDirectInputDevice8_Acquire( device );
        IDirectInputDevice8_Release( device );
    }
}

/*********************************************************************
 * test_dlgproc [internal]
 *
 */
static void di_update_select_combo( HWND hwnd )
{
    struct device *entry;

    SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_RESETCONTENT, 0, 0 );

    LIST_FOR_EACH_ENTRY( entry, &devices, struct device, entry )
    {
        DIDEVICEINSTANCEW info = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
        if (FAILED(IDirectInputDevice8_GetDeviceInfo( entry->device, &info ))) continue;
        SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_ADDSTRING, 0, (LPARAM)info.tszInstanceName );
    }
}

static void update_device_views( HWND hwnd )
{
    HWND parent, view;

    parent = GetDlgItem( hwnd, IDC_DI_AXES );
    view = FindWindowExW( parent, NULL, L"JoyCplDInputAxes", NULL );
    InvalidateRect( view, NULL, TRUE );

    parent = GetDlgItem( hwnd, IDC_DI_POVS );
    view = FindWindowExW( parent, NULL, L"JoyCplDInputPOVs", NULL );
    InvalidateRect( view, NULL, TRUE );

    parent = GetDlgItem( hwnd, IDC_DI_BUTTONS );
    view = FindWindowExW( parent, NULL, L"JoyCplDInputButtons", NULL );
    InvalidateRect( view, NULL, TRUE );
}

static void create_device_views( HWND hwnd )
{
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    HWND parent;
    LONG margin;
    RECT rect;

    parent = GetDlgItem( hwnd, IDC_DI_AXES );
    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 10 / 100;
    InflateRect( &rect, -margin, -margin );

    CreateWindowW( L"JoyCplDInputAxes", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );

    parent = GetDlgItem( hwnd, IDC_DI_POVS );
    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 10 / 100;
    InflateRect( &rect, -margin, -margin );

    CreateWindowW( L"JoyCplDInputPOVs", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );

    parent = GetDlgItem( hwnd, IDC_DI_BUTTONS );
    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 10 / 100;
    InflateRect( &rect, -margin, -margin );

    CreateWindowW( L"JoyCplDInputButtons", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );
}

static INT_PTR CALLBACK test_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static HANDLE thread;
    static struct JoystickData *data;
    TRACE("(%p, 0x%08x/%d, 0x%Ix)\n", hwnd, msg, msg, lparam);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            data = (struct JoystickData*) ((PROPSHEETPAGEW*)lparam)->lParam;

            di_update_select_combo( hwnd );
            create_device_views( hwnd );

            return TRUE;
        }

        case WM_COMMAND:
            switch(wparam)
            {
            case MAKEWPARAM( IDC_DI_DEVICES, CBN_SELCHANGE ):
                test_handle_joychange( hwnd, data );

                SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_SETCURSEL, 0, 0 );
                ff_handle_effectchange( hwnd );
                break;

            case MAKEWPARAM( IDC_DI_EFFECTS, LBN_SELCHANGE ):
                ff_handle_effectchange( hwnd );
                break;
            }
            return TRUE;

        case WM_NOTIFY:
            switch(((LPNMHDR)lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    DWORD tid;

                    di_update_select_combo( hwnd );

                    data->stop = FALSE;
                    data->di_dialog_hwnd = hwnd;

                    /* Set the first joystick as default */
                    SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_SETCURSEL, 0, 0 );
                    test_handle_joychange(hwnd, data);

                    SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_SETCURSEL, 0, 0 );
                    ff_handle_effectchange( hwnd );

                    thread = CreateThread(NULL, 0, input_thread, (void*) data, 0, &tid);
                }
                break;

                case PSN_RESET: /* intentional fall-through */
                case PSN_KILLACTIVE:
                    /* Stop input thread */
                    data->stop = TRUE;
                    MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, 0);
                    CloseHandle(thread);
                break;
            }
            return TRUE;

        case WM_USER:
            update_device_views( hwnd );
            return TRUE;
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

static void display_cpl_sheets( HWND parent, struct JoystickData *data )
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
            .lParam = (INT_PTR)data,
        },
        {
            .dwSize = sizeof(PROPSHEETPAGEW),
            .hInstance = hcpl,
            .pszTemplate = MAKEINTRESOURCEW( IDD_TEST_DI ),
            .pfnDlgProc = test_dlgproc,
            .lParam = (INT_PTR)data,
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
    static struct JoystickData data;
    TRACE("(%p, %u, 0x%Ix, 0x%Ix)\n", hwnd, command, lParam1, lParam2);

    switch (command)
    {
        case CPL_INIT:
        {
            HRESULT hr;

            register_window_class();
            device_state_event = CreateEventW( NULL, FALSE, FALSE, NULL );

            /* Initialize dinput */
            hr = DirectInput8Create(GetModuleHandleW(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void**)&data.di, NULL);

            if (FAILED(hr))
            {
                ERR("Failed to initialize DirectInput: 0x%08lx\n", hr);
                return FALSE;
            }

            IDirectInput8_EnumDevices( data.di, DI8DEVCLASS_GAMECTRL, enum_devices, &data, DIEDFL_ATTACHEDONLY );

            return TRUE;
        }
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
            display_cpl_sheets(hwnd, &data);
            break;

        case CPL_STOP:
            clear_effects();
            clear_devices();

            /* And destroy dinput too */
            IDirectInput8_Release(data.di);

            CloseHandle( device_state_event );
            unregister_window_class();
            break;
    }

    return FALSE;
}
