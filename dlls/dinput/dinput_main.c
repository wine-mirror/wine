/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 *
 */
/* Status:
 *
 * - Tomb Raider 2 Demo:
 *   Playable using keyboard only.
 * - WingCommander Prophecy Demo:
 *   Doesn't get Input Focus.
 * 
 * - Fallout : works great in X and DGA mode
 *
 * FIXME: The keyboard handling needs to (and will) be merged into keyboard.c
 *	  (The current implementation is currently only a proof of concept and
 *	   an utter mess.)
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#ifdef HAVE_LINUX_JOYSTICK_H
# include <linux/joystick.h>
# define JOYDEV	"/dev/js0"
#endif
#include "wine/obj_base.h"
#include "debugtools.h"
#include "dinput.h"
#include "display.h"
#include "input.h"
#include "keyboard.h"
#include "message.h"
#include "mouse.h"
#include "sysmetrics.h"
#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"

DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine mouse driver object instances */
#define WINE_MOUSE_X_AXIS_INSTANCE 0x0001
#define WINE_MOUSE_Y_AXIS_INSTANCE 0x0002
#define WINE_MOUSE_L_BUTTON_INSTANCE 0x0004
#define WINE_MOUSE_R_BUTTON_INSTANCE 0x0008
#define WINE_MOUSE_M_BUTTON_INSTANCE 0x0010

/* Wine joystick driver object instances */
#define WINE_JOYSTICK_AXIS_BASE   0
#define WINE_JOYSTICK_BUTTON_BASE 8

extern BYTE InputKeyStateTable[256];
extern int min_keycode, max_keycode;
extern WORD keyc2vkey[256];

/* Routines to do DataFormat / WineFormat conversions */
typedef struct {
  int size;
  int offset_in;
  int offset_out;
  int value;
} DataTransform;

typedef struct {
  int size;
  int internal_format_size;
  DataTransform *dt;
} DataFormat;

/* ------------------------------- */
/* Wine mouse internal data format */
/* ------------------------------- */

/* Constants used to access the offset array */
#define WINE_MOUSE_X_POSITION 0
#define WINE_MOUSE_Y_POSITION 1
#define WINE_MOUSE_L_POSITION 2
#define WINE_MOUSE_R_POSITION 3
#define WINE_MOUSE_M_POSITION 4

typedef struct {
  LONG lX;
  LONG lY;
  BYTE rgbButtons[4];
} Wine_InternalMouseData;

#define WINE_INTERNALMOUSE_NUM_OBJS 5

static DIOBJECTDATAFORMAT Wine_InternalMouseObjectFormat[WINE_INTERNALMOUSE_NUM_OBJS] = {
  { &GUID_XAxis,   FIELD_OFFSET(Wine_InternalMouseData, lX),
      DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
  { &GUID_YAxis,   FIELD_OFFSET(Wine_InternalMouseData, lY),
      DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
  { &GUID_Button, (FIELD_OFFSET(Wine_InternalMouseData, rgbButtons)) + 0,
      DIDFT_MAKEINSTANCE(WINE_MOUSE_L_BUTTON_INSTANCE) | DIDFT_PSHBUTTON, 0 },
  { &GUID_Button, (FIELD_OFFSET(Wine_InternalMouseData, rgbButtons)) + 1,
      DIDFT_MAKEINSTANCE(WINE_MOUSE_R_BUTTON_INSTANCE) | DIDFT_PSHBUTTON, 0 },
  { &GUID_Button, (FIELD_OFFSET(Wine_InternalMouseData, rgbButtons)) + 2,
      DIDFT_MAKEINSTANCE(WINE_MOUSE_M_BUTTON_INSTANCE) | DIDFT_PSHBUTTON, 0 }
};

static DIDATAFORMAT Wine_InternalMouseFormat = {
  0, /* dwSize - unused */
  0, /* dwObjsize - unused */
  0, /* dwFlags - unused */
  sizeof(Wine_InternalMouseData),
  WINE_INTERNALMOUSE_NUM_OBJS, /* dwNumObjs */
  Wine_InternalMouseObjectFormat
};

static ICOM_VTABLE(IDirectInputA) ddiavt;
static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt;
static ICOM_VTABLE(IDirectInputDevice2A) SysMouseAvt;

typedef struct IDirectInputAImpl IDirectInputAImpl;
typedef struct IDirectInputDevice2AImpl IDirectInputDevice2AImpl;
typedef struct SysKeyboardAImpl SysKeyboardAImpl;
typedef struct SysMouseAImpl SysMouseAImpl;

struct IDirectInputDevice2AImpl
{
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;
};

struct SysKeyboardAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;
        /* SysKeyboardAImpl */
        BYTE                            keystate[256];
	KEYBOARD_CONFIG                 initial_config;
	int                             acquired;
};

#ifdef HAVE_LINUX_22_JOYSTICK_API
typedef struct JoystickAImpl JoystickAImpl;
static ICOM_VTABLE(IDirectInputDevice2A) JoystickAvt;
struct JoystickAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;

	/* joystick private */
	int				joyfd;
	LPDIDATAFORMAT			df;
        HANDLE				hEvent;
	LONG				lMin,lMax,deadzone;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_pos, queue_len;
	DIJOYSTATE			js;
};
#endif

struct SysMouseAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;

	/* The current data format and the conversion between internal
	   and external data formats */
	LPDIDATAFORMAT			df;
	DataFormat                     *wine_df;
	int                             offset_array[5];
	
        /* SysMouseAImpl */
        BYTE                            absolute;
        /* Previous position for relative moves */
        LONG				prevX, prevY;
        LPMOUSE_EVENT_PROC		prev_handler;
        HWND				win;
        DWORD				win_centerX, win_centerY;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_pos, queue_len;
        int				need_warp;
        int				acquired;
        HANDLE				hEvent;
	CRITICAL_SECTION		crit;

	/* This is for mouse reporting. */
	Wine_InternalMouseData          m_state;
};

static int evsequence=0;


/* UIDs for Wine "drivers".
   When enumerating each device supporting DInput, they have two UIDs :
    - the 'windows' UID
    - a vendor UID */
#ifdef HAVE_LINUX_22_JOYSTICK_API
static GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};
#endif
static GUID DInput_Wine_Mouse_GUID = { /* 9e573ed8-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed8,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};
static GUID DInput_Wine_Keyboard_GUID = { /* 0ab8648a-7735-11d2-8c73-71df54a96441 */
  0x0ab8648a,
  0x7735,
  0x11d2,
  {0x8c, 0x73, 0x71, 0xdf, 0x54, 0xa9, 0x64, 0x41}
};

/* FIXME: This is ugly and not thread safe :/ */
static IDirectInputDevice2A* current_lock = NULL;

