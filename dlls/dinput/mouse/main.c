/*		DirectInput Mouse device
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

#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "wine/debug.h"

#define MOUSE_HACK

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine mouse driver object instances */
#define WINE_MOUSE_X_AXIS_INSTANCE 0x0001
#define WINE_MOUSE_Y_AXIS_INSTANCE 0x0002
#define WINE_MOUSE_Z_AXIS_INSTANCE 0x0004
#define WINE_MOUSE_L_BUTTON_INSTANCE 0x0008
#define WINE_MOUSE_R_BUTTON_INSTANCE 0x0010
#define WINE_MOUSE_M_BUTTON_INSTANCE 0x0020

/* ------------------------------- */
/* Wine mouse internal data format */
/* ------------------------------- */

/* Constants used to access the offset array */
#define WINE_MOUSE_X_POSITION 0
#define WINE_MOUSE_Y_POSITION 1
#define WINE_MOUSE_Z_POSITION 2
#define WINE_MOUSE_L_POSITION 3
#define WINE_MOUSE_R_POSITION 4
#define WINE_MOUSE_M_POSITION 5

typedef struct {
  LONG lX;
  LONG lY;
  LONG lZ;
  BYTE rgbButtons[4];
} Wine_InternalMouseData;

#define WINE_INTERNALMOUSE_NUM_OBJS 6

