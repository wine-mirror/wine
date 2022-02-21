/*
 * W32SYS
 * helper DLL for Win32s
 *
 * Copyright (c) 1996 Anand Kumria
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dll);

typedef struct
{
    BYTE   bMajor;
    BYTE   bMinor;
    WORD   wBuildNumber;
    BOOL16 fDebug;
} WIN32SINFO, *LPWIN32SINFO;

/***********************************************************************
 *           GetWin32sInfo   (W32SYS.12)
 * RETURNS
 *  0 on success, 1 on failure
 */
WORD WINAPI GetWin32sInfo16(
	LPWIN32SINFO lpInfo	/* [out] Win32S special information */
) {
    lpInfo->bMajor = 1;
    lpInfo->bMinor = 3;
    lpInfo->wBuildNumber = 0;
    lpInfo->fDebug = FALSE;

    return 0;
}

/***********************************************************************
 *            GetW32SysVersion  (W32SYS.5)
 */
WORD WINAPI GetW32SysVersion16(void)
{
    return 0x100;
}

/***********************************************************************
 *           GetPEResourceTable   (W32SYS.7)
 * retrieves the resourcetable from the passed filedescriptor
 * RETURNS
 *	dunno what.
 */
WORD WINAPI GetPEResourceTable16(
	HFILE16 hf		/* [in] Handle of executable file */
) {
	return 0;
}

/***********************************************************************
 *           LoadPeResource (W32SYS.11)
 */
DWORD WINAPI LoadPeResource16(WORD x,SEGPTR y) {
	FIXME("(0x%04x,0x%08lx),stub!\n",x,y);
	return 0;
}


/**********************************************************************
 *		IsPeFormat		(W32SYS.2)
 * Check if a file is a PE format executable.
 * RETURNS
 *  TRUE, if it is.
 *  FALSE if not.
 */
BOOL16 WINAPI IsPeFormat16( LPSTR fn,      /* [in] filename of executable */
                            HFILE16 hf16 ) /* [in] open file, if filename is NULL */
{
    BOOL16 ret = FALSE;
    IMAGE_DOS_HEADER mzh;
    OFSTRUCT ofs;
    DWORD xmagic;
    HFILE hf;

    if (fn) hf = OpenFile(fn,&ofs,OF_READ);
    else hf = (HFILE)DosFileHandleToWin32Handle( hf16 );
    if (hf == HFILE_ERROR) return FALSE;
    _llseek(hf,0,SEEK_SET);
    if (sizeof(mzh)!=_lread(hf,&mzh,sizeof(mzh))) goto done;
    if (mzh.e_magic!=IMAGE_DOS_SIGNATURE) goto done;
    _llseek(hf,mzh.e_lfanew,SEEK_SET);
    if (sizeof(DWORD)!=_lread(hf,&xmagic,sizeof(DWORD))) goto done;
    ret = (xmagic == IMAGE_NT_SIGNATURE);
 done:
    if (fn) _lclose(hf);
    return ret;
}
