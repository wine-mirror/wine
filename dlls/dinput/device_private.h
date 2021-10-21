/*
 * Copyright 2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
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

#ifndef __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H
#define __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dinput.h"
#include "wine/list.h"
#include "dinput_private.h"

typedef struct
{
    int size;
    int offset_in;
    int offset_out;
    int value;
} DataTransform;

typedef struct
{
    int                         size;
    int                         internal_format_size;
    DataTransform              *dt;

    int                        *offsets;     /* object offsets */
    LPDIDATAFORMAT              wine_df;     /* wine internal data format */
    LPDIDATAFORMAT              user_df;     /* user defined data format */
} DataFormat;

typedef struct
{
    unsigned int offset;
    UINT_PTR uAppData;
} ActionMap;

typedef HRESULT dinput_device_read_state( IDirectInputDevice8W *iface );

struct dinput_device_vtbl
{
    void (*release)( IDirectInputDevice8W *iface );
    HRESULT (*poll)( IDirectInputDevice8W *iface );
    HRESULT (*read)( IDirectInputDevice8W *iface );
    HRESULT (*acquire)( IDirectInputDevice8W *iface );
    HRESULT (*unacquire)( IDirectInputDevice8W *iface );
    HRESULT (*enum_objects)( IDirectInputDevice8W *iface, const DIPROPHEADER *filter, DWORD flags,
                             LPDIENUMDEVICEOBJECTSCALLBACKW callback, void *context );
    HRESULT (*get_property)( IDirectInputDevice8W *iface, DWORD property, DIPROPHEADER *header,
                             DIDEVICEOBJECTINSTANCEW *instance );
    HRESULT (*set_property)( IDirectInputDevice8W *iface, DWORD property, const DIPROPHEADER *header,
                             const DIDEVICEOBJECTINSTANCEW *instance );
    HRESULT (*get_effect_info)( IDirectInputDevice8W *iface, DIEFFECTINFOW *info, const GUID *guid );
    HRESULT (*create_effect)( IDirectInputDevice8W *iface, IDirectInputEffect **out );
    HRESULT (*send_force_feedback_command)( IDirectInputDevice8W *iface, DWORD command );
    HRESULT (*enum_created_effect_objects)( IDirectInputDevice8W *iface, LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                            void *context, DWORD flags );
};

#define DEVICE_STATE_MAX_SIZE 1024

/* Device implementation */
typedef struct IDirectInputDeviceImpl IDirectInputDeviceImpl;
struct IDirectInputDeviceImpl
{
    IDirectInputDevice8W        IDirectInputDevice8W_iface;
    IDirectInputDevice8A        IDirectInputDevice8A_iface;
    LONG                        ref;
    GUID                        guid;
    CRITICAL_SECTION            crit;
    IDirectInputImpl           *dinput;
    struct list                 entry;       /* entry into acquired device list */
    HANDLE                      hEvent;
    DIDEVICEINSTANCEW           instance;
    DIDEVCAPS                   caps;
    DWORD                       dwCoopLevel;
    HWND                        win;
    int                         acquired;

    BOOL                        use_raw_input; /* use raw input instead of low-level messages */
    RAWINPUTDEVICE              raw_device;    /* raw device to (un)register */

    LPDIDEVICEOBJECTDATA        data_queue;  /* buffer for 'GetDeviceData'.                 */
    int                         queue_len;   /* valid size of the queue                     */
    int                         queue_head;  /* position to write new event into queue      */
    int                         queue_tail;  /* next event to read from queue               */
    BOOL                        overflow;    /* return DI_BUFFEROVERFLOW in 'GetDeviceData' */
    DWORD                       buffersize;  /* size of the queue - set in 'SetProperty'    */

    DataFormat                  data_format; /* user data format and wine to user format converter */

    /* Action mapping */
    int                         num_actions; /* number of actions mapped */
    ActionMap                  *action_map;  /* array of mappings */

    /* internal device callbacks */
    HANDLE read_event;
    const struct dinput_device_vtbl *vtbl;

    BYTE device_state_report_id;
    BYTE device_state[DEVICE_STATE_MAX_SIZE];
};

extern HRESULT direct_input_device_alloc( SIZE_T size, const IDirectInputDevice8WVtbl *vtbl, const struct dinput_device_vtbl *internal_vtbl,
                                          const GUID *guid, IDirectInputImpl *dinput, void **out ) DECLSPEC_HIDDEN;
extern HRESULT direct_input_device_init( IDirectInputDevice8W *iface );
extern void direct_input_device_destroy( IDirectInputDevice8W *iface );
extern const IDirectInputDevice8AVtbl dinput_device_a_vtbl DECLSPEC_HIDDEN;

extern BOOL get_app_key(HKEY*, HKEY*) DECLSPEC_HIDDEN;
extern DWORD get_config_key( HKEY, HKEY, const WCHAR *, WCHAR *, DWORD ) DECLSPEC_HIDDEN;
extern BOOL device_instance_is_disabled( DIDEVICEINSTANCEW *instance, BOOL *override ) DECLSPEC_HIDDEN;