/******************************************************************************
 *	Various debugging tools
 */
static void _dump_cooperativelevel(DWORD dwFlags) {
  int   i;
  const struct {
    DWORD       mask;
    char        *name;
  } flags[] = {
#define FE(x) { x, #x},
    FE(DISCL_BACKGROUND)
    FE(DISCL_EXCLUSIVE)
    FE(DISCL_FOREGROUND)
    FE(DISCL_NONEXCLUSIVE)
#undef FE
  };
  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
    if (flags[i].mask & dwFlags)
      DPRINTF("%s ",flags[i].name);
  DPRINTF("\n");
}

static void _dump_EnumObjects_flags(DWORD dwFlags) {
  int   i;
  const struct {
    DWORD       mask;
    char        *name;
  } flags[] = {
#define FE(x) { x, #x},
    FE(DIDFT_ABSAXIS)
    FE(DIDFT_ALL)
    FE(DIDFT_AXIS)
    FE(DIDFT_BUTTON)
    FE(DIDFT_COLLECTION)
    FE(DIDFT_FFACTUATOR)
    FE(DIDFT_FFEFFECTTRIGGER)
    FE(DIDFT_NOCOLLECTION)
    FE(DIDFT_NODATA)
    FE(DIDFT_OUTPUT)
    FE(DIDFT_POV)
    FE(DIDFT_PSHBUTTON)
    FE(DIDFT_RELAXIS)
    FE(DIDFT_TGLBUTTON)
#undef FE
  };
  if (dwFlags == DIDFT_ALL) {
    DPRINTF("DIDFT_ALL");
    return;
  }
  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
    if (flags[i].mask & dwFlags)
      DPRINTF("%s ",flags[i].name);
  if (dwFlags & DIDFT_INSTANCEMASK)
    DPRINTF("Instance(%04lx) ", dwFlags >> 8);
}

static void _dump_DIPROPHEADER(DIPROPHEADER *diph) {
  DPRINTF("  - dwObj = 0x%08lx\n", diph->dwObj);
  DPRINTF("  - dwHow = %s\n",
	  ((diph->dwHow == DIPH_DEVICE) ? "DIPH_DEVICE" :
	   ((diph->dwHow == DIPH_BYOFFSET) ? "DIPH_BYOFFSET" :
	    ((diph->dwHow == DIPH_BYID)) ? "DIPH_BYID" : "unknown")));
}

static void _dump_OBJECTINSTANCE(DIDEVICEOBJECTINSTANCE *ddoi) {
  if (TRACE_ON(dinput)) {
    DPRINTF("    - enumerating : 0x%08lx - %2ld - 0x%08lx - %s\n",
	    ddoi->guidType.Data1, ddoi->dwOfs, ddoi->dwType, ddoi->tszName);
  }
}

struct IDirectInputAImpl
{
        ICOM_VFIELD(IDirectInputA);
        DWORD                   ref;
};

/* Conversion between internal data buffer and external data buffer */
static void fill_DataFormat(void *out, void *in, DataFormat *df) {
  int i;
  char *in_c = (char *) in;
  char *out_c = (char *) out;

  if (df->dt == NULL) {
    /* This means that the app uses Wine's internal data format */
    memcpy(out, in, df->internal_format_size);
  } else {
    for (i = 0; i < df->size; i++) {
      if (df->dt[i].offset_in >= 0) {
	switch (df->dt[i].size) {
	case 1:
	  TRACE("Copying (c) to %d from %d (value %d)\n",
		df->dt[i].offset_out, df->dt[i].offset_in, *((char *) (in_c + df->dt[i].offset_in)));
	  *((char *) (out_c + df->dt[i].offset_out)) = *((char *) (in_c + df->dt[i].offset_in));
	  break;
	
	case 2:
	  TRACE("Copying (s) to %d from %d (value %d)\n",
		df->dt[i].offset_out, df->dt[i].offset_in, *((short *) (in_c + df->dt[i].offset_in)));
	  *((short *) (out_c + df->dt[i].offset_out)) = *((short *) (in_c + df->dt[i].offset_in));
	  break;
	
	case 4:
	  TRACE("Copying (i) to %d from %d (value %d)\n",
		df->dt[i].offset_out, df->dt[i].offset_in, *((int *) (in_c + df->dt[i].offset_in)));
	  *((int *) (out_c + df->dt[i].offset_out)) = *((int *) (in_c + df->dt[i].offset_in));
	  break;
	
	default:
	  memcpy((out_c + df->dt[i].offset_out), (in_c + df->dt[i].offset_in), df->dt[i].size);
	}
      } else {
	switch (df->dt[i].size) {
	case 1:
	  TRACE("Copying (c) to %d default value %d\n",
		df->dt[i].offset_out, df->dt[i].value);
	  *((char *) (out_c + df->dt[i].offset_out)) = (char) df->dt[i].value;
	  break;
	
	case 2:
	  TRACE("Copying (s) to %d default value %d\n",
		df->dt[i].offset_out, df->dt[i].value);
	  *((short *) (out_c + df->dt[i].offset_out)) = (short) df->dt[i].value;
	  break;
	
	case 4:
	  TRACE("Copying (i) to %d default value %d\n",
		df->dt[i].offset_out, df->dt[i].value);
	  *((int *) (out_c + df->dt[i].offset_out)) = (int) df->dt[i].value;
	  break;
	
	default:
	  memset((out_c + df->dt[i].offset_out), df->dt[i].size, 0);
	}
      }
    }
  }
}

