/*
 * Copyright 1998 Douglas Ridgway
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>

#include "resource.h"

static HINSTANCE hInst;
static HWND hMainWnd;
static const WCHAR szAppName[] = L"View";
static WCHAR szTitle[MAX_PATH];
static WCHAR szFileTitle[MAX_PATH];

static HMETAFILE hmf;
static HENHMETAFILE enhmf;
static int deltax = 0, deltay = 0;
static int width = 0, height = 0;
static BOOL isAldus, isEnhanced;

#pragma pack(push,1)
typedef struct
{
	DWORD		key;
	WORD		hmf;
	SMALL_RECT	bbox;
	WORD		inch;
	DWORD		reserved;
	WORD		checksum;
} APMFILEHEADER;
#pragma pack(pop)

#define APMHEADER_KEY	0x9AC6CDD7l


static BOOL FileOpen(HWND hWnd, WCHAR *fn, int fnsz)
{
  WCHAR filter[120], metafileFilter[100];
  OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW),
                        0, 0, NULL, NULL, 0, 0, NULL,
                        fnsz, NULL, 0, NULL, NULL,
                        OFN_SHOWHELP, 0, 0, NULL, 0, NULL };

  LoadStringW( hInst, IDS_OPEN_META_STRING, metafileFilter, ARRAY_SIZE(metafileFilter) );
  swprintf( filter, ARRAY_SIZE(filter), L"%s%c*.wmf;*.emf%c", metafileFilter, 0, 0 );

  ofn.lpstrFilter = filter;
  ofn.hwndOwner = hWnd;
  ofn.lpstrFile = fn;
  if( fnsz < 1 )
    return FALSE;
  *fn = 0;
  return GetOpenFileNameW(&ofn);
}

static BOOL FileIsEnhanced( LPCWSTR szFileName )
{
  ENHMETAHEADER enh;
  HANDLE handle;
  DWORD size;

  handle = CreateFileW( szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
  if (handle == INVALID_HANDLE_VALUE)
    return FALSE;

  if (!ReadFile( handle, &enh, sizeof(ENHMETAHEADER), &size, NULL ) || size != sizeof(ENHMETAHEADER) )
  {
      CloseHandle( handle );
      return FALSE;
  }
  CloseHandle( handle );

  /* Is it enhanced? */
  return (enh.dSignature == ENHMETA_SIGNATURE);
}

