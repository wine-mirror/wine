/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 */

#ifndef PROGMAN_H
#define PROGMAN_H

#ifndef RC_INVOKED

#include "windows.h"

#define MAX_PATHNAME_LEN 1024

/* Fallback icon */
#ifdef WINELIB
#define DEFAULTICON OIC_WINEICON
#else
#define DEFAULTICON OIC_LANDSCAPE
#endif

/* Icon index in M$ Window's progman.exe  */
#define PROGMAN_ICON_INDEX 0
#define GROUP_ICON_INDEX   6
#define DEFAULT_ICON_INDEX 7

#define DEF_GROUP_WIN_XPOS   100
#define DEF_GROUP_WIN_YPOS   100
#define DEF_GROUP_WIN_WIDTH  300
#define DEF_GROUP_WIN_HEIGHT 200

typedef struct
{
  HLOCAL   hGroup;
  HLOCAL   hPrior;
  HLOCAL   hNext;
  HWND     hWnd;
  /**/              /* Numbers are byte indexes in *.grp */

  /**/                       /* Program entry */
  INT      x, y;               /*  0 -  3 */
  INT      nIconIndex;         /*  4 -  5 */
  HICON    hIcon;
  /* icon flags ??? */         /*  6 -  7 */
  /* iconANDsize */            /*  8 -  9 */
  /* iconXORsize */            /* 10 - 11 */
  /* pointer to IconInfo    */ /* 12 - 13 */
  /* pointer to iconXORbits */ /* 14 - 15 */ /* sometimes iconANDbits ?! */
  /* pointer to iconANDbits */ /* 16 - 17 */ /* sometimes iconXORbits ?! */
  HLOCAL   hName;              /* 18 - 19 */
  HLOCAL   hCmdLine;           /* 20 - 21 */
  HLOCAL   hIconFile;          /* 22 - 23 */        
  HLOCAL   hWorkDir;           /* Extension 0x8101 */
  INT      nHotKey;            /* Extension 0x8102 */
  /* Modifier: bit 8... */
  INT      nCmdShow;           /* Extension 0x8103 */

  /**/                         /* IconInfo */
  /* HotSpot x   ??? */        /*  0 -  1 */
  /* HotSpot y   ??? */        /*  2 -  3 */
  /* Width           */        /*  4 -  5 */
  /* Height          */        /*  6 -  7 */
  /* WidthBytes  ??? */        /*  8 -  9 */
  /* Planes          */        /* 10 - 10 */
  /* BitsPerPixel    */        /* 11 - 11 */
} PROGRAM;

typedef struct
{
  HLOCAL   hPrior;
  HLOCAL   hNext;
  HWND     hWnd;
  HLOCAL   hGrpFile;
  HLOCAL   hActiveProgram;
  BOOL     bFileNameModified;
  BOOL     bOverwriteFileOk;
  INT      seqnum;

  /**/                         /* Absolute */
  /* magic `PMCC'  */          /*  0 -  3 */
  /* checksum      */          /*  4 -  5 */
  /* Extension ptr */          /*  6 -  7 */
  INT      nCmdShow;           /*  8 -  9 */
  INT      x, y;               /* 10 - 13 */
  INT      width, height;      /* 14 - 17 */
  INT      iconx, icony;       /* 18 - 21 */
  HLOCAL   hName;              /* 22 - 23 */
  /* unknown */                /* 24 - 31 */
  /* number of programs */     /* 32 - 33 */
  HLOCAL   hPrograms;          /* 34 ...  */

  /**/                        /* Extensions */
  /* Extension type */         /*  0 -  1 */           
  /* Program number */         /*  2 -  3 */
  /* Size of entry  */         /*  4 -  5 */
  /* Data           */         /*  6 ...  */

  /* magic `PMCC' */           /* Extension 0x8000 */
  /* End of Extensions */      /* Extension 0xffff */
} GROUP;

typedef struct
{
  HANDLE  hInstance;
  HANDLE  hAccel;
  HWND    hMainWnd;
  HWND    hMDIWnd;
  HICON   hMainIcon;
  HICON   hGroupIcon;
  HICON   hDefaultIcon;
  HMENU   hMainMenu;
  HMENU   hFileMenu;
  HMENU   hOptionMenu;
  HMENU   hWindowsMenu;
  LPCSTR  lpszIniFile;
  LPCSTR  lpszIcoFile;
  BOOL    bAutoArrange;
  BOOL    bSaveSettings;
  BOOL    bMinOnRun;
  HLOCAL  hGroups;
  LPCSTR  lpszLanguage;
  LPCSTR *StringTable;
  /* FIXME should use MDI */
  HLOCAL  hActiveGroup;
} GLOBALS;

extern GLOBALS Globals;

