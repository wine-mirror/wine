/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 */

#include <windows.h>
#include <commdlg.h>
#include "progman.h"

static BOOL DIALOG_Browse(HWND, LPCSTR, LPSTR, INT);
static LRESULT DIALOG_NEW_DlgProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT DIALOG_COPY_MOVE_DlgProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT DIALOG_GROUP_DlgProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT DIALOG_PROGRAM_DlgProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT DIALOG_SYMBOL_DlgProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT DIALOG_EXECUTE_DlgProc(HWND, UINT, WPARAM, LPARAM);

/***********************************************************************
 *
 *           DIALOG_New
 */

static struct
{
  INT nDefault;
} New;

INT DIALOG_New(INT nDefault)
{
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_NEW_DlgProc, Globals.hInstance);
  INT ret;

  New.nDefault = nDefault;

  ret = DialogBox(Globals.hInstance,  STRING_NEW_Xx,
		  Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);
  return ret;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_NEW_DlgProc
 */

static LRESULT DIALOG_NEW_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_INITDIALOG:
      CheckRadioButton(hDlg, PM_NEW_GROUP, PM_NEW_PROGRAM, New.nDefault);
      break;

    case WM_COMMAND:
      switch (wParam)
	{
	case PM_NEW_GROUP:
	case PM_NEW_PROGRAM:
	  CheckRadioButton(hDlg, PM_NEW_GROUP, PM_NEW_PROGRAM, wParam);
	  return TRUE;

	case IDOK:
	  EndDialog(hDlg, IsDlgButtonChecked(hDlg, PM_NEW_GROUP) ?
		    PM_NEW_GROUP : PM_NEW_PROGRAM);
	  return TRUE;

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
    }
  return FALSE;
}

/***********************************************************************
 *
 *           DIALOG_CopyMove
 */

static struct
{
  LPCSTR lpszProgramName, lpszFromGroupName;
  HLOCAL hToGroup;
} CopyMove;

HLOCAL DIALOG_CopyMove(LPCSTR lpszProgramName, LPCSTR lpszFromGroupName,
		     BOOL bMove)
{
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_COPY_MOVE_DlgProc, Globals.hInstance);
  INT ret;

  CopyMove.lpszProgramName   = lpszProgramName;
  CopyMove.lpszFromGroupName = lpszFromGroupName;
  CopyMove.hToGroup          = 0;

  ret = DialogBox(Globals.hInstance,
		  bMove ? STRING_MOVE_Xx : STRING_COPY_Xx,
		  Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);

  return((ret == IDOK) ? CopyMove.hToGroup : 0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_COPY_MOVE_DlgProc
 */

static LRESULT DIALOG_COPY_MOVE_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HLOCAL hGroup;

  switch (msg)
    {
    case WM_INITDIALOG:
      /* List all group names */
      for (hGroup = GROUP_FirstGroup(); hGroup; hGroup = GROUP_NextGroup(hGroup))
	SendDlgItemMessage(hDlg, PM_TO_GROUP, CB_ADDSTRING, 0,
			   (LPARAM) GROUP_GroupName(hGroup));

      SetDlgItemText(hDlg, PM_PROGRAM,    (LPSTR)CopyMove.lpszProgramName);
      SetDlgItemText(hDlg, PM_FROM_GROUP, (LPSTR)CopyMove.lpszFromGroupName);
      break;

    case WM_COMMAND:
      switch (wParam)
	{
	case IDOK:
	{
	  /* Get selected group */
	  INT nCurSel    = SendDlgItemMessage(hDlg, PM_TO_GROUP, CB_GETCURSEL, 0, 0);
	  INT nLen       = SendDlgItemMessage(hDlg, PM_TO_GROUP, CB_GETLBTEXTLEN, nCurSel, 0);
	  HLOCAL hBuffer = LocalAlloc(LMEM_FIXED, nLen + 1);
	  LPSTR   buffer = LocalLock(hBuffer);

	  SendDlgItemMessage(hDlg, PM_TO_GROUP, CB_GETLBTEXT, nCurSel, (LPARAM)buffer);
	  for (hGroup = GROUP_FirstGroup(); hGroup; hGroup = GROUP_NextGroup(hGroup))
	    if (!lstrcmp(buffer, GROUP_GroupName(hGroup))) break;
	  LocalFree(hBuffer);

	  CopyMove.hToGroup = hGroup;
	  EndDialog(hDlg, IDOK);
	  return TRUE;
	}

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
    }
  return FALSE;
}

/***********************************************************************
 *
 *           DIALOG_Delete
 */

BOOL DIALOG_Delete(LPCSTR lpszFormat_s, LPCSTR lpszName)
{
  CHAR msg[1000];
  if (sizeof(msg) <= lstrlen(lpszFormat_s) + lstrlen(lpszName)) return FALSE;
  wsprintf(msg, (LPSTR)lpszFormat_s, lpszName);
  return (IDYES == MessageBox(Globals.hMainWnd, msg, STRING_DELETE,
			      MB_YESNO | MB_DEFBUTTON2));
}


/***********************************************************************
 *
 *           DIALOG_GroupAttributes
 */

static struct
{
  LPSTR lpszTitle, lpszGrpFile;
  INT   nSize;
} GroupAttributes;

BOOL DIALOG_GroupAttributes(LPSTR lpszTitle, LPSTR lpszGrpFile, INT nSize)
{
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_GROUP_DlgProc, Globals.hInstance);
  INT ret;

  GroupAttributes.nSize       = nSize;
  GroupAttributes.lpszTitle   = lpszTitle;
  GroupAttributes.lpszGrpFile = lpszGrpFile;

  ret = DialogBox(Globals.hInstance,  STRING_GROUP_Xx,
		  Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);
  return(ret == IDOK);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_GROUP_DlgProc
 */

static LRESULT DIALOG_GROUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_INITDIALOG:
      SetDlgItemText(hDlg, PM_DESCRIPTION, GroupAttributes.lpszTitle);
      SetDlgItemText(hDlg, PM_FILE, GroupAttributes.lpszGrpFile);
      break;

    case WM_COMMAND:
      switch (wParam)
	{
	case IDOK:
	  GetDlgItemText(hDlg, PM_DESCRIPTION, GroupAttributes.lpszTitle,
			 GroupAttributes.nSize);
	  GetDlgItemText(hDlg, PM_FILE, GroupAttributes.lpszGrpFile,
			 GroupAttributes.nSize);
	  EndDialog(hDlg, IDOK);
	  return TRUE;

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
    }
  return FALSE;
}

