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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_LINUX_22_JOYSTICK_API

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/fcntl.h>
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
#endif
#define JOYDEV	"/dev/js0"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine joystick driver object instances */
#define WINE_JOYSTICK_AXIS_BASE   0
#define WINE_JOYSTICK_BUTTON_BASE 8

typedef struct {
    LONG lMin;
    LONG lMax;
    LONG lDeadZone;
    LONG lSaturation;
} ObjProps;

typedef struct JoystickImpl JoystickImpl;
static IDirectInputDevice8AVtbl JoystickAvt;
static IDirectInputDevice8WVtbl JoystickWvt;
struct JoystickImpl
{
        LPVOID                          lpVtbl;
        DWORD                           ref;
        GUID                            guid;

	/* The 'parent' DInput */
	IDirectInputImpl               *dinput;

	/* joystick private */
	int				joyfd;
	DIJOYSTATE2			js;		/* wine data */
	LPDIDATAFORMAT			user_df;	/* user defined format */
	DataFormat			*transform;	/* wine to user format converter */
	int				*offsets;	/* object offsets */
	ObjProps			*props;
        HANDLE				hEvent;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_head, queue_tail, queue_len;
	BOOL				acquired;
};

static GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, int version)
{
  int fd = -1;

  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;

  if ((dwDevType==0) || (GET_DIDEVICE_TYPE(dwDevType)==DIDEVTYPE_JOYSTICK)) {
    /* check whether we have a joystick */
    if ((fd = open(JOYDEV,O_RDONLY) != -1) || (errno!=ENODEV && errno!=ENOENT)) {
      TRACE("Enumerating the linux Joystick device: %s\n",JOYDEV);

      /* Return joystick */
      lpddi->guidInstance	= GUID_Joystick;
      lpddi->guidProduct	= DInput_Wine_Joystick_GUID;
      /* we only support traditional joysticks for now */
      if (version >= 8)
        lpddi->dwDevType	= DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
      else
        lpddi->dwDevType	= DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
      strcpy(lpddi->tszInstanceName,	"Joystick");
      /* ioctl JSIOCGNAME(len) */
      strcpy(lpddi->tszProductName,	"Wine Joystick");

      lpddi->guidFFDriver = GUID_NULL;
      if (fd != -1)
	close(fd);
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, int version)
{
  int fd = -1;

  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;

  if ((dwDevType==0) || (GET_DIDEVICE_TYPE(dwDevType)==DIDEVTYPE_JOYSTICK)) {
    /* check whether we have a joystick */
    if ((fd = open(JOYDEV,O_RDONLY) != -1) || (errno!=ENODEV && errno!=ENOENT)) {
      TRACE("Enumerating the linux Joystick device\n");

      /* Return joystick */
      lpddi->guidInstance	= GUID_Joystick;
      lpddi->guidProduct	= DInput_Wine_Joystick_GUID;
      /* we only support traditional joysticks for now */
      lpddi->dwDevType	= DIDEVTYPE_JOYSTICK |
	 		 (DIDEVTYPEJOYSTICK_TRADITIONAL<<8);

      MultiByteToWideChar(CP_ACP, 0, "Joystick", -1, lpddi->tszInstanceName, MAX_PATH);
      /* ioctl JSIOCGNAME(len) */
      MultiByteToWideChar(CP_ACP, 0, "Wine Joystick", -1, lpddi->tszProductName, MAX_PATH);
      lpddi->guidFFDriver = GUID_NULL;
      if (fd != -1)
	close(fd);
      return TRUE;
    }
  }

  return FALSE;
}

static JoystickImpl *alloc_device(REFGUID rguid, LPVOID jvt, IDirectInputImpl *dinput)
{
    DWORD i;
    JoystickImpl* newDevice;

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0)
        return 0;

    newDevice->lpVtbl = jvt;
    newDevice->ref = 1;
    newDevice->joyfd = -1;
    newDevice->dinput = dinput;
    newDevice->acquired = FALSE;
    CopyMemory(&(newDevice->guid),rguid,sizeof(*rguid));

    /* wine uses DIJOYSTATE2 as it's internal format so copy
     * the already defined format c_dfDIJoystick2 */
    newDevice->user_df = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwSize);
    if (newDevice->user_df == 0)
        goto FAILED;

    CopyMemory(newDevice->user_df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);

    /* copy default objects */
    newDevice->user_df->rgodf = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwNumObjs*c_dfDIJoystick2.dwObjSize);
    if (newDevice->user_df->rgodf == 0)
        goto FAILED;

    CopyMemory(newDevice->user_df->rgodf,c_dfDIJoystick2.rgodf,c_dfDIJoystick2.dwNumObjs*c_dfDIJoystick2.dwObjSize);

    /* create default properties */
    newDevice->props = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwNumObjs*sizeof(ObjProps));
    if (newDevice->props == 0)
        goto FAILED;

    /* initialize default properties */
    for (i = 0; i < c_dfDIJoystick2.dwNumObjs; i++) {
        newDevice->props[i].lMin = 0;
	newDevice->props[i].lMax = 0xffff;
        newDevice->props[i].lDeadZone = 1000;
        newDevice->props[i].lSaturation = 0;
    }

    /* create an offsets array */
    newDevice->offsets = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,c_dfDIJoystick2.dwNumObjs*sizeof(int));
    if (newDevice->offsets == 0)
        goto FAILED;

    /* create the default transform filter */
    newDevice->transform = create_DataFormat(&c_dfDIJoystick2, newDevice->user_df, newDevice->offsets);

    IDirectInputDevice_AddRef((LPDIRECTINPUTDEVICE8A)newDevice->dinput);

    if (TRACE_ON(dinput))
        _dump_DIDATAFORMAT(newDevice->user_df);

    return newDevice;

