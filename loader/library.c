
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "wine.h"



/**********************************************************************
 *				LoadLibrary	[KERNEL.95]
 */
HANDLE LoadLibrary(LPSTR libname)
{
    HANDLE hRet;
    printf("LoadLibrary '%s'\n", libname);
    hRet = LoadImage(libname);
    printf("after LoadLibrary hRet=%04X\n", hRet);
    return hRet;
}


/**********************************************************************
 *				FreeLibrary	[KERNEL.96]
 */
void FreeLibrary(HANDLE hLib)
{
    printf("FreeLibrary(%04X);\n", hLib);
    if (hLib != (HANDLE)NULL) GlobalFree(hLib);
}