static DIOBJECTDATAFORMAT Wine_InternalMouseObjectFormat[WINE_INTERNALMOUSE_NUM_OBJS] = {
  { &GUID_XAxis,   FIELD_OFFSET(Wine_InternalMouseData, lX),
      DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
  { &GUID_YAxis,   FIELD_OFFSET(Wine_InternalMouseData, lY),
      DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
  { &GUID_ZAxis,   FIELD_OFFSET(Wine_InternalMouseData, lZ),
      DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
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

static ICOM_VTABLE(IDirectInputDevice8A) SysMouseAvt;
typedef struct SysMouseAImpl SysMouseAImpl;

typedef enum {
  WARP_NEEDED,  /* Warping is needed */
  WARP_STARTED, /* Warping has been done, waiting for the warp event */
  WARP_DONE     /* Warping has been done */
} WARP_STATUS;

struct SysMouseAImpl
{
        LPVOID                          lpVtbl;
        DWORD                           ref;
        GUID                            guid;

	IDirectInputAImpl *dinput;

	/* The current data format and the conversion between internal
	   and external data formats */
	LPDIDATAFORMAT			df;
	DataFormat                     *wine_df;
        int                             offset_array[WINE_INTERNALMOUSE_NUM_OBJS];

        /* SysMouseAImpl */
        BYTE                            absolute;
        /* Previous position for relative moves */
        LONG				prevX, prevY;
        HHOOK                           hook;
        HWND				win;
        DWORD				dwCoopLevel;
        POINT      			mapped_center;
        DWORD				win_centerX, win_centerY;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_head, queue_tail, queue_len;
	/* warping: whether we need to move mouse back to middle once we
	 * reach window borders (for e.g. shooters, "surface movement" games) */
        WARP_STATUS		        need_warp;
        int				acquired;
        HANDLE				hEvent;
	CRITICAL_SECTION		crit;

	/* This is for mouse reporting. */
	Wine_InternalMouseData          m_state;
};

/* FIXME: This is ugly and not thread safe :/ */
static IDirectInputDevice8A* current_lock = NULL;

static GUID DInput_Wine_Mouse_GUID = { /* 9e573ed8-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed8,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static void fill_mouse_dideviceinstancea(LPDIDEVICEINSTANCEA lpddi) {
    DWORD dwSize;
    DIDEVICEINSTANCEA ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%ld %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysMouse;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
    ddi.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_UNKNOWN << 8);
    strcpy(ddi.tszInstanceName, "Mouse");
    strcpy(ddi.tszProductName, "Wine Mouse");

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static BOOL mousedev_enum_device(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi)
{
  if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_MOUSE)) {
    TRACE("Enumerating the mouse device\n");

    fill_mouse_dideviceinstancea(lpddi);

    return TRUE;
  }

  return FALSE;
}

static SysMouseAImpl *alloc_device(REFGUID rguid, LPVOID mvt, IDirectInputAImpl *dinput)
{
    int offset_array[WINE_INTERNALMOUSE_NUM_OBJS] = {
      FIELD_OFFSET(Wine_InternalMouseData, lX),
      FIELD_OFFSET(Wine_InternalMouseData, lY),
      FIELD_OFFSET(Wine_InternalMouseData, lZ),
      FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 0,
      FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 1,
      FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 2
    };
    SysMouseAImpl* newDevice;
    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseAImpl));
    newDevice->ref = 1;
    newDevice->lpVtbl = mvt;
    InitializeCriticalSection(&(newDevice->crit));
    memcpy(&(newDevice->guid),rguid,sizeof(*rguid));

    /* Per default, Wine uses its internal data format */
    newDevice->df = &Wine_InternalMouseFormat;
    memcpy(newDevice->offset_array, offset_array, WINE_INTERNALMOUSE_NUM_OBJS * sizeof(int));
    newDevice->wine_df = (DataFormat *) HeapAlloc(GetProcessHeap(), 0, sizeof(DataFormat));
    newDevice->wine_df->size = 0;
    newDevice->wine_df->internal_format_size = Wine_InternalMouseFormat.dwDataSize;
    newDevice->wine_df->dt = NULL;
    newDevice->dinput = dinput;

    return newDevice;
}

static HRESULT mousedev_create_device(IDirectInputAImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  if ((IsEqualGUID(&GUID_SysMouse,rguid)) ||             /* Generic Mouse */
      (IsEqualGUID(&DInput_Wine_Mouse_GUID,rguid))) { /* Wine Mouse */
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
      *pdev=(IDirectInputDeviceA*) alloc_device(rguid, &SysMouseAvt, dinput);
      TRACE("Creating a Mouse device (%p)\n", *pdev);
      return DI_OK;
    } else
      return DIERR_NOINTERFACE;
  }

  return DIERR_DEVICENOTREG;
}

static dinput_device mousedev = {
  100,
  mousedev_enum_device,
  mousedev_create_device
};

DECL_GLOBAL_CONSTRUCTOR(mousedev_register) { dinput_register_device(&mousedev); }

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

/******************************************************************************
  *     Release : release the mouse buffer.
  */
static ULONG WINAPI SysMouseAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
	ICOM_THIS(SysMouseAImpl,iface);

	This->ref--;
	if (This->ref)
		return This->ref;

	/* Free the data queue */
	if (This->data_queue != NULL)
	  HeapFree(GetProcessHeap(),0,This->data_queue);

        if (This->hook) {
          UnhookWindowsHookEx( This->hook );
          if (This->dwCoopLevel & DISCL_EXCLUSIVE)
            ShowCursor(TRUE); /* show cursor */
        }
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
	LPDIRECTINPUTDEVICE8A iface,HWND hwnd,DWORD dwflags
)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,0x%08lx,0x%08lx)\n",This,(DWORD)hwnd,dwflags);

  if (TRACE_ON(dinput)) {
      TRACE(" cooperative level : ");
      _dump_cooperativelevel_DI(dwflags);
  }

  /* Store the window which asks for the mouse */
  if (!hwnd)
    hwnd = GetDesktopWindow();
  This->win = hwnd;
  This->dwCoopLevel = dwflags;

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
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df
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

  /* Tests under windows show that a call to SetDataFormat always sets the mouse
     in relative mode whatever the dwFlags value (DIDF_ABSAXIS/DIDF_RELAXIS).
     To switch in absolute mode, SetProperty must be used. */
  This->absolute = 0;

  /* Store the new data format */
  This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
  memcpy(This->df, df, df->dwSize);
  This->df->rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
  memcpy(This->df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);

  /* Prepare all the data-conversion filters */
  This->wine_df = create_DataFormat(&(Wine_InternalMouseFormat), df, This->offset_array);

  return 0;
}

