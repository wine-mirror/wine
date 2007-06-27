/*		DirectInput Joystick device
 *
 * Copyright 1998,2000 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2005 Daniel Remenak
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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
#include <sys/fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#ifdef HAVE_LINUX_INPUT_H
# include <linux/input.h>
# undef SW_MAX
# if defined(EVIOCGBIT) && defined(EV_ABS) && defined(BTN_PINKIE)
#  define HAVE_CORRECT_LINUXINPUT_H
# endif
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

#ifdef HAVE_CORRECT_LINUXINPUT_H

#define EVDEVPREFIX	"/dev/input/event"

/* Wine joystick driver object instances */
#define WINE_JOYSTICK_MAX_AXES    8
#define WINE_JOYSTICK_MAX_POVS    4
#define WINE_JOYSTICK_MAX_BUTTONS 128

typedef struct EffectListItem EffectListItem;
struct EffectListItem
{
        LPDIRECTINPUTEFFECT ref;
	struct EffectListItem* next;
};

/* implemented in effect_linuxinput.c */
HRESULT linuxinput_create_effect(int* fd, REFGUID rguid, LPDIRECTINPUTEFFECT* peff);
HRESULT linuxinput_get_info_A(int fd, REFGUID rguid, LPDIEFFECTINFOA info);
HRESULT linuxinput_get_info_W(int fd, REFGUID rguid, LPDIEFFECTINFOW info);

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;

struct JoyDev {
	char *device;
	char *name;
	GUID guid;

	int has_ff;
        int num_effects;

	/* data returned by EVIOCGBIT for caps, EV_ABS, EV_KEY, and EV_FF */
	BYTE				evbits[(EV_MAX+7)/8];
	BYTE				absbits[(ABS_MAX+7)/8];
	BYTE				keybits[(KEY_MAX+7)/8];
	BYTE				ffbits[(FF_MAX+7)/8];	

#define AXIS_ABS     0
#define AXIS_ABSMIN  1
#define AXIS_ABSMAX  2
#define AXIS_ABSFUZZ 3
#define AXIS_ABSFLAT 4

	/* data returned by the EVIOCGABS() ioctl */
	int				axes[ABS_MAX][5];
};

struct ObjProps
{
	/* what we have */
	LONG				havemax;
	LONG				havemin;
	/* what range and deadzone the game wants */
	LONG				wantmin;
	LONG				wantmax;
	LONG				deadzone;
};

struct JoystickImpl
{
        struct IDirectInputDevice2AImpl base;

        struct JoyDev                  *joydev;

	/* joystick private */
	int				joyfd;

	DIJOYSTATE2			js;

	struct ObjProps                 props[ABS_MAX];

	int                             axes[ABS_MAX];

	/* LUT for KEY_ to offset in rgbButtons */
	BYTE				buttons[KEY_MAX];

	DWORD                           numAxes;
	DWORD                           numPOVs;
	DWORD                           numButtons;

	/* Force feedback variables */
	EffectListItem*			top_effect;
	int				ff_state;
};

static void fake_current_js_state(JoystickImpl *ji);
static DWORD map_pov(int event_value, int is_x);
static void find_joydevs(void);

