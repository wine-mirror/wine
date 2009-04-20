/*		DirectInput Joystick device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
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

/*
 * To Do:
 *	dead zone
 *	force feedback
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#ifdef HAVE_LINUX_IOCTL_H
# include <linux/ioctl.h>
#endif
#ifdef HAVE_LINUX_JOYSTICK_H
# include <linux/joystick.h>
# undef SW_MAX
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "joystick_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

#ifdef HAVE_LINUX_22_JOYSTICK_API

#define JOYDEV_NEW "/dev/input/js"
#define JOYDEV_OLD "/dev/js"

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;
struct JoystickImpl
{
        struct JoystickGenericImpl generic;

	char				dev[32];

	/* joystick private */
	int				joyfd;
	LONG				deadzone;
	int				*axis_map;
	int				axes;
        POINTL                          povs[4];
};

static const GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

#define MAX_JOYSTICKS 64
static INT joystick_devices_count = -1;
static LPSTR joystick_devices[MAX_JOYSTICKS];

static void joy_polldev(JoystickGenericImpl *This);

static INT find_joystick_devices(void)
{
    INT i;

    if (joystick_devices_count != -1) return joystick_devices_count;

    joystick_devices_count = 0;
    for (i = 0; i < MAX_JOYSTICKS; i++)
    {
        CHAR device_name[MAX_PATH], *str;
        INT len;
        int fd;

        len = sprintf(device_name, "%s%d", JOYDEV_NEW, i) + 1;
        if ((fd = open(device_name, O_RDONLY)) < 0)
        {
            len = sprintf(device_name, "%s%d", JOYDEV_OLD, i) + 1;
            if ((fd = open(device_name, O_RDONLY)) < 0) continue;
        }

        close(fd);

        if (!(str = HeapAlloc(GetProcessHeap(), 0, len))) break;
        memcpy(str, device_name, len);

        joystick_devices[joystick_devices_count++] = str;
    }

    return joystick_devices_count;
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    int fd = -1;

    if (id >= find_joystick_devices()) return FALSE;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        if ((fd = open(joystick_devices[id], O_RDONLY)) < 0)
        {
            WARN("open(%s, O_RDONLY) failed: %s\n", joystick_devices[id], strerror(errno));
            return FALSE;
        }

        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_Wine_Joystick_GUID;
        /* we only support traditional joysticks for now */
        if (version >= 0x0800)
            lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        else
            lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
        sprintf(lpddi->tszInstanceName, "Joystick %d", id);
#if defined(JSIOCGNAME)
        if (ioctl(fd,JSIOCGNAME(sizeof(lpddi->tszProductName)),lpddi->tszProductName) < 0) {
            WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", joystick_devices[id], strerror(errno));
            strcpy(lpddi->tszProductName, "Wine Joystick");
        }
#else
        strcpy(lpddi->tszProductName, "Wine Joystick");
#endif

        lpddi->guidFFDriver = GUID_NULL;
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", joystick_devices[id], lpddi->tszProductName);
        return TRUE;
    }

    return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    int fd = -1;
    char name[MAX_PATH];
    char friendly[32];

    if (id >= find_joystick_devices()) return FALSE;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        if ((fd = open(joystick_devices[id], O_RDONLY)) < 0)
        {
            WARN("open(%s,O_RDONLY) failed: %s\n", joystick_devices[id], strerror(errno));
            return FALSE;
        }

        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_Wine_Joystick_GUID;
        /* we only support traditional joysticks for now */
        if (version >= 0x0800)
            lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        else
            lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
        sprintf(friendly, "Joystick %d", id);
        MultiByteToWideChar(CP_ACP, 0, friendly, -1, lpddi->tszInstanceName, MAX_PATH);
#if defined(JSIOCGNAME)
        if (ioctl(fd,JSIOCGNAME(sizeof(name)),name) < 0) {
            WARN("ioctl(%s, JSIOCGNAME) failed: %s\n", joystick_devices[id], strerror(errno));
            strcpy(name, "Wine Joystick");
        }
