/*
 *  OLE32 Initialization
 *
 * Copyright 2000 Huw D M Davies for Codeweavers
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

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole32_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HINSTANCE OLE32_hInstance = 0;

/***********************************************************************
 *		OleMetafilePictFromIconAndLabel (OLE32.115)
 */
HGLOBAL WINAPI OleMetafilePictFromIconAndLabel(HICON hIcon, LPOLESTR lpszLabel,
                                               LPOLESTR lpszSourceFile, UINT iIconIndex)
{
	HMETAFILE hmf;
	HDC hdc;
	UINT dy, mfsize;
	HGLOBAL hmem = 0;
	LPVOID mfdata;

	TRACE("%p %p %p %d\n", hIcon, lpszLabel, lpszSourceFile, iIconIndex);

	if( !hIcon )
		return 0;

	hdc = CreateMetaFileW(NULL);
	if( !hdc )
		return 0;

	/* FIXME: things are drawn in the wrong place */
	DrawIcon(hdc, 0, 0, hIcon);
	dy = GetSystemMetrics(SM_CXICON);
	if(lpszLabel)
		TextOutW(hdc, 0, dy, lpszLabel, lstrlenW(lpszLabel));

	hmf = CloseMetaFile(hdc);
	if( !hmf )
		return 0;

	mfsize = GetMetaFileBitsEx( hmf, 0, NULL);
	if( !mfsize )
		goto end;

	hmem = GlobalAlloc( GMEM_MOVEABLE, mfsize );
	if( !hmem )
		goto end;

	mfdata = GlobalLock( hmem );
	if( !mfdata )
	{
		GlobalFree( hmem );
		hmem = 0;
		goto end;
	}

	GetMetaFileBitsEx( hmf, mfsize, mfdata );
	GlobalUnlock( hmem );
end:
	DeleteMetaFile(hmf);

	TRACE("returning %p\n",hmem);

	return hmem;
}


/***********************************************************************
 *		DllMain (OLE32.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        OLE32_hInstance = hinstDLL;
        COMPOBJ_InitProcess();
	if (TRACE_ON(ole)) CoRegisterMallocSpy((LPVOID)-1);
	break;

    case DLL_PROCESS_DETACH:
        if (TRACE_ON(ole)) CoRevokeMallocSpy();
        COMPOBJ_UninitProcess();
        OLE32_hInstance = 0;
	break;
    }
    return TRUE;
}

/* NOTE: DllRegisterServer and DllUnregisterServer are in regsvr.c */
