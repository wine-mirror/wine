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


INT16 StartDoc16( HDC16 hdc, const DOCINFO16 *lpdoc )
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

INT16
EndDoc16(HDC16 hdc)
{
  return  Escape16(hdc, ENDDOC, 0, 0, 0);
}



DWORD
DrvGetPrinterData(LPSTR lpPrinter, LPSTR lpProfile, LPDWORD lpType,
                  LPBYTE lpPrinterData, int cbData, LPDWORD lpNeeded)
{
    fprintf(stderr,"In DrvGetPrinterData printer %p profile %p lpType %p \n",
           lpPrinter, lpProfile, lpType);
    return 0;
}



DWORD
DrvSetPrinterData(LPSTR lpPrinter, LPSTR lpProfile, LPDWORD lpType,
                  LPBYTE lpPrinterData, DWORD dwSize)
{
    fprintf(stderr,"In DrvSetPrinterData printer %p profile %p lpType %p \n",
           lpPrinter, lpProfile, lpType);
    return 0;
}



