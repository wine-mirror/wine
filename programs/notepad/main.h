/*
 * Notepad (notepad.h)
 *
 * Copyright 1997 Marcel Baur <mbaur@g26.ethz.ch>
 */

#define MAX_STRING_LEN      255
#define MAX_PATHNAME_LEN    1024
#define MAX_LANGUAGE_NUMBER (NP_LAST_LANGUAGE - NP_FIRST_LANGUAGE)

#define HELPFILE "notepad.hlp"
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
  HMENU   hFileMenu;
  HMENU   hEditMenu;
  HMENU   hSearchMenu;
  HMENU   hLanguageMenu;
  HMENU   hHelpMenu;
  LPCSTR  lpszIniFile;
  LPCSTR  lpszIcoFile;
  LPCSTR  lpszLanguage;
  UINT    wStringTableOffset;
  BOOL    bWrapLongLines;
} NOTEPAD_GLOBALS;

extern NOTEPAD_GLOBALS Globals;

/* function prototypes */

/* class names */

/* Resource names */
extern CHAR STRING_MENU_Xx[];
extern CHAR STRING_PAGESETUP_Xx[];

#define STRINGID(id) (0x##id + Globals.wStringTableOffset)
   
#else  /* RC_INVOKED */

#define STRINGID(id) id
   
#endif

/* string table index */
#define IDS_LANGUAGE_ID                	STRINGID(00)
#define IDS_LANGUAGE_MENU_ITEM         	STRINGID(01)
#define IDS_NOTEPAD                    	STRINGID(02)
#define IDS_TEXT_FILES_TXT	       	STRINGID(03)
#define IDS_ALL_FILES                  	STRINGID(04)
#define IDS_ERROR			STRINGID(05)
#define IDS_WARNING			STRINGID(06)
#define IDS_INFO			STRINGID(07)
#define IDS_TOOLARGE			STRINGID(08)
#define IDS_NOTEXT			STRINGID(09)
#define IDS_NOTFOUND			STRINGID(0A)
#define IDS_OUT_OF_MEMORY		STRINGID(0B)

/* main menu */

#define NP_FILE_NEW              100
#define NP_FILE_OPEN             101
#define NP_FILE_SAVE             102
#define NP_FILE_SAVEAS           103
#define NP_FILE_PRINT            104
#define NP_FILE_PAGESETUP        105
#define NP_FILE_PRINTSETUP       106
#define NP_FILE_EXIT             107

#define NP_EDIT_UNDO             200
#define NP_EDIT_CUT              201
#define NP_EDIT_COPY             202
#define NP_EDIT_PASTE            203
#define NP_EDIT_DELETE           204
#define NP_EDIT_SELECTALL        205
#define NP_EDIT_TIMEDATE         206
#define NP_EDIT_WRAP             207

#define NP_SEARCH_SEARCH         300
#define NP_SEARCH_NEXT           301

#define NP_FIRST_LANGUAGE        400
#define NP_LAST_LANGUAGE         499

#define NP_HELP_CONTENTS         500
#define NP_HELP_SEARCH           501
#define NP_HELP_ON_HELP          502
#define NP_HELP_LICENSE          503
#define NP_HELP_NO_WARRANTY      504
#define NP_HELP_ABOUT_WINE       505


/* Dialog `Page Setup' */

#define NP_PAGESETUP_HEAD	1000
#define NP_PAGESETUP_HEAD_TXT	1001
#define NP_PAGESETUP_TAIL	1002
#define NP_PAGESETUP_TAIL_TXT	1003
#define NP_PAGESETUP_LEFT       1005
#define NP_PAGESETUP_LEFT_TXT	1006
#define NP_PAGESETUP_RIGHT   	1007
#define NP_PAGESETUP_RIGHT_TXT	1008
#define NP_PAGESETUP_TOP      	1009
#define NP_PAGESETUP_TOP_TXT	1010
#define NP_PAGESETUP_BOTTOM     1011
#define NP_PAGESETUP_BOTTOM_TXT	1012
#define NP_HELP			1013
#define NP_PAGESETUP_MARGIN 	1014


/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