VOID MAIN_ReplaceString(HLOCAL *handle, LPSTR replacestring);
VOID MAIN_NotImplementedError(void);
VOID MAIN_FileReadError(LPCSTR lpszPath);
VOID MAIN_FileWriteError(LPCSTR lpszPath);
VOID MAIN_GrpFileReadError(LPCSTR lpszPath);
VOID MAIN_OutOfMemoryError(void);
VOID MAIN_WinHelpError(void);

HLOCAL GRPFILE_ReadGroupFile(const char* path);
BOOL   GRPFILE_WriteGroupFile(HLOCAL hGroup);

ATOM   GROUP_RegisterGroupWinClass(void);
HLOCAL GROUP_AddGroup(LPCSTR lpszName, LPCSTR lpszGrpFile, INT showcmd,
		      INT x, INT y, INT width, INT heiht,
		      INT iconx, INT icony,
		      BOOL bModifiedFileName, BOOL bOverwriteFileOk,
		      /* FIXME shouldn't be necessary */
		      BOOL bSuppressShowWindow);
VOID   GROUP_NewGroup(void);
VOID   GROUP_ModifyGroup(HLOCAL hGroup);
VOID   GROUP_DeleteGroup(HLOCAL hGroup);
/* FIXME shouldn't be necessary */
VOID   GROUP_ShowGroupWindow(HLOCAL hGroup);
HLOCAL GROUP_FirstGroup(void);
HLOCAL GROUP_NextGroup(HLOCAL hGroup);
HLOCAL GROUP_ActiveGroup(void);
HWND   GROUP_GroupWnd(HLOCAL hGroup);
LPCSTR GROUP_GroupName(HLOCAL hGroup);

ATOM   PROGRAM_RegisterProgramWinClass(void);
HLOCAL PROGRAM_AddProgram(HLOCAL hGroup, HICON hIcon, LPCSTR lpszName,
			  INT x, INT y, LPCSTR lpszCmdLine,
			  LPCSTR lpszIconFile, INT nIconIndex,
			  LPCSTR lpszWorkDir, INT nHotKey, INT nCmdShow);
VOID   PROGRAM_NewProgram(HLOCAL hGroup);
VOID   PROGRAM_ModifyProgram(HLOCAL hProgram);
VOID   PROGRAM_CopyMoveProgram(HLOCAL hProgram, BOOL bMove);
VOID   PROGRAM_DeleteProgram(HLOCAL hProgram, BOOL BUpdateGrpFile);
HLOCAL PROGRAM_FirstProgram(HLOCAL hGroup);
HLOCAL PROGRAM_NextProgram(HLOCAL hProgram);
HLOCAL PROGRAM_ActiveProgram(HLOCAL hGroup);
LPCSTR PROGRAM_ProgramName(HLOCAL hProgram);
VOID   PROGRAM_ExecuteProgram(HLOCAL hLocal);

INT    DIALOG_New(INT nDefault);
HLOCAL DIALOG_CopyMove(LPCSTR lpszProgramName, LPCSTR lpszGroupName, BOOL bMove);
BOOL   DIALOG_Delete(LPCSTR lpszFormat, LPCSTR lpszName);
BOOL   DIALOG_GroupAttributes(LPSTR lpszTitle, LPSTR lpszPath, INT nSize);
BOOL   DIALOG_ProgramAttributes(LPSTR lpszTitle, LPSTR lpszCmdLine,
				LPSTR lpszWorkDir, LPSTR lpszIconFile,
				HICON *lphIcon, INT *nIconIndex,
				INT *lpnHotKey, INT *lpnCmdShow, INT nSize);
VOID   DIALOG_Symbol(HICON *lphIcon, LPSTR lpszIconFile,
		     INT *lpnIconIndex, INT nSize);
VOID   DIALOG_Execute(void);

VOID STRING_SelectLanguage(LPCSTR lang);

/* Class names */
extern CHAR STRING_MAIN_WIN_CLASS_NAME[];
extern CHAR STRING_MDI_WIN_CLASS_NAME[];
extern CHAR STRING_GROUP_WIN_CLASS_NAME[];
extern CHAR STRING_PROGRAM_WIN_CLASS_NAME[];

/* Resource names */
extern CHAR STRING_ACCEL[];
extern CHAR STRING_MAIN_Xx[];
extern CHAR STRING_NEW_Xx[];
extern CHAR STRING_OPEN_Xx[];
extern CHAR STRING_MOVE_Xx[];
extern CHAR STRING_COPY_Xx[];
extern CHAR STRING_DELETE_Xx[];
extern CHAR STRING_GROUP_Xx[];
extern CHAR STRING_PROGRAM_Xx[];
extern CHAR STRING_SYMBOL_Xx[];
extern CHAR STRING_EXECUTE_Xx[];