static DataFormat *create_DataFormat(DIDATAFORMAT *wine_format, DIDATAFORMAT *asked_format, int *offset) {
  DataFormat *ret;
  DataTransform *dt;
  int i, j;
  int same = 1;
  int *done;
  int index = 0;
  
  ret = (DataFormat *) HeapAlloc(GetProcessHeap(), 0, sizeof(DataFormat));
  
  done = (int *) HeapAlloc(GetProcessHeap(), 0, sizeof(int) * asked_format->dwNumObjs);
  memset(done, 0, sizeof(int) * asked_format->dwNumObjs);

  dt = (DataTransform *) HeapAlloc(GetProcessHeap(), 0, asked_format->dwNumObjs * sizeof(DataTransform));

  TRACE("Creating DataTransorm : \n");
  
  for (i = 0; i < wine_format->dwNumObjs; i++) {
    offset[i] = -1;
    
    for (j = 0; j < asked_format->dwNumObjs; j++) {
      if (done[j] == 1)
	continue;
      
      if (((asked_format->rgodf[j].pguid == NULL) || (IsEqualGUID(wine_format->rgodf[i].pguid, asked_format->rgodf[j].pguid)))
	  &&
	  (wine_format->rgodf[i].dwType & asked_format->rgodf[j].dwType)) {

	done[j] = 1;

	TRACE("Matching : \n"); 
	TRACE("   - Asked (%d) : %s - Ofs = %3ld - (Type = 0x%02x | Instance = %04x)\n",
	      j, debugstr_guid(asked_format->rgodf[j].pguid), 
	      asked_format->rgodf[j].dwOfs,
	      DIDFT_GETTYPE(asked_format->rgodf[j].dwType), DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType));
	
	TRACE("   - Wine  (%d) : %s - Ofs = %3ld - (Type = 0x%02x | Instance = %04x)\n",
	      j, debugstr_guid(wine_format->rgodf[i].pguid), 
	      wine_format->rgodf[i].dwOfs,
	      DIDFT_GETTYPE(wine_format->rgodf[i].dwType), DIDFT_GETINSTANCE(wine_format->rgodf[i].dwType));
	
	if (wine_format->rgodf[i].dwType & DIDFT_BUTTON)
	  dt[index].size = sizeof(BYTE);
	else
	  dt[index].size = sizeof(DWORD);
	dt[index].offset_in  = wine_format ->rgodf[i].dwOfs;
	dt[index].offset_out = asked_format->rgodf[j].dwOfs;
	dt[index].value = 0;
	index++;
	
	if (wine_format->rgodf[i].dwOfs != asked_format->rgodf[j].dwOfs)
	  same = 0;

	offset[i] = asked_format->rgodf[j].dwOfs;
	break;
      }
    }

    if (j == asked_format->dwNumObjs)
      same = 0;
  }

  TRACE("Setting to default value :\n");
  for (j = 0; j < asked_format->dwNumObjs; j++) {
    if (done[j] == 0) {
      TRACE(" - Asked (%d) : %s - Ofs = %3ld - (Type = 0x%02x | Instance = %04x)\n",
	    j, debugstr_guid(asked_format->rgodf[j].pguid), 
	    asked_format->rgodf[j].dwOfs,
	    DIDFT_GETTYPE(asked_format->rgodf[j].dwType), DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType));

      
      if (asked_format->rgodf[j].dwType & DIDFT_BUTTON)
	dt[index].size = sizeof(BYTE);
      else
	dt[index].size = sizeof(DWORD);
      dt[index].offset_in  = -1;
      dt[index].offset_out = asked_format->rgodf[j].dwOfs;
      dt[index].value = 0;
      index++;

      same = 0;
    }
  }

  ret->internal_format_size = wine_format->dwDataSize;
  ret->size = index;
  if (same) {
    ret->dt = NULL;
    HeapFree(GetProcessHeap(), 0, dt);
  } else {
    ret->dt = dt;
  }
  
  HeapFree(GetProcessHeap(), 0, done);

  return ret;
}

/******************************************************************************
 *	DirectInputCreate32A
 */
HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{
	IDirectInputAImpl* This;
	TRACE("(0x%08lx,%04lx,%p,%p)\n",
		(DWORD)hinst,dwVersion,ppDI,punkOuter
	);
	This = (IDirectInputAImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputAImpl));
	This->ref = 1;
	ICOM_VTBL(This) = &ddiavt;
	*ppDI=(IDirectInputA*)This;
	return 0;
}
/******************************************************************************
 *	IDirectInputA_EnumDevices
 */
static HRESULT WINAPI IDirectInputAImpl_EnumDevices(
	LPDIRECTINPUTA iface, DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback,
	LPVOID pvRef, DWORD dwFlags
)
{
	ICOM_THIS(IDirectInputAImpl,iface);
	DIDEVICEINSTANCEA devInstance;
	int ret;

	TRACE("(this=%p,0x%04lx,%p,%p,%04lx)\n", This, dwDevType, lpCallback, pvRef, dwFlags);

	devInstance.dwSize = sizeof(DIDEVICEINSTANCEA);
	if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_KEYBOARD)) {
		/* Return keyboard */
		devInstance.guidInstance = GUID_SysKeyboard;/* DInput's GUID */
		devInstance.guidProduct = DInput_Wine_Keyboard_GUID; /* Vendor's GUID */
		devInstance.dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_UNKNOWN << 8);
		strcpy(devInstance.tszInstanceName, "Keyboard");
		strcpy(devInstance.tszProductName, "Wine Keyboard");

		ret = lpCallback(&devInstance, pvRef);
		TRACE("Keyboard registered\n");
		if (ret == DIENUM_STOP)
			return 0;
	}
  
	if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_MOUSE)) {
		/* Return mouse */
		devInstance.guidInstance = GUID_SysMouse;/* DInput's GUID */
		devInstance.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
		devInstance.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_UNKNOWN << 8);
		strcpy(devInstance.tszInstanceName, "Mouse");
		strcpy(devInstance.tszProductName, "Wine Mouse");

		ret = lpCallback(&devInstance, pvRef);
		TRACE("Mouse registered\n");
		if (ret == DIENUM_STOP)
			return 0;
	}
	if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_JOYSTICK)) {
		/* check whether we have a joystick */
#ifdef HAVE_LINUX_22_JOYSTICK_API
		if (	(access(JOYDEV,O_RDONLY)!=-1)		||
			(errno!=ENODEV && errno!=ENOENT)
		) {
		    /* Return joystick */
		    devInstance.guidInstance	= GUID_Joystick;
		    devInstance.guidProduct	= DInput_Wine_Joystick_GUID;
		    /* we only support traditional joysticks for now */
		    devInstance.dwDevType	= DIDEVTYPE_JOYSTICK | DIDEVTYPEJOYSTICK_TRADITIONAL;
		    strcpy(devInstance.tszInstanceName,	"Joystick");
		    /* ioctl JSIOCGNAME(len) */
		    strcpy(devInstance.tszProductName,	"Wine Joystick");

		    ret = lpCallback(&devInstance,pvRef);
		    TRACE("Joystick registered\n");
		    if (ret == DIENUM_STOP)
			    return 0;
		}
#endif
	}
	return 0;
}

