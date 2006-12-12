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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define MOUSE_HACK

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine mouse driver object instances */
#define WINE_MOUSE_X_AXIS_INSTANCE   0
#define WINE_MOUSE_Y_AXIS_INSTANCE   1
#define WINE_MOUSE_Z_AXIS_INSTANCE   2
#define WINE_MOUSE_L_BUTTON_INSTANCE 0
#define WINE_MOUSE_R_BUTTON_INSTANCE 1
#define WINE_MOUSE_M_BUTTON_INSTANCE 2
#define WINE_MOUSE_D_BUTTON_INSTANCE 3

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

static const DIOBJECTDATAFORMAT Wine_InternalMouseObjectFormat[WINE_INTERNALMOUSE_NUM_OBJS] = {
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

static const DIDATAFORMAT Wine_InternalMouseFormat = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_RELAXIS,
    sizeof(Wine_InternalMouseData),
    WINE_INTERNALMOUSE_NUM_OBJS, /* dwNumObjs */
    (LPDIOBJECTDATAFORMAT) Wine_InternalMouseObjectFormat
};

static const IDirectInputDevice8AVtbl SysMouseAvt;
static const IDirectInputDevice8WVtbl SysMouseWvt;

typedef struct SysMouseImpl SysMouseImpl;

typedef enum {
    WARP_DONE,   /* Warping has been done */
    WARP_NEEDED, /* Warping is needed */
    WARP_STARTED /* Warping has been done, waiting for the warp event */
} WARP_STATUS;

struct SysMouseImpl
{
    struct IDirectInputDevice2AImpl base;
    
    IDirectInputImpl               *dinput;
    
    /* SysMouseAImpl */
    BYTE                            absolute;
    /* Previous position for relative moves */
    LONG			    prevX, prevY;
    /* These are used in case of relative -> absolute transitions */
    POINT                           org_coords;
    POINT      			    mapped_center;
    DWORD			    win_centerX, win_centerY;
    /* warping: whether we need to move mouse back to middle once we
     * reach window borders (for e.g. shooters, "surface movement" games) */
    WARP_STATUS		            need_warp;
    DWORD                           last_warped;
    
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

static void fill_mouse_dideviceinstanceA(LPDIDEVICEINSTANCEA lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEA ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysMouse;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
        ddi.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    strcpy(ddi.tszInstanceName, "Mouse");
    strcpy(ddi.tszProductName, "Wine Mouse");

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static void fill_mouse_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEW ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysMouse;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
        ddi.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    MultiByteToWideChar(CP_ACP, 0, "Mouse", -1, ddi.tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, "Wine Mouse", -1, ddi.tszProductName, MAX_PATH);

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static BOOL mousedev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    if (id != 0)
        return FALSE;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceA(lpddi, version);
	
	return TRUE;
    }
    
    return FALSE;
}

static BOOL mousedev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    if (id != 0)
        return FALSE;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceW(lpddi, version);
	
	return TRUE;
    }
    
    return FALSE;
}

static SysMouseImpl *alloc_device(REFGUID rguid, const void *mvt, IDirectInputImpl *dinput)
{
    SysMouseImpl* newDevice;

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseImpl));
    if (!newDevice) return NULL;
    newDevice->base.lpVtbl = mvt;
    newDevice->base.ref = 1;
    newDevice->base.dwCoopLevel = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
    memcpy(&newDevice->base.guid, rguid, sizeof(*rguid));
    InitializeCriticalSection(&newDevice->base.crit);
    newDevice->dinput = dinput;

    newDevice->base.data_format.wine_df = &Wine_InternalMouseFormat;
    if (create_DataFormat(&Wine_InternalMouseFormat, &Wine_InternalMouseFormat, &newDevice->base.data_format) == DI_OK)
        return newDevice;

    return NULL;
}

static HRESULT mousedev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
    if ((IsEqualGUID(&GUID_SysMouse,rguid)) ||             /* Generic Mouse */
	(IsEqualGUID(&DInput_Wine_Mouse_GUID,rguid))) { /* Wine Mouse */
	if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
	    *pdev = (IDirectInputDeviceA*) alloc_device(rguid, &SysMouseAvt, dinput);
	    TRACE("Creating a Mouse device (%p)\n", *pdev);
            if (!*pdev) return DIERR_OUTOFMEMORY;
	    return DI_OK;
	} else
	    return DIERR_NOINTERFACE;
    }
    
    return DIERR_DEVICENOTREG;
}