FAILED:
    if (newDevice->props)
        HeapFree(GetProcessHeap(),0,newDevice->props);
    if (newDevice->user_df->rgodf)
        HeapFree(GetProcessHeap(),0,newDevice->user_df->rgodf);
    if (newDevice->user_df)
        HeapFree(GetProcessHeap(),0,newDevice->user_df);
    if (newDevice)
        HeapFree(GetProcessHeap(),0,newDevice);
    return 0;
}

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
      (IsEqualGUID(&DInput_Wine_Joystick_GUID,rguid))) {
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
      *pdev=(IDirectInputDeviceA*) alloc_device(rguid, &JoystickAvt, dinput);
      if (*pdev == 0) {
        WARN("out of memory\n");
        return DIERR_OUTOFMEMORY;
      }

      TRACE("Creating a Joystick device (%p)\n", *pdev);
      return DI_OK;
    } else {
      WARN("no interface\n");
      *pdev = 0;
      return DIERR_NOINTERFACE;
    }
  }

  WARN("invalid device GUID\n");
  *pdev = 0;
  return DIERR_DEVICENOTREG;
}

static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
  if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
      (IsEqualGUID(&DInput_Wine_Joystick_GUID,rguid))) {
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceW,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2W,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7W,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
      *pdev = (IDirectInputDeviceW*) alloc_device(rguid, &JoystickWvt, dinput);
      if (*pdev == 0) {
        WARN("out of memory\n");
        return DIERR_OUTOFMEMORY;
      }

      TRACE("Creating a Joystick device (%p)\n", *pdev);
      return DI_OK;
    } else {
      WARN("no interface\n");
      *pdev = 0;
      return DIERR_NOINTERFACE;
    }
  }

  WARN("invalid device GUID\n");
  *pdev = 0;
  return DIERR_DEVICENOTREG;
}

static dinput_device joydev = {
  10,
  "Wine Linux joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_deviceA,
  joydev_create_deviceW
};

DECL_GLOBAL_CONSTRUCTOR(joydev_register) { dinput_register_device(&joydev); }

/******************************************************************************
 *	Joystick
 */