/* This GUID is slightly different from the linux joystick one. Take note. */
static const GUID DInput_Wine_Joystick_Base_GUID = { /* 9e573eda-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573eda,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

#define test_bit(arr,bit) (((BYTE*)(arr))[(bit)>>3]&(1<<((bit)&7)))

#define MAX_JOYDEV 64

static int have_joydevs = -1;
static struct JoyDev *joydevs = NULL;

static void find_joydevs(void)
{
  int i;

  if (have_joydevs!=-1) {
    return;
  }

  have_joydevs = 0;

  for (i=0;i<MAX_JOYDEV;i++) {
    char	buf[MAX_PATH];
    struct JoyDev joydev = {0};
    int fd;
    int no_ff_check = 0;
    int j;

    snprintf(buf,MAX_PATH,EVDEVPREFIX"%d",i);
    buf[MAX_PATH-1] = 0;

    if ((fd=open(buf, O_RDWR))==-1) {
      fd = open(buf, O_RDONLY);
      no_ff_check = 1;
    }

    if (fd!=-1) {
      if ((-1==ioctl(fd,EVIOCGBIT(0,sizeof(joydev.evbits)),joydev.evbits))) {
        perror("EVIOCGBIT 0");
        close(fd);
        continue; 
      }
      if (-1==ioctl(fd,EVIOCGBIT(EV_ABS,sizeof(joydev.absbits)),joydev.absbits)) {
        perror("EVIOCGBIT EV_ABS");
        close(fd);
        continue;
      }
      if (-1==ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(joydev.keybits)),joydev.keybits)) {
        perror("EVIOCGBIT EV_KEY");
        close(fd);
        continue;
      }
      /* A true joystick has at least axis X and Y, and at least 1
       * button. copied from linux/drivers/input/joydev.c */
      if (test_bit(joydev.absbits,ABS_X) && test_bit(joydev.absbits,ABS_Y) &&
          (   test_bit(joydev.keybits,BTN_TRIGGER)	||
              test_bit(joydev.keybits,BTN_A) 	||
              test_bit(joydev.keybits,BTN_1)
          )
         ) {

        joydev.device = strdup(buf);

        if (-1!=ioctl(fd, EVIOCGNAME(MAX_PATH-1), buf)) {
          buf[MAX_PATH-1] = 0;
          joydev.name = strdup(buf);
        } else {
          joydev.name = joydev.device;
        }

	joydev.guid = DInput_Wine_Joystick_Base_GUID;
	joydev.guid.Data3 += have_joydevs;

        TRACE("Found a joystick on %s: %s (%s)\n", 
            joydev.device, joydev.name, 
            debugstr_guid(&joydev.guid)
            );

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
        if (!no_ff_check &&
            test_bit(joydev.evbits, EV_FF) &&
            ioctl(fd, EVIOCGBIT(EV_FF, sizeof(joydev.ffbits)), joydev.ffbits) != -1 &&
            ioctl(fd, EVIOCGEFFECTS, &joydev.num_effects) != -1 &&
            joydev.num_effects > 0)
        {
	    TRACE(" ... with force feedback\n");
	    joydev.has_ff = 1;
        }
#endif

	for (j=0;j<ABS_MAX;j++) {
	  if (test_bit(joydev.absbits,j)) {
	    if (-1!=ioctl(fd,EVIOCGABS(j),&(joydev.axes[j]))) {
	      TRACE(" ... with axis %d: cur=%d, min=%d, max=%d, fuzz=%d, flat=%d\n",
		  j,
		  joydev.axes[j][AXIS_ABS],
		  joydev.axes[j][AXIS_ABSMIN],
		  joydev.axes[j][AXIS_ABSMAX],
		  joydev.axes[j][AXIS_ABSFUZZ],
		  joydev.axes[j][AXIS_ABSFLAT]
		  );
	    }
	  }
	}

	if (have_joydevs==0) {
	  joydevs = HeapAlloc(GetProcessHeap(), 0, sizeof(struct JoyDev));
	} else {
	  HeapReAlloc(GetProcessHeap(), 0, joydevs, (1+have_joydevs) * sizeof(struct JoyDev));
	}
	memcpy(joydevs+have_joydevs, &joydev, sizeof(struct JoyDev));
        have_joydevs++;
      }

      close(fd);
    }
  }
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
  find_joydevs();

  if (id >= have_joydevs) {
    return FALSE;
  }
 
  if (!((dwDevType == 0) ||
        ((dwDevType == DIDEVTYPE_JOYSTICK) && (version < 0x0800)) ||
        (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))))
    return FALSE;

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;
#endif

  if (!(dwFlags & DIEDFL_FORCEFEEDBACK) || joydevs[id].has_ff) {
    lpddi->guidInstance	= joydevs[id].guid;
    lpddi->guidProduct	= DInput_Wine_Joystick_Base_GUID;

    lpddi->guidFFDriver = GUID_NULL;
    if (version >= 0x0800)
      lpddi->dwDevType    = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
      lpddi->dwDevType    = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    strcpy(lpddi->tszInstanceName, joydevs[id].name);
    strcpy(lpddi->tszProductName, joydevs[id].device);
    return TRUE;
  }
  return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
  find_joydevs();

  if (id >= have_joydevs) {
    return FALSE;
  }

  if (!((dwDevType == 0) ||
        ((dwDevType == DIDEVTYPE_JOYSTICK) && (version < 0x0800)) ||
        (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))))
    return FALSE;

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;
#endif

  if (!(dwFlags & DIEDFL_FORCEFEEDBACK) || joydevs[id].has_ff) {
    lpddi->guidInstance	= joydevs[id].guid;
    lpddi->guidProduct	= DInput_Wine_Joystick_Base_GUID;

    lpddi->guidFFDriver = GUID_NULL;
    if (version >= 0x0800)
      lpddi->dwDevType    = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
      lpddi->dwDevType    = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    MultiByteToWideChar(CP_ACP, 0, joydevs[id].name, -1, lpddi->tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, joydevs[id].device, -1, lpddi->tszProductName, MAX_PATH);
    return TRUE;
  }
  return FALSE;
}

static JoystickImpl *alloc_device(REFGUID rguid, const void *jvt, IDirectInputImpl *dinput, struct JoyDev *joydev)
{
    JoystickImpl* newDevice;
    LPDIDATAFORMAT df = NULL;
    int i, idx = 0;

    newDevice = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(JoystickImpl));
    if (!newDevice) return NULL;

  newDevice->base.lpVtbl = jvt;
  newDevice->base.ref = 1;
  memcpy(&newDevice->base.guid, rguid, sizeof(*rguid));
  InitializeCriticalSection(&newDevice->base.crit);
  newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->base.crit");
  newDevice->joyfd = -1;
  newDevice->base.dinput = dinput;
  newDevice->joydev = joydev;
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
  newDevice->ff_state = FF_STATUS_STOPPED;
