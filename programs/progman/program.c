/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 */

#include <string.h>
#include "windows.h"
#include "windowsx.h"
#include "progman.h"

/***********************************************************************
 *
 *           PROGRAM_ProgramWndProc
 */

static LRESULT PROGRAM_ProgramWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_NCLBUTTONDOWN:
      {
	HLOCAL  hProgram = (HLOCAL) GetWindowLong(hWnd, 0);
	PROGRAM *program = LocalLock(hProgram);
	GROUP   *group   = LocalLock(program->hGroup);
	group->hActiveProgram = hProgram;
	EnableMenuItem(Globals.hFileMenu, PM_MOVE , MF_ENABLED);
	EnableMenuItem(Globals.hFileMenu, PM_COPY , MF_ENABLED);
	break;
      }
    case WM_NCLBUTTONDBLCLK:
      {
	PROGRAM_ExecuteProgram((HLOCAL) GetWindowLong(hWnd, 0));
	return(0);
      }

    case WM_PAINTICON:
    case WM_NCPAINT:
      {
	PROGRAM *program;
	PAINTSTRUCT      ps;
	HDC              hdc;
	hdc     = BeginPaint(hWnd,&ps);
	program = LocalLock((HLOCAL) GetWindowLong(hWnd, 0));
	if (program->hIcon)
	  DrawIcon(hdc, 0, 0, program->hIcon);
	EndPaint(hWnd,&ps);
	break;
      }
    }
  return(DefWindowProc(hWnd, msg, wParam, lParam));
}

/***********************************************************************
 *
 *           PROGRAM_RegisterProgramWinClass
 */

ATOM PROGRAM_RegisterProgramWinClass()
{
  WNDCLASS class;

  class.style         = CS_HREDRAW | CS_VREDRAW;
  class.lpfnWndProc   = PROGRAM_ProgramWndProc;
  class.cbClsExtra    = 0;
  class.cbWndExtra    = sizeof(LONG);
  class.hInstance     = Globals.hInstance;
  class.hIcon         = 0;
  class.hCursor       = LoadCursor (0, IDC_ARROW);
  class.hbrBackground = GetStockObject (WHITE_BRUSH);
  class.lpszMenuName  = 0;
  class.lpszClassName = STRING_PROGRAM_WIN_CLASS_NAME;

  return RegisterClass(&class);
}

/***********************************************************************
 *
 *           PROGRAM_NewProgram
 */

VOID PROGRAM_NewProgram(HLOCAL hGroup)
{
  INT  nCmdShow = SW_SHOWNORMAL;
  INT  nHotKey = 0;
  INT  nIconIndex = 0;
  CHAR szName[MAX_PATHNAME_LEN] = "";
  CHAR szCmdLine[MAX_PATHNAME_LEN] = "";
  CHAR szIconFile[MAX_PATHNAME_LEN] = "";
  CHAR szWorkDir[MAX_PATHNAME_LEN] = "";
  HICON hIcon = 0;

  if (!DIALOG_ProgramAttributes(szName, szCmdLine, szWorkDir, szIconFile,
				&hIcon, &nIconIndex, &nHotKey,
				&nCmdShow, MAX_PATHNAME_LEN))
    return;

  if (!hIcon) hIcon = LoadIcon(0, MAKEINTRESOURCE(OIC_WINEICON));


  if (!PROGRAM_AddProgram(hGroup, hIcon, szName, 0, 0, szCmdLine, szIconFile,
			  nIconIndex, szWorkDir, nHotKey, nCmdShow))
    return;

  GRPFILE_WriteGroupFile(hGroup);
}

/***********************************************************************
 *
 *           PROGRAM_ModifyProgram
 */