static ULONG WINAPI IDirectInputAImpl_AddRef(LPDIRECTINPUTA iface)
{
	ICOM_THIS(IDirectInputAImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI IDirectInputAImpl_Release(LPDIRECTINPUTA iface)
{
	ICOM_THIS(IDirectInputAImpl,iface);
	if (!(--This->ref)) {
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IDirectInputAImpl_CreateDevice(
	LPDIRECTINPUTA iface,REFGUID rguid,LPDIRECTINPUTDEVICEA* pdev,
	LPUNKNOWN punk
) {
	ICOM_THIS(IDirectInputAImpl,iface);
	
	TRACE("(this=%p,%s,%p,%p)\n",This,debugstr_guid(rguid),pdev,punk);
	if ((IsEqualGUID(&GUID_SysKeyboard,rguid)) ||          /* Generic Keyboard */
	    (IsEqualGUID(&DInput_Wine_Keyboard_GUID,rguid))) { /* Wine Keyboard */
                SysKeyboardAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboardAImpl));
		newDevice->ref = 1;
		ICOM_VTBL(newDevice) = &SysKeyboardAvt;
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
		memset(newDevice->keystate,0,256);
                *pdev=(IDirectInputDeviceA*)newDevice;

		TRACE("Creating a Keyboard device (%p)\n", newDevice);
		return DI_OK;
	}
	if ((IsEqualGUID(&GUID_SysMouse,rguid)) ||             /* Generic Mouse */
	    (IsEqualGUID(&DInput_Wine_Mouse_GUID,rguid))) { /* Wine Mouse */
                SysMouseAImpl* newDevice;
		int offset_array[5] = {
		  FIELD_OFFSET(Wine_InternalMouseData, lX),
		  FIELD_OFFSET(Wine_InternalMouseData, lY),
		  FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 0,
		  FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 1,
		  FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 2
		};
		
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseAImpl));
		newDevice->ref = 1;
		ICOM_VTBL(newDevice) = &SysMouseAvt;
		InitializeCriticalSection(&(newDevice->crit));
		MakeCriticalSectionGlobal(&(newDevice->crit));
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
                *pdev=(IDirectInputDeviceA*)newDevice;

		/* Per default, Wine uses its internal data format */
		newDevice->df = &Wine_InternalMouseFormat;
		memcpy(newDevice->offset_array, offset_array, 5 * sizeof(int));
		newDevice->wine_df = (DataFormat *) HeapAlloc(GetProcessHeap(), 0, sizeof(DataFormat));
		newDevice->wine_df->size = 0;
		newDevice->wine_df->internal_format_size = Wine_InternalMouseFormat.dwDataSize;
		newDevice->wine_df->dt = NULL;
		
		TRACE("Creating a Mouse device (%p)\n", newDevice);
		return DI_OK;
	}
#ifdef HAVE_LINUX_22_JOYSTICK_API
	if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
	    (IsEqualGUID(&DInput_Wine_Joystick_GUID,rguid))) {
                JoystickAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickAImpl));
		newDevice->ref		= 1;
		ICOM_VTBL(newDevice)    = &JoystickAvt;
		newDevice->joyfd	= -1;
		newDevice->lMin = -32768;
		newDevice->lMax = +32767;
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
                *pdev=(IDirectInputDeviceA*)newDevice;

		TRACE("Creating a Joystick device (%p)\n", newDevice);
		return DI_OK;
	}