#else
        strcpy(name, "Wine Joystick");
#endif
        MultiByteToWideChar(CP_ACP, 0, name, -1, lpddi->tszProductName, MAX_PATH);
        lpddi->guidFFDriver = GUID_NULL;
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", joystick_devices[id], name);
        return TRUE;
    }

    return FALSE;
}

/*
 * Setup the dinput options.
 */

static HRESULT setup_dinput_options(JoystickImpl * device)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey;
    int tokens = 0;
    int axis = 0;
    int pov = 0;

    buffer[MAX_PATH]='\0';

    get_app_key(&hkey, &appkey);

    /* get options */

    if (!get_config_key( hkey, appkey, "DefaultDeadZone", buffer, MAX_PATH )) {
        device->deadzone = atoi(buffer);
        TRACE("setting default deadzone to: \"%s\" %d\n", buffer, device->deadzone);
    }

    device->axis_map = HeapAlloc(GetProcessHeap(), 0, device->axes * sizeof(int));
    if (!device->axis_map) return DIERR_OUTOFMEMORY;

    if (!get_config_key( hkey, appkey, device->generic.name, buffer, MAX_PATH )) {
        static const char *axis_names[] = {"X", "Y", "Z", "Rx", "Ry", "Rz",
                                           "Slider1", "Slider2",
                                           "POV1", "POV2", "POV3", "POV4"};
        const char *delim = ",";
        char * ptr;
        TRACE("\"%s\" = \"%s\"\n", device->generic.name, buffer);

        if ((ptr = strtok(buffer, delim)) != NULL) {
            do {
                int i;

                for (i = 0; i < sizeof(axis_names) / sizeof(axis_names[0]); i++)
                    if (!strcmp(ptr, axis_names[i]))
                    {
                        if (!strncmp(ptr, "POV", 3))
                        {
                            if (pov >= 4)
                            {
                                WARN("Only 4 POVs supported - ignoring extra\n");
                                i = -1;
                            }
                            else
                            {
                                /* Pov takes two axes */
                                device->axis_map[tokens++] = i;
                                pov++;
                            }
                        }
                        else
                        {
                            if (axis >= 8)
                            {
                                FIXME("Only 8 Axes supported - ignoring extra\n");
                                i = -1;
                            }
                            else
                                axis++;
                        }
                        break;
                    }

                if (i == sizeof(axis_names) / sizeof(axis_names[0]))
                {
                    ERR("invalid joystick axis type: \"%s\"\n", ptr);
                    i = -1;
                }

                device->axis_map[tokens] = i;
                tokens++;
            } while ((ptr = strtok(NULL, delim)) != NULL);

            if (tokens != device->axes) {
                ERR("not all joystick axes mapped: %d axes(%d,%d), %d arguments\n", device->axes, axis, pov,tokens);
                while (tokens < device->axes) {
                    device->axis_map[tokens] = -1;
                    tokens++;
                }
            }
        }
    }
    else
    {
        for (tokens = 0; tokens < device->axes; tokens++)
        {
            if (tokens < 8)
                device->axis_map[tokens] = axis++;
            else if (tokens < 16)
            {
                device->axis_map[tokens++] = 8 + pov;
                device->axis_map[tokens  ] = 8 + pov++;
            }
            else
                device->axis_map[tokens] = -1;
        }
    }
    device->generic.devcaps.dwAxes = axis;
    device->generic.devcaps.dwPOVs = pov;

    if (appkey)
        RegCloseKey( appkey );

    if (hkey)
        RegCloseKey( hkey );

    return DI_OK;
}

