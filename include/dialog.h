/*
 * Dialog definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef DIALOG_H
#define DIALOG_H

#include "windows.h"

#define DIALOG_CLASS_NAME    "#32770"  /* Integer atom */


  /* Dialog info structure.
   * This structure is stored into the window extra bytes (cbWndExtra).
   * sizeof(DIALOGINFO) must be <= DLGWINDOWEXTRA (=30).
   */
typedef struct
{
    LONG      msgResult;
    FARPROC   dlgProc;
    LONG      userInfo;
    HWND      hwndFocus;
    HFONT     hUserFont;
    HMENU     hMenu;
    WORD      xBaseUnit;
    WORD      yBaseUnit;
    WORD      fEnd;
} DIALOGINFO;


  /* Dialog template header */
typedef struct
{
    DWORD     style;    
    BYTE      nbItems __attribute__ ((packed));
    WORD      x __attribute__ ((packed));
    WORD      y __attribute__ ((packed));
    WORD      cx __attribute__ ((packed));
    WORD      cy __attribute__ ((packed));
} DLGTEMPLATEHEADER;


  /* Dialog control header */
typedef struct
{
    WORD       x;
    WORD       y;
    WORD       cx;
    WORD       cy;
    WORD       id;
    DWORD      style __attribute__ ((packed));
} DLGCONTROLHEADER;


  /* Dialog control data */
typedef struct
{
    DLGCONTROLHEADER * header;
    LPSTR              class;
    LPSTR              text;
} DLGCONTROL;


  /* Dialog template */
typedef struct
{
    DLGTEMPLATEHEADER * header;
    unsigned char *     menuName;
    LPSTR               className;
    LPSTR               caption;
    WORD                pointSize;
    LPSTR               faceName;
    DLGCONTROL *        controls;
} DLGTEMPLATE;


#endif  /* DIALOG_H */