#endif

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto failed;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto failed;

    /* Supported Axis & POVs should map 1-to-1 */
    for (i = 0; i < WINE_JOYSTICK_MAX_AXES; i++)
    {
        if (!test_bit(newDevice->joydev->absbits, i)) {
            newDevice->axes[i] = -1;
            continue;
        }

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i], df->dwObjSize);
        newDevice->axes[i] = idx;
        newDevice->props[idx].havemin = newDevice->joydev->axes[i][AXIS_ABSMIN];
        newDevice->props[idx].havemax = newDevice->joydev->axes[i][AXIS_ABSMAX];
        newDevice->props[idx].wantmin = 0;
        newDevice->props[idx].wantmax = 0xffff;
        newDevice->props[idx].deadzone = 0;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->numAxes++) | DIDFT_ABSAXIS;
    }

    for (i = 0; i < WINE_JOYSTICK_MAX_POVS; i++)
    {
        if (!test_bit(newDevice->joydev->absbits, ABS_HAT0X + i * 2) ||
            !test_bit(newDevice->joydev->absbits, ABS_HAT0Y + i * 2)) {
            newDevice->axes[ABS_HAT0X + i * 2] = newDevice->axes[ABS_HAT0Y + i * 2] = -1;
            continue;
        }

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + WINE_JOYSTICK_MAX_AXES], df->dwObjSize);
        newDevice->axes[ABS_HAT0X + i * 2] = newDevice->axes[ABS_HAT0Y + i * 2] = i;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->numPOVs++) | DIDFT_POV;
    }

    /* Buttons can be anywhere, so check all */
    for (i = 0; i < KEY_MAX && newDevice->numButtons < WINE_JOYSTICK_MAX_BUTTONS; i++)
    {
        if (!test_bit(newDevice->joydev->keybits, i)) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[newDevice->numButtons + WINE_JOYSTICK_MAX_AXES + WINE_JOYSTICK_MAX_POVS], df->dwObjSize);
	newDevice->buttons[i] = 0x80 | newDevice->numButtons;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->numButtons++) | DIDFT_PSHBUTTON;
    }
    df->dwNumObjs = idx;

    fake_current_js_state(newDevice);

    newDevice->base.data_format.wine_df = df;
    IDirectInput_AddRef((LPDIRECTINPUTDEVICE8A)newDevice->base.dinput);
    return newDevice;

failed:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    HeapFree(GetProcessHeap(), 0, newDevice);
    return NULL;
}

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  int i;

  find_joydevs();

  for (i=0; i<have_joydevs; i++) {
    if (IsEqualGUID(&GUID_Joystick,rguid) ||
        IsEqualGUID(&joydevs[i].guid,rguid)
        ) {
      if ((riid == NULL) ||
          IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
          IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
          IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
          IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
        *pdev = (IDirectInputDeviceA*) alloc_device(rguid, &JoystickAvt, dinput, &joydevs[i]);
        TRACE("Creating a Joystick device (%p)\n", *pdev);
        if (*pdev==0) {
          ERR("out of memory\n");
          return DIERR_OUTOFMEMORY;
        }
        return DI_OK;
      } else {
        return DIERR_NOINTERFACE;
      }
    }
  }

  return DIERR_DEVICENOTREG;
}


static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
  int i;

  find_joydevs();

  for (i=0; i<have_joydevs; i++) {
    if (IsEqualGUID(&GUID_Joystick,rguid) ||
        IsEqualGUID(&joydevs[i].guid,rguid)
        ) {
      if ((riid == NULL) ||
          IsEqualGUID(&IID_IDirectInputDeviceW,riid) ||
          IsEqualGUID(&IID_IDirectInputDevice2W,riid) ||
          IsEqualGUID(&IID_IDirectInputDevice7W,riid) ||
          IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
        *pdev = (IDirectInputDeviceW*) alloc_device(rguid, &JoystickWvt, dinput, &joydevs[i]);
        TRACE("Creating a Joystick device (%p)\n", *pdev);
        if (*pdev==0) {
          ERR("out of memory\n");
          return DIERR_OUTOFMEMORY;
        }
        return DI_OK;
      } else {
        return DIERR_NOINTERFACE;
      }
    }
  }

  return DIERR_DEVICENOTREG;
}

const struct dinput_device joystick_linuxinput_device = {
  "Wine Linux-input joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_deviceA,
  joydev_create_deviceW
};

/******************************************************************************
 *	Joystick
 */
static ULONG WINAPI JoystickAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
	JoystickImpl *This = (JoystickImpl *)iface;
	ULONG ref;

	ref = InterlockedDecrement(&This->base.ref);
	if (ref)
		return ref;

        IDirectInputDevice_Unacquire(iface);

	/* Reset the FF state, free all effects, etc */
	IDirectInputDevice8_SendForceFeedbackCommand(iface, DISFFC_RESET);

	/* Free the data queue */
	HeapFree(GetProcessHeap(), 0, This->base.data_queue);

        /* release the data transform filter */
        HeapFree(GetProcessHeap(), 0, This->base.data_format.wine_df->rgodf);
        HeapFree(GetProcessHeap(), 0, This->base.data_format.wine_df);
        release_DataFormat(&This->base.data_format);

        IDirectInput_Release((LPDIRECTINPUTDEVICE8A)This->base.dinput);
        This->base.crit.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->base.crit);
        
        HeapFree(GetProcessHeap(),0,This);
        return 0;
}

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    HRESULT res;

    TRACE("(this=%p)\n",This);

    res = IDirectInputDevice2AImpl_Acquire(iface);
    if (res==DI_OK) {
      if (-1==(This->joyfd=open(This->joydev->device,O_RDWR))) {
        if (-1==(This->joyfd=open(This->joydev->device,O_RDONLY))) {
          /* Couldn't open the device at all */
          perror(This->joydev->device);
          IDirectInputDevice2AImpl_Unacquire(iface);
          return DIERR_NOTFOUND;
        } else {
          /* Couldn't open in r/w but opened in read-only. */
          WARN("Could not open %s in read-write mode.  Force feedback will be disabled.\n", This->joydev->device);
        }
      }
    }

    return res;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    HRESULT res;

    TRACE("(this=%p)\n",This);
    res = IDirectInputDevice2AImpl_Unacquire(iface);
    if (res==DI_OK && This->joyfd!=-1) {
      close(This->joyfd);
      This->joyfd = -1;
    }
    return res;
}

