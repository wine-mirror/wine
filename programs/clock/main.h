/*
 * Clock (main.h)
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 */

#define MAX_STRING_LEN      255
#define MAX_PATHNAME_LEN    1024
#define MAX_LANGUAGE_NUMBER (CL_LAST_LANGUAGE - CL_FIRST_LANGUAGE)

#define HELPFILE    "clock.hlp"
#define DEFAULTICON OIC_WINEICON

/* hide the following from winerc */
#ifndef RC_INVOKED

typedef struct
{
  HANDLE  hInstance;
  HWND    hMainWnd;
  HICON   hMainIcon;
  HICON   hDefaultIcon;
  HMENU   hMainMenu;
  HMENU   hPropertiesMenu;
  HMENU   hLanguageMenu;
  HMENU   hInfoMenu;
  HMENU   hSystemMenu;
  HMENU   hPopupMenu1;
  LPCSTR  lpszIniFile;
  LPCSTR  lpszIcoFile;
  LPSTR   lpszLanguage;
  UINT    wStringTableOffset;

  BOOL    bAnalog;
  BOOL    bAlwaysOnTop;
  BOOL    bWithoutTitle;
  BOOL    bSeconds;
  BOOL    bDate;  

  int     MaxX;
  int     MaxY;
} CLOCK_GLOBALS;

extern CLOCK_GLOBALS Globals;

/* function prototypes */
VOID MAIN_FileChooseFont(VOID);

/* class names */

/* Resource names */
extern CHAR STRING_MENU_Xx[];

   #define STRINGID(id) (0x##id + Globals.wStringTableOffset)
   
#else  /* RC_INVOKED */

   #define STRINGID(id) id
   
#endif

/* string table index */
#define IDS_LANGUAGE_ID                STRINGID(00)
#define IDS_LANGUAGE_MENU_ITEM         STRINGID(01)
#define IDS_CLOCK                      STRINGID(02)
#define IDS_MENU_ON_TOP                STRINGID(03)

/* main menu */

#define CL_ON_TOP                99

#define CL_ANALOG                100
#define CL_DIGITAL               101
#define CL_FONT                  102
#define CL_WITHOUT_TITLE         103
#define CL_SECONDS               104
#define CL_DATE                  105

#define CL_LANGUAGE              200

#define CL_INFO                  301
#define CL_INFO_LICENSE          302
#define CL_INFO_NO_WARRANTY      303
#define CL_INFO_ABOUT_WINE       304

#define CL_FIRST_LANGUAGE        200
#define CL_LAST_LANGUAGE         220