VOID PROGRAM_ModifyProgram(HLOCAL hProgram)
{
  PROGRAM *program = LocalLock(hProgram);
  CHAR szName[MAX_PATHNAME_LEN];
  CHAR szCmdLine[MAX_PATHNAME_LEN];
  CHAR szIconFile[MAX_PATHNAME_LEN];
  CHAR szWorkDir[MAX_PATHNAME_LEN];

  lstrcpyn(szName, LocalLock(program->hName), MAX_PATHNAME_LEN);
  lstrcpyn(szCmdLine, LocalLock(program->hCmdLine), MAX_PATHNAME_LEN);
  lstrcpyn(szIconFile, LocalLock(program->hIconFile), MAX_PATHNAME_LEN);
  lstrcpyn(szWorkDir, LocalLock(program->hWorkDir), MAX_PATHNAME_LEN);

  if (!DIALOG_ProgramAttributes(szName, szCmdLine, szWorkDir, szIconFile,
				&program->hIcon, &program->nIconIndex,
				&program->nHotKey, &program->nCmdShow,
				MAX_PATHNAME_LEN))
    return;

  MAIN_ReplaceString(&program->hName, szName);
  MAIN_ReplaceString(&program->hCmdLine, szCmdLine);
  MAIN_ReplaceString(&program->hIconFile, szIconFile);
  MAIN_ReplaceString(&program->hWorkDir, szWorkDir);

  SetWindowText(program->hWnd, szName);
  UpdateWindow(program->hWnd);

  GRPFILE_WriteGroupFile(program->hGroup);

  return;
}

/***********************************************************************
 *
 *           PROGRAM_AddProgram
 */

HLOCAL PROGRAM_AddProgram(HLOCAL hGroup, HICON hIcon, LPCSTR lpszName,
			  INT x, INT y, LPCSTR lpszCmdLine,
			  LPCSTR lpszIconFile, INT nIconIndex,
			  LPCSTR lpszWorkDir, INT nHotKey, INT nCmdShow)
{
  GROUP *group = LocalLock(hGroup);
  PROGRAM *program;
  HLOCAL hPrior, *p;
  HLOCAL hProgram  = LocalAlloc(LMEM_FIXED, sizeof(PROGRAM));
  HLOCAL hName     = LocalAlloc(LMEM_FIXED, 1 + lstrlen(lpszName));
  HLOCAL hCmdLine  = LocalAlloc(LMEM_FIXED, 1 + lstrlen(lpszCmdLine));
  HLOCAL hIconFile = LocalAlloc(LMEM_FIXED, 1 + lstrlen(lpszIconFile));
  HLOCAL hWorkDir  = LocalAlloc(LMEM_FIXED, 1 + lstrlen(lpszWorkDir));
  if (!hProgram || !hName || !hCmdLine || !hIconFile || !hWorkDir)
    {
      MAIN_MessageBoxIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_OK);
      if (hProgram)  LocalFree(hProgram);
      if (hName)     LocalFree(hName);
      if (hCmdLine)  LocalFree(hCmdLine);
      if (hIconFile) LocalFree(hIconFile);
      if (hWorkDir)  LocalFree(hWorkDir);
      return(0);
    }
  hmemcpy16(LocalLock(hName),     lpszName,     1 + lstrlen(lpszName));
  hmemcpy16(LocalLock(hCmdLine),  lpszCmdLine,  1 + lstrlen(lpszCmdLine));
  hmemcpy16(LocalLock(hIconFile), lpszIconFile, 1 + lstrlen(lpszIconFile));
  hmemcpy16(LocalLock(hWorkDir),  lpszWorkDir,  1 + lstrlen(lpszWorkDir));

  group->hActiveProgram  = hProgram;

  hPrior = 0;
  p = &group->hPrograms;
  while (*p)
    {
      hPrior = *p;
      p = &((PROGRAM*)LocalLock(hPrior))->hNext;
    }
  *p = hProgram;

  program = LocalLock(hProgram);
  program->hGroup     = hGroup;
  program->hPrior     = hPrior;
  program->hNext      = 0;
  program->hName      = hName;
  program->hCmdLine   = hCmdLine;
  program->hIconFile  = hIconFile;
  program->nIconIndex = nIconIndex;
  program->hWorkDir   = hWorkDir;
  program->hIcon      = hIcon;
  program->nCmdShow   = nCmdShow;
  program->nHotKey    = nHotKey;

  program->hWnd =
    CreateWindow (STRING_PROGRAM_WIN_CLASS_NAME, (LPSTR)lpszName,
		  WS_CHILD | WS_CAPTION,
		  x, y, CW_USEDEFAULT, CW_USEDEFAULT,
		  group->hWnd, 0, Globals.hInstance, 0);

  SetWindowLong(program->hWnd, 0, (LONG) hProgram);

  ShowWindow (program->hWnd, SW_SHOWMINIMIZED);
  SetWindowPos (program->hWnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
  UpdateWindow (program->hWnd);

  return hProgram;
}