static HRESULT mousedev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
    if ((IsEqualGUID(&GUID_SysMouse,rguid)) ||             /* Generic Mouse */
	(IsEqualGUID(&DInput_Wine_Mouse_GUID,rguid))) { /* Wine Mouse */
	if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceW,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2W,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7W,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
	    *pdev = (IDirectInputDeviceW*) alloc_device(rguid, &SysMouseWvt, dinput);
	    TRACE("Creating a Mouse device (%p)\n", *pdev);
            if (!*pdev) return DIERR_OUTOFMEMORY;
	    return DI_OK;
	} else
	    return DIERR_NOINTERFACE;
    }
    
    return DIERR_DEVICENOTREG;
}

const struct dinput_device mouse_device = {
    "Wine mouse driver",
    mousedev_enum_deviceA,
    mousedev_enum_deviceW,
    mousedev_create_deviceA,
    mousedev_create_deviceW
};

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

/******************************************************************************
  *     Release : release the mouse buffer.
  */
static ULONG WINAPI SysMouseAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    ULONG ref;
 
    ref = InterlockedDecrement(&This->base.ref);
    if (ref)
	return ref;

    set_dinput_hook(WH_MOUSE_LL, NULL);

    /* Free the data queue */
    HeapFree(GetProcessHeap(), 0, This->base.data_queue);

    release_DataFormat(&This->base.data_format);

    DeleteCriticalSection(&This->base.crit);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

/* low-level mouse hook */
static LRESULT CALLBACK dinput_mouse_hook( int code, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    SysMouseImpl* This = (SysMouseImpl*) current_lock;
    DWORD dwCoop;
    int wdata;

    if (code != HC_ACTION) return CallNextHookEx( 0, code, wparam, lparam );

    EnterCriticalSection(&This->base.crit);
    dwCoop = This->base.dwCoopLevel;

    if (wparam == WM_MOUSEMOVE) {
	if (This->absolute) {
	    if (hook->pt.x != This->prevX)
                queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_X_POSITION],
                            hook->pt.x, hook->time, This->dinput->evsequence);
	    if (hook->pt.y != This->prevY)
                queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_Y_POSITION],
                            hook->pt.y, hook->time, This->dinput->evsequence);
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
                    queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_X_POSITION],
                                hook->pt.x - This->prevX, hook->time, This->dinput->evsequence);
		if (hook->pt.y != This->prevY)
                    queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_Y_POSITION],
                                hook->pt.y - This->prevY, hook->time, This->dinput->evsequence);
	    } else {
		/* This is the first time the event handler has been called after a
		   GetDeviceData or GetDeviceState. */
		if (hook->pt.x != This->mapped_center.x) {
                    queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_X_POSITION],
                                hook->pt.x - This->mapped_center.x, hook->time, This->dinput->evsequence);
		    This->need_warp = WARP_NEEDED;
		}
		
		if (hook->pt.y != This->mapped_center.y) {
                    queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_Y_POSITION],
                                hook->pt.y - This->mapped_center.y, hook->time, This->dinput->evsequence);
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
    
    TRACE(" msg %x pt %d %d (W=%d)\n",
          wparam, hook->pt.x, hook->pt.y, (!This->absolute) && This->need_warp );
    
    switch(wparam) {
        case WM_LBUTTONDOWN:
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_L_POSITION],
                        0x80, hook->time, This->dinput->evsequence);
	    This->m_state.rgbButtons[0] = 0x80;
	    break;
	case WM_LBUTTONUP:
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_L_POSITION],
                        0x00, hook->time, This->dinput->evsequence);
	    This->m_state.rgbButtons[0] = 0x00;
	    break;
	case WM_RBUTTONDOWN:
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_R_POSITION],
                        0x80, hook->time, This->dinput->evsequence);
	    This->m_state.rgbButtons[1] = 0x80;
	    break;
	case WM_RBUTTONUP:
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_R_POSITION],
                        0x00, hook->time, This->dinput->evsequence);
	    This->m_state.rgbButtons[1] = 0x00;
	    break;
	case WM_MBUTTONDOWN:
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_M_POSITION],
                        0x80, hook->time, This->dinput->evsequence);
	    This->m_state.rgbButtons[2] = 0x80;
	    break;
	case WM_MBUTTONUP:
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_M_POSITION],
                        0x00, hook->time, This->dinput->evsequence);
	    This->m_state.rgbButtons[2] = 0x00;
	    break;
	case WM_MOUSEWHEEL:
	    wdata = (short)HIWORD(hook->mouseData);
            queue_event((LPDIRECTINPUTDEVICE8A)This, This->base.data_format.offsets[WINE_MOUSE_Z_POSITION],
                        wdata, hook->time, This->dinput->evsequence);
	    This->m_state.lZ += wdata;
	    break;
    }
    
    TRACE("(X: %d - Y: %d   L: %02x M: %02x R: %02x)\n",
	  This->m_state.lX, This->m_state.lY,
	  This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);
    
    This->dinput->evsequence++;

  end:
    /* Mouse moved -> send event if asked */
    if (This->base.hEvent) SetEvent(This->base.hEvent);
    
    LeaveCriticalSection(&This->base.crit);
    
    /* Ignore message */
    if (dwCoop & DISCL_EXCLUSIVE) return 1;

    /* Pass the events down to previous handlers (e.g. win32 input) */
    return CallNextHookEx( 0, code, wparam, lparam );
}