static ULONG WINAPI JoystickAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(JoystickImpl,iface);

    This->ref--;
    if (This->ref)
        return This->ref;

    /* Free the data queue */
    if (This->data_queue != NULL)
        HeapFree(GetProcessHeap(),0,This->data_queue);

    /* Free the DataFormat */
    HeapFree(GetProcessHeap(), 0, This->user_df->rgodf);
    HeapFree(GetProcessHeap(), 0, This->user_df);

    /* Free the properties */
    HeapFree(GetProcessHeap(), 0, This->props);

    /* Free the offsets array */
    HeapFree(GetProcessHeap(),0,This->offsets);

    /* release the data transform filter */
    release_DataFormat(This->transform);

    IDirectInputDevice_Release((LPDIRECTINPUTDEVICE8A)This->dinput);

    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

/******************************************************************************
  *   SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  */
static HRESULT WINAPI JoystickAImpl_SetDataFormat(
    LPDIRECTINPUTDEVICE8A iface,
    LPCDIDATAFORMAT df)
{
    ICOM_THIS(JoystickImpl,iface);
    int i;
    LPDIDATAFORMAT new_df = 0;
    LPDIOBJECTDATAFORMAT new_rgodf = 0;
    ObjProps * new_props = 0;
    int * new_offsets = 0;

    TRACE("(%p,%p)\n",This,df);

    if (This->acquired) {
        WARN("acquired\n");
        return DIERR_ACQUIRED;
    }

    if (TRACE_ON(dinput))
        _dump_DIDATAFORMAT(df);

    /* Store the new data format */
    new_df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
    if (new_df == 0)
        goto FAILED;

    new_rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
    if (new_rgodf == 0)
        goto FAILED;

    new_props = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*sizeof(ObjProps));
    if (new_props == 0)
        goto FAILED;

    new_offsets = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*sizeof(int));
    if (new_offsets == 0)
        goto FAILED;

    HeapFree(GetProcessHeap(),0,This->user_df);
    HeapFree(GetProcessHeap(),0,This->user_df->rgodf);
    HeapFree(GetProcessHeap(),0,This->props);
    HeapFree(GetProcessHeap(),0,This->offsets);
    release_DataFormat(This->transform);

    This->user_df = new_df;
    CopyMemory(This->user_df, df, df->dwSize);
    This->user_df->rgodf = new_rgodf;
    CopyMemory(This->user_df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);
    This->props = new_props;
    for (i = 0; i < df->dwNumObjs; i++) {
        This->props[i].lMin = 0;
        This->props[i].lMax = 0xffff;
        This->props[i].lDeadZone = 1000;
        This->props[i].lSaturation = 0;
    }
    This->offsets = new_offsets;
    This->transform = create_DataFormat(&c_dfDIJoystick2, This->user_df, This->offsets);

    return DI_OK;

FAILED:
    WARN("out of memory\n");
    if (new_offsets)
        HeapFree(GetProcessHeap(),0,new_offsets);
    if (new_props)
        HeapFree(GetProcessHeap(),0,new_props);
    if (new_rgodf)
        HeapFree(GetProcessHeap(),0,new_rgodf);
    if (new_df)
        HeapFree(GetProcessHeap(),0,new_df);
    return DIERR_OUTOFMEMORY;
}

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(%p)\n",This);

    if (This->acquired) {
        WARN("already acquired\n");
        return S_FALSE;
    }

    /* open the joystick device */
    if (This->joyfd==-1) {
        TRACE("opening joystick device %s\n", JOYDEV);

        This->joyfd=open(JOYDEV,O_RDONLY);
        if (This->joyfd==-1) {
            ERR("open(%s) failed: %s\n", JOYDEV, strerror(errno));
            return DIERR_NOTFOUND;
        }
    }

    This->acquired = TRUE;

    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(%p)\n",This);

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    if (This->joyfd!=-1) {
        TRACE("closing joystick device\n");
        close(This->joyfd);
        This->joyfd = -1;
        This->acquired = FALSE;
        return DI_OK;
    }

    This->acquired = FALSE;

    return DI_NOEFFECT;
}

