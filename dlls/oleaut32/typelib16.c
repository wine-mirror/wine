/*
 *	TYPELIB 16bit part.
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1999 Rein Klazes
 * Copyright 2000 Francois Jacques
 * Copyright 2001 Huw D M Davies for CodeWeavers
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "winerror.h"
#include "winnls.h"         /* for PRIMARYLANGID */
#include "winreg.h"         /* for HKEY_LOCAL_MACHINE */
#include "winuser.h"

#include "objbase.h"
#include "ole2disp.h"
#include "typelib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/****************************************************************************
 *		QueryPathOfRegTypeLib	[TYPELIB.14]
 *
 * the path is "Classes\Typelib\<guid>\<major>.<minor>\<lcid>\win16\"
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib16(
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR16 path	/* [out] path of typelib */
) {
	char	xguid[80];
	char	typelibkey[100],pathname[260];
	DWORD	plen;

       	TRACE("\n");

	if (HIWORD(guid)) {
            sprintf( typelibkey, "SOFTWARE\\Classes\\Typelib\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\%d.%d\\%lx\\win16",
                     guid->Data1, guid->Data2, guid->Data3,
                     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7],
                     wMaj,wMin,lcid);
	} else {
		sprintf(xguid,"<guid 0x%08lx>",(DWORD)guid);
		FIXME("(%s,%d,%d,0x%04lx,%p),can't handle non-string guids.\n",xguid,wMaj,wMin,(DWORD)lcid,path);
		return E_FAIL;
	}
	plen = sizeof(pathname);
	if (RegQueryValueA(HKEY_LOCAL_MACHINE,typelibkey,pathname,&plen)) {
		/* try again without lang specific id */
		if (SUBLANGID(lcid))
			return QueryPathOfRegTypeLib16(guid,wMaj,wMin,PRIMARYLANGID(lcid),path);
		FIXME("key %s not found\n",typelibkey);
		return E_FAIL;
	}
	*path = SysAllocString16(pathname);
	return S_OK;
}

/******************************************************************************
 * LoadTypeLib [TYPELIB.3]  Loads and registers a type library
 * NOTES
 *    Docs: OLECHAR FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLib16(
    LPOLESTR szFile, /* [in] Name of file to load from */
    ITypeLib** pptLib) /* [out] Pointer to pointer to loaded type library */
{
    FIXME("(%s,%p): stub\n",debugstr_w((LPWSTR)szFile),pptLib);

    if (pptLib!=0)
      *pptLib=0;

    return E_FAIL;
}

/****************************************************************************
 *	OaBuildVersion				(TYPELIB.15)
 *
 * known TYPELIB.DLL versions:
 *
 * OLE 2.01 no OaBuildVersion() avail	1993	--	---
 * OLE 2.02				1993-94	02     3002
 * OLE 2.03					23	730
 * OLE 2.03					03     3025
 * OLE 2.03 W98 SE orig. file !!	1993-95	10     3024
 * OLE 2.1   NT				1993-95	??	???
 * OLE 2.3.1 W95				23	700
 * OLE2 4.0  NT4SP6			1993-98	40     4277
 */
DWORD WINAPI OaBuildVersion16(void)
{
    /* FIXME: I'd like to return the highest currently known version value
     * in case the user didn't force a --winver, but I don't know how
     * to retrieve the "versionForced" info from misc/version.c :(
     * (this would be useful in other places, too) */
    FIXME("If you get version error messages, please report them\n");
    switch(GetVersion() & 0x8000ffff)  /* mask off build number */
    {
    case 0x80000a03:  /* WIN31 */
		return MAKELONG(3027, 3); /* WfW 3.11 */
    case 0x80000004:  /* WIN95 */
		return MAKELONG(700, 23); /* Win95A */
    case 0x80000a04:  /* WIN98 */
		return MAKELONG(3024, 10); /* W98 SE */
    case 0x00000004:  /* NT4 */
		return MAKELONG(4277, 40); /* NT4 SP6 */
    default:
	FIXME("Version value not known yet. Please investigate it!\n");
		return 0;
    }
}
