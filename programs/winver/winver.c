/*
 * Windows Version Program
 *
 * Copyright 1997 by Marcel Baur (mbaur@g26.ethz.ch)
 *
*/

#include "windows.h"
#include "version.h"

int PASCAL WinMain (HANDLE inst, HANDLE prev, LPSTR cmdline, int show)
{
   return ShellAbout((HWND)0, "WINE", WINE_RELEASE_INFO, 0);
}

/* Local Variables:     */
/* c-files style: "GNU" */
/* End:                 */