/***********************************************************************
 *
 *           PROGRAM_CopyMoveProgram
 */

VOID PROGRAM_CopyMoveProgram(HLOCAL hProgram, BOOL bMove)
{
  PROGRAM *program = LocalLock(hProgram);
  GROUP   *fromgroup = LocalLock(program->hGroup);
  HLOCAL hGroup = DIALOG_CopyMove(LocalLock(program->hName),
				  LocalLock(fromgroup->hName), bMove);
  if (!hGroup) return;

  /* FIXME shouldn't be necessary */
  OpenIcon(((GROUP*)LocalLock(hGroup))->hWnd);

  if (!PROGRAM_AddProgram(hGroup,
#if 0
			  CopyIcon(program->hIcon),
#else
			  program->hIcon,
#endif
			  LocalLock(program->hName),
			  program->x, program->y,
			  LocalLock(program->hCmdLine),
			  LocalLock(program->hIconFile),
			  program->nIconIndex,
			  LocalLock(program->hWorkDir),
			  program->nHotKey, program->nCmdShow)) return;
  GRPFILE_WriteGroupFile(hGroup);

  if (bMove) PROGRAM_DeleteProgram(hProgram, TRUE);
}

/***********************************************************************
 *
 *           PROGRAM_ExecuteProgram
 */

VOID PROGRAM_ExecuteProgram(HLOCAL hProgram)
{
  PROGRAM *program = LocalLock(hProgram);
  LPSTR lpszCmdLine = LocalLock(program->hCmdLine);
  LPSTR lpszWorkDir = LocalLock(program->hWorkDir);

  /* FIXME set working directory */
  lpszWorkDir = lpszWorkDir;

  WinExec(lpszCmdLine, program->nCmdShow);
  if (Globals.bMinOnRun) CloseWindow(Globals.hMainWnd);
}

/***********************************************************************
 *
 *           PROGRAM_DeleteProgram
 */

VOID PROGRAM_DeleteProgram(HLOCAL hProgram, BOOL bUpdateGrpFile)
{
  PROGRAM *program = LocalLock(hProgram);
  GROUP   *group   = LocalLock(program->hGroup);

  group->hActiveProgram = 0;

  if (program->hPrior)
    ((PROGRAM*)LocalLock(program->hPrior))->hNext = program->hNext;
  else
    ((GROUP*)LocalLock(program->hGroup))->hPrograms = program->hNext;
	
  if (program->hNext)
    ((PROGRAM*)LocalLock(program->hNext))->hPrior = program->hPrior;

  if (bUpdateGrpFile)
    GRPFILE_WriteGroupFile(program->hGroup);

  DestroyWindow(program->hWnd);
#if 0
  if (program->hIcon)
    DestroyIcon(program->hIcon);
#endif
  LocalFree(program->hName);
  LocalFree(program->hCmdLine);
  LocalFree(program->hIconFile);
  LocalFree(program->hWorkDir);
  LocalFree(hProgram);
}

/***********************************************************************
 *
 *           PROGRAM_FirstProgram
 */

HLOCAL PROGRAM_FirstProgram(HLOCAL hGroup)
{
  GROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  return(group->hPrograms);
}

/***********************************************************************
 *
 *           PROGRAM_NextProgram
 */

HLOCAL PROGRAM_NextProgram(HLOCAL hProgram)
{
  PROGRAM *program;
  if (!hProgram) return(0);
  program = LocalLock(hProgram);
  return(program->hNext);
}

/***********************************************************************
 *
 *           PROGRAM_ActiveProgram
 */

HLOCAL PROGRAM_ActiveProgram(HLOCAL hGroup)
{
  GROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  if (IsIconic(group->hWnd)) return(0);

  return(group->hActiveProgram);
}

/***********************************************************************
 *
 *           PROGRAM_ProgramName
 */

LPCSTR PROGRAM_ProgramName(HLOCAL hProgram)
{
  PROGRAM *program;
  if (!hProgram) return(0);
  program = LocalLock(hProgram);
  return(LocalLock(program->hName));
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