static HRESULT alloc_device(REFGUID rguid, const void *jvt, IDirectInputImpl *dinput,
    LPDIRECTINPUTDEVICEA* pdev, unsigned short index)
{
    DWORD i;
    JoystickImpl* newDevice;
    char name[MAX_PATH];
    HRESULT hr;
    LPDIDATAFORMAT df = NULL;
    int idx = 0;

    TRACE("%s %p %p %p %hu\n", debugstr_guid(rguid), jvt, dinput, pdev, index);

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0) {
        WARN("out of memory\n");
        *pdev = 0;
        return DIERR_OUTOFMEMORY;
    }

    if (!lstrcpynA(newDevice->dev, joystick_devices[index], sizeof(newDevice->dev)) ||
        (newDevice->joyfd = open(newDevice->dev, O_RDONLY)) < 0)
    {
        WARN("open(%s, O_RDONLY) failed: %s\n", newDevice->dev, strerror(errno));
        HeapFree(GetProcessHeap(), 0, newDevice);
        return DIERR_DEVICENOTREG;
    }

    newDevice->generic.guidInstance = DInput_Wine_Joystick_GUID;
    newDevice->generic.guidInstance.Data3 = index;
    newDevice->generic.guidProduct = DInput_Wine_Joystick_GUID;
    newDevice->generic.joy_polldev = joy_polldev;

    /* get the device name */
#if defined(JSIOCGNAME)
    if (ioctl(newDevice->joyfd,JSIOCGNAME(MAX_PATH),name) < 0) {
        WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", newDevice->dev, strerror(errno));
        strcpy(name, "Wine Joystick");
    }
#else
    strcpy(name, "Wine Joystick");
#endif

    /* copy the device name */
    newDevice->generic.name = HeapAlloc(GetProcessHeap(),0,strlen(name) + 1);
    strcpy(newDevice->generic.name, name);

#ifdef JSIOCGAXES
    if (ioctl(newDevice->joyfd,JSIOCGAXES,&newDevice->axes) < 0) {
        WARN("ioctl(%s,JSIOCGAXES) failed: %s, defauting to 2\n", newDevice->dev, strerror(errno));
        newDevice->axes = 2;
    }
#endif
#ifdef JSIOCGBUTTONS
    if (ioctl(newDevice->joyfd, JSIOCGBUTTONS, &newDevice->generic.devcaps.dwButtons) < 0) {
        WARN("ioctl(%s,JSIOCGBUTTONS) failed: %s, defauting to 2\n", newDevice->dev, strerror(errno));
        newDevice->generic.devcaps.dwButtons = 2;
    }
