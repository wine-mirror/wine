/*
 * JBP (Jim Peterson <jspeter@birch.ee.vt.edu>): Lots of stubs needed for
 *      libwine.a.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wintypes.h"


/* for windows/winproc.c */
void CallFrom16_long_wwwll(void) {}

extern LRESULT ColorDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT FileOpenDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT FileSaveDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT FindTextDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT MDIClientWndProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT PrintDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT PrintSetupDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
extern LRESULT ReplaceTextDlgProc(HWND16,UINT16,WPARAM16,LPARAM);

/***********************************************************************
 *           MODULE_GetWndProcEntry16 (not a Windows API function)
 *
 * Return an entry point from the WPROCS dll.
 */
FARPROC16 MODULE_GetWndProcEntry16( char *name )
{
#define MAP_STR_TO_PROC(str,proc) if(!strcmp(name,str))return (FARPROC16)proc
  MAP_STR_TO_PROC("ColorDlgProc",ColorDlgProc);
  MAP_STR_TO_PROC("FileOpenDlgProc",FileOpenDlgProc);
  MAP_STR_TO_PROC("FileSaveDlgProc",FileSaveDlgProc);
  MAP_STR_TO_PROC("FindTextDlgProc",FindTextDlgProc);
  MAP_STR_TO_PROC("MDIClientWndProc",MDIClientWndProc);
  MAP_STR_TO_PROC("PrintDlgProc",PrintDlgProc);
  MAP_STR_TO_PROC("PrintSetupDlgProc",PrintSetupDlgProc);
  MAP_STR_TO_PROC("ReplaceTextDlgProc",ReplaceTextDlgProc);
  fprintf(stderr,"warning: No mapping for %s(), add one in library/miscstubs.c\n",name);
  assert( FALSE );
  return NULL;
}
