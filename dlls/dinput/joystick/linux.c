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

#include <stdio.h>
#include <string.h>
#include <time.h>
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
#ifdef HAVE_LINUX_JOYSTICK_H
# include <linux/joystick.h>
#endif
#define JOYDEV	"/dev/js0"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine joystick driver object instances */
#define WINE_JOYSTICK_AXIS_BASE   0
#define WINE_JOYSTICK_BUTTON_BASE 8

typedef struct JoystickImpl JoystickImpl;
static ICOM_VTABLE(IDirectInputDevice8A) JoystickAvt;
static ICOM_VTABLE(IDirectInputDevice8W) JoystickWvt;
struct JoystickImpl
{
        LPVOID                          lpVtbl;
        DWORD                           ref;
        GUID                            guid;

	/* The 'parent' DInput */
	IDirectInputImpl               *dinput;

	/* joystick private */
	int				joyfd;
	LPDIDATAFORMAT			df;
        HANDLE				hEvent;
	LONG				lMin,lMax,deadzone;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_head, queue_tail, queue_len;
	DIJOYSTATE			js;
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
      TRACE("Enumerating the linux Joystick device\n");

      /* Return joystick */
      lpddi->guidInstance	= GUID_Joystick;
      lpddi->guidProduct	= DInput_Wine_Joystick_GUID;
      /* we only support traditional joysticks for now */
      lpddi->dwDevType	= DIDEVTYPE_JOYSTICK |
	 		 (DIDEVTYPEJOYSTICK_TRADITIONAL<<8);
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
  JoystickImpl* newDevice;

  newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
  newDevice->lpVtbl = jvt;
  newDevice->ref = 1;
  newDevice->joyfd = -1;
  newDevice->lMin = -32768;
  newDevice->lMax = +32767;
  newDevice->dinput = dinput;
  memcpy(&(newDevice->guid),rguid,sizeof(*rguid));

  return newDevice;
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

      TRACE("Creating a Joystick device (%p)\n", *pdev);
      return DI_OK;
    } else
      return DIERR_NOINTERFACE;
  }

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

      TRACE("Creating a Joystick device (%p)\n", *pdev);
      return DI_OK;
    } else
      return DIERR_NOINTERFACE;
  }

  return DIERR_DEVICENOTREG;
}

static dinput_device joydev = {
  10,
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
	HeapFree(GetProcessHeap(), 0, This->df);

	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

/******************************************************************************
  *   SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  */
static HRESULT WINAPI JoystickAImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df
)
{
  ICOM_THIS(JoystickImpl,iface);
  int i;

  TRACE("(this=%p,%p)\n",This,df);

  TRACE("(df.dwSize=%ld)\n",df->dwSize);
  TRACE("(df.dwObjsize=%ld)\n",df->dwObjSize);
  TRACE("(df.dwFlags=0x%08lx)\n",df->dwFlags);
  TRACE("(df.dwDataSize=%ld)\n",df->dwDataSize);
  TRACE("(df.dwNumObjs=%ld)\n",df->dwNumObjs);

  for (i=0;i<df->dwNumObjs;i++) {
    TRACE("df.rgodf[%d].guid %s (%p)\n",i,debugstr_guid(df->rgodf[i].pguid), df->rgodf[i].pguid);
    TRACE("df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
    TRACE("dwType 0x%02x,dwInstance %d\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
    TRACE("df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
  }

  /* Store the new data format */
  This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
  memcpy(This->df, df, df->dwSize);
  This->df->rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
  memcpy(This->df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);

  return 0;
}

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1)
    	return 0;
    This->joyfd=open(JOYDEV,O_RDONLY);
    if (This->joyfd==-1)
    	return DIERR_NOTFOUND;
    return 0;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(JoystickImpl,iface);

    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1) {
  	close(This->joyfd);
	This->joyfd = -1;
    }
    return DI_OK;
}

#define map_axis(val) ((val+32768)*(This->lMax-This->lMin)/65536+This->lMin)