LONG map_axis(JoystickImpl * This, short val, short index)
{
    double    fval = val;
    double    fmin = This->props[index].lMin;
    double    fmax = This->props[index].lMax;
    double    fret;

    fret = (((fval + 32767.0) * (fmax - fmin)) / (32767.0*2.0)) + fmin;

    if (fret >= 0.0)
        fret += 0.5;
    else
        fret -= 0.5;

    return fret;
}

/* convert user format offset to user format object index */
int offset_to_object(JoystickImpl *This, int offset)
{
    int i;

    for (i = 0; i < This->user_df->dwNumObjs; i++) {
        if (This->user_df->rgodf[i].dwOfs == offset)
            return i;
    }

    return -1;
}

static void joy_polldev(JoystickImpl *This) {
    struct timeval tv;
    fd_set	readfds;
    struct	js_event jse;
    TRACE("(%p)\n", This);

    if (This->joyfd==-1) {
        WARN("no device\n");
        return;
    }
    while (1) {
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);FD_SET(This->joyfd,&readfds);
	if (1>select(This->joyfd+1,&readfds,NULL,NULL,&tv))
	    return;
	/* we have one event, so we can read */
	if (sizeof(jse)!=read(This->joyfd,&jse,sizeof(jse))) {
	    return;
	}
        TRACE("js_event: type 0x%x, number %d, value %d\n",
              jse.type,jse.number,jse.value);
        if (jse.type & JS_EVENT_BUTTON) {
            int offset = This->offsets[jse.number + 12];
            int value = jse.value?0x80:0x00;

            This->js.rgbButtons[jse.number] = value;
            GEN_EVENT(offset,value,jse.time,(This->dinput->evsequence)++);
        } else if (jse.type & JS_EVENT_AXIS) {
            if (jse.number < 8) {
                int offset = This->offsets[jse.number];
                int index = offset_to_object(This, offset);
                LONG value = map_axis(This, jse.value, index);

                /* FIXME do deadzone and saturation here */

                switch (jse.number) {
                case 0:
                    This->js.lX = value;
                    break;
                case 1:
                    This->js.lY = value;
                    break;
                case 2:
                    This->js.lZ = value;
                    break;
                case 3:
                    This->js.lRx = value;
                    break;
                case 4:
                    This->js.lRy = value;
                    break;
                case 5:
                    This->js.lRz = value;
                    break;
                case 6:
                    This->js.rglSlider[0] = value;
                    break;
                case 7:
                    This->js.rglSlider[1] = value;
                    break;
                }

                GEN_EVENT(offset,value,jse.time,(This->dinput->evsequence)++);
            } else 
                WARN("axis %d not supported\n", jse.number);
        }
    }
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceState(
    LPDIRECTINPUTDEVICE8A iface,
    DWORD len,
    LPVOID ptr)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(%p,0x%08lx,%p)\n",This,len,ptr);

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    /* update joystick state */
    joy_polldev(This);

    /* convert and copy data to user supplied buffer */
    fill_DataFormat(ptr, &This->js, This->transform);

    return DI_OK;
}

