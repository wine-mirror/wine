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

struct dinput_device;
struct hid_value_caps;

typedef BOOL (*enum_object_callback)( struct dinput_device *impl, UINT index, struct hid_value_caps *caps,
                                      const DIDEVICEOBJECTINSTANCEW *instance, void *data );

struct dinput_device_vtbl
{
    void (*destroy)( IDirectInputDevice8W *iface );
    HRESULT (*poll)( IDirectInputDevice8W *iface );
    HRESULT (*read)( IDirectInputDevice8W *iface );
    HRESULT (*acquire)( IDirectInputDevice8W *iface );
    HRESULT (*unacquire)( IDirectInputDevice8W *iface );
    HRESULT (*enum_objects)( IDirectInputDevice8W *iface, const DIPROPHEADER *filter, DWORD flags,
                             enum_object_callback callback, void *context );
    HRESULT (*get_property)( IDirectInputDevice8W *iface, DWORD property, DIPROPHEADER *header,
                             const DIDEVICEOBJECTINSTANCEW *instance );
    HRESULT (*get_effect_info)( IDirectInputDevice8W *iface, DIEFFECTINFOW *info, const GUID *guid );
    HRESULT (*create_effect)( IDirectInputDevice8W *iface, IDirectInputEffect **out );
    HRESULT (*send_force_feedback_command)( IDirectInputDevice8W *iface, DWORD command, BOOL unacquire );
    HRESULT (*send_device_gain)( IDirectInputDevice8W *iface, LONG device_gain );
    HRESULT (*enum_created_effect_objects)( IDirectInputDevice8W *iface, LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                            void *context, DWORD flags );
};

#define DEVICE_STATE_MAX_SIZE 1024

struct object_properties
{
    LONG bit_size;
    LONG physical_min;
    LONG physical_max;
    LONG logical_min;
    LONG logical_max;
    LONG range_min;
    LONG range_max;
    LONG deadzone;
    LONG saturation;
    UINT_PTR app_data;
    DWORD calibration_mode;
    DWORD granularity;
    DWORD scan_code;
};

enum device_status
{
    STATUS_UNACQUIRED,
    STATUS_ACQUIRED,
    STATUS_UNPLUGGED,
};

struct dinput_device
{
    IDirectInputDevice8W        IDirectInputDevice8W_iface;
    IDirectInputDevice8A        IDirectInputDevice8A_iface;
    LONG                        internal_ref;
    LONG                        ref;

    GUID                        guid;
    CRITICAL_SECTION            crit;
    struct dinput              *dinput;
    struct list                 entry;       /* entry into acquired device list */
    HANDLE                      hEvent;
    DIDEVICEINSTANCEW           instance;
    DIDEVCAPS                   caps;
    DWORD                       dwCoopLevel;
    HWND                        win;
    enum device_status          status;
    BOOL                        use_raw_input; /* use raw input instead of low-level messages */
    HHOOK                       cbt_hook;    /* CBT hook to track foreground changes */

    LPDIDEVICEOBJECTDATA        data_queue;  /* buffer for 'GetDeviceData'.                 */
    int                         queue_len;   /* valid size of the queue                     */
    int                         queue_head;  /* position to write new event into queue      */
    int                         queue_tail;  /* next event to read from queue               */
    BOOL                        overflow;    /* return DI_BUFFEROVERFLOW in 'GetDeviceData' */
    DWORD                       buffersize;  /* size of the queue - set in 'SetProperty'    */

    DIDATAFORMAT device_format;
    DIDATAFORMAT user_format;

    /* internal device callbacks */
    HANDLE read_event;
    const struct dinput_device_vtbl *vtbl;

    BYTE device_state_report_id;
    BYTE device_state[DEVICE_STATE_MAX_SIZE];

    BOOL autocenter;
    LONG device_gain;
    DWORD force_feedback_state;
    struct object_properties *object_properties;
};

extern void dinput_device_init( struct dinput_device *device, const struct dinput_device_vtbl *vtbl,
                                const GUID *guid, struct dinput *dinput );
extern void dinput_device_internal_addref( struct dinput_device *device );
extern void dinput_device_internal_release( struct dinput_device *device );

extern HRESULT dinput_device_init_device_format( IDirectInputDevice8W *iface );
extern int dinput_device_object_index_from_id( IDirectInputDevice8W *iface, DWORD id );
extern BOOL device_object_matches_semantic( const DIDEVICEINSTANCEW *instance, const DIOBJECTDATAFORMAT *object,
                                            DWORD semantic, BOOL exact );

extern BOOL get_app_key(HKEY*, HKEY*);
extern DWORD get_config_key( HKEY, HKEY, const WCHAR *, WCHAR *, DWORD );
extern BOOL device_instance_is_disabled( DIDEVICEINSTANCEW *instance, BOOL *override );
extern void queue_event( IDirectInputDevice8W *iface, int index, DWORD data, DWORD time, DWORD seq );

extern const GUID dinput_pidvid_guid;

#endif /* __WINE_DLLS_DINPUT_DINPUTDEVICE_PRIVATE_H */