#endif
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputAImpl_QueryInterface(
	LPDIRECTINPUTA iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectInputAImpl,iface);

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	if (IsEqualGUID(&IID_IUnknown,riid)) {
		IDirectInputA_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	if (IsEqualGUID(&IID_IDirectInputA,riid)) {
		IDirectInputA_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	TRACE("Unsupported interface !\n");
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputAImpl_Initialize(
	LPDIRECTINPUTA iface,HINSTANCE hinst,DWORD x
) {
	return DIERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectInputAImpl_GetDeviceStatus(LPDIRECTINPUTA iface,
							REFGUID rguid) {
  ICOM_THIS(IDirectInputAImpl,iface);
  
  FIXME("(%p)->(%s): stub\n",This,debugstr_guid(rguid));
  
  return DI_OK;
}

static HRESULT WINAPI IDirectInputAImpl_RunControlPanel(LPDIRECTINPUTA iface,
							HWND hwndOwner,
							DWORD dwFlags) {
  ICOM_THIS(IDirectInputAImpl,iface);
  FIXME("(%p)->(%08lx,%08lx): stub\n",This, (DWORD) hwndOwner, dwFlags);
  
  return DI_OK;
}

static ICOM_VTABLE(IDirectInputA) ddiavt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectInputAImpl_QueryInterface,
	IDirectInputAImpl_AddRef,
	IDirectInputAImpl_Release,
	IDirectInputAImpl_CreateDevice,
	IDirectInputAImpl_EnumDevices,
	IDirectInputAImpl_GetDeviceStatus,
	IDirectInputAImpl_RunControlPanel,
	IDirectInputAImpl_Initialize
};

/******************************************************************************
 *	IDirectInputDeviceA
 */

static HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df
) {
	/*
	int i;
	TRACE(dinput,"(this=%p,%p)\n",This,df);

	TRACE(dinput,"df.dwSize=%ld\n",df->dwSize);
	TRACE(dinput,"(df.dwObjsize=%ld)\n",df->dwObjSize);
	TRACE(dinput,"(df.dwFlags=0x%08lx)\n",df->dwFlags);
	TRACE(dinput,"(df.dwDataSize=%ld)\n",df->dwDataSize);
	TRACE(dinput,"(df.dwNumObjs=%ld)\n",df->dwNumObjs);

	for (i=0;i<df->dwNumObjs;i++) {
		TRACE(dinput,"df.rgodf[%d].guid %s\n",i,debugstr_guid(df->rgodf[i].pguid));
		TRACE(dinput,"df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
		TRACE(dinput,"dwType 0x%02lx,dwInstance %ld\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
		TRACE(dinput,"df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
	}
	*/
	return 0;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE2A iface,HWND hwnd,DWORD dwflags
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	TRACE("(this=%p,0x%08lx,0x%08lx)\n",This,(DWORD)hwnd,dwflags);
	if (TRACE_ON(dinput))
	  _dump_cooperativelevel(dwflags);
	return 0;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE2A iface,HANDLE hnd
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	FIXME("(this=%p,0x%08lx): stub\n",This,(DWORD)hnd);
	return 0;
}

static ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	This->ref--;
	if (This->ref)
		return This->ref;
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_SetProperty(
	LPDIRECTINPUTDEVICE2A iface,REFGUID rguid,LPCDIPROPHEADER ph
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
	TRACE("(size=%ld,headersize=%ld,obj=%ld,how=%ld\n",
            ph->dwSize,ph->dwHeaderSize,ph->dwObj,ph->dwHow);
	if (!HIWORD(rguid)) {
		switch ((DWORD)rguid) {
		case (DWORD) DIPROP_BUFFERSIZE: {
			LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

			TRACE("(buffersize=%ld)\n",pd->dwData);
			break;
		}
		default:
			WARN("Unknown type %ld\n",(DWORD)rguid);
			break;
		}
	}
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
)
{
	return KEYBOARD_Driver->pGetDIState(len, ptr)?DI_OK:E_FAIL;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceData(
	LPDIRECTINPUTDEVICE2A iface,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	HRESULT	ret;
	int	i;

	TRACE("(this=%p,%ld,%p,%p(%ld)),0x%08lx)\n",
	      This,dodsize,dod,entries,entries?*entries:0,flags);

	ret=KEYBOARD_Driver->pGetDIData(
		This->keystate, dodsize, dod, entries, flags)?DI_OK:E_FAIL;
	for (i=0;i<*entries;i++) {
		dod[i].dwTimeStamp = GetTickCount();
		dod[i].dwSequence = evsequence++;
	}
	return ret;
}

static HRESULT WINAPI SysKeyboardAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	
	TRACE("(this=%p)\n",This);
	
	if (This->acquired == 0) {
	  KEYBOARD_CONFIG no_auto;
	  
	  /* Save the original config */
	  KEYBOARD_Driver->pGetKeyboardConfig(&(This->initial_config));
	  
	  /* Now, remove auto-repeat */
	  no_auto.auto_repeat = FALSE;
	  KEYBOARD_Driver->pSetKeyboardConfig(&no_auto, WINE_KEYBOARD_CONFIG_AUTO_REPEAT);

	  This->acquired = 1;
	}
	
	return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	TRACE("(this=%p)\n",This);

	if (This->acquired == 1) {
	  /* Restore the original configuration */
	  KEYBOARD_Driver->pSetKeyboardConfig(&(This->initial_config), 0xFFFFFFFF);
	  This->acquired = 0;
	} else {
	  ERR("Unacquiring a not-acquired device !!!\n");
	}

	return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capablitites
  */
static HRESULT WINAPI SysKeyboardAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

  if (lpDIDevCaps->dwSize == sizeof(DIDEVCAPS)) {
    lpDIDevCaps->dwFlags = DIDC_ATTACHED;
    lpDIDevCaps->dwDevType = DIDEVTYPE_KEYBOARD;
    lpDIDevCaps->dwAxes = 0;
    lpDIDevCaps->dwButtons = 0;
    lpDIDevCaps->dwPOVs = 0;
    lpDIDevCaps->dwFFSamplePeriod = 0;
    lpDIDevCaps->dwFFMinTimeResolution = 0;
    lpDIDevCaps->dwFirmwareRevision = 100;
    lpDIDevCaps->dwHardwareRevision = 100;
    lpDIDevCaps->dwFFDriverVersion = 0;
  } else {
    /* DirectX 3.0 */
    FIXME("DirectX 3.0 not supported....\n");
  }
  
  return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE2A iface,REFIID riid,LPVOID *ppobj
)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	if (IsEqualGUID(&IID_IUnknown,riid)) {
		IDirectInputDevice2_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	if (IsEqualGUID(&IID_IDirectInputDeviceA,riid)) {
		IDirectInputDevice2_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	if (IsEqualGUID(&IID_IDirectInputDevice2A,riid)) {
		IDirectInputDevice2_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	TRACE("Unsupported interface !\n");
	return E_FAIL;
}

static ULONG WINAPI IDirectInputDevice2AImpl_AddRef(
	LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	return ++This->ref;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME("(this=%p,%p,%p,%08lx): stub!\n", iface, lpCallback, lpvRef, dwFlags);
	if (TRACE_ON(dinput)) {
	  DPRINTF("  - flags = ");
	  _dump_EnumObjects_flags(dwFlags);
	  DPRINTF("\n");
	}
	
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE2A iface,
	REFGUID rguid,
	LPDIPROPHEADER pdiph)
{
	FIXME("(this=%p,%s,%p): stub!\n",
	      iface, debugstr_guid(rguid), pdiph);
	
	if (TRACE_ON(dinput))
	  _dump_DIPROPHEADER(pdiph);
	
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
	FIXME("(this=%p,%p,%ld,0x%08lx): stub!\n",
	      iface, pdidoi, dwObj, dwHow);
	
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
	FIXME("(this=%p,%p): stub!\n",
	      iface, pdidi);
	
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE2A iface,
	HWND hwndOwner,
	DWORD dwFlags)
{
  FIXME("(this=%p,0x%08x,0x%08lx): stub!\n",
	iface, hwndOwner, dwFlags);
	
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE2A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
	FIXME("(this=%p,%d,%ld,%s): stub!\n",
	      iface, hinst, dwVersion, debugstr_guid(rguid));
	return DI_OK;
}
	
/******************************************************************************
 *	IDirectInputDevice2A
 */

static HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(
	LPDIRECTINPUTDEVICE2A iface,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter)
{
	FIXME("(this=%p,%s,%p,%p,%p): stub!\n",
	      iface, debugstr_guid(rguid), lpeff, ppdef, pUnkOuter);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME("(this=%p,%p,%p,0x%08lx): stub!\n",
	      iface, lpCallback, lpvRef, dwFlags);
	
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid)
{
	FIXME("(this=%p,%p,%s): stub!\n",
	      iface, lpdei, debugstr_guid(rguid));
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE2A iface,
	LPDWORD pdwOut)
{
	FIXME("(this=%p,%p): stub!\n",
	      iface, pdwOut);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD dwFlags)
{
	FIXME("(this=%p,0x%08lx): stub!\n",
	      iface, dwFlags);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME("(this=%p,%p,%p,0x%08lx): stub!\n",
	      iface, lpCallback, lpvRef, dwFlags);
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFESCAPE lpDIEEsc)
{
	FIXME("(this=%p,%p): stub!\n",
	      iface, lpDIEEsc);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE2A iface)
{
	FIXME("(this=%p): stub!\n",
	      iface);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD cbObjectData,
	LPDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags)
{
	FIXME("(this=%p,0x%08lx,%p,%p,0x%08lx): stub!\n",
	      iface, cbObjectData, rgdod, pdwInOut, dwFlags);
	
	return DI_OK;
}

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

/******************************************************************************
  *     Release : release the mouse buffer.
  */
static ULONG WINAPI SysMouseAImpl_Release(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysMouseAImpl,iface);

	This->ref--;
	if (This->ref)
		return This->ref;

	/* Free the data queue */
	if (This->data_queue != NULL)
	  HeapFree(GetProcessHeap(),0,This->data_queue);

	/* Install the previous event handler (in case of releasing an aquired
	   mouse device) */
	if (This->prev_handler != NULL)
	  MOUSE_Enable(This->prev_handler);
	DeleteCriticalSection(&(This->crit));

	/* Free the DataFormat */
	if (This->df != &(Wine_InternalMouseFormat)) {
	  HeapFree(GetProcessHeap(), 0, This->df->rgodf);
	  HeapFree(GetProcessHeap(), 0, This->df);
	}
	
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}


/******************************************************************************
  *     SetCooperativeLevel : store the window in which we will do our
  *   grabbing.
  */
static HRESULT WINAPI SysMouseAImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE2A iface,HWND hwnd,DWORD dwflags
)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,0x%08lx,0x%08lx)\n",This,(DWORD)hwnd,dwflags);

  if (TRACE_ON(dinput))
    _dump_cooperativelevel(dwflags);

  /* Store the window which asks for the mouse */
  This->win = hwnd;
  
  return 0;
}