#endif

    if (newDevice->generic.devcaps.dwButtons > 128)
    {
        WARN("Can't support %d buttons. Clamping down to 128\n", newDevice->generic.devcaps.dwButtons);
        newDevice->generic.devcaps.dwButtons = 128;
    }

    newDevice->generic.base.lpVtbl = jvt;
    newDevice->generic.base.ref = 1;
    newDevice->generic.base.dinput = dinput;
    newDevice->generic.base.guid = *rguid;
    InitializeCriticalSection(&newDevice->generic.base.crit);
    newDevice->generic.base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->generic.base.crit");

    /* setup_dinput_options may change these */
    newDevice->deadzone = 0;

    /* do any user specified configuration */
    hr = setup_dinput_options(newDevice);
    if (hr != DI_OK)
        goto FAILED1;

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto FAILED;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);

    df->dwNumObjs = newDevice->generic.devcaps.dwAxes + newDevice->generic.devcaps.dwPOVs + newDevice->generic.devcaps.dwButtons;
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto FAILED;

    for (i = 0; i < newDevice->axes; i++)
    {
        int wine_obj = newDevice->axis_map[i];

        if (wine_obj < 0) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[wine_obj], df->dwObjSize);
        if (wine_obj < 8)
            df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
        else
        {
            df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(wine_obj - 8) | DIDFT_POV;
            i++; /* POV takes 2 axes */
        }
    }
    for (i = 0; i < newDevice->generic.devcaps.dwButtons; i++)
    {
        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + 12], df->dwObjSize);
        df->rgodf[idx  ].pguid = &GUID_Button;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_PSHBUTTON;
    }
    newDevice->generic.base.data_format.wine_df = df;

    /* initialize default properties */
    for (i = 0; i < c_dfDIJoystick2.dwNumObjs; i++) {
        newDevice->generic.props[i].lDevMin = -32767;
        newDevice->generic.props[i].lDevMax = +32767;
        newDevice->generic.props[i].lMin = 0;
        newDevice->generic.props[i].lMax = 0xffff;
        newDevice->generic.props[i].lDeadZone = newDevice->deadzone;	/* % * 1000 */
        newDevice->generic.props[i].lSaturation = 0;
    }

    IDirectInput_AddRef((LPDIRECTINPUTDEVICE8A)newDevice->generic.base.dinput);

    newDevice->generic.devcaps.dwSize = sizeof(newDevice->generic.devcaps);
    newDevice->generic.devcaps.dwFlags = DIDC_ATTACHED;
    if (newDevice->generic.base.dinput->dwVersion >= 0x0800)
        newDevice->generic.devcaps.dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        newDevice->generic.devcaps.dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
    newDevice->generic.devcaps.dwFFSamplePeriod = 0;
    newDevice->generic.devcaps.dwFFMinTimeResolution = 0;
    newDevice->generic.devcaps.dwFirmwareRevision = 0;
    newDevice->generic.devcaps.dwHardwareRevision = 0;
    newDevice->generic.devcaps.dwFFDriverVersion = 0;

    if (TRACE_ON(dinput)) {
        _dump_DIDATAFORMAT(newDevice->generic.base.data_format.wine_df);
       for (i = 0; i < (newDevice->axes); i++)
           TRACE("axis_map[%d] = %d\n", i, newDevice->axis_map[i]);
        _dump_DIDEVCAPS(&newDevice->generic.devcaps);
    }

    *pdev = (LPDIRECTINPUTDEVICEA)newDevice;

    return DI_OK;

FAILED:
    hr = DIERR_OUTOFMEMORY;
FAILED1:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    release_DataFormat(&newDevice->generic.base.data_format);
    HeapFree(GetProcessHeap(),0,newDevice->axis_map);
    HeapFree(GetProcessHeap(),0,newDevice->generic.name);
    HeapFree(GetProcessHeap(),0,newDevice);
    *pdev = 0;

    return hr;
}

/******************************************************************************
  *     get_joystick_index : Get the joystick index from a given GUID
  */
static unsigned short get_joystick_index(REFGUID guid)
{
    GUID wine_joystick = DInput_Wine_Joystick_GUID;
    GUID dev_guid = *guid;

    wine_joystick.Data3 = 0;
    dev_guid.Data3 = 0;

    /* for the standard joystick GUID use index 0 */
    if(IsEqualGUID(&GUID_Joystick,guid)) return 0;

    /* for the wine joystick GUIDs use the index stored in Data3 */
    if(IsEqualGUID(&wine_joystick, &dev_guid)) return guid->Data3;

    return MAX_JOYSTICKS;
}

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
    unsigned short index;

    TRACE("%p %s %p %p\n",dinput, debugstr_guid(rguid), riid, pdev);
    find_joystick_devices();
    *pdev = NULL;

    if ((index = get_joystick_index(rguid)) < MAX_JOYSTICKS &&
        joystick_devices_count && index < joystick_devices_count)
    {
        if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceA,  riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8A, riid))
        {
            return alloc_device(rguid, &JoystickAvt, dinput, pdev, index);
        }

        WARN("no interface\n");
        return DIERR_NOINTERFACE;
    }

    return DIERR_DEVICENOTREG;
}

static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
    unsigned short index;

    TRACE("%p %s %p %p\n",dinput, debugstr_guid(rguid), riid, pdev);
    find_joystick_devices();
    *pdev = NULL;

    if ((index = get_joystick_index(rguid)) < MAX_JOYSTICKS &&
        joystick_devices_count && index < joystick_devices_count)
    {
        if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceW,  riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8W, riid))
        {
            return alloc_device(rguid, &JoystickWvt, dinput, (LPDIRECTINPUTDEVICEA *)pdev, index);
        }
        WARN("no interface\n");
        return DIERR_NOINTERFACE;
    }

    WARN("invalid device GUID %s\n",debugstr_guid(rguid));
    return DIERR_DEVICENOTREG;
}

