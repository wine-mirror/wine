/* 
 *   Control
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *   To be distributed under the Wine license
 */

#include <win.h>
#include <shell.h>
#include "params.h"

void launch(char what[255])
{ 
  HINSTANCE hChild;
  char szArgs[255];

  lstrcpy(szArgs, szEXEC_ARGS);
  strcat(szArgs, what);

  hChild = ShellExecute((HWND)0, 0, szEXEC_PREFIX, szArgs, "", SW_SHOWNORMAL);
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
             if (strcmp(szParams,szP_DESKTOP)      ==0) launch(szC_DESKTOP);
             if (strcmp(szParams,szP_COLOR)        ==0) launch(szC_COLOR);
             if (strcmp(szParams,szP_DATETIME)     ==0) launch(szC_DATETIME);
             if (strcmp(szParams,szP_DESKTOP)      ==0) launch(szC_DESKTOP);
             if (strcmp(szParams,szP_INTERNATIONAL)==0) launch(szC_INTERNATIONAL);
             if (strcmp(szParams,szP_KEYBOARD)     ==0) launch(szC_KEYBOARD);
             if (strcmp(szParams,szP_MOUSE)        ==0) launch(szC_MOUSE);
             if (strcmp(szParams,szP_PORTS)        ==0) launch(szC_PORTS);
             if (strcmp(szParams,szP_PRINTERS)     ==0) launch(szC_PRINTERS);

             /* couldn't recognize desired panel, going default mode */
             launch("");
             break;

    default: printf("Syntax error.");
  }
  return 0;  
}