/*
 * This maps the read value (from the input event) to a value in the
 * 'wanted' range. It also autodetects the possible range of the axis and
 * adapts values accordingly.
 */
static int
map_axis(JoystickImpl* This, int axis, int val) {
    int hmax = This->props[axis].havemax;
    int hmin = This->props[axis].havemin;
    int	wmin = This->props[axis].wantmin;
    int	wmax = This->props[axis].wantmax;
    int ret;

    /* map the value from the hmin-hmax range into the wmin-wmax range */
    ret = MulDiv( val - hmin, wmax - wmin, hmax - hmin ) + wmin;

    TRACE("hmin=%d hmax=%d wmin=%d wmax=%d val=%d ret=%d\n", hmin, hmax, wmin, wmax, val, ret);

#if 0
    /* deadzone doesn't work comfortably enough right now. needs more testing*/
    if ((ret > -deadz/2 ) && (ret < deadz/2)) {
        FIXME("%d in deadzone, return mid.\n",val);
	return (wmax-wmin)/2+wmin;
    }
#endif
    return ret;
}

/* 
 * set the current state of the js device as it would be with the middle
 * values on the axes
 */
static void fake_current_js_state(JoystickImpl *ji)
{
	int i;
	/* center the axes */
	ji->js.lX           = ji->axes[ABS_X]!=-1        ? map_axis(ji, ji->axes[ABS_X],        ji->joydev->axes[ABS_X       ][AXIS_ABS]) : 0;
	ji->js.lY           = ji->axes[ABS_Y]!=-1        ? map_axis(ji, ji->axes[ABS_Y],        ji->joydev->axes[ABS_Y       ][AXIS_ABS]) : 0;
	ji->js.lZ           = ji->axes[ABS_Z]!=-1        ? map_axis(ji, ji->axes[ABS_Z],        ji->joydev->axes[ABS_Z       ][AXIS_ABS]) : 0;
	ji->js.lRx          = ji->axes[ABS_RX]!=-1       ? map_axis(ji, ji->axes[ABS_RX],       ji->joydev->axes[ABS_RX      ][AXIS_ABS]) : 0;
	ji->js.lRy          = ji->axes[ABS_RY]!=-1       ? map_axis(ji, ji->axes[ABS_RY],       ji->joydev->axes[ABS_RY      ][AXIS_ABS]) : 0;
	ji->js.lRz          = ji->axes[ABS_RZ]!=-1       ? map_axis(ji, ji->axes[ABS_RZ],       ji->joydev->axes[ABS_RZ      ][AXIS_ABS]) : 0;
	ji->js.rglSlider[0] = ji->axes[ABS_THROTTLE]!=-1 ? map_axis(ji, ji->axes[ABS_THROTTLE], ji->joydev->axes[ABS_THROTTLE][AXIS_ABS]) : 0;
	ji->js.rglSlider[1] = ji->axes[ABS_RUDDER]!=-1   ? map_axis(ji, ji->axes[ABS_RUDDER],   ji->joydev->axes[ABS_RUDDER  ][AXIS_ABS]) : 0;
	/* POV center is -1 */
	for (i=0; i<4; i++) {
		ji->js.rgdwPOV[i] = -1;
	}
}

/*
 * Maps an event value to a DX "clock" position:
 *           0
 * 27000    -1 9000
 *       18000
 */
static DWORD map_pov(int event_value, int is_x) 
{
	DWORD ret = -1;
	if (is_x) {
		if (event_value<0) {
			ret = 27000;
		} else if (event_value>0) {
			ret = 9000;
		}
	} else {
		if (event_value<0) {
			ret = 0;
		} else if (event_value>0) {
			ret = 18000;
		}
	}
	return ret;
}