/* Strings */
#define STRING_PROGRAM_MANAGER            Globals.StringTable[ 0]
#define STRING_ERROR                      Globals.StringTable[ 1]
#define STRING_INFO                       Globals.StringTable[ 2]
#define STRING_DELETE                     Globals.StringTable[ 3]
#define STRING_DELETE_GROUP_s             Globals.StringTable[ 4]
#define STRING_DELETE_PROGRAM_s           Globals.StringTable[ 5]
#define STRING_NOT_IMPLEMENTED            Globals.StringTable[ 6]
#define STRING_FILE_READ_ERROR_s          Globals.StringTable[ 7]
#define STRING_FILE_WRITE_ERROR_s         Globals.StringTable[ 8]
#define STRING_GRPFILE_READ_ERROR_s       Globals.StringTable[ 9]
#define STRING_OUT_OF_MEMORY              Globals.StringTable[10]
#define STRING_WINHELP_ERROR              Globals.StringTable[11]
#define STRING_UNKNOWN_FEATURE_IN_GRPFILE Globals.StringTable[12]
#define STRING_FILE_NOT_OVERWRITTEN_s     Globals.StringTable[13]
#define STRING_SAVE_GROUP_AS_s            Globals.StringTable[14]
#define STRING_NO_HOT_KEY                 Globals.StringTable[15]
#define STRING_BROWSE_EXE_FILTER          Globals.StringTable[16]
#define STRING_BROWSE_ICO_FILTER          Globals.StringTable[17]
#define NUMBER_OF_STRINGS                                     18

extern LPCSTR StringTableCz[];
extern LPCSTR StringTableDa[];
extern LPCSTR StringTableDe[];
extern LPCSTR StringTableEn[];
extern LPCSTR StringTableEs[];
extern LPCSTR StringTableFi[];
extern LPCSTR StringTableFr[];
extern LPCSTR StringTableNo[];

#if defined(WINELIB) && !defined(HAVE_WINE_CONSTRUCTOR)
  VOID LIBWINE_Register_accel(void);
  VOID LIBWINE_Register_Cz(void);
  VOID LIBWINE_Register_Da(void);
  VOID LIBWINE_Register_De(void);
  VOID LIBWINE_Register_Es(void);
  VOID LIBWINE_Register_En(void);
  VOID LIBWINE_Register_Fi(void);
  VOID LIBWINE_Register_Fr(void);
  VOID LIBWINE_Register_No(void);
#endif

#endif /* !RC_INVOKED */

/* Menu */

#define PM_NEW              100
#define PM_OPEN             101
#define PM_MOVE             102
#define PM_COPY             103
#define PM_DELETE           104
#define PM_ATTRIBUTES       105
#define PM_EXECUTE          107
#define PM_EXIT             108

#define PM_AUTO_ARRANGE     200
#define PM_MIN_ON_RUN       201
#define PM_SAVE_SETTINGS    203

#define PM_OVERLAP          300
#define PM_SIDE_BY_SIDE     301
#define PM_ARRANGE          302
#define PM_FIRST_CHILD      3030

#define PM_En               400
#define PM_Es               401
#define PM_De               402
#define PM_No               403
#define PM_Fr               404
#define PM_Fi               405
#define PM_Da               406
#define PM_Cz               407

#define PM_CONTENTS         501
#define PM_SEARCH           502
#define PM_HELPONHELP       503
#define PM_TUTORIAL         504

#define PM_LICENSE          510
#define PM_NO_WARRANTY      511
#define PM_ABOUT_WINE       512

/* Dialog `New' */

/* RADIOBUTTON: The next two must be in sequence */
#define PM_NEW_GROUP        1000
#define PM_NEW_PROGRAM      1001
#define PM_NEW_GROUP_TXT    1002
#define PM_NEW_PROGRAM_TXT  1003

/* Dialogs `Copy', `Move' */

#define PM_PROGRAM          1200
#define PM_FROM_GROUP       1201
#define PM_TO_GROUP         1202
#define PM_TO_GROUP_TXT     1203

/* Dialogs `Group attributes' */

#define PM_DESCRIPTION      1500
#define PM_DESCRIPTION_TXT  1501
#define PM_FILE             1502
#define PM_FILE_TXT         1503

/* Dialogs `Program attributes' */
#define PM_COMMAND_LINE     1510
#define PM_COMMAND_LINE_TXT 1511
#define PM_DIRECTORY        1512
#define PM_DIRECTORY_TXT    1513
#define PM_HOT_KEY          1514
#define PM_HOT_KEY_TXT      1515
#define PM_ICON             1516
#define PM_OTHER_SYMBOL     1517

/* Dialog `Symbol' */

#define PM_ICON_FILE        1520
#define PM_ICON_FILE_TXT    1521
#define PM_SYMBOL_LIST      1522
#define PM_SYMBOL_LIST_TXT  1523

/* Dialog `Execute' */

#define PM_COMMAND          1600
#define PM_SYMBOL           1601
#define PM_BROWSE           1602
#define PM_HELP             1603

#endif /* PROGMAN_H */

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