/* Routines to do DataFormat / WineFormat conversions */
extern void fill_DataFormat(void *out, DWORD size, const void *in, const DataFormat *df)  DECLSPEC_HIDDEN;
extern void queue_event( IDirectInputDevice8W *iface, int inst_id, DWORD data, DWORD time, DWORD seq ) DECLSPEC_HIDDEN;

extern const GUID dinput_pidvid_guid DECLSPEC_HIDDEN;

/* Various debug tools */
extern void _dump_DIPROPHEADER(LPCDIPROPHEADER diph)  DECLSPEC_HIDDEN;
extern void _dump_OBJECTINSTANCEW(const DIDEVICEOBJECTINSTANCEW *ddoi)  DECLSPEC_HIDDEN;
extern void _dump_DIDATAFORMAT(const DIDATAFORMAT *df)  DECLSPEC_HIDDEN;
extern const char *_dump_dinput_GUID(const GUID *guid)  DECLSPEC_HIDDEN;

/* And the stubs */
extern HRESULT WINAPI IDirectInputDevice2WImpl_Acquire(LPDIRECTINPUTDEVICE8W iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_Unacquire(LPDIRECTINPUTDEVICE8W iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetCapabilities( IDirectInputDevice8W *iface, DIDEVCAPS *caps );
extern HRESULT WINAPI IDirectInputDevice2WImpl_SetDataFormat(LPDIRECTINPUTDEVICE8W iface, LPCDIDATAFORMAT df) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_SetCooperativeLevel(LPDIRECTINPUTDEVICE8W iface, HWND hwnd, DWORD dwflags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceInfo( IDirectInputDevice8W *iface,
                                                              DIDEVICEINSTANCEW *instance );
extern HRESULT WINAPI IDirectInputDevice2WImpl_SetEventNotification(LPDIRECTINPUTDEVICE8W iface, HANDLE hnd) DECLSPEC_HIDDEN;
extern ULONG WINAPI IDirectInputDevice2WImpl_Release(LPDIRECTINPUTDEVICE8W iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_QueryInterface(LPDIRECTINPUTDEVICE8W iface, REFIID riid, LPVOID *ppobj) DECLSPEC_HIDDEN;
extern ULONG WINAPI IDirectInputDevice2WImpl_AddRef(LPDIRECTINPUTDEVICE8W iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)  DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_SetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIPROPHEADER pdiph) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface, 
							     LPDIDEVICEOBJECTINSTANCEW pdidoi,
							     DWORD dwObj,
							     DWORD dwHow) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceState( IDirectInputDevice8W *iface, DWORD len, void *ptr );
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceData(LPDIRECTINPUTDEVICE8W iface, DWORD dodsize, LPDIDEVICEOBJECTDATA dod,
                                                             LPDWORD entries, DWORD flags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_RunControlPanel(LPDIRECTINPUTDEVICE8W iface, HWND hwndOwner, DWORD dwFlags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_Initialize(LPDIRECTINPUTDEVICE8W iface, HINSTANCE hinst, DWORD dwVersion,
                                                          REFGUID rguid) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_CreateEffect(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIEFFECT lpeff,
                                                            LPDIRECTINPUTEFFECT *ppdef, LPUNKNOWN pUnkOuter) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_EnumEffects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMEFFECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)  DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIEFFECTINFOW lpdei,
	REFGUID rguid)  DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_GetForceFeedbackState(LPDIRECTINPUTDEVICE8W iface, LPDWORD pdwOut) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_SendForceFeedbackCommand(LPDIRECTINPUTDEVICE8W iface, DWORD dwFlags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_EnumCreatedEffectObjects(LPDIRECTINPUTDEVICE8W iface,
                                                                        LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
                                                                        LPVOID lpvRef, DWORD dwFlags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_Escape(LPDIRECTINPUTDEVICE8W iface, LPDIEFFESCAPE lpDIEEsc) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_Poll(LPDIRECTINPUTDEVICE8W iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice2WImpl_SendDeviceData(LPDIRECTINPUTDEVICE8W iface, DWORD cbObjectData,
                                                              LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice7WImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8W iface,
								 LPCWSTR lpszFileName,
								 LPDIENUMEFFECTSINFILECALLBACK pec,
								 LPVOID pvRef,
								 DWORD dwFlags)  DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice7WImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8W iface,
								 LPCWSTR lpszFileName,
								 DWORD dwEntries,
								 LPDIFILEEFFECT rgDiFileEft,
								 DWORD dwFlags)  DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice8WImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
							      LPDIACTIONFORMATW lpdiaf,
							      LPCWSTR lpszUserName,
							      DWORD dwFlags) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirectInputDevice8WImpl_SetActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                             const WCHAR *username, DWORD flags );
extern HRESULT WINAPI IDirectInputDevice8WImpl_GetImageInfo(LPDIRECTINPUTDEVICE8W iface,
							    LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader) DECLSPEC_HIDDEN;

#endif /* __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H */