/* convert wine format offset to user format object index */
static void joy_polldev(JoystickImpl *This)
{
    struct pollfd plfd;
    struct input_event ie;

    if (This->joyfd==-1)
	return;

    while (1)
    {
        LONG value = 0;
        int inst_id = -1;

	plfd.fd = This->joyfd;
	plfd.events = POLLIN;

	if (poll(&plfd,1,0) != 1)
	    return;

	/* we have one event, so we can read */
	if (sizeof(ie)!=read(This->joyfd,&ie,sizeof(ie)))
	    return;

	TRACE("input_event: type %d, code %d, value %d\n",ie.type,ie.code,ie.value);
	switch (ie.type) {
	case EV_KEY:	/* button */
        {
            int btn = This->buttons[ie.code];

            TRACE("(%p) %d -> %d\n", This, ie.code, btn);
            if (btn & 0x80)
            {
                btn &= 0x7F;
                inst_id = DIDFT_MAKEINSTANCE(btn) | DIDFT_PSHBUTTON;
                This->js.rgbButtons[btn] = value = ie.value ? 0x80 : 0x00;
            }
            break;
        }
	case EV_ABS:
        {
            int axis = This->axes[ie.code];
            if (axis==-1) {
                break;
            }
            inst_id = DIDFT_MAKEINSTANCE(axis) | (ie.code < ABS_HAT0X ? DIDFT_ABSAXIS : DIDFT_POV);
            value = map_axis(This, axis, ie.value);

	    switch (ie.code) {
            case ABS_X:         This->js.lX  = value; break;
            case ABS_Y:         This->js.lY  = value; break;
            case ABS_Z:         This->js.lZ  = value; break;
            case ABS_RX:        This->js.lRx = value; break;
            case ABS_RY:        This->js.lRy = value; break;
            case ABS_RZ:        This->js.lRz = value; break;
            case ABS_THROTTLE:  This->js.rglSlider[0] = value; break;
            case ABS_RUDDER:    This->js.rglSlider[1] = value; break;
	    case ABS_HAT0X:
	    case ABS_HAT0Y:
                This->js.rgdwPOV[0] = value = map_pov(ie.value, ie.code==ABS_HAT0X);
                break;
	    case ABS_HAT1X:
	    case ABS_HAT1Y:
                This->js.rgdwPOV[1] = value = map_pov(ie.value, ie.code==ABS_HAT1X);
                break;
	    case ABS_HAT2X:
	    case ABS_HAT2Y:
                This->js.rgdwPOV[2] = value  = map_pov(ie.value, ie.code==ABS_HAT2X);
                break;
	    case ABS_HAT3X:
	    case ABS_HAT3Y:
                This->js.rgdwPOV[3] = value  = map_pov(ie.value, ie.code==ABS_HAT3X);
                break;
	    default:
		FIXME("unhandled joystick axis event (code %d, value %d)\n",ie.code,ie.value);
		break;
	    }
	    break;
        }
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
	case EV_FF_STATUS:
	    This->ff_state = ie.value;
	    break;
#endif
#ifdef EV_SYN
	case EV_SYN:
	    /* there is nothing to do */
	    break;
#endif
	default:
	    FIXME("joystick cannot handle type %d event (code %d)\n",ie.type,ie.code);
	    break;
	}
        if (inst_id >= 0)
            queue_event((LPDIRECTINPUTDEVICE8A)This,
                        id_to_offset(&This->base.data_format, inst_id),
                        value, ie.time.tv_usec, This->base.dinput->evsequence++);
    }
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE8A iface,DWORD len,LPVOID ptr
) {
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p,0x%08x,%p)\n", This, len, ptr);

    if (This->joyfd==-1) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    joy_polldev(This);

    /* convert and copy data to user supplied buffer */
    fill_DataFormat(ptr, &This->js, &This->base.data_format);

    return DI_OK;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  JoystickImpl *This = (JoystickImpl *)iface;

  if (!ph) {
    WARN("invalid argument\n");
    return DIERR_INVALIDPARAM;
  }

  TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  TRACE("ph.dwSize = %d, ph.dwHeaderSize =%d, ph.dwObj = %d, ph.dwHow= %d\n",
        ph->dwSize, ph->dwHeaderSize, ph->dwObj, ph->dwHow);

  if (!HIWORD(rguid)) {
    switch (LOWORD(rguid)) {
    case (DWORD)DIPROP_RANGE: {
      LPCDIPROPRANGE pr = (LPCDIPROPRANGE)ph;

      if (ph->dwHow == DIPH_DEVICE) {
        int i;
        TRACE("proprange(%d,%d) all\n", pr->lMin, pr->lMax);
        for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++) {
          This->props[i].wantmin = pr->lMin;
          This->props[i].wantmax = pr->lMax;
        }
      } else {
        int obj = find_property(&This->base.data_format, ph);

        TRACE("proprange(%d,%d) obj=%d\n", pr->lMin, pr->lMax, obj);
        if (obj >= 0) {
          This->props[obj].wantmin = pr->lMin;
          This->props[obj].wantmax = pr->lMax;
        }
      }
      fake_current_js_state(This);
      break;
    }
    case (DWORD)DIPROP_DEADZONE: {
      LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;
      if (ph->dwHow == DIPH_DEVICE) {
        int i;
        TRACE("deadzone(%d) all\n", pd->dwData);
        for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++) {
          This->props[i].deadzone = pd->dwData;
        }
      } else {
        int obj = find_property(&This->base.data_format, ph);

        TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
        if (obj >= 0) {
          This->props[obj].deadzone = pd->dwData;
        }
      }
      fake_current_js_state(This);
      break;
    }
    case (DWORD)DIPROP_CALIBRATIONMODE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
      FIXME("DIPROP_CALIBRATIONMODE(%d)\n", pd->dwData);
      break;
    }
    default:
      return IDirectInputDevice2AImpl_SetProperty(iface, rguid, ph);
    }
  }
  return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);

    if (!lpDIDevCaps) {
	WARN("invalid pointer\n");
	return E_POINTER;
    }

    if (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) {
        WARN("invalid argument\n");
        return DIERR_INVALIDPARAM;
    }

    lpDIDevCaps->dwFlags	= DIDC_ATTACHED;
    if (This->base.dinput->dwVersion >= 0x0800)
        lpDIDevCaps->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        lpDIDevCaps->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    if (This->joydev->has_ff) 
	 lpDIDevCaps->dwFlags |= DIDC_FORCEFEEDBACK;

    lpDIDevCaps->dwAxes = This->numAxes;
    lpDIDevCaps->dwButtons = This->numButtons;
    lpDIDevCaps->dwPOVs = This->numPOVs;

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE8A iface) {
    JoystickImpl *This = (JoystickImpl *)iface;
    TRACE("(%p)\n",This);

    if (This->joyfd==-1) {
      return DIERR_NOTACQUIRED;
    }

    joy_polldev(This);
    return DI_OK;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (HIWORD(rguid)) return DI_OK;

    switch (LOWORD(rguid)) {
    case (DWORD) DIPROP_RANGE:
    {
        LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph;
        int obj = find_property(&This->base.data_format, pdiph);

        if (obj < 0) return DIERR_OBJECTNOTFOUND;

        pr->lMin = This->props[obj].wantmin;
        pr->lMax = This->props[obj].wantmax;
	TRACE("range(%d, %d) obj=%d\n", pr->lMin, pr->lMax, obj);
        break;
    }
    case (DWORD) DIPROP_DEADZONE:
    {
        LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
        int obj = find_property(&This->base.data_format, pdiph);

        if (obj < 0) return DIERR_OBJECTNOTFOUND;

        pd->dwData = This->props[obj].deadzone;
        TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
        break;
    }

    default:
        return IDirectInputDevice2AImpl_GetProperty(iface, rguid, pdiph);
    }

    return DI_OK;
}