/***********************************************************************
 *
 *           DIALOG_ProgramAttributes
 */

static struct
{
  LPSTR lpszTitle, lpszCmdLine, lpszWorkDir, lpszIconFile, lpszTmpIconFile;
  INT   nSize;
  INT   *lpnCmdShow;
  INT   *lpnHotKey;
  HWND  hSelGroupWnd;
  HICON *lphIcon, hTmpIcon;
  INT   *lpnIconIndex, nTmpIconIndex;
} ProgramAttributes;

BOOL DIALOG_ProgramAttributes(LPSTR lpszTitle, LPSTR lpszCmdLine,
			      LPSTR lpszWorkDir, LPSTR lpszIconFile,
			      HICON *lphIcon, INT *lpnIconIndex,
			      INT *lpnHotKey, INT *lpnCmdShow, INT nSize)
{
  CHAR szTmpIconFile[MAX_PATHNAME_LEN];
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_PROGRAM_DlgProc, Globals.hInstance);
  INT ret;

  ProgramAttributes.nSize = nSize;
  ProgramAttributes.lpszTitle = lpszTitle;
  ProgramAttributes.lpszCmdLine = lpszCmdLine;
  ProgramAttributes.lpszWorkDir = lpszWorkDir;
  ProgramAttributes.lpszIconFile = lpszIconFile;
  ProgramAttributes.lpnCmdShow = lpnCmdShow;
  ProgramAttributes.lpnHotKey = lpnHotKey;
  ProgramAttributes.lphIcon = lphIcon;
  ProgramAttributes.lpnIconIndex = lpnIconIndex;

#if 0
  ProgramAttributes.hTmpIcon = 0;
#else
  ProgramAttributes.hTmpIcon = *lphIcon;
#endif
  ProgramAttributes.nTmpIconIndex = *lpnIconIndex;
  ProgramAttributes.lpszTmpIconFile = szTmpIconFile;
  lstrcpyn(ProgramAttributes.lpszTmpIconFile, lpszIconFile, MAX_PATHNAME_LEN);

  ret = DialogBox(Globals.hInstance,  STRING_PROGRAM_Xx,
		  Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);

  return(ret == IDOK);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PROGRAM_DlgProc
 */

