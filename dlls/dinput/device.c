/*		DirectInput Device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 *
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

/* This file contains all the Device specific functions that can be used as stubs
   by real device implementations.

   It also contains all the helper functions.
*/
#include "config.h"

#include <string.h>
#include "wine/debug.h"
#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "dinput.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/******************************************************************************
 *	Various debugging tools
 */
void _dump_cooperativelevel_DI(DWORD dwFlags) {
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

void _dump_EnumObjects_flags(DWORD dwFlags) {
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

void _dump_DIPROPHEADER(DIPROPHEADER *diph) {
  DPRINTF("  - dwObj = 0x%08lx\n", diph->dwObj);
  DPRINTF("  - dwHow = %s\n",
	  ((diph->dwHow == DIPH_DEVICE) ? "DIPH_DEVICE" :
	   ((diph->dwHow == DIPH_BYOFFSET) ? "DIPH_BYOFFSET" :
	    ((diph->dwHow == DIPH_BYID)) ? "DIPH_BYID" : "unknown")));
}

void _dump_OBJECTINSTANCEA(DIDEVICEOBJECTINSTANCEA *ddoi) {
  if (TRACE_ON(dinput)) {
    DPRINTF("    - enumerating : 0x%08lx - %2ld - 0x%08lx - %s\n",
	    ddoi->guidType.Data1, ddoi->dwOfs, ddoi->dwType, ddoi->tszName);
  }
}

/* Conversion between internal data buffer and external data buffer */
void fill_DataFormat(void *out, void *in, DataFormat *df) {
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

DataFormat *create_DataFormat(DIDATAFORMAT *wine_format, LPCDIDATAFORMAT asked_format, int *offset) {
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

  TRACE("Creating DataTransform : \n");

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
 *	IDirectInputDeviceA
 */

HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df
) {
  int i;
  ICOM_THIS(IDirectInputDevice2AImpl,iface);

  TRACE("(this=%p,%p)\n",This,df);

  TRACE("df.dwSize=%ld\n",df->dwSize);
  TRACE("(df.dwObjsize=%ld)\n",df->dwObjSize);
  TRACE("(df.dwFlags=0x%08lx)\n",df->dwFlags);
  TRACE("(df.dwDataSize=%ld)\n",df->dwDataSize);
  TRACE("(df.dwNumObjs=%ld)\n",df->dwNumObjs);

  for (i=0;i<df->dwNumObjs;i++) {
    TRACE("df.rgodf[%d].guid %s\n",i,debugstr_guid(df->rgodf[i].pguid));
    TRACE("df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
    TRACE("dwType 0x%02x,dwInstance %d\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
    TRACE("df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
  }
  return 0;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE8A iface,HWND hwnd,DWORD dwflags
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	TRACE("(this=%p,0x%08lx,0x%08lx)\n",This,(DWORD)hwnd,dwflags);
	if (TRACE_ON(dinput))
	  _dump_cooperativelevel_DI(dwflags);
	return 0;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface,HANDLE hnd
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	FIXME("(this=%p,0x%08lx): stub\n",This,(DWORD)hnd);
	return 0;
}

ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	This->ref--;
	if (This->ref)
		return This->ref;
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE8A iface,REFIID riid,LPVOID *ppobj
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

ULONG WINAPI IDirectInputDevice2AImpl_AddRef(
	LPDIRECTINPUTDEVICE8A iface)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	return ++This->ref;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8A iface,
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

HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE8A iface,
	REFGUID rguid,
	LPDIPROPHEADER pdiph)
{
	FIXME("(this=%p,%s,%p): stub!\n",
	      iface, debugstr_guid(rguid), pdiph);

	if (TRACE_ON(dinput))
	  _dump_DIPROPHEADER(pdiph);

	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
	FIXME("(this=%p,%p,%ld,0x%08lx): stub!\n",
	      iface, pdidoi, dwObj, dwHow);

	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
	FIXME("(this=%p,%p): stub!\n",
	      iface, pdidi);

	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE8A iface,
	HWND hwndOwner,
	DWORD dwFlags)
{
  FIXME("(this=%p,0x%08x,0x%08lx): stub!\n",
	iface, hwndOwner, dwFlags);

	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE8A iface,
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

HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(
	LPDIRECTINPUTDEVICE8A iface,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter)
{
	FIXME("(this=%p,%s,%p,%p,%p): stub!\n",
	      iface, debugstr_guid(rguid), lpeff, ppdef, pUnkOuter);
	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE8A iface,
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

HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid)
{
	FIXME("(this=%p,%p,%s): stub!\n",
	      iface, lpdei, debugstr_guid(rguid));
	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE8A iface,
	LPDWORD pdwOut)
{
	FIXME("(this=%p,%p): stub!\n",
	      iface, pdwOut);
	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD dwFlags)
{
	FIXME("(this=%p,0x%08lx): stub!\n",
	      iface, dwFlags);
	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE8A iface,
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

HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIEFFESCAPE lpDIEEsc)
{
	FIXME("(this=%p,%p): stub!\n",
	      iface, lpDIEEsc);
	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE8A iface)
{
	/* Because wine devices do not need to be polled, just return DI_NOEFFECT */
	return DI_NOEFFECT;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD cbObjectData,
	LPCDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags)
{
	FIXME("(this=%p,0x%08lx,%p,%p,0x%08lx): stub!\n",
	      iface, cbObjectData, rgdod, pdwInOut, dwFlags);

	return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7AImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8A iface,
							  LPCSTR lpszFileName,
							  LPDIENUMEFFECTSINFILECALLBACK pec,
							  LPVOID pvRef,
							  DWORD dwFlags)
{
  FIXME("(%p)->(%s,%p,%p,%08lx): stub !\n", iface, lpszFileName, pec, pvRef, dwFlags);

  return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7AImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8A iface,
							  LPCSTR lpszFileName,
							  DWORD dwEntries,
							  LPDIFILEEFFECT rgDiFileEft,
							  DWORD dwFlags)
{
  FIXME("(%p)->(%s,%08lx,%p,%08lx): stub !\n", iface, lpszFileName, dwEntries, rgDiFileEft, dwFlags);

  return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
						       LPDIACTIONFORMATA lpdiaf,
						       LPCSTR lpszUserName,
						       DWORD dwFlags)
{
  FIXME("(%p)->(%p,%s,%08lx): stub !\n", iface, lpdiaf, lpszUserName, dwFlags);

  return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
						     LPDIACTIONFORMATA lpdiaf,
						     LPCSTR lpszUserName,
						     DWORD dwFlags)
{
  FIXME("(%p)->(%p,%s,%08lx): stub !\n", iface, lpdiaf, lpszUserName, dwFlags);

  return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_GetImageInfo(LPDIRECTINPUTDEVICE8A iface,
						     LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader)
{
  FIXME("(%p)->(%p): stub !\n", iface, lpdiDevImageInfoHeader);

  return DI_OK;
}