static BOOL dinput_window_check(SysMouseImpl* This) {
    RECT rect;
    DWORD centerX, centerY;

    /* make sure the window hasn't moved */
    if(!GetWindowRect(This->base.win, &rect))
        return FALSE;
    centerX = (rect.right  - rect.left) / 2;
    centerY = (rect.bottom - rect.top ) / 2;
    if (This->win_centerX != centerX || This->win_centerY != centerY) {
	This->win_centerX = centerX;
	This->win_centerY = centerY;
    }
    This->mapped_center.x = This->win_centerX;
    This->mapped_center.y = This->win_centerY;
    MapWindowPoints(This->base.win, HWND_DESKTOP, &This->mapped_center, 1);
    return TRUE;
}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    RECT  rect;
    POINT point;
    HRESULT res;
    
    TRACE("(this=%p)\n",This);

    if ((res = IDirectInputDevice2AImpl_Acquire(iface)) != DI_OK) return res;

    /* Store (in a global variable) the current lock */
    current_lock = (IDirectInputDevice8A*)This;
    
    /* Init the mouse state */
    GetCursorPos( &point );
    if (This->absolute) {
      This->m_state.lX = point.x;
      This->m_state.lY = point.y;
      This->prevX = point.x;
      This->prevY = point.y;
    } else {
      This->m_state.lX = 0;
      This->m_state.lY = 0;
      This->org_coords = point;
    }
    This->m_state.lZ = 0;
    This->m_state.rgbButtons[0] = GetKeyState(VK_LBUTTON) & 0x80;
    This->m_state.rgbButtons[1] = GetKeyState(VK_RBUTTON) & 0x80;
    This->m_state.rgbButtons[2] = GetKeyState(VK_MBUTTON) & 0x80;
    
    /* Install our mouse hook */
    if (This->base.dwCoopLevel & DISCL_EXCLUSIVE)
      ShowCursor(FALSE); /* hide cursor */
    set_dinput_hook(WH_MOUSE_LL, dinput_mouse_hook);
    
    /* Get the window dimension and find the center */
    GetWindowRect(This->base.win, &rect);
    This->win_centerX = (rect.right  - rect.left) / 2;
    This->win_centerY = (rect.bottom - rect.top ) / 2;
    
    /* Warp the mouse to the center of the window */
    if (This->absolute == 0) {
      This->mapped_center.x = This->win_centerX;
      This->mapped_center.y = This->win_centerY;
      MapWindowPoints(This->base.win, HWND_DESKTOP, &This->mapped_center, 1);
      TRACE("Warping mouse to %d - %d\n", This->mapped_center.x, This->mapped_center.y);
      SetCursorPos( This->mapped_center.x, This->mapped_center.y );
      This->last_warped = GetCurrentTime();

#ifdef MOUSE_HACK
      This->need_warp = WARP_DONE;
#else
      This->need_warp = WARP_STARTED;
#endif
    }
	
    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    HRESULT res;
    
    TRACE("(this=%p)\n",This);

    if ((res = IDirectInputDevice2AImpl_Unacquire(iface)) != DI_OK) return res;

    set_dinput_hook(WH_MOUSE_LL, NULL);
    if (This->base.dwCoopLevel & DISCL_EXCLUSIVE)
        ShowCursor(TRUE); /* show cursor */

    /* No more locks */
    if (current_lock == (IDirectInputDevice8A*) This)
      current_lock = NULL;
    else
      ERR("this(%p) != current_lock(%p)\n", This, current_lock);

    /* And put the mouse cursor back where it was at acquire time */
    if (This->absolute == 0) {
      TRACE(" warping mouse back to (%d , %d)\n", This->org_coords.x, This->org_coords.y);
      SetCursorPos(This->org_coords.x, This->org_coords.y);
    }
	
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
    SysMouseImpl *This = (SysMouseImpl *)iface;

    if(This->base.acquired == 0) return DIERR_NOTACQUIRED;

    EnterCriticalSection(&This->base.crit);
    TRACE("(this=%p,0x%08x,%p):\n", This, len, ptr);
    TRACE("(X: %d - Y: %d - Z: %d  L: %02x M: %02x R: %02x)\n",
	  This->m_state.lX, This->m_state.lY, This->m_state.lZ,
	  This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);
    
    /* Copy the current mouse state */
    fill_DataFormat(ptr, &(This->m_state), &This->base.data_format);
    
    /* Initialize the buffer when in relative mode */
    if (This->absolute == 0) {
	This->m_state.lX = 0;
	This->m_state.lY = 0;
	This->m_state.lZ = 0;
    }
    
    /* Check if we need to do a mouse warping */
    if (This->need_warp == WARP_NEEDED && (GetCurrentTime() - This->last_warped > 10)) {
        if(!dinput_window_check(This))
        {
            LeaveCriticalSection(&This->base.crit);
            return DIERR_GENERIC;
        }
	TRACE("Warping mouse to %d - %d\n", This->mapped_center.x, This->mapped_center.y);
	SetCursorPos( This->mapped_center.x, This->mapped_center.y );
        This->last_warped = GetCurrentTime();

#ifdef MOUSE_HACK
	This->need_warp = WARP_DONE;
#else
	This->need_warp = WARP_STARTED;
#endif
    }
    
    LeaveCriticalSection(&This->base.crit);
    
    return DI_OK;
}