static LRESULT DIALOG_PROGRAM_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_INITDIALOG:
      SetDlgItemText(hDlg, PM_DESCRIPTION, ProgramAttributes.lpszTitle);
      SetDlgItemText(hDlg, PM_COMMAND_LINE, ProgramAttributes.lpszCmdLine);
      SetDlgItemText(hDlg, PM_DIRECTORY, ProgramAttributes.lpszWorkDir);
      if (!*ProgramAttributes.lpnHotKey)
	SetDlgItemText(hDlg, PM_HOT_KEY, (LPSTR)STRING_NO_HOT_KEY);

      CheckDlgButton(hDlg, PM_SYMBOL,
		     (*ProgramAttributes.lpnCmdShow == SW_SHOWMINIMIZED));
      SendDlgItemMessage(hDlg, PM_ICON, STM_SETICON,
			 (WPARAM) ProgramAttributes.hTmpIcon, 0);
      break;

    case WM_COMMAND:
      switch (wParam)
	{
	case PM_SYMBOL:
	  CheckDlgButton(hDlg, PM_SYMBOL, !IsDlgButtonChecked(hDlg, PM_SYMBOL));
	  return TRUE;

	case PM_BROWSE:
	  {
	    CHAR filename[MAX_PATHNAME_LEN];
	    if (DIALOG_Browse(hDlg, STRING_BROWSE_EXE_FILTER,
			      filename, sizeof(filename)))
	      SetDlgItemText(hDlg, PM_COMMAND_LINE, filename);
	    return TRUE;
	  }

	case PM_OTHER_SYMBOL:
	  {
	    DIALOG_Symbol(&ProgramAttributes.hTmpIcon,
			  ProgramAttributes.lpszTmpIconFile,
			  &ProgramAttributes.nTmpIconIndex,
			  MAX_PATHNAME_LEN);

	    SendDlgItemMessage(hDlg, PM_ICON, STM_SETICON,
			       (WPARAM) ProgramAttributes.hTmpIcon, 0);
	    return TRUE;
	  }

	case IDOK:
	  GetDlgItemText(hDlg, PM_DESCRIPTION,
			 ProgramAttributes.lpszTitle,
			 ProgramAttributes.nSize);
	  GetDlgItemText(hDlg, PM_COMMAND_LINE,
			 ProgramAttributes.lpszCmdLine,
			 ProgramAttributes.nSize);
	  GetDlgItemText(hDlg, PM_DIRECTORY,
			 ProgramAttributes.lpszWorkDir,
			 ProgramAttributes.nSize);

	  if (ProgramAttributes.hTmpIcon)
	    {
#if 0
	      if (*ProgramAttributes.lphIcon)
		DestroyIcon(*ProgramAttributes.lphIcon);
#endif
	      *ProgramAttributes.lphIcon = ProgramAttributes.hTmpIcon;
	      *ProgramAttributes.lpnIconIndex = ProgramAttributes.nTmpIconIndex;
	      lstrcpyn(ProgramAttributes.lpszIconFile,
		       ProgramAttributes.lpszTmpIconFile,
		       ProgramAttributes.nSize);
	    }

	  *ProgramAttributes.lpnCmdShow =
	    IsDlgButtonChecked(hDlg, PM_SYMBOL) ?
	    SW_SHOWMINIMIZED : SW_SHOWNORMAL;
	  EndDialog(hDlg, IDOK);
	  return TRUE;

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
      return FALSE;
    }
  return FALSE;
}

/***********************************************************************
 *
 *           DIALOG_Symbol
 */

static struct
{
  LPSTR  lpszIconFile;
  INT    nSize;
  HICON  *lphIcon;
  INT    *lpnIconIndex;
} Symbol;

VOID DIALOG_Symbol(HICON *lphIcon, LPSTR lpszIconFile,
		   INT *lpnIconIndex, INT nSize)
{
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_SYMBOL_DlgProc, Globals.hInstance);

  Symbol.nSize = nSize;
  Symbol.lpszIconFile = lpszIconFile;
  Symbol.lphIcon = lphIcon;
  Symbol.lpnIconIndex = lpnIconIndex;

  DialogBox(Globals.hInstance, STRING_SYMBOL_Xx,
	    Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_SYMBOL_DlgProc
 */

static LRESULT DIALOG_SYMBOL_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_INITDIALOG:
      SetDlgItemText(hDlg, PM_ICON_FILE, Symbol.lpszIconFile);
      SendDlgItemMessage(hDlg, PM_SYMBOL_LIST, CB_SETITEMHEIGHT, 0, (LPARAM) 32);
      SendDlgItemMessage(hDlg, PM_SYMBOL_LIST, CB_ADDSTRING, 0, (LPARAM)*Symbol.lphIcon);
      SendDlgItemMessage(hDlg, PM_SYMBOL_LIST, CB_ADDSTRING, 0, (LPARAM)Globals.hDefaultIcon);
      SendDlgItemMessage(hDlg, PM_SYMBOL_LIST, CB_SETCURSEL, 0, 0);
      return TRUE;

    case WM_MEASUREITEM:
      {
	PMEASUREITEMSTRUCT measure = (PMEASUREITEMSTRUCT) lParam;
	measure->itemWidth = 32;
	measure->itemHeight = 32;
	return TRUE;
      }

    case WM_DRAWITEM:
      {
	PDRAWITEMSTRUCT dis = (PDRAWITEMSTRUCT) lParam;
	DrawIcon(dis->hDC, dis->rcItem.left, dis->rcItem.top, (HICON)dis->itemData);
	return TRUE;
      }

    case WM_COMMAND:
      switch (wParam)
	{
	case PM_BROWSE:
	  {
	    CHAR filename[MAX_PATHNAME_LEN];
	    if (DIALOG_Browse(hDlg, STRING_BROWSE_ICO_FILTER,
			      filename, sizeof(filename)))
	      SetDlgItemText(hDlg, PM_ICON_FILE, filename);
	    return TRUE;
	  }

	case PM_HELP:
	  MAIN_NotImplementedError();
	  return TRUE;

	case IDOK:
	  {
	    INT nCurSel = SendDlgItemMessage(hDlg, PM_SYMBOL_LIST, CB_GETCURSEL, 0, 0);

	    GetDlgItemText(hDlg, PM_ICON_FILE, Symbol.lpszIconFile, Symbol.nSize);

	    *Symbol.lphIcon = (HICON)SendDlgItemMessage(hDlg, PM_SYMBOL_LIST,
							CB_GETITEMDATA,
							(WPARAM) nCurSel, 0);
#if 0
	    *Symbol.lphIcon = CopyIcon(*Symbol.lphIcon);
#endif

	    EndDialog(hDlg, IDOK);
	    return TRUE;
	  }

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
    }
  return FALSE;
}

