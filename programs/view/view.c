#include <windows.h>

/* 
#include <windowsx.h>
   Wine doesn't have windowsx.h, so we use this
*/
#define       GET_WM_COMMAND_ID(wp,lp)        LOWORD(wp)

/* Wine seems to need this */
#include <commdlg.h>

#include "globals.h"        
#include "resource.h"

BOOL FileIsPlaceable( LPCSTR szFileName );
HMETAFILE GetPlaceableMetaFile( LPCSTR szFileName );

#define FN_LENGTH 80

HMETAFILE hmf;
int deltax = 0, deltay = 0;
int width = 0, height = 0;
BOOL isAldus;

BOOL FileOpen(HWND hWnd, char *fn)
{
  OPENFILENAME ofn = { sizeof(OPENFILENAME),
		       0, NULL, "Metafiles\0*.wmf\0", NULL, 0, 0, NULL, 
		       FN_LENGTH, NULL, 0, NULL, NULL, OFN_CREATEPROMPT |
		       OFN_SHOWHELP, 0, 0, NULL, 0, NULL };
  ofn.hwndOwner = hWnd;
  ofn.lpstrFile = fn;
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
	SetViewportExt(ps.hdc, width, height);
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
	    char filename[FN_LENGTH];
	    if (FileOpen(hwnd, filename)) {
	      isAldus = FileIsPlaceable(filename);
	      if (isAldus) {
#if 0
		hmf = GetPlaceableMetaFile(filename);
#else
		MessageBox(hwnd, "This is an Aldus placeable metafile: I can't deal with those!",
			   "Aldus", MB_OK);
#endif
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

    default:          /* Passes it on if unproccessed */
      return DefWindowProc(hwnd, uMessage, wparam, lparam);
    }
    return 0;
}

BOOL FileIsPlaceable( LPCSTR szFileName )
{
  HFILE		hInFile;
  OFSTRUCT		inof;
  APMFILEHEADER	apmh;

  if( (hInFile = OpenFile( szFileName, &inof, OF_READ ) ) == HFILE_ERROR )
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

/* this code doesn't work */
HMETAFILE GetPlaceableMetaFile( LPCSTR szFileName )
{
  HANDLE hData;
  LPSTR	lpData;
  METAHEADER mfHeader;
  APMFILEHEADER	APMHeader;
  HMETAFILE hmf = NULL;
  HFILE	fh;
  OFSTRUCT inof;

  if( (fh = OpenFile( szFileName, &inof, OF_READ ) ) == HFILE_ERROR ) return NULL;
  _llseek(fh, 0, 0);
  if (!_lread(fh, (LPSTR)&APMHeader, sizeof(APMFILEHEADER))) return NULL;
  _llseek(fh, sizeof(APMFILEHEADER), 0);
  if (!_lread(fh, (LPSTR)&mfHeader, sizeof(METAHEADER))) return NULL;

  /* somehow mtSize is being swapped */

  if (!(hData = GlobalAlloc(GHND, (mfHeader.mtSize * 2L)))) return NULL;

  if (!(lpData = GlobalLock(hData)))
  {
    GlobalFree(hData);
    return NULL;
  }

  _llseek(fh, sizeof(APMFILEHEADER), 0);
  if (!_lread(fh, lpData, (UINT)(mfHeader.mtSize * 2L)))
  {
    GlobalUnlock(hData);
    GlobalFree(hData);
    return NULL;
  }
  _lclose(fh);
  GlobalUnlock(hData);

  if (!(hmf = (HMETAFILE) SetMetaFileBits(hData))) return NULL;

  
  width = APMHeader.bbox.right - APMHeader.bbox.left;
  height = APMHeader.bbox.bottom - APMHeader.bbox.top;
  deltax = 0;
  deltay = 0 ;
  return hmf;
}



