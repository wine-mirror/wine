/*
 * Misc 16-bit USER functions
 *
 * Copyright 2002 Patrik Stridvall
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/winuser16.h"
#include "winbase.h"

/* handle to handle 16 conversions */
#define HANDLE_16(h32)		(LOWORD(h32))
#define HBITMAP_16(h32)		(LOWORD(h32))
#define HCURSOR_16(h32)		(LOWORD(h32))
#define HICON_16(h32)		(LOWORD(h32))

/* handle16 to handle conversions */
#define HANDLE_32(h16)		((HANDLE)(ULONG_PTR)(h16))
#define HBRUSH_32(h16)		((HBRUSH)(ULONG_PTR)(h16))
#define HCURSOR_32(h16)		((HCURSOR)(ULONG_PTR)(h16))
#define HDC_32(h16)		((HDC)(ULONG_PTR)(h16))
#define HICON_32(h16)		((HICON)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))

WORD WINAPI DestroyIcon32(HGLOBAL16, UINT16);


/***********************************************************************
 *		SetCursor (USER.69)
 */
HCURSOR16 WINAPI SetCursor16(HCURSOR16 hCursor)
{
  return HCURSOR_16(SetCursor(HCURSOR_32(hCursor)));
}

/***********************************************************************
 *		ShowCursor (USER.71)
 */
INT16 WINAPI ShowCursor16(BOOL16 bShow)
{
  return ShowCursor(bShow);
}

/***********************************************************************
 *		DrawIcon (USER.84)
 */
BOOL16 WINAPI DrawIcon16(HDC16 hdc, INT16 x, INT16 y, HICON16 hIcon)
{
  return DrawIcon(HDC_32(hdc), x, y, HICON_32(hIcon));
}

/***********************************************************************
 *		IconSize (USER.86)
 *
 * See "Undocumented Windows". Used by W2.0 paint.exe.
 */
DWORD WINAPI IconSize16(void)
{
  return MAKELONG(GetSystemMetrics(SM_CYICON), GetSystemMetrics(SM_CXICON));
}

/***********************************************************************
 *		LoadCursor (USER.173)
 */
HCURSOR16 WINAPI LoadCursor16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HCURSOR_16(LoadCursorA(HINSTANCE_32(hInstance), name));
}


/***********************************************************************
 *		LoadIcon (USER.174)
 */
HICON16 WINAPI LoadIcon16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HICON_16(LoadIconA(HINSTANCE_32(hInstance), name));
}

/**********************************************************************
 *		LoadBitmap (USER.175)
 */
HBITMAP16 WINAPI LoadBitmap16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HBITMAP_16(LoadBitmapA(HINSTANCE_32(hInstance), name));
}

/***********************************************************************
 *		GetCursor (USER.247)
 */
HCURSOR16 WINAPI GetCursor16(void)
{
  return HCURSOR_16(GetCursor());
}

/***********************************************************************
 *		GlobalAddAtom (USER.268)
 */
ATOM WINAPI GlobalAddAtom16(LPCSTR lpString)
{
  return GlobalAddAtomA(lpString);
}

/***********************************************************************
 *		GlobalDeleteAtom (USER.269)
 */
ATOM WINAPI GlobalDeleteAtom16(ATOM nAtom)
{
  return GlobalDeleteAtom(nAtom);
}

/***********************************************************************
 *		GlobalFindAtom (USER.270)
 */
ATOM WINAPI GlobalFindAtom16(LPCSTR lpString)
{
  return GlobalFindAtomA(lpString);
}

/***********************************************************************
 *		GlobalGetAtomName (USER.271)
 */
UINT16 WINAPI GlobalGetAtomName16(ATOM nAtom, LPSTR lpBuffer, INT16 nSize)
{
  return GlobalGetAtomNameA(nAtom, lpBuffer, nSize);
}

/***********************************************************************
 *		LoadImage (USER.389)
 *
 */
HANDLE16 WINAPI LoadImage16(HINSTANCE16 hinst, LPCSTR name, UINT16 type,
			    INT16 desiredx, INT16 desiredy, UINT16 loadflags)
{
  return HANDLE_16(LoadImageA(HINSTANCE_32(hinst), name, type, desiredx,
			      desiredy, loadflags));
}

/******************************************************************************
 *		CopyImage (USER.390) Creates new image and copies attributes to it
 *
 */
HICON16 WINAPI CopyImage16(HANDLE16 hnd, UINT16 type, INT16 desiredx,
			   INT16 desiredy, UINT16 flags)
{
  return HICON_16(CopyImage(HANDLE_32(hnd), (UINT)type, (INT)desiredx,
			    (INT)desiredy, (UINT)flags));
}

/**********************************************************************
 *		DrawIconEx (USER.394)
 */
BOOL16 WINAPI DrawIconEx16(HDC16 hdc, INT16 xLeft, INT16 yTop, HICON16 hIcon,
			   INT16 cxWidth, INT16 cyWidth, UINT16 istep,
			   HBRUSH16 hbr, UINT16 flags)
{
  return DrawIconEx(HDC_32(hdc), xLeft, yTop, HICON_32(hIcon), cxWidth, cyWidth,
		    istep, HBRUSH_32(hbr), flags);
}

/**********************************************************************
 *		GetIconInfo (USER.395)
 */
BOOL16 WINAPI GetIconInfo16(HICON16 hIcon, LPICONINFO16 iconinfo)
{
  ICONINFO ii32;
  BOOL16 ret = GetIconInfo(HICON_32(hIcon), &ii32);

  iconinfo->fIcon = ii32.fIcon;
  iconinfo->xHotspot = ii32.xHotspot;
  iconinfo->yHotspot = ii32.yHotspot;
  iconinfo->hbmMask  = HBITMAP_16(ii32.hbmMask);
  iconinfo->hbmColor = HBITMAP_16(ii32.hbmColor);
  return ret;
}

/***********************************************************************
 *		CreateCursor (USER.406)
 */
HCURSOR16 WINAPI CreateCursor16(HINSTANCE16 hInstance,
				INT16 xHotSpot, INT16 yHotSpot,
				INT16 nWidth, INT16 nHeight,
				LPCVOID lpANDbits, LPCVOID lpXORbits)
{
  CURSORICONINFO info;

  info.ptHotSpot.x = xHotSpot;
  info.ptHotSpot.y = yHotSpot;
  info.nWidth = nWidth;
  info.nHeight = nHeight;
  info.nWidthBytes = 0;
  info.bPlanes = 1;
  info.bBitsPerPixel = 1;

  return CreateCursorIconIndirect16(hInstance, &info, lpANDbits, lpXORbits);
}

/**********************************************************************
 *		CreateIconFromResourceEx (USER.450)
 *
 * FIXME: not sure about exact parameter types
 */
HICON16 WINAPI CreateIconFromResourceEx16(LPBYTE bits, UINT16 cbSize,
					  BOOL16 bIcon, DWORD dwVersion,
					  INT16 width, INT16 height,
					  UINT16 cFlag)
{
  return HICON_16(CreateIconFromResourceEx(bits, cbSize, bIcon, dwVersion,
					   width, height, cFlag));
}

/***********************************************************************
 *		DestroyIcon (USER.457)
 */
BOOL16 WINAPI DestroyIcon16(HICON16 hIcon)
{
  return DestroyIcon32(hIcon, 0);
}

/***********************************************************************
 *		DestroyCursor (USER.458)
 */
BOOL16 WINAPI DestroyCursor16(HCURSOR16 hCursor)
{
  return DestroyIcon32(hCursor, 0);
}
