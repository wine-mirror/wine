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


DWORD
DrvGetPrinterData(LPSTR lpPrinter, LPSTR lpProfile, LPDWORD lpType,
                  LPBYTE lpPrinterData, int cbData, LPDWORD lpNeeded)
{
    fprintf(stderr,"In DrvGetPrinterData printer %s profile %s lpType %p \n",
           lpPrinter, lpProfile, lpType);
    return 0;
}



DWORD
DrvSetPrinterData(LPSTR lpPrinter, LPSTR lpProfile, LPDWORD lpType,
                  LPBYTE lpPrinterData, DWORD dwSize)
{
    fprintf(stderr,"In DrvSetPrinterData printer %s profile %s lpType %p \n",
           lpPrinter, lpProfile, lpType);
    return 0;
}