/***********************************************************************
 *
 *           DIALOG_Execute
 */

VOID DIALOG_Execute()
{
  WNDPROC lpfnDlg = MakeProcInstance(DIALOG_EXECUTE_DlgProc, Globals.hInstance);
  DialogBox(Globals.hInstance, STRING_EXECUTE_Xx,
	    Globals.hMainWnd, lpfnDlg);
  FreeProcInstance(lpfnDlg);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_EXECUTE_DlgProc
 */

static LRESULT DIALOG_EXECUTE_DlgProc(HWND hDlg, UINT msg,
				      WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_COMMAND:
      switch (wParam)
	{
	case PM_SYMBOL:
	  CheckDlgButton(hDlg, PM_SYMBOL, !IsDlgButtonChecked(hDlg, PM_SYMBOL));
	  return TRUE;

	case PM_BROWSE:
	  {
	    CHAR filename[MAX_PATHNAME_LEN];
	    if (DIALOG_Browse(hDlg, STRING_BROWSE_EXE_FILTER,
			      filename, sizeof(filename)))
	      SetDlgItemText(hDlg, PM_COMMAND, filename);
	    return TRUE;
	  }

	case PM_HELP:
	  MAIN_NotImplementedError();
	  return TRUE;

	case IDOK:
	  {
	    CHAR cmdline[MAX_PATHNAME_LEN];
	    GetDlgItemText(hDlg, PM_COMMAND, cmdline, sizeof(cmdline));

	    WinExec(cmdline, IsDlgButtonChecked(hDlg, PM_SYMBOL) ?
		    SW_SHOWMINIMIZED : SW_SHOWNORMAL);
	    if (Globals.bMinOnRun) CloseWindow(Globals.hMainWnd);

	    EndDialog(hDlg, IDOK);
	    return TRUE;
	  }

	case IDCANCEL:
	  EndDialog(hDlg, IDCANCEL);
	  return TRUE;
	}
    }
  return FALSE;
}

/***********************************************************************
 *
 *           DIALOG_Browse
 */

/* FIXME is this correct ? */
static BOOL DIALOG_Browse(HWND hDlg, LPCSTR lpcstrFilter,
			  LPSTR lpstrFile, INT nMaxFile)
{
  OPENFILENAME openfilename;
  openfilename.lStructSize       = 0;
  openfilename.hwndOwner         = hDlg;
  openfilename.hInstance         = Globals.hInstance;
  openfilename.lpstrFilter       = (LPSTR)lpcstrFilter;
  openfilename.lpstrCustomFilter = 0;
  openfilename.nMaxCustFilter    = 0;
  openfilename.nFilterIndex      = 0;
  openfilename.lpstrFile         = lpstrFile;
  openfilename.nMaxFile          = sizeof(lpstrFile);
  openfilename.lpstrFileTitle    = 0;
  openfilename.nMaxFileTitle     = 0;
  openfilename.lpstrInitialDir   = 0;
  openfilename.lpstrTitle        = 0;
  openfilename.Flags             = 0;
  openfilename.nFileOffset       = 0;
  openfilename.nFileExtension    = 0;
  openfilename.lpstrDefExt       = 0;
  openfilename.lCustData         = 0;
  openfilename.lpfnHook          = 0;
  openfilename.lpTemplateName    = 0;
  return GetOpenFileName(&openfilename);
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