static void joy_polldev(JoystickImpl *This) {
    struct timeval tv;
    fd_set	readfds;
    struct	js_event jse;

    if (This->joyfd==-1)
	return;
    while (1) {
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);FD_SET(This->joyfd,&readfds);
	if (1>select(This->joyfd+1,&readfds,NULL,NULL,&tv))
	    return;
	/* we have one event, so we can read */
	if (sizeof(jse)!=read(This->joyfd,&jse,sizeof(jse))) {
	    return;
	}
	TRACE("js_event: type 0x%x, number %d, value %d\n",jse.type,jse.number,jse.value);
	if (jse.type & JS_EVENT_BUTTON) {
	    GEN_EVENT(DIJOFS_BUTTON(jse.number),jse.value?0x80:0x00,jse.time,(This->dinput->evsequence)++);
	    This->js.rgbButtons[jse.number] = jse.value?0x80:0x00;
	}
	if (jse.type & JS_EVENT_AXIS) {
	    switch (jse.number) {
	    case 0:
		GEN_EVENT(jse.number*4,jse.value,jse.time,(This->dinput->evsequence)++);
		This->js.lX = map_axis(jse.value);
		break;
	    case 1:
		GEN_EVENT(jse.number*4,jse.value,jse.time,(This->dinput->evsequence)++);
		This->js.lY = map_axis(jse.value);
		break;
	    case 2:
		GEN_EVENT(jse.number*4,jse.value,jse.time,(This->dinput->evsequence)++);
		This->js.lZ = map_axis(jse.value);
		break;
	    default:
		FIXME("more than 3 axes (%d) not handled!\n",jse.number);
		break;
	    }
	}
    }
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE8A iface,DWORD len,LPVOID ptr
) {
    ICOM_THIS(JoystickImpl,iface);

    joy_polldev(This);
    TRACE("(this=%p,0x%08lx,%p)\n",This,len,ptr);
    if (len != sizeof(DIJOYSTATE)) {
    	FIXME("len %ld is not sizeof(DIJOYSTATE), unsupported format.\n",len);
    }
    memcpy(ptr,&(This->js),len);
    This->queue_head = 0;
    This->queue_tail = 0;
    return 0;
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
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  ICOM_THIS(JoystickImpl,iface);

  FIXME("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  FIXME("ph.dwSize = %ld, ph.dwHeaderSize =%ld, ph.dwObj = %ld, ph.dwHow= %ld\n",ph->dwSize, ph->dwHeaderSize,ph->dwObj,ph->dwHow);

  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case (DWORD) DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

      FIXME("buffersize = %ld\n",pd->dwData);
      break;
    }
    case (DWORD)DIPROP_RANGE: {
      LPCDIPROPRANGE	pr = (LPCDIPROPRANGE)ph;

      FIXME("proprange(%ld,%ld)\n",pr->lMin,pr->lMax);
      This->lMin = pr->lMin;
      This->lMax = pr->lMax;
      break;
    }
    case (DWORD)DIPROP_DEADZONE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

      FIXME("deadzone(%ld)\n",pd->dwData);
      This->deadzone = pd->dwData;
      break;
    }
    default:
      FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
      break;
    }
  }
  return 0;
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
    return DI_OK;
}
static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE8A iface) {
    ICOM_THIS(JoystickImpl,iface);
    TRACE("(),stub!\n");

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
      default:
	ddoi.guidType = GUID_Unknown;
	ddoi.dwOfs = DIJOFS_Z + (i - 2) * sizeof(LONG);
      }
      ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << i) << WINE_JOYSTICK_AXIS_BASE) | DIDFT_ABSAXIS;
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
      ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << i) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
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
static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
  ICOM_THIS(JoystickImpl,iface);

  TRACE("(this=%p,%s,%p): stub!\n",
	iface, debugstr_guid(rguid), pdiph);

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

      if ((pdiph->dwHow == DIPH_BYID) &&
	  (pdiph->dwObj & DIDFT_ABSAXIS)) {
	/* The app is querying the current range of the axis : return the lMin and lMax values */
	pr->lMin = This->lMin;
	pr->lMax = This->lMax;
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

static ICOM_VTABLE(IDirectInputDevice8A) JoystickAvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	IDirectInputDevice2AImpl_GetObjectInfo,
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

static ICOM_VTABLE(IDirectInputDevice8W) SysJoystickWvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