/******************************************************************************
  *     GetDeviceData : gets buffered input data.
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  ICOM_THIS(JoystickImpl,iface);

  FIXME("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx),STUB!\n",This,dodsize,*entries,flags);

  joy_polldev(This);
  if (flags & DIGDD_PEEK)
    FIXME("DIGDD_PEEK\n");

  if (dod == NULL) {
  } else {
  }
  return DI_OK;
}

int find_property(JoystickImpl * This, LPCDIPROPHEADER ph)
{
    int i;
    if (ph->dwHow == DIPH_BYOFFSET) {
        return offset_to_object(This, ph->dwObj);
    } else if (ph->dwHow == DIPH_BYID) {
        int axis = 0;
        int button = 0;
        for (i = 0; i < This->user_df->dwNumObjs; i++) {
            DWORD type = 0;
            if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_AXIS) {
                axis++;
                type = DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) |
                    DIDFT_MAKEINSTANCE(axis << WINE_JOYSTICK_AXIS_BASE);
                TRACE("type = 0x%08lx\n", type);
            }
            if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_BUTTON) {
                button++;
                type = DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) |
                    DIDFT_MAKEINSTANCE(button << WINE_JOYSTICK_BUTTON_BASE);
                TRACE("type = 0x%08lx\n", type);
            }
            if (type == ph->dwObj) {
                return i;
            }
        }
    }

    return -1;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(
    LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid,
    LPCDIPROPHEADER ph)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(rguid),ph);

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(ph);

    if (!HIWORD(rguid)) {
        switch ((DWORD)rguid) {
        case (DWORD) DIPROP_BUFFERSIZE: {
            LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
            FIXME("buffersize = %ld\n",pd->dwData);
            break;
        }
        case (DWORD)DIPROP_RANGE: {
            LPCDIPROPRANGE	pr = (LPCDIPROPRANGE)ph;
            int obj = find_property(This, ph);
            TRACE("proprange(%ld,%ld) obj=%d\n",pr->lMin,pr->lMax,obj);
            if (obj >= 0) {
                This->props[obj].lMin = pr->lMin;
                This->props[obj].lMax = pr->lMax;
                return DI_OK;
            }
            break;
        }
        case (DWORD)DIPROP_DEADZONE: {
            LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
            int obj = find_property(This, ph);
            TRACE("deadzone(%ld) obj=%d\n",pd->dwData,obj);
            if (obj >= 0) {
                This->props[obj].lDeadZone  = pd->dwData;
                return DI_OK;
            }
            break;
        }
        case (DWORD)DIPROP_SATURATION: {
            LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
            int obj = find_property(This, ph);
            TRACE("saturation(%ld) obj=%d\n",pd->dwData,obj);
            if (obj >= 0) {
                This->props[obj].lSaturation = pd->dwData;
                return DI_OK;
            }
            break;
        }
        default:
            FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
            break;
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
static HRESULT WINAPI JoystickAImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface, HANDLE hnd
) {
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);
    This->hEvent = hnd;
    return DI_OK;
}

static void _dump_DIDEVCAPS(LPDIDEVCAPS lpDIDevCaps)
{
    TRACE("dwSize: %ld\n", lpDIDevCaps->dwSize);
    TRACE("dwFlags: %08lx\n",lpDIDevCaps->dwFlags);
    TRACE("dwDevType: %08lx %s\n", lpDIDevCaps->dwDevType,
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_MOUSE ? "DIDEVTYPE_MOUSE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_KEYBOARD ? "DIDEVTYPE_KEYBOARD" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_JOYSTICK ? "DIDEVTYPE_JOYSTICK" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_HID ? "DIDEVTYPE_HID" : "UNKNOWN");
    TRACE("dwAxes: %ld\n",lpDIDevCaps->dwAxes);
    TRACE("dwButtons: %ld\n",lpDIDevCaps->dwButtons);
    TRACE("dwPOVs: %ld\n",lpDIDevCaps->dwPOVs);
    if (lpDIDevCaps->dwSize > sizeof(DIDEVCAPS_DX3)) {
        TRACE("dwFFSamplePeriod: %ld\n",lpDIDevCaps->dwFFSamplePeriod);
        TRACE("dwFFMinTimeResolution: %ld\n",lpDIDevCaps->dwFFMinTimeResolution);
        TRACE("dwFirmwareRevision: %ld\n",lpDIDevCaps->dwFirmwareRevision);
        TRACE("dwHardwareRevision: %ld\n",lpDIDevCaps->dwHardwareRevision);
        TRACE("dwFFDriverVersion: %ld\n",lpDIDevCaps->dwFFDriverVersion);
    }
}

static HRESULT WINAPI JoystickAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    ICOM_THIS(JoystickImpl,iface);
    BYTE	axes,buttons;
    int		xfd = This->joyfd;

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);

    if (xfd==-1)
    	xfd = open(JOYDEV,O_RDONLY);
    lpDIDevCaps->dwFlags	= DIDC_ATTACHED;
    lpDIDevCaps->dwDevType	= DIDEVTYPE_JOYSTICK;
#ifdef JSIOCGAXES
    if (-1==ioctl(xfd,JSIOCGAXES,&axes))
    	axes = 2;
    lpDIDevCaps->dwAxes = axes;
#endif
#ifdef JSIOCGBUTTONS
    if (-1==ioctl(xfd,JSIOCGAXES,&buttons))
    	buttons = 2;
    lpDIDevCaps->dwButtons = buttons;
#endif
    if (xfd!=This->joyfd)
    	close(xfd);
    if (lpDIDevCaps->dwSize > sizeof(DIDEVCAPS_DX3)) {
        lpDIDevCaps->dwFFSamplePeriod = 0;
        lpDIDevCaps->dwFFMinTimeResolution = 0;
        lpDIDevCaps->dwFirmwareRevision = 0;
        lpDIDevCaps->dwHardwareRevision = 0;
        lpDIDevCaps->dwFFDriverVersion = 0;
    }

    if (TRACE_ON(dinput))
        _dump_DIDEVCAPS(lpDIDevCaps);

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(%p)\n",This);

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    joy_polldev(This);
    return DI_OK;
}

/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI JoystickAImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  ICOM_THIS(JoystickImpl,iface);
  DIDEVICEOBJECTINSTANCEA ddoi;
  int xfd = This->joyfd;

  TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
  if (TRACE_ON(dinput)) {
    TRACE("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    TRACE("\n");
  }

  /* Only the fields till dwFFMaxForce are relevant */
  ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEA, dwFFMaxForce);

  /* For the joystick, do as is done in the GetCapabilities function */
  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_AXIS)) {
    BYTE axes, i;

#ifdef JSIOCGAXES
    if (-1==ioctl(xfd,JSIOCGAXES,&axes))
      axes = 2;
#endif

    for (i = 0; i < axes; i++) {
      switch (i) {
      case 0:
	ddoi.guidType = GUID_XAxis;
	ddoi.dwOfs = DIJOFS_X;
	break;
      case 1:
	ddoi.guidType = GUID_YAxis;
	ddoi.dwOfs = DIJOFS_Y;
	break;
      case 2:
	ddoi.guidType = GUID_ZAxis;
	ddoi.dwOfs = DIJOFS_Z;
	break;
      case 3:
	ddoi.guidType = GUID_RxAxis;
	ddoi.dwOfs = DIJOFS_RX;
	break;
      case 4:
	ddoi.guidType = GUID_RyAxis;
	ddoi.dwOfs = DIJOFS_RY;
	break;
      case 5:
	ddoi.guidType = GUID_RzAxis;
	ddoi.dwOfs = DIJOFS_RZ;
	break;
      case 6:
	ddoi.guidType = GUID_Slider;
	ddoi.dwOfs = DIJOFS_SLIDER(0);
	break;
      case 7:
	ddoi.guidType = GUID_Slider;
	ddoi.dwOfs = DIJOFS_SLIDER(1);
	break;
      default:
	ddoi.guidType = GUID_Unknown;
	ddoi.dwOfs = DIJOFS_Z + (i - 2) * sizeof(LONG);
      }
      ddoi.dwType = DIDFT_MAKEINSTANCE((i + 1) << WINE_JOYSTICK_AXIS_BASE) | DIDFT_AXIS;
      sprintf(ddoi.tszName, "%d-Axis", i);
      _dump_OBJECTINSTANCEA(&ddoi);
      if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }
  }

  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_BUTTON)) {
    BYTE buttons, i;

#ifdef JSIOCGBUTTONS
    if (-1==ioctl(xfd,JSIOCGAXES,&buttons))
      buttons = 2;
#endif

    /* The DInput SDK says that GUID_Button is only for mouse buttons but well... */
    ddoi.guidType = GUID_Button;

    for (i = 0; i < buttons; i++) {
      ddoi.dwOfs = DIJOFS_BUTTON(i);
      ddoi.dwType = DIDFT_MAKEINSTANCE((i + 1) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_BUTTON;
      sprintf(ddoi.tszName, "%d-Button", i);
      _dump_OBJECTINSTANCEA(&ddoi);
      if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }
  }

  if (xfd!=This->joyfd)
    close(xfd);

  return DI_OK;
}