/******************************************************************************
  *     GetDeviceData : gets buffered input data.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface,
        DWORD dodsize, LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    HRESULT res;

    res = IDirectInputDevice2AImpl_GetDeviceData(iface, dodsize, dod, entries, flags);
    if (FAILED(res)) return res;
    
    /* Check if we need to do a mouse warping */
    if (This->need_warp == WARP_NEEDED && (GetCurrentTime() - This->last_warped > 10)) {
        if(!dinput_window_check(This))
            return DIERR_GENERIC;
	TRACE("Warping mouse to %d - %d\n", This->mapped_center.x, This->mapped_center.y);
	SetCursorPos( This->mapped_center.x, This->mapped_center.y );
        This->last_warped = GetCurrentTime();

#ifdef MOUSE_HACK
	This->need_warp = WARP_DONE;
#else
	This->need_warp = WARP_STARTED;
#endif
    }
    return res;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI SysMouseAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
    
    if (!HIWORD(rguid)) {
	switch (LOWORD(rguid)) {
	    case (DWORD) DIPROP_AXISMODE: {
		LPCDIPROPDWORD    pd = (LPCDIPROPDWORD)ph;
		This->absolute = !(pd->dwData);
		TRACE("Using %s coordinates mode now\n", This->absolute ? "absolute" : "relative");
		break;
	    }
	    default:
                return IDirectInputDevice2AImpl_SetProperty(iface, rguid, ph);
	}
    }
    
    return DI_OK;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI SysMouseAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(%p) %s,%p\n", This, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);
    
    if (!HIWORD(rguid)) {
	switch (LOWORD(rguid)) {
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
                return IDirectInputDevice2AImpl_GetProperty(iface, rguid, pdiph);
        }
    }
    
    return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capablitites
  */