#undef MAX_JOYSTICKS

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_deviceA,
  joydev_create_deviceW
};

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickLinuxAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    HRESULT res;

    TRACE("(%p)\n",This);

    res = IDirectInputDevice2AImpl_Acquire(iface);
    if (res != DI_OK)
        return res;

    /* open the joystick device */
    if (This->joyfd==-1) {
        TRACE("opening joystick device %s\n", This->dev);

        This->joyfd=open(This->dev,O_RDONLY);
        if (This->joyfd==-1) {
            ERR("open(%s) failed: %s\n", This->dev, strerror(errno));
            IDirectInputDevice2AImpl_Unacquire(iface);
            return DIERR_NOTFOUND;
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickLinuxAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    HRESULT res;

    TRACE("(%p)\n",This);

    res = IDirectInputDevice2AImpl_Unacquire(iface);

    if (res != DI_OK)
        return res;

    if (This->joyfd!=-1) {
        TRACE("closing joystick device\n");
        close(This->joyfd);
        This->joyfd = -1;
        return DI_OK;
    }

    return DI_NOEFFECT;
}

static void joy_polldev(JoystickGenericImpl *This_in) {
    struct pollfd plfd;
    struct	js_event jse;
    JoystickImpl *This = (JoystickImpl*) This_in;

    TRACE("(%p)\n", This);

    if (This->joyfd==-1) {
        WARN("no device\n");
        return;
    }
    while (1)
    {
        LONG value;
        int inst_id = -1;

	plfd.fd = This->joyfd;
	plfd.events = POLLIN;
	if (poll(&plfd,1,0) != 1)
	    return;
	/* we have one event, so we can read */
	if (sizeof(jse)!=read(This->joyfd,&jse,sizeof(jse))) {
	    return;
	}
        TRACE("js_event: type 0x%x, number %d, value %d\n",
              jse.type,jse.number,jse.value);
        if (jse.type & JS_EVENT_BUTTON)
        {
            if (jse.number >= This->generic.devcaps.dwButtons) return;

            inst_id = DIDFT_MAKEINSTANCE(jse.number) | DIDFT_PSHBUTTON;
            This->generic.js.rgbButtons[jse.number] = value = jse.value ? 0x80 : 0x00;
        }
        else if (jse.type & JS_EVENT_AXIS)
        {
            int number = This->axis_map[jse.number];	/* wine format object index */

            if (number < 0) return;
            inst_id = DIDFT_MAKEINSTANCE(number) | (number < 8 ? DIDFT_ABSAXIS : DIDFT_POV);
            value = joystick_map_axis(&This->generic.props[id_to_object(This->generic.base.data_format.wine_df, inst_id)], jse.value);

            TRACE("changing axis %d => %d\n", jse.number, number);
            switch (number)
            {
                case 0: This->generic.js.lX  = value; break;
                case 1: This->generic.js.lY  = value; break;
                case 2: This->generic.js.lZ  = value; break;
                case 3: This->generic.js.lRx = value; break;
                case 4: This->generic.js.lRy = value; break;
                case 5: This->generic.js.lRz = value; break;
                case 6: This->generic.js.rglSlider[0] = value; break;
                case 7: This->generic.js.rglSlider[1] = value; break;
                case 8: case 9: case 10: case 11:
                {
                    int idx = number - 8;

                    if (jse.number % 2)
                        This->povs[idx].y = jse.value;
                    else
                        This->povs[idx].x = jse.value;

                    This->generic.js.rgdwPOV[idx] = value = joystick_map_pov(&This->povs[idx]);
                    break;
                }
                default:
                    WARN("axis %d not supported\n", number);
            }
        }
        if (inst_id >= 0)
            queue_event((LPDIRECTINPUTDEVICE8A)This,
                        id_to_offset(&This->generic.base.data_format, inst_id),
                        value, jse.time, This->generic.base.dinput->evsequence++);
    }
}

static const IDirectInputDevice8AVtbl JoystickAvt =
{
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
        IDirectInputDevice2AImpl_Release,
	JoystickAGenericImpl_GetCapabilities,
        IDirectInputDevice2AImpl_EnumObjects,
	JoystickAGenericImpl_GetProperty,
	JoystickAGenericImpl_SetProperty,
	JoystickLinuxAImpl_Acquire,
	JoystickLinuxAImpl_Unacquire,
	JoystickAGenericImpl_GetDeviceState,
	IDirectInputDevice2AImpl_GetDeviceData,
	IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
	JoystickAGenericImpl_GetObjectInfo,
	JoystickAGenericImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2AImpl_EnumEffects,
	IDirectInputDevice2AImpl_GetEffectInfo,
	IDirectInputDevice2AImpl_GetForceFeedbackState,
	IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	IDirectInputDevice2AImpl_Escape,
	JoystickAGenericImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
	IDirectInputDevice7AImpl_EnumEffectsInFile,
	IDirectInputDevice7AImpl_WriteEffectToFile,
	IDirectInputDevice8AImpl_BuildActionMap,
	IDirectInputDevice8AImpl_SetActionMap,
	IDirectInputDevice8AImpl_GetImageInfo
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(JoystickWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static const IDirectInputDevice8WVtbl JoystickWvt =
{
	IDirectInputDevice2WImpl_QueryInterface,
	XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
        XCAST(Release)IDirectInputDevice2AImpl_Release,
	XCAST(GetCapabilities)JoystickAGenericImpl_GetCapabilities,
        IDirectInputDevice2WImpl_EnumObjects,
	XCAST(GetProperty)JoystickAGenericImpl_GetProperty,
	XCAST(SetProperty)JoystickAGenericImpl_SetProperty,
	XCAST(Acquire)JoystickLinuxAImpl_Acquire,
	XCAST(Unacquire)JoystickLinuxAImpl_Unacquire,
	XCAST(GetDeviceState)JoystickAGenericImpl_GetDeviceState,
	XCAST(GetDeviceData)IDirectInputDevice2AImpl_GetDeviceData,
	XCAST(SetDataFormat)IDirectInputDevice2AImpl_SetDataFormat,
	XCAST(SetEventNotification)IDirectInputDevice2AImpl_SetEventNotification,
	XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
        JoystickWGenericImpl_GetObjectInfo,
	JoystickWGenericImpl_GetDeviceInfo,
	XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
	XCAST(CreateEffect)IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2WImpl_EnumEffects,
	IDirectInputDevice2WImpl_GetEffectInfo,
	XCAST(GetForceFeedbackState)IDirectInputDevice2AImpl_GetForceFeedbackState,
	XCAST(SendForceFeedbackCommand)IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	XCAST(EnumCreatedEffectObjects)IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	XCAST(Escape)IDirectInputDevice2AImpl_Escape,
	XCAST(Poll)JoystickAGenericImpl_Poll,
	XCAST(SendDeviceData)IDirectInputDevice2AImpl_SendDeviceData,
        IDirectInputDevice7WImpl_EnumEffectsInFile,
        IDirectInputDevice7WImpl_WriteEffectToFile,
        IDirectInputDevice8WImpl_BuildActionMap,
        IDirectInputDevice8WImpl_SetActionMap,
        IDirectInputDevice8WImpl_GetImageInfo
};
#undef XCAST

#else  /* HAVE_LINUX_22_JOYSTICK_API */

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  NULL,
  NULL,
  NULL,
  NULL
};

#endif  /* HAVE_LINUX_22_JOYSTICK_API */