/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI JoystickWImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  ICOM_THIS(JoystickImpl,iface);

  device_enumobjects_AtoWcb_data data;

  data.lpCallBack = lpCallback;
  data.lpvRef = lpvRef;

  return JoystickAImpl_EnumObjects((LPDIRECTINPUTDEVICE8A) This, (LPDIENUMDEVICEOBJECTSCALLBACKA) DIEnumDevicesCallbackAtoW, (LPVOID) &data, dwFlags);
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickAImpl_GetProperty(
    LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid,
    LPDIPROPHEADER pdiph)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(pdiph);

    if (!HIWORD(rguid)) {
        switch ((DWORD)rguid) {
        case (DWORD) DIPROP_BUFFERSIZE: {
            LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;
            TRACE(" return buffersize = %d\n",This->queue_len);
            pd->dwData = This->queue_len;
            break;
        }
        case (DWORD) DIPROP_RANGE: {
            LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph;
            int obj = find_property(This, pdiph);
            /* The app is querying the current range of the axis
             * return the lMin and lMax values */
            if (obj >= 0) {
                pr->lMin = This->props[obj].lMin;
                pr->lMax = This->props[obj].lMax;
                TRACE("range(%ld, %ld) obj=%d\n", pr->lMin, pr->lMax, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD) DIPROP_DEADZONE: {
            LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(This, pdiph);
            if (obj >= 0) {
                pd->dwData = This->props[obj].lDeadZone;
                TRACE("deadzone(%ld) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD) DIPROP_SATURATION: {
            LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(This, pdiph);
            if (obj >= 0) {
                pd->dwData = This->props[obj].lSaturation;
                TRACE("saturation(%ld) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        default:
            FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
            break;
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     GetObjectInfo : get object info
  */
HRESULT WINAPI JoystickAImpl_GetObjectInfo(
        LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi,
        DWORD dwObj,
        DWORD dwHow)
{
    ICOM_THIS(JoystickImpl,iface);
    DIDEVICEOBJECTINSTANCEA didoiA;
    int i;

    TRACE("(%p,%p,%ld,0x%08lx(%s))\n",
          iface, pdidoi, dwObj, dwHow,
          dwHow == DIPH_BYOFFSET ? "DIPH_BYOFFSET" :
          dwHow == DIPH_BYID ? "DIPH_BYID" :
          dwHow == DIPH_BYUSAGE ? "DIPH_BYUSAGE" :
          "UNKNOWN");

    if (pdidoi == NULL) {
        WARN("invalid parameter: pdidoi = NULL\n");
        return DIERR_INVALIDPARAM;
    }

    if ((pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCEA)) &&
        (pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3A))) {
        WARN("invalid parameter: pdidoi->dwSize = %ld != %d or %d\n",
             pdidoi->dwSize, sizeof(DIDEVICEOBJECTINSTANCEA),
             sizeof(DIDEVICEOBJECTINSTANCE_DX3A));
        return DIERR_INVALIDPARAM;
    }

    ZeroMemory(&didoiA, sizeof(didoiA));
    didoiA.dwSize = pdidoi->dwSize;

    switch (dwHow) {
    case DIPH_BYOFFSET:
        for (i = 0; i < This->user_df->dwNumObjs; i++) {
            if (This->user_df->rgodf[i].dwOfs == dwObj) {
                if (This->user_df->rgodf[i].pguid)
                    didoiA.guidType = *This->user_df->rgodf[i].pguid;
                else
                    didoiA.guidType = GUID_NULL;
                didoiA.dwOfs = dwObj;
                didoiA.dwType = This->user_df->rgodf[i].dwType;
                didoiA.dwFlags = This->user_df->rgodf[i].dwFlags;
                strcpy(didoiA.tszName, "?????");
                CopyMemory(pdidoi, &didoiA, pdidoi->dwSize);
                return DI_OK;
            }
        }
        break;
    case DIPH_BYID:
        FIXME("dwHow = DIPH_BYID not implemented\n");
        break;
    case DIPH_BYUSAGE:
        FIXME("dwHow = DIPH_BYUSAGE not implemented\n");
        break;
    default:
        WARN("invalid parameter: dwHow = %08lx\n", dwHow);
        return DIERR_INVALIDPARAM;
    }

    CopyMemory(pdidoi, &didoiA, pdidoi->dwSize);

    return DI_OK;
}


static IDirectInputDevice8AVtbl JoystickAvt =
{
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	JoystickAImpl_Release,
	JoystickAImpl_GetCapabilities,
	JoystickAImpl_EnumObjects,
	JoystickAImpl_GetProperty,
	JoystickAImpl_SetProperty,
	JoystickAImpl_Acquire,
	JoystickAImpl_Unacquire,
	JoystickAImpl_GetDeviceState,
	JoystickAImpl_GetDeviceData,
	JoystickAImpl_SetDataFormat,
	JoystickAImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
	JoystickAImpl_GetObjectInfo,
	IDirectInputDevice2AImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2AImpl_EnumEffects,
	IDirectInputDevice2AImpl_GetEffectInfo,
	IDirectInputDevice2AImpl_GetForceFeedbackState,
	IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	IDirectInputDevice2AImpl_Escape,
	JoystickAImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
	IDirectInputDevice7AImpl_EnumEffectsInFile,
	IDirectInputDevice7AImpl_WriteEffectToFile,
	IDirectInputDevice8AImpl_BuildActionMap,
	IDirectInputDevice8AImpl_SetActionMap,
	IDirectInputDevice8AImpl_GetImageInfo
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(SysJoystickWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static IDirectInputDevice8WVtbl SysJoystickWvt =
{
	IDirectInputDevice2WImpl_QueryInterface,
	XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
	XCAST(Release)JoystickAImpl_Release,
	XCAST(GetCapabilities)JoystickAImpl_GetCapabilities,
	JoystickWImpl_EnumObjects,
	XCAST(GetProperty)JoystickAImpl_GetProperty,
	XCAST(SetProperty)JoystickAImpl_SetProperty,
	XCAST(Acquire)JoystickAImpl_Acquire,
	XCAST(Unacquire)JoystickAImpl_Unacquire,
	XCAST(GetDeviceState)JoystickAImpl_GetDeviceState,
	XCAST(GetDeviceData)JoystickAImpl_GetDeviceData,
	XCAST(SetDataFormat)JoystickAImpl_SetDataFormat,
	XCAST(SetEventNotification)JoystickAImpl_SetEventNotification,
	XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
	IDirectInputDevice2WImpl_GetObjectInfo,
	IDirectInputDevice2WImpl_GetDeviceInfo,
	XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
	XCAST(CreateEffect)IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2WImpl_EnumEffects,
	IDirectInputDevice2WImpl_GetEffectInfo,
	XCAST(GetForceFeedbackState)IDirectInputDevice2AImpl_GetForceFeedbackState,
	XCAST(SendForceFeedbackCommand)IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	XCAST(EnumCreatedEffectObjects)IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	XCAST(Escape)IDirectInputDevice2AImpl_Escape,
	XCAST(Poll)JoystickAImpl_Poll,
	XCAST(SendDeviceData)IDirectInputDevice2AImpl_SendDeviceData,
        IDirectInputDevice7WImpl_EnumEffectsInFile,
        IDirectInputDevice7WImpl_WriteEffectToFile,
        IDirectInputDevice8WImpl_BuildActionMap,
        IDirectInputDevice8WImpl_SetActionMap,
        IDirectInputDevice8WImpl_GetImageInfo
};
#undef XCAST

#endif  /* HAVE_LINUX_22_JOYSTICK_API */