static BOOL FileIsPlaceable( LPCWSTR szFileName )
{
  APMFILEHEADER	apmh;
  HANDLE handle;
  DWORD size;

  handle = CreateFileW( szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
  if (handle == INVALID_HANDLE_VALUE)
    return FALSE;

  if (!ReadFile( handle, &apmh, sizeof(APMFILEHEADER), &size, NULL ) || size != sizeof(APMFILEHEADER))
  {
      CloseHandle( handle );
      return FALSE;
  }
  CloseHandle( handle );

  /* Is it placeable? */
  return (apmh.key == APMHEADER_KEY);
}

static HMETAFILE GetPlaceableMetaFile( LPCWSTR szFileName )
{
  LPBYTE lpData;
  METAHEADER mfHeader;
  APMFILEHEADER	APMHeader;
  HANDLE handle;
  DWORD size;
  HMETAFILE hmf;
  WORD checksum, *p;
  HDC hdc;
  int i;

  handle = CreateFileW( szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
  if (handle == INVALID_HANDLE_VALUE)
    return 0;

  if (!ReadFile( handle, &APMHeader, sizeof(APMFILEHEADER), &size, NULL ) || size != sizeof(APMFILEHEADER))
  {
      CloseHandle( handle );
      return 0;
  }
  checksum = 0;
  p = (WORD *) &APMHeader;

  for(i=0; i<10; i++)
    checksum ^= *p++;
  if (checksum != APMHeader.checksum) {
    char msg[128];
    sprintf(msg, "Computed checksum %04x != stored checksum %04x\n",
	   checksum, APMHeader.checksum);
    MessageBoxA(hMainWnd, msg, "Checksum failed", MB_OK);
    CloseHandle( handle );
    return 0;
  }

  if (!ReadFile( handle, &mfHeader, sizeof(METAHEADER), &size, NULL) || size != sizeof(METAHEADER))
  {
      CloseHandle( handle );
      return 0;
  }

  if (!(lpData = GlobalAlloc(GPTR, (mfHeader.mtSize * 2L))))
  {
      CloseHandle( handle );
      return 0;
  }

  SetFilePointer( handle, sizeof(APMFILEHEADER), NULL, FILE_BEGIN );
  if (!ReadFile(handle, lpData, mfHeader.mtSize * 2, &size, NULL ) || size != mfHeader.mtSize * 2)
  {
    GlobalFree(lpData);
    CloseHandle( handle );
    return 0;
  }
  CloseHandle( handle );

  if (!(hmf = SetMetaFileBitsEx(mfHeader.mtSize*2, lpData))) {
    GlobalFree(lpData);
    return 0;
  }


  width = APMHeader.bbox.Right - APMHeader.bbox.Left;
  height = APMHeader.bbox.Bottom - APMHeader.bbox.Top;

  /*      printf("Ok! width %d height %d inch %d\n", width, height, APMHeader.inch);  */
  hdc = GetDC(hMainWnd);
  width = width * GetDeviceCaps(hdc, LOGPIXELSX)/APMHeader.inch;
  height = height * GetDeviceCaps(hdc,LOGPIXELSY)/APMHeader.inch;
  ReleaseDC(hMainWnd, hdc);

  deltax = 0;
  deltay = 0 ;
  GlobalFree(lpData);
  return hmf;
}

static void DoOpenFile(LPCWSTR filename)
{
  if (!filename) return;

  isAldus = FileIsPlaceable(filename);
  if (isAldus) {
    hmf = GetPlaceableMetaFile(filename);
  } else {
    RECT r;
    isEnhanced = FileIsEnhanced(filename);
    if (isEnhanced)
       enhmf = GetEnhMetaFileW(filename);
    else
       hmf = GetMetaFileW(filename);
    GetClientRect(hMainWnd, &r);
    width = r.right - r.left;
    height = r.bottom - r.top;
  }
  InvalidateRect( hMainWnd, NULL, TRUE );
}

static void UpdateWindowCaption(void)
{
  WCHAR szCaption[MAX_PATH];
  WCHAR szView[MAX_PATH];

  LoadStringW(hInst, IDS_DESCRIPTION, szView, ARRAY_SIZE(szView));

  if (szFileTitle[0] != '\0')
  {
    lstrcpyW(szCaption, szFileTitle);
    LoadStringW(hInst, IDS_DESCRIPTION, szView, ARRAY_SIZE(szView));
    lstrcatW(szCaption, L" - ");
    lstrcatW(szCaption, szView);
  }
  else
    lstrcpyW(szCaption, szView);

  SetWindowTextW(hMainWnd, szCaption);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
  switch (uMessage)
    {
    case WM_PAINT:
      {
	PAINTSTRUCT ps;
	BeginPaint(hwnd, &ps);
	SetMapMode(ps.hdc, MM_ANISOTROPIC);
	/* Set the window extent to a sane value in case the metafile doesn't */
	SetWindowExtEx(ps.hdc, width, height, NULL);
	SetViewportExtEx(ps.hdc, width, height, NULL);
	SetViewportOrgEx(ps.hdc, deltax, deltay, NULL);
       if (isEnhanced && enhmf)
       {
           RECT r;
           GetClientRect(hwnd, &r);
           PlayEnhMetaFile(ps.hdc, enhmf, &r);
       }
       else if (hmf)
           PlayMetaFile(ps.hdc, hmf);
	EndPaint(hwnd, &ps);
      }
      break;

    case WM_COMMAND: /* message: command from application menu */
        switch (LOWORD(wparam))
	{
	case IDM_OPEN:
	  {
              WCHAR filename[MAX_PATH];
              if (FileOpen(hwnd, filename, ARRAY_SIZE(filename)))
              {
                  szFileTitle[0] = 0;
                  GetFileTitleW(filename, szFileTitle, ARRAY_SIZE(szFileTitle));
                  DoOpenFile(filename);
                  UpdateWindowCaption();
              }
	  }
	  break;

	case IDM_SET_EXT_TO_WIN:
	  {
	    RECT r;
	    GetClientRect(hwnd, &r);
	    width = r.right - r.left;
	    height = r.bottom - r.top;
	    deltax = deltay = 0;
	    InvalidateRect( hwnd, NULL, TRUE );
	  }
	  break;


	case IDM_LEFT:
	  deltax += 100;
	  InvalidateRect( hwnd, NULL, TRUE );
	  break;
	case IDM_RIGHT:
	  deltax -= 100;
	  InvalidateRect( hwnd, NULL, TRUE );
	  break;
	case IDM_UP:
	  deltay += 100;
	  InvalidateRect( hwnd, NULL, TRUE );
	  break;
	case IDM_DOWN:
	  deltay -= 100;
	  InvalidateRect( hwnd, NULL, TRUE );
	  break;

	case IDM_EXIT:
	  DestroyWindow(hwnd);
	  break;

	default:
	  return DefWindowProcW(hwnd, uMessage, wparam, lparam);
	}
      break;

    case WM_DESTROY:  /* message: window being destroyed */
      PostQuitMessage(0);
      break;

    default:          /* Passes it on if unprocessed */
      return DefWindowProcW(hwnd, uMessage, wparam, lparam);
    }
    return 0;
}

static BOOL InitApplication(HINSTANCE hInstance)
{
  WNDCLASSEXW wc;

  /* Load the application description strings */
  LoadStringW(hInstance, IDS_DESCRIPTION, szTitle, ARRAY_SIZE(szTitle));

  /* Fill in window class structure with parameters that describe the
     main window */

  wc.cbSize        = sizeof(WNDCLASSEXW);
  wc.style         = CS_HREDRAW | CS_VREDRAW;             /* Class style(s) */
  wc.lpfnWndProc   = WndProc;                             /* Window Procedure */
  wc.cbClsExtra    = 0;                          /* No per-class extra data */
  wc.cbWndExtra    = 0;                         /* No per-window extra data */
  wc.hInstance     = hInstance;                      /* Owner of this class */
  wc.hIcon         = NULL;
  wc.hIconSm       = NULL;
  wc.hCursor       = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);           /* Default color */
  wc.lpszMenuName  = szAppName;                       /* Menu name from .rc */
  wc.lpszClassName = szAppName;                      /* Name to register as */

  if (!RegisterClassExW(&wc)) return FALSE;

  /* Call module specific initialization functions here */

  return TRUE;
}

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    /* Save the instance handle in a global variable for later use */
    hInst = hInstance;

    /* Create main window */
    hMainWnd = CreateWindowW(szAppName,          /* See RegisterClass() call */
                            szTitle,             /* window title */
                            WS_OVERLAPPEDWINDOW, /* Window style */
                            CW_USEDEFAULT, 0,    /* positioning */
                            CW_USEDEFAULT, 0,    /* size */
                            NULL,                /* Overlapped has no parent */
                            NULL,                /* Use the window class menu */
                            hInstance,
                            NULL);

    if (!hMainWnd)
        return FALSE;

    /* Call module specific instance initialization functions here */

    /* show the window, and paint it for the first time */
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    return TRUE;
}

static void HandleCommandLine(LPWSTR cmdline)
{
    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    if (*cmdline)
    {
        /* file name is passed on the command line */
        if (cmdline[0] == '"')
        {
            cmdline++;
            cmdline[lstrlenW(cmdline) - 1] = 0;
        }
        szFileTitle[0] = 0;
        GetFileTitleW(cmdline, szFileTitle, ARRAY_SIZE(szFileTitle));
        DoOpenFile(cmdline);
        UpdateWindowCaption();
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    InitCommonControls();

    /* Other instances of app running? */
    if (!hPrevInstance)
    {
      /* stuff to be done once */
      if (!InitApplication(hInstance))
      {
        return FALSE;      /* exit */
      }
    }

    /* stuff to be done every time */
    if (!InitInstance(hInstance, nCmdShow))
    {
      return FALSE;
    }

    HandleCommandLine(lpCmdLine);

    /* Main loop */
    /* Acquire and dispatch messages until a WM_QUIT message is received */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return msg.wParam;
}
