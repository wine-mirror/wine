/* 
 *   Control
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *   To be distributed under the Wine license
 */

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include "params.h"

void launch(char what[255])
{ 
  HMODULE hMod;
  FARPROC pControl_RunDLL;

  hMod = LoadLibrary("shell32.dll");
  pControl_RunDLL = GetProcAddress(hMod, "Control_RunDLL");
  pControl_RunDLL(GetDesktopWindow(), 0, what, SW_SHOW);
  FreeLibrary(hMod);
  exit(0);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, CHAR *szParam, INT argc) 
{

  char szParams[255];
  lstrcpy(szParams, szParam);
  CharUpper(szParams);

  switch (argc) {
    case 0:  /* no parameters - pop up whole "Control Panel" by default */
             launch("");
             break;

    case 1:  /* check for optional parameter */
             if (!strcmp(szParams,szP_DESKTOP))
		 launch(szC_DESKTOP);
             if (!strcmp(szParams,szP_COLOR))
		 launch(szC_COLOR);
             if (!strcmp(szParams,szP_DATETIME))
		 launch(szC_DATETIME);
             if (!strcmp(szParams,szP_DESKTOP))
		 launch(szC_DESKTOP);
             if (!strcmp(szParams,szP_INTERNATIONAL))
		 launch(szC_INTERNATIONAL);
             if (!strcmp(szParams,szP_KEYBOARD))
		 launch(szC_KEYBOARD);
             if (!strcmp(szParams,szP_MOUSE))
		 launch(szC_MOUSE);
             if (!strcmp(szParams,szP_PORTS))
		 launch(szC_PORTS);
             if (!strcmp(szParams,szP_PRINTERS))
		 launch(szC_PRINTERS);

	     /* try to launch if a .cpl file is given directly */
	     launch(szParams);
             break;

    default: printf("Syntax error.");
  }
  return 0;  
}