/******************************************************************************
  *     SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  *
  *   For the moment, only the "standard" configuration (c_dfDIMouse) is supported
  *   in absolute and relative mode.
  */
static HRESULT WINAPI SysMouseAImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df
)
{
  ICOM_THIS(SysMouseAImpl,iface);
  int i;
  
  TRACE("(this=%p,%p)\n",This,df);

  TRACE("(df.dwSize=%ld)\n",df->dwSize);
  TRACE("(df.dwObjsize=%ld)\n",df->dwObjSize);
  TRACE("(df.dwFlags=0x%08lx)\n",df->dwFlags);
  TRACE("(df.dwDataSize=%ld)\n",df->dwDataSize);
  TRACE("(df.dwNumObjs=%ld)\n",df->dwNumObjs);

  for (i=0;i<df->dwNumObjs;i++) {

    TRACE("df.rgodf[%d].guid %s (%p)\n",i, debugstr_guid(df->rgodf[i].pguid), df->rgodf[i].pguid);
    TRACE("df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
    TRACE("dwType 0x%02x,dwInstance %d\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
    TRACE("df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
  }

  /* Check if the mouse is in absolute or relative mode */
  if (df->dwFlags == DIDF_ABSAXIS)
    This->absolute = 1;
  else if (df->dwFlags == DIDF_RELAXIS)
    This->absolute = 0;
  else
    ERR("Neither absolute nor relative flag set.");
  
  /* Store the new data format */
  This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
  memcpy(This->df, df, df->dwSize);
  This->df->rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
  memcpy(This->df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);

  /* Prepare all the data-conversion filters */
  This->wine_df = create_DataFormat(&(Wine_InternalMouseFormat), df, This->offset_array);
  
  return 0;
}

#define GEN_EVENT(offset,data,xtime,seq)						\
{											\
  if ((offset >= 0) && (This->queue_pos < This->queue_len)) {				\
    This->data_queue[This->queue_pos].dwOfs = offset;					\
    This->data_queue[This->queue_pos].dwData = data;					\
    This->data_queue[This->queue_pos].dwTimeStamp = xtime;				\
    This->data_queue[This->queue_pos].dwSequence = seq;					\
    This->queue_pos++;									\
  }											\
}

  
/* Our private mouse event handler */
static void WINAPI dinput_mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
				      DWORD cButtons, DWORD dwExtraInfo )
{
  DWORD posX, posY, keyState, xtime, extra;
  SysMouseAImpl* This = (SysMouseAImpl*) current_lock;
  
  EnterCriticalSection(&(This->crit));
  /* Mouse moved -> send event if asked */
  if (This->hEvent)
    SetEvent(This->hEvent);
  
  if (   !IsBadReadPtr( (LPVOID)dwExtraInfo, sizeof(WINE_MOUSEEVENT) )
      && ((WINE_MOUSEEVENT *)dwExtraInfo)->magic == WINE_MOUSEEVENT_MAGIC ) {
    WINE_MOUSEEVENT *wme = (WINE_MOUSEEVENT *)dwExtraInfo;
    keyState = wme->keyState;
    xtime = wme->time;
    extra = (DWORD)wme->hWnd;
    
    if ((dwFlags & MOUSEEVENTF_MOVE) &&
	(dwFlags & MOUSEEVENTF_ABSOLUTE)) {
      posX = (dx * GetSystemMetrics(SM_CXSCREEN)) >> 16;
      posY = (dy * GetSystemMetrics(SM_CYSCREEN)) >> 16;
    } else {
      posX = This->prevX;
      posY = This->prevY;
    }
  } else {
    ERR("Mouse event not supported...\n");
    LeaveCriticalSection(&(This->crit));
    return ;
  }

  TRACE(" %ld %ld ", posX, posY);

  if ( dwFlags & MOUSEEVENTF_MOVE ) {
    if (This->absolute) {
      if (posX != This->prevX)
	GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], posX, xtime, 0);
      if (posY != This->prevY)
	GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], posY, xtime, 0);
    } else {
      /* Relative mouse input : the real fun starts here... */
      if (This->need_warp) {
	if (posX != This->prevX)
	  GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], posX - This->prevX, xtime, evsequence++);
	if (posY != This->prevY)
	  GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], posY - This->prevY, xtime, evsequence++);
      } else {
	/* This is the first time the event handler has been called after a
	   GetData of GetState. */
	if (posX != This->win_centerX) {
	  GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], posX - This->win_centerX, xtime, evsequence++);
	  This->need_warp = 1;
	}
	  
	if (posY != This->win_centerY) {
	  GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], posY - This->win_centerY, xtime, evsequence++);
	  This->need_warp = 1;
	}
      }
    }
  }
  if ( dwFlags & MOUSEEVENTF_LEFTDOWN ) {
    if (TRACE_ON(dinput))
      DPRINTF(" LD ");

    GEN_EVENT(This->offset_array[WINE_MOUSE_L_POSITION], 0xFF, xtime, evsequence++);
    This->m_state.rgbButtons[0] = 0xFF;
  }
  if ( dwFlags & MOUSEEVENTF_LEFTUP ) {
    if (TRACE_ON(dinput))
      DPRINTF(" LU ");

    GEN_EVENT(This->offset_array[WINE_MOUSE_L_POSITION], 0x00, xtime, evsequence++);
    This->m_state.rgbButtons[0] = 0x00;
  }
  if ( dwFlags & MOUSEEVENTF_RIGHTDOWN ) {
    if (TRACE_ON(dinput))
      DPRINTF(" RD ");

    GEN_EVENT(This->offset_array[WINE_MOUSE_R_POSITION], 0xFF, xtime, evsequence++);
    This->m_state.rgbButtons[1] = 0xFF;
  }
  if ( dwFlags & MOUSEEVENTF_RIGHTUP ) {
    if (TRACE_ON(dinput))
      DPRINTF(" RU ");

    GEN_EVENT(This->offset_array[WINE_MOUSE_R_POSITION], 0x00, xtime, evsequence++);
    This->m_state.rgbButtons[1] = 0x00;
  }
  if ( dwFlags & MOUSEEVENTF_MIDDLEDOWN ) {
    if (TRACE_ON(dinput))
      DPRINTF(" MD ");

    GEN_EVENT(This->offset_array[WINE_MOUSE_M_POSITION], 0xFF, xtime, evsequence++);
    This->m_state.rgbButtons[2] = 0xFF;
  }
  if ( dwFlags & MOUSEEVENTF_MIDDLEUP ) {
    if (TRACE_ON(dinput))
      DPRINTF(" MU ");

    GEN_EVENT(This->offset_array[WINE_MOUSE_M_POSITION], 0x00, xtime, evsequence++);
    This->m_state.rgbButtons[2] = 0x00;
  }
  if (TRACE_ON(dinput))
    DPRINTF("\n");
  
  This->prevX = posX;
  This->prevY = posY;

  if (This->absolute) {
    This->m_state.lX = posX;
    This->m_state.lY = posY;
  } else {
    This->m_state.lX = posX - This->win_centerX;
    This->m_state.lY = posY - This->win_centerY;
  }
  
  LeaveCriticalSection(&(This->crit));

}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
  ICOM_THIS(SysMouseAImpl,iface);
  RECT	rect;
  
  TRACE("(this=%p)\n",This);

  if (This->acquired == 0) {
    POINT       point;
    
    /* This stores the current mouse handler. */
    This->prev_handler = mouse_event;
    
    /* Store (in a global variable) the current lock */
    current_lock = (IDirectInputDevice2A*)This;

    /* Init the mouse state */
    This->m_state.lX = PosX;
    This->m_state.lY = PosY;
    This->m_state.rgbButtons[0] = (MouseButtonsStates[0] ? 0xFF : 0x00);
    This->m_state.rgbButtons[1] = (MouseButtonsStates[1] ? 0xFF : 0x00);
    This->m_state.rgbButtons[2] = (MouseButtonsStates[2] ? 0xFF : 0x00);

    /* Install our own mouse event handler */
    MOUSE_Enable(dinput_mouse_event);
    
    /* Get the window dimension and find the center */
    GetWindowRect(This->win, &rect);
    This->win_centerX = (rect.right  - rect.left) / 2;
    This->win_centerY = (rect.bottom - rect.top ) / 2;

    /* Warp the mouse to the center of the window */
    TRACE("Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    point.x = This->win_centerX;
    point.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &point, 1);
    DISPLAY_MoveCursor(point.x, point.y);

    This->acquired = 1;
  }
  return 0;
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p)\n",This);

  /* Reinstall previous mouse event handler */
  MOUSE_Enable(This->prev_handler);
  This->prev_handler = NULL;
  
  /* No more locks */
  current_lock = NULL;

  /* Unacquire device */
  This->acquired = 0;
  
  return 0;
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the mouse.
  *
  *   For the moment, only the "standard" return structure (DIMOUSESTATE) is
  *   supported.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
) {
  ICOM_THIS(SysMouseAImpl,iface);
  
  EnterCriticalSection(&(This->crit));
  TRACE("(this=%p,0x%08lx,%p): \n",This,len,ptr);
  
  /* Copy the current mouse state */
  fill_DataFormat(ptr, &(This->m_state), This->wine_df);
  
  /* Check if we need to do a mouse warping */
  if (This->need_warp) {
    POINT point;

    TRACE("Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    point.x = This->win_centerX;
    point.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &point, 1);
    DISPLAY_MoveCursor(point.x, point.y);

    This->need_warp = 0;
  }

  LeaveCriticalSection(&(This->crit));
  
  TRACE("(X: %ld - Y: %ld   L: %02x M: %02x R: %02x)\n",
	This->m_state.lX, This->m_state.lY,
	This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);
  
  return 0;
}

