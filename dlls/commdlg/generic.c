/*
 * COMMDLG/COMDLG32 functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1998,1999 Bertho Stultiens
 * Copyright 1999 Klaas van Gend
 */

#include "winbase.h"
#include "commdlg.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"

HINSTANCE16	COMMDLG_hInstance = 0;
HINSTANCE	COMMDLG_hInstance32 = 0;
static int	COMMDLG_Attach = 0;

/***********************************************************************
 *	COMMDLG_DllEntryPoint			[COMMDLG.entry]
 *
 *    Initialization code for the COMMDLG DLL
 *
 * RETURNS:
 */
BOOL WINAPI COMMDLG_DllEntryPoint(DWORD Reason, HINSTANCE16 hInst, WORD ds, WORD HeapSize, DWORD res1, WORD res2)
{
	TRACE("(%08lx, %04x, %04x, %04x, %08lx, %04x)\n", Reason, hInst, ds, HeapSize, res1, res2);
	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		COMMDLG_Attach++;
		if(COMMDLG_hInstance)
		{
			ERR("commdlg.dll instantiated twice!\n");
			/*
			 * We should return FALSE here, but that will break
			 * most apps that use CreateProcess because we do
			 * not yet support seperate address-spaces.
			 */
			return TRUE;
		}

		COMMDLG_hInstance = hInst;
		if(!COMMDLG_hInstance32)
		{
			if(!(COMMDLG_hInstance32 = LoadLibraryA("comdlg32.dll")))
			{
				ERR("Could not load sibling comdlg32.dll\n");
				return FALSE;
			}
		}
		break;

	case DLL_PROCESS_DETACH:
		if(!--COMMDLG_Attach)
		{
			COMMDLG_hInstance = 0;
			if(COMMDLG_hInstance32)
				FreeLibrary(COMMDLG_hInstance32);
		}
		break;
	}
	return TRUE;
}


/***********************************************************************
 *	CommDlgExtendedError16			[COMMDLG.26]
 *
 * Get the last error value if a commdlg function fails.
 *	RETURNS
 *		Current error value which might not be valid
 *		if a previous call succeeded.
 */
DWORD WINAPI CommDlgExtendedError16(void)
{
	return CommDlgExtendedError();
}
