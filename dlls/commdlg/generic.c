/*
 * COMMDLG/COMDLG32 functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1998 Bertho Stultiens
 * Copyright 1999 Klaas van Gend
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "win.h"
#include "commdlg.h"
#include "module.h"
#include "debug.h"
#include "winproc.h"


#define COMDLG32_LAST_ERROR_NOT_ALLOCATED 0xF684F684
static DWORD		COMDLG32_TlsIndex  = COMDLG32_LAST_ERROR_NOT_ALLOCATED;

/***********************************************************************
 *	COMDLG32_DllEntryPoint			[COMDLG32.entry]
 *
 *    Initialisation code for the COMDLG32 DLL
 *    This call should implement the allocation of the TLS.
 *
 * RETURNS:
 *
 * BUGS:
 *    Remains unimplemented until Bertho finishes his ELF-DLL code
 */
BOOL WINAPI COMDLG32_DllEntryPoint(HINSTANCE hInstance, 
                                   DWORD Reason, 
				   LPVOID Reserved
				   );



/***********************************************************************
 *	COMDLG32_AllocTlsForCommDlgExtError	[internal]
 *
 * Allocates Thread Local Storage for the ComDlg32 local
 * last extended error
 *
 * RETURNS:
 *    nothing. 
 *
 * BUGS:
 * 1) FIXME: This function is only temporary, as this code *SHOULD*
 *        be executed in the DLL Entrypoint. For now, it is done
 *        this way.
 * 2) This allocated memory is NEVER freed again!
 */
void COMDLG32_AllocTlsForCommDlgExtError()
{
 FIXME(commdlg, "TLS for CommDlgExtendedError allocated on-the-fly\n");
    if (COMDLG32_TlsIndex == COMDLG32_LAST_ERROR_NOT_ALLOCATED)
	    COMDLG32_TlsIndex = TlsAlloc();
    if (COMDLG32_TlsIndex == 0xFFFFFFFF)
    	ERR(commdlg, "No space for COMDLG32 TLS\n");
}

/***********************************************************************
 *	COMDLG32_SetCommDlgExtendedError	[internal]
 *
 * Used to set the thread's local error value if a comdlg32 function fails.
 */
void COMDLG32_SetCommDlgExtendedError(DWORD err)
{
    /*FIXME: This check and the resulting alloc should be removed
     *       when the DLL Entry code is finished
     */
    if (COMDLG32_TlsIndex==COMDLG32_LAST_ERROR_NOT_ALLOCATED)
        COMDLG32_AllocTlsForCommDlgExtError();
    TlsSetValue(COMDLG32_TlsIndex, (void *)err);
}


/***********************************************************************
 *	CommDlgExtendedError			[COMDLG32.5]
 *                                              [COMMDLG.26]
 * Get the thread's local error value if a comdlg32 function fails.
 *	RETURNS
 *		Current error value which might not be valid
 *		if a previous call succeeded.
 */
DWORD WINAPI CommDlgExtendedError(void)
{
    /*FIXME: This check and the resulting alloc should be removed
     *       when the DLL Entry code is finished
     */
    if (COMDLG32_TlsIndex==COMDLG32_LAST_ERROR_NOT_ALLOCATED)
        COMDLG32_AllocTlsForCommDlgExtError();
    return (DWORD)TlsGetValue(COMDLG32_TlsIndex);
}
