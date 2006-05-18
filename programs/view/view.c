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
#include <windowsx.h>
#include "resource.h"

#include "globals.h"
#include <stdio.h>

BOOL FileIsPlaceable( LPCSTR szFileName );
HMETAFILE GetPlaceableMetaFile( HWND hwnd, LPCSTR szFileName );

HMETAFILE hmf;
int deltax = 0, deltay = 0;
int width = 0, height = 0;
BOOL isAldus;

static BOOL FileOpen(HWND hWnd, char *fn, int fnsz)
{
  OPENFILENAME ofn = { sizeof(OPENFILENAME),
		       0, 0, NULL, NULL, 0, 0, NULL,
		       fnsz, NULL, 0, NULL, NULL, 
		       OFN_SHOWHELP, 0, 0, NULL, 0, NULL };
  ofn.lpstrFilter = "Metafiles\0*.wmf\0";
  ofn.hwndOwner = hWnd;
  ofn.lpstrFile = fn;
  if( fnsz < 1 )
    return FALSE;
  *fn = 0;
  return GetOpenFileName(&ofn);
}


LRESULT CALLBACK WndProc(HWND hwnd,
                         UINT uMessage,
                         WPARAM wparam,
                         LPARAM lparam)
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
	if(hmf) PlayMetaFile(ps.hdc, hmf);
	EndPaint(hwnd, &ps);
      }
      break;

    case WM_COMMAND: /* message: command from application menu */
      switch (GET_WM_COMMAND_ID(wparam,lparam))
	{
	case IDM_HELLO:
	  MessageBox( hwnd , "Hello there world!", "Hello", MB_OK);
	  break;

	case IDM_OPEN:
	  {
	    char filename[MAX_PATH];
	    if (FileOpen(hwnd, filename, sizeof(filename))) {
	      isAldus = FileIsPlaceable(filename);
	      if (isAldus) {
		hmf = GetPlaceableMetaFile(hwnd, filename);
	      } else {
		RECT r;
		hmf = GetMetaFile(filename);
		GetClientRect(hwnd, &r);
		width = r.right - r.left;
		height = r.bottom - r.top;
	      }
	      InvalidateRect( hwnd, NULL, TRUE );
	    }
	  }
	  break;

	case IDM_SET_EXT_TO_WIN:
	  {
	    RECT r;
	    GetClientRect(hwnd, &r);
	    width = r.right - r.left;
	    height = r.bottom - r.top;
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
	  return DefWindowProc(hwnd, uMessage, wparam, lparam);
	}
      break;

    case WM_DESTROY:  /* message: window being destroyed */
      PostQuitMessage(0);
      break;

    default:          /* Passes it on if unprocessed */
      return DefWindowProc(hwnd, uMessage, wparam, lparam);
    }
    return 0;
}

BOOL FileIsPlaceable( LPCSTR szFileName )
{
  HFILE		hInFile;
  APMFILEHEADER	apmh;

  if( (hInFile = _lopen( szFileName, OF_READ ) ) == HFILE_ERROR )
    return FALSE;

  if( _lread( hInFile, &apmh, sizeof(APMFILEHEADER) )
      != sizeof(APMFILEHEADER) )
    {
      _lclose( hInFile );
      return FALSE;
    }
  _lclose( hInFile );

  /* Is it placeable? */
  return (apmh.key == APMHEADER_KEY);
}

HMETAFILE GetPlaceableMetaFile( HWND hwnd, LPCSTR szFileName )
{
  LPBYTE lpData;
  METAHEADER mfHeader;
  APMFILEHEADER	APMHeader;
  HFILE	fh;
  HMETAFILE hmf;
  WORD checksum, *p;
  HDC hdc;
  int i;

  if( (fh = _lopen( szFileName, OF_READ ) ) == HFILE_ERROR ) return 0;
  _llseek(fh, 0, 0);
  if (!_lread(fh, (LPSTR)&APMHeader, sizeof(APMFILEHEADER))) return 0;
  _llseek(fh, sizeof(APMFILEHEADER), 0);
  checksum = 0;
  p = (WORD *) &APMHeader;

  for(i=0; i<10; i++)
    checksum ^= *p++;
  if (checksum != APMHeader.checksum) {
    char msg[128];
    sprintf(msg, "Computed checksum %04x != stored checksum %04x\n",
	   checksum, APMHeader.checksum);
        MessageBox(hwnd, msg, "Checksum failed", MB_OK);
    return 0;
  }

  if (!_lread(fh, (LPSTR)&mfHeader, sizeof(METAHEADER))) return 0;

  if (!(lpData = GlobalAlloc(GPTR, (mfHeader.mtSize * 2L)))) return 0;

  _llseek(fh, sizeof(APMFILEHEADER), 0);
  if (!_lread(fh, lpData, (UINT)(mfHeader.mtSize * 2L)))
  {
    GlobalFree((HGLOBAL)lpData);
    _lclose(fh);
    return 0;
  }
  _lclose(fh);

  if (!(hmf = SetMetaFileBitsEx(mfHeader.mtSize*2, lpData)))
    return 0;


  width = APMHeader.bbox.Right - APMHeader.bbox.Left;
  height = APMHeader.bbox.Bottom - APMHeader.bbox.Top;

  /*      printf("Ok! width %d height %d inch %d\n", width, height, APMHeader.inch);  */
  hdc = GetDC(hwnd);
  width = width * GetDeviceCaps(hdc, LOGPIXELSX)/APMHeader.inch;
  height = height * GetDeviceCaps(hdc,LOGPIXELSY)/APMHeader.inch;
  ReleaseDC(hwnd, hdc);

  deltax = 0;
  deltay = 0 ;
  return hmf;
}