/* low-level mouse hook */
static LRESULT CALLBACK dinput_mouse_hook( int code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    SysMouseAImpl* This = (SysMouseAImpl*) current_lock;
    DWORD dwCoop;
    static long last_event = 0;
    int wdata;

    if (code != HC_ACTION) return CallNextHookEx( This->hook, code, wparam, lparam );

    EnterCriticalSection(&(This->crit));
    dwCoop = This->dwCoopLevel;

    /* Only allow mouse events every 10 ms.
     * This is to allow the cursor to start acceleration before
     * the warps happen. But if it involves a mouse button event we
     * allow it since we dont want to loose the clicks.
     */
    if (((GetCurrentTime() - last_event) < 10)
        && wparam == WM_MOUSEMOVE)
      goto end;
    else last_event = GetCurrentTime();

    /* Mouse moved -> send event if asked */
    if (This->hEvent)
        SetEvent(This->hEvent);

    if (wparam == WM_MOUSEMOVE) {
	if (This->absolute) {
	  if (hook->pt.x != This->prevX)
	    GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], hook->pt.x, hook->time, 0);
	  if (hook->pt.y != This->prevY)
	    GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], hook->pt.y, hook->time, 0);
	} else {
	  /* Now, warp handling */
	  if ((This->need_warp == WARP_STARTED) &&
	      (hook->pt.x == This->mapped_center.x) && (hook->pt.y == This->mapped_center.y)) {
	    /* Warp has been done... */
	    This->need_warp = WARP_DONE;
	    goto end;
	  }

	  /* Relative mouse input with absolute mouse event : the real fun starts here... */
	  if ((This->need_warp == WARP_NEEDED) ||
	      (This->need_warp == WARP_STARTED)) {
	    if (hook->pt.x != This->prevX)
	      GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], hook->pt.x - This->prevX, hook->time, (This->dinput->evsequence)++);
	    if (hook->pt.y != This->prevY)
	      GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], hook->pt.y - This->prevY, hook->time, (This->dinput->evsequence)++);
	  } else {
	    /* This is the first time the event handler has been called after a
	       GetDeviceData or GetDeviceState. */
	    if (hook->pt.x != This->mapped_center.x) {
	      GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], hook->pt.x - This->mapped_center.x, hook->time, (This->dinput->evsequence)++);
	      This->need_warp = WARP_NEEDED;
	    }

	    if (hook->pt.y != This->mapped_center.y) {
	      GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], hook->pt.y - This->mapped_center.y, hook->time, (This->dinput->evsequence)++);
	      This->need_warp = WARP_NEEDED;
	    }
	  }
	}

	This->prevX = hook->pt.x;
	This->prevY = hook->pt.y;

	if (This->absolute) {
	  This->m_state.lX = hook->pt.x;
	  This->m_state.lY = hook->pt.y;
	} else {
	  This->m_state.lX = hook->pt.x - This->mapped_center.x;
	  This->m_state.lY = hook->pt.y - This->mapped_center.y;
	}
    }

    TRACE(" msg %x pt %ld %ld (W=%d)\n",
          wparam, hook->pt.x, hook->pt.y, (!This->absolute) && This->need_warp );

    switch(wparam)
    {
    case WM_LBUTTONDOWN:
        GEN_EVENT(This->offset_array[WINE_MOUSE_L_POSITION], 0xFF,
                  hook->time, This->dinput->evsequence++);
        This->m_state.rgbButtons[0] = 0xFF;
        break;
    case WM_LBUTTONUP:
        GEN_EVENT(This->offset_array[WINE_MOUSE_L_POSITION], 0x00,
                  hook->time, This->dinput->evsequence++);
        This->m_state.rgbButtons[0] = 0x00;
        break;
    case WM_RBUTTONDOWN:
        GEN_EVENT(This->offset_array[WINE_MOUSE_R_POSITION], 0xFF,
                  hook->time, This->dinput->evsequence++);
        This->m_state.rgbButtons[1] = 0xFF;
        break;
    case WM_RBUTTONUP:
        GEN_EVENT(This->offset_array[WINE_MOUSE_R_POSITION], 0x00,
                  hook->time, This->dinput->evsequence++);
        This->m_state.rgbButtons[1] = 0x00;
        break;
    case WM_MBUTTONDOWN:
        GEN_EVENT(This->offset_array[WINE_MOUSE_M_POSITION], 0xFF,
                  hook->time, This->dinput->evsequence++);
        This->m_state.rgbButtons[2] = 0xFF;
        break;
    case WM_MBUTTONUP:
        GEN_EVENT(This->offset_array[WINE_MOUSE_M_POSITION], 0x00,
                  hook->time, This->dinput->evsequence++);
        This->m_state.rgbButtons[2] = 0x00;
        break;
    case WM_MOUSEWHEEL:
        wdata = (short)HIWORD(hook->mouseData);
        GEN_EVENT(This->offset_array[WINE_MOUSE_Z_POSITION], wdata,
                  hook->time, This->dinput->evsequence++);
        This->m_state.lZ += wdata;
        break;
    }

  TRACE("(X: %ld - Y: %ld   L: %02x M: %02x R: %02x)\n",
	This->m_state.lX, This->m_state.lY,
	This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);

