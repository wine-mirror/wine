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
 *		SysFreeString	[OLE2DISP.6]
 */
void WINAPI SysFreeString16(BSTR16 in)
{
	BSTR_Free(in);
}

/******************************************************************************
 *		SysStringLen	[OLE2DISP.7]
 */
int WINAPI SysStringLen16(BSTR16 str)
{
	return strlen(BSTR_GetAddr(str));
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
 * RegisterActiveObject [OLE2DISP.35]
 */
HRESULT WINAPI RegisterActiveObject16(
	IUnknown *punk, REFCLSID rclsid, DWORD dwFlags, unsigned long *pdwRegister
) {
	FIXME("(%p,%s,0x%08lx,%p):stub\n",punk,debugstr_guid(rclsid),dwFlags,pdwRegister);
	return E_NOTIMPL;
}
