/*
 * W32SYS
 * helper DLL for Win32s
 *
 * Copyright (c) 1996 Anand Kumria
 */

#include "windef.h"
#include "w32sys.h"

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
 *            GetW32SysVersion16  (W32SYS.5)
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
	HFILE16 hf		/* [in] filedescriptor to opened executeable */
) {
	return 0;
}