end:
  LeaveCriticalSection(&(This->crit));

  if (dwCoop & DISCL_NONEXCLUSIVE)
  { /* pass the events down to previous handlers (e.g. win32 input) */
      ret = CallNextHookEx( This->hook, code, wparam, lparam );
  }
  else ret = 1;  /* ignore message */
  return ret;
}


static void dinput_window_check(SysMouseAImpl* This)
{
  RECT rect;
  DWORD centerX, centerY;

  /* make sure the window hasn't moved */
  GetWindowRect(This->win, &rect);
  centerX = (rect.right  - rect.left) / 2;
  centerY = (rect.bottom - rect.top ) / 2;
  if (This->win_centerX != centerX || This->win_centerY != centerY) {
    This->win_centerX = centerX;
    This->win_centerY = centerY;
  }
  This->mapped_center.x = This->win_centerX;
  This->mapped_center.y = This->win_centerY;
  MapWindowPoints(This->win, HWND_DESKTOP, &This->mapped_center, 1);
}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
  ICOM_THIS(SysMouseAImpl,iface);
  RECT	rect;

  TRACE("(this=%p)\n",This);

  if (This->acquired == 0) {
    POINT point;

    /* Store (in a global variable) the current lock */
    current_lock = (IDirectInputDevice8A*)This;

    /* Init the mouse state */
    if (This->absolute) {
      GetCursorPos( &point );
      This->m_state.lX = point.x;
      This->m_state.lY = point.y;
      This->prevX = point.x;
      This->prevY = point.y;
    } else {
      This->m_state.lX = 0;
      This->m_state.lY = 0;
    }
    This->m_state.lZ = 0;
    This->m_state.rgbButtons[0] = (GetKeyState(VK_LBUTTON) ? 0xFF : 0x00);
    This->m_state.rgbButtons[1] = (GetKeyState(VK_MBUTTON) ? 0xFF : 0x00);
    This->m_state.rgbButtons[2] = (GetKeyState(VK_RBUTTON) ? 0xFF : 0x00);

    /* Install our mouse hook */
    if (This->dwCoopLevel & DISCL_EXCLUSIVE)
      ShowCursor(FALSE); /* hide cursor */
    This->hook = SetWindowsHookExA( WH_MOUSE_LL, dinput_mouse_hook, DINPUT_instance, 0 );

    /* Get the window dimension and find the center */
    GetWindowRect(This->win, &rect);
    This->win_centerX = (rect.right  - rect.left) / 2;
    This->win_centerY = (rect.bottom - rect.top ) / 2;

    /* Warp the mouse to the center of the window */
    if (This->absolute == 0) {
      This->mapped_center.x = This->win_centerX;
      This->mapped_center.y = This->win_centerY;
      MapWindowPoints(This->win, HWND_DESKTOP, &This->mapped_center, 1);
      TRACE("Warping mouse to %ld - %ld\n", This->mapped_center.x, This->mapped_center.y);
      SetCursorPos( This->mapped_center.x, This->mapped_center.y );
#ifdef MOUSE_HACK
      This->need_warp = WARP_DONE;
#else
      This->need_warp = WARP_STARTED;
#endif
    }

    This->acquired = 1;
    return DI_OK;
  }
  return S_FALSE;
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    ICOM_THIS(SysMouseAImpl,iface);

    TRACE("(this=%p)\n",This);

    if (This->acquired)
    {
        /* Reinstall previous mouse event handler */
        if (This->hook) {
          UnhookWindowsHookEx( This->hook );
          This->hook = 0;
          if (This->dwCoopLevel & DISCL_EXCLUSIVE)
            ShowCursor(TRUE); /* show cursor */
        }

        /* No more locks */
        current_lock = NULL;

        /* Unacquire device */
        This->acquired = 0;
    }
    else
	ERR("Unacquiring a not-acquired device !!!\n");

    return DI_OK;
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the mouse.
  *
  *   For the moment, only the "standard" return structure (DIMOUSESTATE) is
  *   supported.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE8A iface,DWORD len,LPVOID ptr
) {
  ICOM_THIS(SysMouseAImpl,iface);

  EnterCriticalSection(&(This->crit));
  TRACE("(this=%p,0x%08lx,%p): \n",This,len,ptr);

  /* Copy the current mouse state */
  fill_DataFormat(ptr, &(This->m_state), This->wine_df);

  /* Initialize the buffer when in relative mode */
  if (This->absolute == 0) {
    This->m_state.lX = 0;
    This->m_state.lY = 0;
    This->m_state.lZ = 0;
  }

  /* Check if we need to do a mouse warping */
  if (This->need_warp == WARP_NEEDED) {
    dinput_window_check(This);
    TRACE("Warping mouse to %ld - %ld\n", This->mapped_center.x, This->mapped_center.y);
    SetCursorPos( This->mapped_center.x, This->mapped_center.y );

#ifdef MOUSE_HACK
    This->need_warp = WARP_DONE;
#else
    This->need_warp = WARP_STARTED;
#endif
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
static HRESULT WINAPI SysMouseAImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  ICOM_THIS(SysMouseAImpl,iface);
  DWORD len, nqtail;

  TRACE("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx)\n",This,dodsize,*entries,flags);

  if (This->acquired == 0) {
      WARN(" application tries to get data from an unacquired device !\n");
      return DIERR_NOTACQUIRED;
  }
  
  EnterCriticalSection(&(This->crit));

  len = ((This->queue_head < This->queue_tail) ? This->queue_len : 0)
      + (This->queue_head - This->queue_tail);
  if (len > *entries) len = *entries;

  if (dod == NULL) {
    if (len)
    	TRACE("Application discarding %ld event(s).\n", len);

    *entries = len;
    nqtail = This->queue_tail + len;
    while (nqtail >= This->queue_len) nqtail -= This->queue_len;
  } else {
    if (dodsize < sizeof(DIDEVICEOBJECTDATA)) {
      ERR("Wrong structure size !\n");
      LeaveCriticalSection(&(This->crit));
      return DIERR_INVALIDPARAM;
    }

    if (len)
    	TRACE("Application retrieving %ld event(s).\n", len);

    *entries = 0;
    nqtail = This->queue_tail;
    while (len) {
      DWORD span = ((This->queue_head < nqtail) ? This->queue_len : This->queue_head)
                 - nqtail;
      if (span > len) span = len;
      /* Copy the buffered data into the application queue */
      memcpy(dod + *entries, This->data_queue + nqtail, span * dodsize);
      /* Advance position */
      nqtail += span;
      if (nqtail >= This->queue_len) nqtail -= This->queue_len;
      *entries += span;
      len -= span;
    }
  }
  if (!(flags & DIGDD_PEEK))
    This->queue_tail = nqtail;

  LeaveCriticalSection(&(This->crit));

  /* Check if we need to do a mouse warping */
  if (This->need_warp == WARP_NEEDED) {
    dinput_window_check(This);
    TRACE("Warping mouse to %ld - %ld\n", This->mapped_center.x, This->mapped_center.y);
    SetCursorPos( This->mapped_center.x, This->mapped_center.y );

#ifdef MOUSE_HACK
    This->need_warp = WARP_DONE;
#else
    This->need_warp = WARP_STARTED;
#endif
  }
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI SysMouseAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
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
      This->queue_head = 0;
      This->queue_tail = 0;
      This->queue_len  = pd->dwData;
      break;
    }
    case (DWORD) DIPROP_AXISMODE: {
      LPCDIPROPDWORD    pd = (LPCDIPROPDWORD)ph;
      This->absolute = !(pd->dwData);
      TRACE("Using %s coordinates mode now\n", This->absolute ? "absolute" : "relative");
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
static HRESULT WINAPI SysMouseAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
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

    case (DWORD) DIPROP_GRANULARITY: {
      LPDIPROPDWORD pr = (LPDIPROPDWORD) pdiph;

      /* We'll just assume that the app asks about the Z axis */
      pr->dwData = WHEEL_DELTA;

      break;
    }

    case (DWORD) DIPROP_RANGE: {
      LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph;

      if ((pdiph->dwHow == DIPH_BYID) &&
	  ((pdiph->dwObj == (DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS)) ||
	   (pdiph->dwObj == (DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS)))) {
	/* Querying the range of either the X or the Y axis.  As I do
	   not know the range, do as if the range were
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
static HRESULT WINAPI SysMouseAImpl_SetEventNotification(LPDIRECTINPUTDEVICE8A iface,
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
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

  if (lpDIDevCaps->dwSize == sizeof(DIDEVCAPS)) {
    lpDIDevCaps->dwFlags = DIDC_ATTACHED;
    lpDIDevCaps->dwDevType = DIDEVTYPE_MOUSE;
    lpDIDevCaps->dwAxes = 3;
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
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  ICOM_THIS(SysMouseAImpl,iface);
  DIDEVICEOBJECTINSTANCEA ddoi;

  TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
  if (TRACE_ON(dinput)) {
    TRACE("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    TRACE("\n");
  }

  /* Only the fields till dwFFMaxForce are relevant */
  memset(&ddoi, 0, sizeof(ddoi));
  ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEA, dwFFMaxForce);

  /* In a mouse, we have : two relative axis and three buttons */
  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_AXIS)) {
    /* X axis */
    ddoi.guidType = GUID_XAxis;
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_X_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS;
    strcpy(ddoi.tszName, "X-Axis");
    _dump_OBJECTINSTANCEA(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;

    /* Y axis */
    ddoi.guidType = GUID_YAxis;
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_Y_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS;
    strcpy(ddoi.tszName, "Y-Axis");
    _dump_OBJECTINSTANCEA(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;

    /* Z axis */
    ddoi.guidType = GUID_ZAxis;
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_Z_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS;
    strcpy(ddoi.tszName, "Z-Axis");
    _dump_OBJECTINSTANCEA(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
  }

  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_BUTTON)) {
    ddoi.guidType = GUID_Button;

    /* Left button */
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_L_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_L_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
    strcpy(ddoi.tszName, "Left-Button");
    _dump_OBJECTINSTANCEA(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;

    /* Right button */
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_R_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_R_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
    strcpy(ddoi.tszName, "Right-Button");
    _dump_OBJECTINSTANCEA(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;

    /* Middle button */
    ddoi.dwOfs = This->offset_array[WINE_MOUSE_M_POSITION];
    ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_M_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
    strcpy(ddoi.tszName, "Middle-Button");
    _dump_OBJECTINSTANCEA(&ddoi);
    if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
  }

  return DI_OK;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
    ICOM_THIS(SysMouseAImpl,iface);
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)) {
        WARN(" dinput3 not supporte yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstancea(pdidi);
    
    return DI_OK;
}

static ICOM_VTABLE(IDirectInputDevice8A) SysMouseAvt =
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
	SysMouseAImpl_GetDeviceInfo,
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
        IDirectInputDevice7AImpl_EnumEffectsInFile,
        IDirectInputDevice7AImpl_WriteEffectToFile,
        IDirectInputDevice8AImpl_BuildActionMap,
        IDirectInputDevice8AImpl_SetActionMap,
        IDirectInputDevice8AImpl_GetImageInfo
};
