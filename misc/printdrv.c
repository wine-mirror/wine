/* 
 * Implementation of some printer driver bits
 * 
 * Copyright 1996 John Harvey
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"


INT16 WINAPI StartDoc16( HDC16 hdc, const DOCINFO16 *lpdoc )
{
  INT16 retVal;
  printf("In startdoc16(%p)\n", lpdoc );
  printf("In StartDoc16 %d 0x%lx:0x%p 0x%lx:0x%p\n",lpdoc->cbSize,
	 lpdoc->lpszDocName,PTR_SEG_TO_LIN(lpdoc->lpszDocName),
	 lpdoc->lpszOutput,PTR_SEG_TO_LIN(lpdoc->lpszOutput));
  printf("In StartDoc16 %d %s %s\n",lpdoc->cbSize,
	 (LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszDocName),
	 (LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszOutput));
  retVal =  Escape16(hdc, STARTDOC, sizeof(DOCINFO16), lpdoc->lpszDocName, 0);
  printf("Escape16 returned %d\n",retVal);
  return retVal;
}

INT16 WINAPI EndDoc16(HDC16 hdc)
{
  return  Escape16(hdc, ENDDOC, 0, 0, 0);
}



DWORD WINAPI DrvGetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
                               LPDWORD lpType, LPBYTE lpPrinterData,
                               int cbData, LPDWORD lpNeeded)
{
    fprintf(stderr,"In DrvGetPrinterData ");
    if (HIWORD(lpPrinter))
	    fprintf(stderr,"printer %s ",lpPrinter);
    else
	    fprintf(stderr,"printer %p ",lpPrinter);
    if (HIWORD(lpProfile))
	    fprintf(stderr,"profile %s ",lpProfile);
    else
	    fprintf(stderr,"profile %p ",lpProfile);
    fprintf(stderr,"lpType %p\n",lpType);
    return 0;
}



DWORD WINAPI DrvSetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
                               LPDWORD lpType, LPBYTE lpPrinterData,
                               DWORD dwSize)
{
    fprintf(stderr,"In DrvSetPrinterData ");
    if (HIWORD(lpPrinter))
	    fprintf(stderr,"printer %s ",lpPrinter);
    else
	    fprintf(stderr,"printer %p ",lpPrinter);
    if (HIWORD(lpProfile))
	    fprintf(stderr,"profile %s ",lpProfile);
    else
	    fprintf(stderr,"profile %p ",lpProfile);
    fprintf(stderr,"lpType %p\n",lpType);
    return 0;
}


INT32 WINAPI DeviceCapabilities32A(LPCSTR printer,LPCSTR target,WORD z,
                                   LPSTR a,LPDEVMODE32A b)
{
    fprintf(stderr,"DeviceCapabilitiesA(%s,%s,%d,%p,%p)\n",printer,target,z,a,b);
    return 1;   	
}

LONG WINAPI DocumentProperties32A(HWND32 hWnd,HANDLE32 hPrinter,
                                LPSTR pDeviceName, LPDEVMODE32A pDevModeOutput,
                                  LPDEVMODE32A pDevModeInput,DWORD fMode )
{
    fprintf(stderr,"DocumentPropertiesA(%d,%d,%s,%p,%p,%d)\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );
    return 1;
}