/******************************************************************************
  *     GetObjectInfo : get information about a device object such as a button
  *                     or axis
  */
static HRESULT WINAPI JoystickWImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
        LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow)
{
    static const WCHAR axisW[] = {'A','x','i','s',' ','%','d',0};
    static const WCHAR povW[] = {'P','O','V',' ','%','d',0};
    static const WCHAR buttonW[] = {'B','u','t','t','o','n',' ','%','d',0};
    HRESULT res;

    res = IDirectInputDevice2WImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    if      (pdidoi->dwType & DIDFT_AXIS)
        sprintfW(pdidoi->tszName, axisW, DIDFT_GETINSTANCE(pdidoi->dwType));
    else if (pdidoi->dwType & DIDFT_POV)
        sprintfW(pdidoi->tszName, povW, DIDFT_GETINSTANCE(pdidoi->dwType));
    else if (pdidoi->dwType & DIDFT_BUTTON)
        sprintfW(pdidoi->tszName, buttonW, DIDFT_GETINSTANCE(pdidoi->dwType));

    _dump_OBJECTINSTANCEW(pdidoi);
    return res;
}

static HRESULT WINAPI JoystickAImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow)
{
    HRESULT res;
    DIDEVICEOBJECTINSTANCEW didoiW;
    DWORD dwSize = pdidoi->dwSize;

    didoiW.dwSize = sizeof(didoiW);
    res = JoystickWImpl_GetObjectInfo((LPDIRECTINPUTDEVICE8W)iface, &didoiW, dwObj, dwHow);
    if (res != DI_OK) return res;

    memset(pdidoi, 0, pdidoi->dwSize);
    memcpy(pdidoi, &didoiW, FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, tszName));
    pdidoi->dwSize = dwSize;
    WideCharToMultiByte(CP_ACP, 0, didoiW.tszName, -1, pdidoi->tszName,
                        sizeof(pdidoi->tszName), NULL, NULL);

    return res;
}

/****************************************************************************** 
  *	CreateEffect - Create a new FF effect with the specified params
  */
static HRESULT WINAPI JoystickAImpl_CreateEffect(LPDIRECTINPUTDEVICE8A iface,
						 REFGUID rguid,
						 LPCDIEFFECT lpeff,
						 LPDIRECTINPUTEFFECT *ppdef,
						 LPUNKNOWN pUnkOuter)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    EffectListItem* new = NULL;
    HRESULT retval = DI_OK;
#endif

    JoystickImpl* This = (JoystickImpl*)iface;
    TRACE("(this=%p,%p,%p,%p,%p)\n", This, rguid, lpeff, ppdef, pUnkOuter);

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
    TRACE("not available (compiled w/o ff support)\n");
    *ppdef = NULL;
    return DI_OK;
#else

    new = HeapAlloc(GetProcessHeap(), 0, sizeof(EffectListItem));
    new->next = This->top_effect;
    This->top_effect = new;

    retval = linuxinput_create_effect(&(This->joyfd), rguid, &(new->ref));
    if (retval != DI_OK)
	return retval;

    if (lpeff != NULL)
	retval = IDirectInputEffect_SetParameters(new->ref, lpeff, 0);
    if (retval != DI_OK && retval != DI_DOWNLOADSKIPPED)
	return retval;

    *ppdef = new->ref;

    if (pUnkOuter != NULL)
	FIXME("Interface aggregation not implemented.\n");

    return DI_OK;

#endif /* HAVE_STRUCT_FF_EFFECT_DIRECTION */
} 

/*******************************************************************************
 *	EnumEffects - Enumerate available FF effects
 */