static HRESULT WINAPI SysMouseAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    DIDEVCAPS devcaps;

    TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

    if ((lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) && (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    devcaps.dwSize = lpDIDevCaps->dwSize;
    devcaps.dwFlags = DIDC_ATTACHED;
    if (This->dinput->dwVersion >= 0x0800)
	devcaps.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
	devcaps.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    devcaps.dwAxes = 3;
    devcaps.dwButtons = 3;
    devcaps.dwPOVs = 0;
    devcaps.dwFFSamplePeriod = 0;
    devcaps.dwFFMinTimeResolution = 0;
    devcaps.dwFirmwareRevision = 100;
    devcaps.dwHardwareRevision = 100;
    devcaps.dwFFDriverVersion = 0;

    memcpy(lpDIDevCaps, &devcaps, lpDIDevCaps->dwSize);
    
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
    SysMouseImpl *This = (SysMouseImpl *)iface;
    DIDEVICEOBJECTINSTANCEA ddoi;
    
    TRACE("(this=%p,%p,%p,%08x)\n", This, lpCallback, lpvRef, dwFlags);
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
        ddoi.dwOfs = This->base.data_format.offsets[WINE_MOUSE_X_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS;
	strcpy(ddoi.tszName, "X-Axis");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Y axis */
	ddoi.guidType = GUID_YAxis;
        ddoi.dwOfs = This->base.data_format.offsets[WINE_MOUSE_Y_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS;
	strcpy(ddoi.tszName, "Y-Axis");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Z axis */
	ddoi.guidType = GUID_ZAxis;
        ddoi.dwOfs = This->base.data_format.offsets[WINE_MOUSE_Z_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS;
	strcpy(ddoi.tszName, "Z-Axis");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }

    if ((dwFlags == DIDFT_ALL) ||
	(dwFlags & DIDFT_BUTTON)) {
	ddoi.guidType = GUID_Button;
	
	/* Left button */
        ddoi.dwOfs = This->base.data_format.offsets[WINE_MOUSE_L_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_L_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
	strcpy(ddoi.tszName, "Left-Button");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Right button */
        ddoi.dwOfs = This->base.data_format.offsets[WINE_MOUSE_R_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_R_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
	strcpy(ddoi.tszName, "Right-Button");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Middle button */
        ddoi.dwOfs = This->base.data_format.offsets[WINE_MOUSE_M_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_M_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
	strcpy(ddoi.tszName, "Middle-Button");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }
    
    return DI_OK;
}

static HRESULT WINAPI SysMouseWImpl_EnumObjects(LPDIRECTINPUTDEVICE8W iface, LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback,	LPVOID lpvRef,DWORD dwFlags)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    device_enumobjects_AtoWcb_data data;
    
    data.lpCallBack = lpCallback;
    data.lpvRef = lpvRef;
    
    return SysMouseAImpl_EnumObjects((LPDIRECTINPUTDEVICE8A) This, (LPDIENUMDEVICEOBJECTSCALLBACKA) DIEnumDevicesCallbackAtoW, (LPVOID) &data, dwFlags);
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)) {
        WARN(" dinput3 not supporte yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstanceA(pdidi, This->dinput->dwVersion);
    
    return DI_OK;
}

static HRESULT WINAPI SysMouseWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface, LPDIDEVICEINSTANCEW pdidi)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)) {
        WARN(" dinput3 not supporte yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstanceW(pdidi, This->dinput->dwVersion);
    
    return DI_OK;
}


static const IDirectInputDevice8AVtbl SysMouseAvt =
{
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
    IDirectInputDevice2AImpl_SetDataFormat,
    IDirectInputDevice2AImpl_SetEventNotification,
    IDirectInputDevice2AImpl_SetCooperativeLevel,
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

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(SysMouseWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static const IDirectInputDevice8WVtbl SysMouseWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
    XCAST(Release)SysMouseAImpl_Release,
    XCAST(GetCapabilities)SysMouseAImpl_GetCapabilities,
    SysMouseWImpl_EnumObjects,
    XCAST(GetProperty)SysMouseAImpl_GetProperty,
    XCAST(SetProperty)SysMouseAImpl_SetProperty,
    XCAST(Acquire)SysMouseAImpl_Acquire,
    XCAST(Unacquire)SysMouseAImpl_Unacquire,
    XCAST(GetDeviceState)SysMouseAImpl_GetDeviceState,
    XCAST(GetDeviceData)SysMouseAImpl_GetDeviceData,
    XCAST(SetDataFormat)IDirectInputDevice2AImpl_SetDataFormat,
    XCAST(SetEventNotification)IDirectInputDevice2AImpl_SetEventNotification,
    XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
    IDirectInputDevice2WImpl_GetObjectInfo,
    SysMouseWImpl_GetDeviceInfo,
    XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
    XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
    XCAST(CreateEffect)IDirectInputDevice2AImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    XCAST(GetForceFeedbackState)IDirectInputDevice2AImpl_GetForceFeedbackState,
    XCAST(SendForceFeedbackCommand)IDirectInputDevice2AImpl_SendForceFeedbackCommand,
    XCAST(EnumCreatedEffectObjects)IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
    XCAST(Escape)IDirectInputDevice2AImpl_Escape,
    XCAST(Poll)IDirectInputDevice2AImpl_Poll,
    XCAST(SendDeviceData)IDirectInputDevice2AImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    IDirectInputDevice8WImpl_BuildActionMap,
    IDirectInputDevice8WImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
#undef XCAST
