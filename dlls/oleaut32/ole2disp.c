/*
 *	OLE2DISP library
 *
 *	Copyright 1995	Martin von Loewis
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

#include <string.h>

#include "wine/windef16.h"
#include "ole2.h"
#include "oleauto.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2disp.h"
#include "olectl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* This implementation of the BSTR API is 16-bit only. It
   represents BSTR as a 16:16 far pointer, and the strings
   as ISO-8859 */

/******************************************************************************
 *		BSTR_AllocBytes	[Internal]
 */
static BSTR16 BSTR_AllocBytes(int n)
{
    void *ptr = HeapAlloc( GetProcessHeap(), 0, n );
    return (BSTR16)MapLS(ptr);
}

/******************************************************************************
 * BSTR_Free [INTERNAL]
 */
static void BSTR_Free(BSTR16 in)
{
    void *ptr = MapSL( (SEGPTR)in );
    UnMapLS( (SEGPTR)in );
    HeapFree( GetProcessHeap(), 0, ptr );
}

/******************************************************************************
 * BSTR_GetAddr [INTERNAL]
 */
static void* BSTR_GetAddr(BSTR16 in)
{
    return in ? MapSL((SEGPTR)in) : 0;
}

/******************************************************************************
 *		SysAllocString	[OLE2DISP.2]
 */
BSTR16 WINAPI SysAllocString16(LPCOLESTR16 in)
{
	BSTR16 out;

	if (!in) return 0;

	out = BSTR_AllocBytes(strlen(in)+1);
	if (!out) return 0;
	strcpy(BSTR_GetAddr(out),in);
	return out;
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
 *		SysReallocString	[OLE2DISP.3]
 */
INT16 WINAPI SysReAllocString16(LPBSTR16 old,LPCOLESTR16 in)
{
	BSTR16 new=SysAllocString16(in);
	BSTR_Free(*old);
	*old=new;
	return 1;
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

/******************************************************************************
 *		SysAllocStringLen	[OLE2DISP.4]
 */
BSTR16 WINAPI SysAllocStringLen16(const char *in, int len)
{
	BSTR16 out=BSTR_AllocBytes(len+1);

	if (!out)
		return 0;

    /*
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (in != 0)
	strcpy(BSTR_GetAddr(out),in);
    else
      memset(BSTR_GetAddr(out), 0, len+1);

	return out;
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
 *		SysReAllocStringLen	[OLE2DISP.5]
 */
int WINAPI SysReAllocStringLen16(BSTR16 *old,const char *in,int len)
{
	BSTR16 new=SysAllocStringLen16(in,len);
	BSTR_Free(*old);
	*old=new;
	return 1;
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

    /*
     * Make sure we free the old string.
     */
    if (*old!=NULL)
      SysFreeString(*old);

    /*
     * Allocate the new string
     */
    *old = SysAllocStringLen(in, len);

    return 1;
}

/******************************************************************************
 *		SysFreeString	[OLE2DISP.6]
 */
void WINAPI SysFreeString16(BSTR16 in)
{
	BSTR_Free(in);
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
 *		SysStringLen	[OLE2DISP.7]
 */
int WINAPI SysStringLen16(BSTR16 str)
{
	return strlen(BSTR_GetAddr(str));
}

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
 * CreateDispTypeInfo [OLEAUT32.31]
 */
HRESULT WINAPI CreateDispTypeInfo(
	INTERFACEDATA *pidata,
	LCID lcid,
	ITypeInfo **pptinfo)
{
	FIXME("(%p,%ld,%p),stub\n",pidata,lcid,pptinfo);
	return 0;
}

/******************************************************************************
 * CreateDispTypeInfo [OLE2DISP.31]
 */
HRESULT WINAPI CreateDispTypeInfo16(
	INTERFACEDATA *pidata,
	LCID lcid,
	ITypeInfo **pptinfo)
{
	FIXME("(%p,%ld,%p),stub\n",pidata,lcid,pptinfo);
	return E_NOTIMPL;
}

/******************************************************************************
 * CreateStdDispatch [OLE2DISP.32]
 */
HRESULT WINAPI CreateStdDispatch16(
        IUnknown* punkOuter,
        void* pvThis,
	ITypeInfo* ptinfo,
	IUnknown** ppunkStdDisp)
{
	FIXME("(%p,%p,%p,%p),stub\n",punkOuter, pvThis, ptinfo,
               ppunkStdDisp);
	return 0;
}

/******************************************************************************
 * CreateStdDispatch [OLEAUT32.32]
 */
HRESULT WINAPI CreateStdDispatch(
        IUnknown* punkOuter,
        void* pvThis,
	ITypeInfo* ptinfo,
	IUnknown** ppunkStdDisp)
{
	FIXME("(%p,%p,%p,%p),stub\n",punkOuter, pvThis, ptinfo,
               ppunkStdDisp);
	return E_NOTIMPL;
}

/******************************************************************************
 * RegisterActiveObject [OLE2DISP.35]
 */
HRESULT WINAPI RegisterActiveObject16(
	IUnknown *punk, REFCLSID rclsid, DWORD dwFlags, unsigned long *pdwRegister
) {
	FIXME("(%p,%s,0x%08lx,%p):stub\n",punk,debugstr_guid(rclsid),dwFlags,pdwRegister);
	return E_NOTIMPL;
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