static HRESULT WINAPI JoystickAImpl_EnumEffects(LPDIRECTINPUTDEVICE8A iface,
						LPDIENUMEFFECTSCALLBACKA lpCallback,
						LPVOID pvRef,
						DWORD dwEffType)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    DIEFFECTINFOA dei; /* feif */
    DWORD type = DIEFT_GETTYPE(dwEffType);
    JoystickImpl* This = (JoystickImpl*)iface;

    TRACE("(this=%p,%p,%d) type=%d\n", This, pvRef, dwEffType, type);

    dei.dwSize = sizeof(DIEFFECTINFOA);          

    if ((type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
	&& test_bit(This->joydev->ffbits, FF_CONSTANT)) {
	IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_ConstantForce);
	(*lpCallback)(&dei, pvRef);
    }

    if ((type == DIEFT_ALL || type == DIEFT_PERIODIC)
	&& test_bit(This->joydev->ffbits, FF_PERIODIC)) {
	if (test_bit(This->joydev->ffbits, FF_SQUARE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Square);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SINE)) {
            IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Sine);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_TRIANGLE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Triangle);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_UP)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothUp);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_DOWN)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothDown);
	    (*lpCallback)(&dei, pvRef);
	}
    } 

    if ((type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
	&& test_bit(This->joydev->ffbits, FF_RAMP)) {
        IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_RampForce);
        (*lpCallback)(&dei, pvRef);
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION) {
	if (test_bit(This->joydev->ffbits, FF_SPRING)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Spring);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_DAMPER)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Damper);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_INERTIA)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Inertia);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_FRICTION)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Friction);
	    (*lpCallback)(&dei, pvRef);
	}
    }

#endif

    return DI_OK;
}

static HRESULT WINAPI JoystickWImpl_EnumEffects(LPDIRECTINPUTDEVICE8W iface,
                                                LPDIENUMEFFECTSCALLBACKW lpCallback,
                                                LPVOID pvRef,
                                                DWORD dwEffType)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    /* seems silly to duplicate all this code but all the structures and functions
     * are actually different (A/W) */
    DIEFFECTINFOW dei; /* feif */
    DWORD type = DIEFT_GETTYPE(dwEffType);
    JoystickImpl* This = (JoystickImpl*)iface;
    int xfd = This->joyfd;

    TRACE("(this=%p,%p,%d) type=%d fd=%d\n", This, pvRef, dwEffType, type, xfd);

    dei.dwSize = sizeof(DIEFFECTINFOW);          

    if ((type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
	&& test_bit(This->joydev->ffbits, FF_CONSTANT)) {
	IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_ConstantForce);
	(*lpCallback)(&dei, pvRef);
    }

    if ((type == DIEFT_ALL || type == DIEFT_PERIODIC)
	&& test_bit(This->joydev->ffbits, FF_PERIODIC)) {
	if (test_bit(This->joydev->ffbits, FF_SQUARE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Square);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SINE)) {
            IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Sine);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_TRIANGLE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Triangle);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_UP)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothUp);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_DOWN)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothDown);
	    (*lpCallback)(&dei, pvRef);
	}
    } 

    if ((type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
	&& test_bit(This->joydev->ffbits, FF_RAMP)) {
        IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_RampForce);
        (*lpCallback)(&dei, pvRef);
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION) {
	if (test_bit(This->joydev->ffbits, FF_SPRING)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Spring);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_DAMPER)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Damper);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_INERTIA)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Inertia);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_FRICTION)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Friction);
	    (*lpCallback)(&dei, pvRef);
	}
    }

    /* return to unacquired state if that's where it was */
    if (xfd == -1)
	IDirectInputDevice8_Unacquire(iface);
#endif

    return DI_OK;
}

/*******************************************************************************
 *      GetEffectInfo - Get information about a particular effect 
 */
static HRESULT WINAPI JoystickAImpl_GetEffectInfo(LPDIRECTINPUTDEVICE8A iface,
						  LPDIEFFECTINFOA pdei,
						  REFGUID guid)
{
    JoystickImpl* This = (JoystickImpl*)iface;

    TRACE("(this=%p,%p,%s)\n", This, pdei, _dump_dinput_GUID(guid));

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    return linuxinput_get_info_A(This->joyfd, guid, pdei); 
#else
    return DI_OK;
#endif
}

static HRESULT WINAPI JoystickWImpl_GetEffectInfo(LPDIRECTINPUTDEVICE8W iface,
                                                  LPDIEFFECTINFOW pdei,
                                                  REFGUID guid)
{
    JoystickImpl* This = (JoystickImpl*)iface;
            
    TRACE("(this=%p,%p,%s)\n", This, pdei, _dump_dinput_GUID(guid));
        
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    return linuxinput_get_info_W(This->joyfd, guid, pdei);
#else
    return DI_OK;
#endif
}

/*******************************************************************************
 *      GetForceFeedbackState - Get information about the device's FF state 
 */
static HRESULT WINAPI JoystickAImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE8A iface,
	LPDWORD pdwOut)
{
    JoystickImpl* This = (JoystickImpl*)iface;

    TRACE("(this=%p,%p)\n", This, pdwOut);

    (*pdwOut) = 0;

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    /* DIGFFS_STOPPED is the only mandatory flag to report */
    if (This->ff_state == FF_STATUS_STOPPED)
	(*pdwOut) |= DIGFFS_STOPPED;
#endif

    return DI_OK;
}