/******************************************************************************
  *     GetDeviceState : gets buffered input data.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceData(LPDIRECTINPUTDEVICE2A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  ICOM_THIS(SysMouseAImpl,iface);
  
  EnterCriticalSection(&(This->crit));
  TRACE("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx)\n",This,dodsize,*entries,flags);

  if (flags & DIGDD_PEEK)
    FIXME("DIGDD_PEEK\n");

  if (dod == NULL) {
    *entries = This->queue_pos;
    This->queue_pos = 0;
  } else {
    /* Check for buffer overflow */
    if (This->queue_pos > *entries) {
      WARN("Buffer overflow not handled properly yet...\n");
      This->queue_pos = *entries;
    }
    if (dodsize != sizeof(DIDEVICEOBJECTDATA)) {
      ERR("Wrong structure size !\n");
      LeaveCriticalSection(&(This->crit));
      return DIERR_INVALIDPARAM;
    }

    if (This->queue_pos)
    	TRACE("Application retrieving %d event(s).\n", This->queue_pos); 
    
    /* Copy the buffered data into the application queue */
    memcpy(dod, This->data_queue, This->queue_pos * dodsize);
    *entries = This->queue_pos;

    /* Reset the event queue */
    This->queue_pos = 0;
  }
  LeaveCriticalSection(&(This->crit));
  
#if 0 /* FIXME: seems to create motion events, which fire back at us. */
  /* Check if we need to do a mouse warping */
  if (This->need_warp) {
    POINT point;

    TRACE("Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    point.x = This->win_centerX;
    point.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &point, 1);

    DISPLAY_MoveCursor(point.x, point.y);

    This->need_warp = 0;
  }
