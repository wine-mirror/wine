/*
 *	OLEAUT32
 *
 * Copyright 1999, 2000 Marcus Meissner
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

#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"

#include "tmarshal.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* The OLE Automation ProxyStub Interface Class (aka Typelib Marshaler) */
extern const GUID CLSID_PSOAInterface;

/* IDispatch marshaler */
extern const GUID CLSID_PSDispatch;

/******************************************************************************
 *             SysStringLen  [OLEAUT32.7]
 *
 * The Windows documentation states that the length returned by this function
 * is not necessarely the same as the length returned by the _lstrlenW method.
 * It is the same number that was passed in as the "len" parameter if the
 * string was allocated with a SysAllocStringLen method call.
 */
int WINAPI SysStringLen(BSTR str)
{
    DWORD* bufferPointer;

     if (!str) return 0;
    /*
     * The length of the string (in bytes) is contained in a DWORD placed
     * just before the BSTR pointer
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    return (int)(*bufferPointer/sizeof(WCHAR));
}

/******************************************************************************
 *             SysStringByteLen  [OLEAUT32.149]
 *
 * The Windows documentation states that the length returned by this function
 * is not necessarely the same as the length returned by the _lstrlenW method.
 * It is the same number that was passed in as the "len" parameter if the
 * string was allocated with a SysAllocStringLen method call.
 */
int WINAPI SysStringByteLen(BSTR str)
{
    DWORD* bufferPointer;

     if (!str) return 0;
    /*
     * The length of the string (in bytes) is contained in a DWORD placed
     * just before the BSTR pointer
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    return (int)(*bufferPointer);
}

/******************************************************************************
 *		SysAllocString	[OLEAUT32.2]
 *
 * MSDN (October 2001) states that this returns a NULL value if the argument
 * is a zero-length string.  This does not appear to be true; certainly it
 * returns a value under Win98 (Oleaut32.dll Ver 2.40.4515.0)
 */
BSTR WINAPI SysAllocString(LPCOLESTR in)
{
    if (!in) return 0;

    /* Delegate this to the SysAllocStringLen32 method. */
    return SysAllocStringLen(in, lstrlenW(in));
}

/******************************************************************************
 *		SysFreeString	[OLEAUT32.6]
 */
void WINAPI SysFreeString(BSTR in)
{
    DWORD* bufferPointer;

    /* NULL is a valid parameter */
    if(!in) return;

    /*
     * We have to be careful when we free a BSTR pointer, it points to
     * the beginning of the string but it skips the byte count contained
     * before the string.
     */
    bufferPointer = (DWORD*)in;

    bufferPointer--;

    /*
     * Free the memory from its "real" origin.
     */
    HeapFree(GetProcessHeap(), 0, bufferPointer);
}

/******************************************************************************
 *             SysAllocStringLen     [OLEAUT32.4]
 *
 * In "Inside OLE, second edition" by Kraig Brockshmidt. In the Automation
 * section, he describes the DWORD value placed *before* the BSTR data type.
 * he describes it as a "DWORD count of characters". By experimenting with
 * a windows application, this count seems to be a DWORD count of bytes in
 * the string. Meaning that the count is double the number of wide
 * characters in the string.
 */
BSTR WINAPI SysAllocStringLen(const OLECHAR *in, unsigned int len)
{
    DWORD  bufferSize;
    DWORD* newBuffer;
    WCHAR* stringBuffer;

    /*
     * Find the length of the buffer passed-in in bytes.
     */
    bufferSize = len * sizeof (WCHAR);

    /*
     * Allocate a new buffer to hold the string.
     * dont't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the NULL.
     */
    newBuffer = (DWORD*)HeapAlloc(GetProcessHeap(),
                                 0,
                                 bufferSize + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = bufferSize;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    /*
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (in != 0)
      memcpy(newBuffer, in, bufferSize);
    else
      memset(newBuffer, 0, bufferSize);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (WCHAR*)newBuffer;
    stringBuffer[len] = L'\0';

    return (LPWSTR)stringBuffer;
}

/******************************************************************************
 *             SysReAllocStringLen   [OLEAUT32.5]
 */
int WINAPI SysReAllocStringLen(BSTR* old, const OLECHAR* in, unsigned int len)
{
    /*
     * Sanity check
     */
    if (old==NULL)
      return 0;

    if (*old!=NULL) {
      DWORD newbytelen = len*sizeof(WCHAR);
      DWORD *ptr = HeapReAlloc(GetProcessHeap(),0,((DWORD*)*old)-1,newbytelen+sizeof(WCHAR)+sizeof(DWORD));
      *old = (BSTR)(ptr+1);
      *ptr = newbytelen;
      if (in) {
        memcpy(*old, in, newbytelen);
        (*old)[len] = 0;
      } else {
	/* Subtle hidden feature: The old string data is still there
	 * when 'in' is NULL! 
	 * Some Microsoft program needs it.
	 */
      }
    } else {
      /*
       * Allocate the new string
       */
      *old = SysAllocStringLen(in, len);
    }

    return 1;
}

/******************************************************************************
 *             SysAllocStringByteLen     [OLEAUT32.150]
 *
 */
BSTR WINAPI SysAllocStringByteLen(LPCSTR in, UINT len)
{
    DWORD* newBuffer;
    char* stringBuffer;

    /*
     * Allocate a new buffer to hold the string.
     * dont't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the NULL.
     */
    newBuffer = (DWORD*)HeapAlloc(GetProcessHeap(),
                                 0,
                                 len + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = len;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    /*
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (in != 0)
      memcpy(newBuffer, in, len);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (char *)newBuffer;
    stringBuffer[len] = 0;
    stringBuffer[len+1] = 0;

    return (LPWSTR)stringBuffer;
}

/******************************************************************************
 *		SysReAllocString	[OLEAUT32.3]
 */
INT WINAPI SysReAllocString(LPBSTR old,LPCOLESTR in)
{
    /*
     * Sanity check
     */
    if (old==NULL)
      return 0;

    /*
     * Make sure we free the old string.
     */
    if (*old!=NULL)
      SysFreeString(*old);

    /*
     * Allocate the new string
     */
    *old = SysAllocString(in);

     return 1;
}

static WCHAR	_delimiter[2] = {'!',0}; /* default delimiter apparently */
static WCHAR	*pdelimiter = &_delimiter[0];

/***********************************************************************
 *		RegisterActiveObject (OLEAUT32.33)
 */
HRESULT WINAPI RegisterActiveObject(
	LPUNKNOWN punk,REFCLSID rcid,DWORD dwFlags,LPDWORD pdwRegister
) {
	WCHAR 			guidbuf[80];
	HRESULT			ret;
	LPRUNNINGOBJECTTABLE	runobtable;
	LPMONIKER		moniker;

	StringFromGUID2(rcid,guidbuf,39);
	ret = CreateItemMoniker(pdelimiter,guidbuf,&moniker);
	if (FAILED(ret))
		return ret;
	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) {
		IMoniker_Release(moniker);
		return ret;
	}
	ret = IRunningObjectTable_Register(runobtable,dwFlags,punk,moniker,pdwRegister);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}

/***********************************************************************
 *		RevokeActiveObject (OLEAUT32.34)
 */
HRESULT WINAPI RevokeActiveObject(DWORD xregister,LPVOID reserved)
{
	LPRUNNINGOBJECTTABLE	runobtable;
	HRESULT			ret;

	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) return ret;
	ret = IRunningObjectTable_Revoke(runobtable,xregister);
	if (SUCCEEDED(ret)) ret = S_OK;
	IRunningObjectTable_Release(runobtable);
	return ret;
}

/***********************************************************************
 *		GetActiveObject (OLEAUT32.35)
 */
HRESULT WINAPI GetActiveObject(REFCLSID rcid,LPVOID preserved,LPUNKNOWN *ppunk)
{
	WCHAR 			guidbuf[80];
	HRESULT			ret;
	LPRUNNINGOBJECTTABLE	runobtable;
	LPMONIKER		moniker;

	StringFromGUID2(rcid,guidbuf,39);
	ret = CreateItemMoniker(pdelimiter,guidbuf,&moniker);
	if (FAILED(ret))
		return ret;
	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) {
		IMoniker_Release(moniker);
		return ret;
	}
	ret = IRunningObjectTable_GetObject(runobtable,moniker,ppunk);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}


/***********************************************************************
 *           OaBuildVersion           [OLEAUT32.170]
 *
 * known OLEAUT32.DLL versions:
 * OLE 2.1  NT				1993-95	10     3023
 * OLE 2.1					10     3027
 * Win32s 1.1e					20     4049
 * OLE 2.20 W95/NT			1993-96	20     4112
 * OLE 2.20 W95/NT			1993-96	20     4118
 * OLE 2.20 W95/NT			1993-96	20     4122
 * OLE 2.30 W95/NT			1993-98	30     4265
 * OLE 2.40 NT??			1993-98	40     4267
 * OLE 2.40 W98 SE orig. file		1993-98	40     4275
 * OLE 2.40 W2K orig. file		1993-XX 40     4514
 *
 * I just decided to use version 2.20 for Win3.1, 2.30 for Win95 & NT 3.51,
 * and 2.40 for all newer OSs. The build number is maximum, i.e. 0xffff.
 */
UINT WINAPI OaBuildVersion()
{
    switch(GetVersion() & 0x8000ffff)  /* mask off build number */
    {
    case 0x80000a03:  /* WIN31 */
		return MAKELONG(0xffff, 20);
    case 0x00003303:  /* NT351 */
		return MAKELONG(0xffff, 30);
    case 0x80000004:  /* WIN95; I'd like to use the "standard" w95 minor
		         version here (30), but as we still use w95
		         as default winver (which is good IMHO), I better
		         play safe and use the latest value for w95 for now.
		         Change this as soon as default winver gets changed
		         to something more recent */
    case 0x80000a04:  /* WIN98 */
    case 0x00000004:  /* NT40 */
    case 0x00000005:  /* W2K */
		return MAKELONG(0xffff, 40);
    default:
		ERR("Version value not known yet. Please investigate it !\n");
		return 0x0;
    }
}

/******************************************************************************
 *		OleTranslateColor	[OLEAUT32.421]
 *
 * Converts an OLE_COLOR to a COLORREF.
 * See the documentation for conversion rules.
 * pColorRef can be NULL. In that case the user only wants to test the
 * conversion.
 */
HRESULT WINAPI OleTranslateColor(
  OLE_COLOR clr,
  HPALETTE  hpal,
  COLORREF* pColorRef)
{
  COLORREF colorref;
  BYTE b = HIBYTE(HIWORD(clr));

  TRACE("(%08lx, %p, %p):stub\n", clr, hpal, pColorRef);

  /*
   * In case pColorRef is NULL, provide our own to simplify the code.
   */
  if (pColorRef == NULL)
    pColorRef = &colorref;

  switch (b)
  {
    case 0x00:
    {
      if (hpal != 0)
        *pColorRef =  PALETTERGB(GetRValue(clr),
                                 GetGValue(clr),
                                 GetBValue(clr));
      else
        *pColorRef = clr;

      break;
    }

    case 0x01:
    {
      if (hpal != 0)
      {
        PALETTEENTRY pe;
        /*
         * Validate the palette index.
         */
        if (GetPaletteEntries(hpal, LOWORD(clr), 1, &pe) == 0)
          return E_INVALIDARG;
      }

      *pColorRef = clr;

      break;
    }

    case 0x02:
      *pColorRef = clr;
      break;

    case 0x80:
    {
      int index = LOBYTE(LOWORD(clr));

      /*
       * Validate GetSysColor index.
       */
      if ((index < COLOR_SCROLLBAR) || (index > COLOR_GRADIENTINACTIVECAPTION))
        return E_INVALIDARG;

      *pColorRef =  GetSysColor(index);

      break;
    }

    default:
      return E_INVALIDARG;
  }

  return S_OK;
}

extern HRESULT OLEAUTPS_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

extern void _get_STDFONT_CF(LPVOID);
extern void _get_STDPIC_CF(LPVOID);

/***********************************************************************
 *		DllGetClassObject (OLEAUT32.1)
 */
HRESULT WINAPI OLEAUT32_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualGUID(rclsid,&CLSID_StdFont)) {
	if (IsEqualGUID(iid,&IID_IClassFactory)) {
	    _get_STDFONT_CF(ppv);
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    }
    if (IsEqualGUID(rclsid,&CLSID_StdPicture)) {
	if (IsEqualGUID(iid,&IID_IClassFactory)) {
	    _get_STDPIC_CF(ppv);
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    }
    if (IsEqualGUID(rclsid,&CLSID_PSDispatch)) {
	return OLEAUTPS_DllGetClassObject(rclsid,iid,ppv);
    }
    if (IsEqualGUID(rclsid,&CLSID_PSOAInterface)) {
	if (S_OK==TypeLibFac_DllGetClassObject(rclsid,iid,ppv))
	    return S_OK;
	/*FALLTHROUGH*/
    }
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (OLEAUT32.410)
 */
HRESULT WINAPI OLEAUT32_DllCanUnloadNow() {
    FIXME("(), stub!\n");
    return S_FALSE;
}