/*******************************************************************************
 *      SendForceFeedbackCommand - Send a command to the device's FF system
 */
static HRESULT WINAPI JoystickAImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD dwFlags)
{
    JoystickImpl* This = (JoystickImpl*)iface;
    TRACE("(this=%p,%d)\n", This, dwFlags);

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    if (dwFlags == DISFFC_STOPALL) {
	/* Stop all effects */
	EffectListItem* itr = This->top_effect;
	while (itr) {
	    IDirectInputEffect_Stop(itr->ref);
	    itr = itr->next;
	}
    } else if (dwFlags == DISFFC_RESET) {
	/* Stop, unload, release and free all effects */
	/* This returns the device to its "bare" state */
	while (This->top_effect) {
	    EffectListItem* temp = This->top_effect;
	    IDirectInputEffect_Stop(temp->ref);
	    IDirectInputEffect_Unload(temp->ref);
	    IDirectInputEffect_Release(temp->ref);
	    This->top_effect = temp->next;
	    HeapFree(GetProcessHeap(), 0, temp);
	}
    } else if (dwFlags == DISFFC_PAUSE || dwFlags == DISFFC_CONTINUE) {
	FIXME("No support for Pause or Continue in linux\n");
    } else if (dwFlags == DISFFC_SETACTUATORSOFF 
		|| dwFlags == DISFFC_SETACTUATORSON) {
	FIXME("No direct actuator control in linux\n");
    } else {
	FIXME("Unknown Force Feedback Command!\n");
	return DIERR_INVALIDPARAM;
    }
    return DI_OK;
#else
    return DIERR_UNSUPPORTED;
#endif
}

/*******************************************************************************
 *      EnumCreatedEffectObjects - Enumerate all the effects that have been
 *		created for this device.
 */
static HRESULT WINAPI JoystickAImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID pvRef,
	DWORD dwFlags)
{
    /* this function is safe to call on non-ff-enabled builds */

    JoystickImpl* This = (JoystickImpl*)iface;
    EffectListItem* itr = This->top_effect;
    TRACE("(this=%p,%p,%p,%d)\n", This, lpCallback, pvRef, dwFlags);

    if (!lpCallback)
	return DIERR_INVALIDPARAM;

    if (dwFlags != 0)
	FIXME("Flags specified, but no flags exist yet (DX9)!\n");

    while (itr) {
	(*lpCallback)(itr->ref, pvRef);
	itr = itr->next;
    }

    return DI_OK;
}

static const IDirectInputDevice8AVtbl JoystickAvt =
{
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	JoystickAImpl_Release,
	JoystickAImpl_GetCapabilities,
        IDirectInputDevice2AImpl_EnumObjects,
	JoystickAImpl_GetProperty,
	JoystickAImpl_SetProperty,
	JoystickAImpl_Acquire,
	JoystickAImpl_Unacquire,
	JoystickAImpl_GetDeviceState,
	IDirectInputDevice2AImpl_GetDeviceData,
        IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
        JoystickAImpl_GetObjectInfo,
	IDirectInputDevice2AImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	JoystickAImpl_CreateEffect,
	JoystickAImpl_EnumEffects,
	JoystickAImpl_GetEffectInfo,
	JoystickAImpl_GetForceFeedbackState,
	JoystickAImpl_SendForceFeedbackCommand,
	JoystickAImpl_EnumCreatedEffectObjects,
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
# define XCAST(fun)	(typeof(JoystickWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static const IDirectInputDevice8WVtbl JoystickWvt =
{
	IDirectInputDevice2WImpl_QueryInterface,
	XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
	XCAST(Release)JoystickAImpl_Release,
	XCAST(GetCapabilities)JoystickAImpl_GetCapabilities,
        IDirectInputDevice2WImpl_EnumObjects,
	XCAST(GetProperty)JoystickAImpl_GetProperty,
	XCAST(SetProperty)JoystickAImpl_SetProperty,
	XCAST(Acquire)JoystickAImpl_Acquire,
	XCAST(Unacquire)JoystickAImpl_Unacquire,
	XCAST(GetDeviceState)JoystickAImpl_GetDeviceState,
	XCAST(GetDeviceData)IDirectInputDevice2AImpl_GetDeviceData,
        XCAST(SetDataFormat)IDirectInputDevice2AImpl_SetDataFormat,
	XCAST(SetEventNotification)IDirectInputDevice2AImpl_SetEventNotification,
	XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
        JoystickWImpl_GetObjectInfo,
	IDirectInputDevice2WImpl_GetDeviceInfo,
	XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
	XCAST(CreateEffect)JoystickAImpl_CreateEffect,
	JoystickWImpl_EnumEffects,
	JoystickWImpl_GetEffectInfo,
	XCAST(GetForceFeedbackState)JoystickAImpl_GetForceFeedbackState,
	XCAST(SendForceFeedbackCommand)JoystickAImpl_SendForceFeedbackCommand,
	XCAST(EnumCreatedEffectObjects)JoystickAImpl_EnumCreatedEffectObjects,
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

#else  /* HAVE_CORRECT_LINUXINPUT_H */

const struct dinput_device joystick_linuxinput_device = {
  "Wine Linux-input joystick driver",
  NULL,
  NULL,
  NULL,
  NULL
};

#endif  /* HAVE_CORRECT_LINUXINPUT_H */