#endif
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI SysMouseAImpl_SetProperty(LPDIRECTINPUTDEVICE2A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  
  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case (DWORD) DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
      
      TRACE("buffersize = %ld\n",pd->dwData);

      This->data_queue = (LPDIDEVICEOBJECTDATA)HeapAlloc(GetProcessHeap(),0,
							  pd->dwData * sizeof(DIDEVICEOBJECTDATA));
      This->queue_pos  = 0;
      This->queue_len  = pd->dwData;
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
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI SysMouseAImpl_GetProperty(LPDIRECTINPUTDEVICE2A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
  ICOM_THIS(SysMouseAImpl,iface);

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
	  ((pdiph->dwObj == (DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS)) ||
	   (pdiph->dwObj == (DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS)))) {
	/* Querying the range of either the X or the Y axis.  As I do
	   not know the range, do as if the range where
	   unrestricted...*/
	pr->lMin = DIPROPRANGE_NOMIN;
	pr->lMax = DIPROPRANGE_NOMAX;
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
static HRESULT WINAPI SysMouseAImpl_SetEventNotification(LPDIRECTINPUTDEVICE2A iface,
							 HANDLE hnd) {
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);

  This->hEvent = hnd;

  return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capablitites
  */
static HRESULT WINAPI SysMouseAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

  if (lpDIDevCaps->dwSize == sizeof(DIDEVCAPS)) {
    lpDIDevCaps->dwFlags = DIDC_ATTACHED;
    lpDIDevCaps->dwDevType = DIDEVTYPE_MOUSE;
    lpDIDevCaps->dwAxes = 2;
    lpDIDevCaps->dwButtons = 3;
    lpDIDevCaps->dwPOVs = 0;
    lpDIDevCaps->dwFFSamplePeriod = 0;
    lpDIDevCaps->dwFFMinTimeResolution = 0;
    lpDIDevCaps->dwFirmwareRevision = 100;
    lpDIDevCaps->dwHardwareRevision = 100;
    lpDIDevCaps->dwFFDriverVersion = 0;
  } else {
    /* DirectX 3.0 */
    FIXME("DirectX 3.0 not supported....\n");
  }
  
  return DI_OK;
}


/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI SysMouseAImpl_EnumObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  ICOM_THIS(SysMouseAImpl,iface);
  DIDEVICEOBJECTINSTANCE ddoi;
  
  TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
  if (TRACE_ON(dinput)) {
    DPRINTF("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    DPRINTF("\n");
  }

  /* Only the fields till dwFFMaxForce are relevant */
  ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCE, dwFFMaxForce);
    
  /* In a mouse, we have : two relative axis and three buttons */
  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_AXIS)) {
    /* X axis */
    ddoi.guidType = GUID_XAxis;
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_X_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS;
    strcpy(ddoi.tszName, "X-Axis");
    _dump_OBJECTINSTANCE(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    
    /* Y axis */
    ddoi.guidType = GUID_YAxis;
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_Y_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS;
    strcpy(ddoi.tszName, "Y-Axis");
    _dump_OBJECTINSTANCE(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
  }

  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_BUTTON)) {
    ddoi.guidType = GUID_Button;

    /* Left button */
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_L_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_L_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
    strcpy(ddoi.tszName, "Left-Button");
    _dump_OBJECTINSTANCE(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;

    /* Right button */
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_R_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_R_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
    strcpy(ddoi.tszName, "Right-Button");
    _dump_OBJECTINSTANCE(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;

    /* Middle button */
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_M_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_M_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
    strcpy(ddoi.tszName, "Middle-Button");
    _dump_OBJECTINSTANCE(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
  }

  return DI_OK;
}
	


#ifdef HAVE_LINUX_22_JOYSTICK_API
/******************************************************************************
 *	Joystick
 */
static ULONG WINAPI JoystickAImpl_Release(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(JoystickAImpl,iface);

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
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df
)
{
  ICOM_THIS(JoystickAImpl,iface);
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
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
    ICOM_THIS(JoystickAImpl,iface);
  
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
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
    ICOM_THIS(JoystickAImpl,iface);

    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1) {
  	close(This->joyfd);
	This->joyfd = -1;
    }
    return 0;
}

#define map_axis(val) ((val+32768)*(This->lMax-This->lMin)/65536+This->lMin)

static void joy_polldev(JoystickAImpl *This) {
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
	    GEN_EVENT(DIJOFS_BUTTON(jse.number),jse.value?0x80:0x00,jse.time,evsequence++);
	    This->js.rgbButtons[jse.number] = jse.value?0x80:0x00;
	}
	if (jse.type & JS_EVENT_AXIS) {
	    switch (jse.number) {
	    case 0:
		GEN_EVENT(jse.number*4,jse.value,jse.time,evsequence++);
		This->js.lX = map_axis(jse.value);
		break;
	    case 1:
		GEN_EVENT(jse.number*4,jse.value,jse.time,evsequence++);
		This->js.lY = map_axis(jse.value);
		break;
	    case 2:
		GEN_EVENT(jse.number*4,jse.value,jse.time,evsequence++);
		This->js.lZ = map_axis(jse.value);
		break;
	    default:
		FIXME("more then 3 axes (%d) not handled!\n",jse.number);
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
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
) {
    ICOM_THIS(JoystickAImpl,iface);
  
    joy_polldev(This);
    TRACE("(this=%p,0x%08lx,%p)\n",This,len,ptr);
    if (len != sizeof(DIJOYSTATE)) {
    	FIXME("len %ld is not sizeof(DIJOYSTATE), unsupported format.\n",len);
    }
    memcpy(ptr,&(This->js),len);
    This->queue_pos = 0;
    return 0;
}

/******************************************************************************
  *     GetDeviceState : gets buffered input data.
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceData(LPDIRECTINPUTDEVICE2A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  ICOM_THIS(JoystickAImpl,iface);
  
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
static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE2A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  ICOM_THIS(JoystickAImpl,iface);

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
	LPDIRECTINPUTDEVICE2A iface, HANDLE hnd
) {
    ICOM_THIS(JoystickAImpl,iface);

    TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);
    This->hEvent = hnd;
    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    ICOM_THIS(JoystickAImpl,iface);
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
static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE2A iface) {
    ICOM_THIS(JoystickAImpl,iface);
    TRACE("(),stub!\n");

    joy_polldev(This);
    return DI_OK;
}

/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI JoystickAImpl_EnumObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  ICOM_THIS(JoystickAImpl,iface);
  DIDEVICEOBJECTINSTANCE ddoi;
  int xfd = This->joyfd;

  TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
  if (TRACE_ON(dinput)) {
    DPRINTF("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    DPRINTF("\n");
  }

  /* Only the fields till dwFFMaxForce are relevant */
  ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCE, dwFFMaxForce);
    
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
      _dump_OBJECTINSTANCE(&ddoi);
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
      _dump_OBJECTINSTANCE(&ddoi);
      if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }
  }

  if (xfd!=This->joyfd)
    close(xfd);

  return DI_OK;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE2A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
  ICOM_THIS(JoystickAImpl,iface);

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

#endif

/****************************************************************************/
/****************************************************************************/

static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	IDirectInputDevice2AImpl_Release,
	SysKeyboardAImpl_GetCapabilities,
	IDirectInputDevice2AImpl_EnumObjects,
	IDirectInputDevice2AImpl_GetProperty,
	SysKeyboardAImpl_SetProperty,
	SysKeyboardAImpl_Acquire,
	SysKeyboardAImpl_Unacquire,
	SysKeyboardAImpl_GetDeviceState,
	SysKeyboardAImpl_GetDeviceData,
	IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
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
	IDirectInputDevice2AImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
};

static ICOM_VTABLE(IDirectInputDevice2A) SysMouseAvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	SysMouseAImpl_Release,
	SysMouseAImpl_GetCapabilities,
	SysMouseAImpl_EnumObjects,
	SysMouseAImpl_GetProperty,
	SysMouseAImpl_SetProperty,
	SysMouseAImpl_Acquire,
	SysMouseAImpl_Unacquire,
	SysMouseAImpl_GetDeviceState,
	SysMouseAImpl_GetDeviceData,
	SysMouseAImpl_SetDataFormat,
	SysMouseAImpl_SetEventNotification,
	SysMouseAImpl_SetCooperativeLevel,
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
	IDirectInputDevice2AImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
};

#ifdef HAVE_LINUX_22_JOYSTICK_API
static ICOM_VTABLE(IDirectInputDevice2A) JoystickAvt = 
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
};
#endif
