/*
  Enhanced metafile functions
  Copyright 1998, Douglas Ridgway
*/

#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "gdi.h"
#include "winbase.h"
#include "winnt.h"
#include "debug.h"

/*****************************************************************************
 *          GetEnhMetaFile32A (GDI32.174)
 *
 *
 */
HENHMETAFILE32 WINAPI GetEnhMetaFile32A( 
	     LPCSTR lpszMetaFile  /* filename of enhanced metafile */
    )
{
  HENHMETAFILE32 hmf = 0;
  ENHMETAHEADER h;
  BYTE *p;
  DWORD read;
  HFILE32 hf = CreateFile32A(lpszMetaFile, GENERIC_READ, 0, 0, 
			     OPEN_EXISTING, 0, 0);
  if (!ReadFile(hf, &h, sizeof(ENHMETAHEADER), &read, NULL)) 
    return 0;
  if (read!=sizeof(ENHMETAHEADER)) return 0;
  SetFilePointer(hf, 0, NULL, FILE_BEGIN); 
  /*  hmf = CreateFileMapping32A( hf, NULL, NULL, NULL, NULL, "temp"); */
  hmf = GlobalAlloc32(GPTR, h.nBytes);
  p = GlobalLock32(hmf);
  if (!ReadFile(hf, p, h.nBytes, &read, NULL)) return 0;
  GlobalUnlock32(hmf);
  return hmf;
}

/*****************************************************************************
 *        GetEnhMetaFileHeader  (GDI32.178)
 *
 *  If _buf_ is NULL, returns the size of buffer required.
 *  Otherwise, copy up to _bufsize_ bytes of enhanced metafile header into 
 *  _buf.
 */
UINT32 WINAPI GetEnhMetaFileHeader( 
       HENHMETAFILE32 hmf, /* enhanced metafile */
       UINT32 bufsize,     /* size of buffer */
       LPENHMETAHEADER buf /* buffer */ 
    )
{
  LPENHMETAHEADER p = GlobalLock32(hmf);
  if (!buf) return sizeof(ENHMETAHEADER);
  memmove(buf, p, MIN(sizeof(ENHMETAHEADER), bufsize));
  GlobalUnlock32(hmf);
  return MIN(sizeof(ENHMETAHEADER), bufsize);
}


/*****************************************************************************
 *          GetEnhMetaFileDescription32A  (GDI32.176)
 */
UINT32 WINAPI GetEnhMetaFileDescription32A( 
       HENHMETAFILE32 hmf, /* enhanced metafile */
       UINT32 size, /* size of buf */ 
       LPSTR buf /* buffer to receive description */
    )
{
  LPENHMETAHEADER p = GlobalLock32(hmf);
  INT32 first  = lstrlen32W( (void *)p+p->offDescription);

  if (!buf || !size) return p->nDescription;

  lstrcpynWtoA(buf, (void *)p+p->offDescription, size);
  buf += first +1;
  lstrcpynWtoA(buf, (void *)p+p->offDescription+2*(first+1), size-first-1);

  /*  memmove(buf, (void *)p+p->offDescription, MIN(size,p->nDescription)); */
  GlobalUnlock32(hmf);
  return MIN(size,p->nDescription);
}

/*****************************************************************************
 *          GetEnhMetaFileDescription32W  (GDI32.xxx)
 *
 *  Copies the description string of an enhanced metafile into a buffer 
 *  _buf_.
 *
 *  If _buf_ is NULL, returns size of _buf_ required. Otherwise, returns
 *  number of characters copied.
 */
UINT32 WINAPI GetEnhMetaFileDescription32W( 
       HENHMETAFILE32 hmf, /* enhanced metafile */
       UINT32 size, /* size of buf */ 
       LPWSTR buf /* buffer to receive description */
    )
{
  LPENHMETAHEADER p = GlobalLock32(hmf);

  if (!buf || !size) return p->nDescription;

  memmove(buf, (void *)p+p->offDescription, MIN(size,p->nDescription));
  GlobalUnlock32(hmf);
  return MIN(size,p->nDescription);
}

/****************************************************************************
 *    SetEnhMetaFileBits (GDI32.315)
 *
 *  Creates an enhanced metafile by copying _bufsize_ bytes from _buf_.
 */
HENHMETAFILE32 WINAPI SetEnhMetaFileBits(UINT32 bufsize, const BYTE *buf)
{
  HENHMETAFILE32 hmf = GlobalAlloc32(GPTR, bufsize);
  LPENHMETAHEADER h = GlobalLock32(hmf);
  memmove(h, buf, bufsize);
  GlobalUnlock32(hmf);
  return hmf;
}

/*****************************************************************************
 *  GetEnhMetaFileBits (GDI32.175)
 *
 */
UINT32 WINAPI GetEnhMetaFileBits(
    HENHMETAFILE32 hmf, 
    UINT32 bufsize, 
    LPBYTE buf  
) {
  return 0;
}

/*****************************************************************************
 *           PlayEnhMetaFileRecord  (GDI32.264)
 *
 *  Render a single enhanced metafile record in the device context hdc.
 *
 *  RETURNS
 *    TRUE on success, FALSE on error.
 *  BUGS
 *    Many unimplemented records.
 */
BOOL32 WINAPI PlayEnhMetaFileRecord( 
     HDC32 hdc, 
     /* device context in which to render EMF record */
     LPHANDLETABLE32 handletable, 
     /* array of handles to be used in rendering record */
     const ENHMETARECORD *mr, /* EMF record to render */
     UINT32 handles  /* size of handle array */
     ) 
{
  int type;
  TRACE(metafile, 
	"hdc = %08x, handletable = %p, record = %p, numHandles = %d\n", 
	  hdc, handletable, mr, handles);
  if (!mr) return FALSE;

  type = mr->iType;

  TRACE(metafile, " type=%d\n", type);
  switch(type) 
    {
    case EMR_HEADER:
      {
	ENHMETAHEADER *h = (LPENHMETAHEADER) mr;
	break;
      }
    case EMR_EOF:
      break;
    case EMR_GDICOMMENT:
      /* application defined and processed */
      break;
    case EMR_SETMAPMODE:
      {
	DWORD mode = mr->dParm[0];
	SetMapMode32(hdc, mode);
	break;
      }
    case EMR_SETBKMODE:
      {
	DWORD mode = mr->dParm[0];
	SetBkMode32(hdc, mode);
	break;
      }
    case EMR_SETBKCOLOR:
      {
	DWORD mode = mr->dParm[0];
	SetBkColor32(hdc, mode);
	break;
      }
    case EMR_SETPOLYFILLMODE:
      {
	DWORD mode = mr->dParm[0];
	SetPolyFillMode32(hdc, mode);
	break;
      }
    case EMR_SETROP2:
      {
	DWORD mode = mr->dParm[0];
	SetROP232(hdc, mode);
	break;
      }
    case EMR_SETSTRETCHBLTMODE:
      {
	DWORD mode = mr->dParm[0];
	SetStretchBltMode32(hdc, mode);
	break;
      }
    case EMR_SETTEXTALIGN:
      {
	DWORD align = mr->dParm[0];
	SetTextAlign32(hdc, align);
	break;
      }
    case EMR_SETTEXTCOLOR:
      {
	DWORD color = mr->dParm[0];
	SetTextColor32(hdc, color);
	break;
      }
    case EMR_SAVEDC:
      {
	SaveDC32(hdc);
	break;
      }
    case EMR_RESTOREDC:
      {
	RestoreDC32(hdc, mr->dParm[0]);
	break;
      }
    case EMR_INTERSECTCLIPRECT:
      {
	INT32 left = mr->dParm[0], top = mr->dParm[1], right = mr->dParm[2],
	      bottom = mr->dParm[3];
	IntersectClipRect32(hdc, left, top, right, bottom);
	break;
      }
    case EMR_SELECTOBJECT:
      {
	DWORD obj = mr->dParm[0];
	SelectObject32(hdc, (handletable->objectHandle)[obj]);
	break;
      }
    case EMR_DELETEOBJECT:
      {
	DWORD obj = mr->dParm[0];
	DeleteObject32( (handletable->objectHandle)[obj]);
	(handletable->objectHandle)[obj] = 0;
	break;
      }
    case EMR_SETWINDOWORGEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetWindowOrgEx32(hdc, x, y, NULL);
	break;
      }
    case EMR_SETWINDOWEXTEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetWindowExtEx32(hdc, x, y, NULL);
	break;
      }
    case EMR_SETVIEWPORTORGEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetViewportOrgEx32(hdc, x, y, NULL);
	break;
      }
    case EMR_SETVIEWPORTEXTEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetViewportExtEx32(hdc, x, y, NULL);
	break;
      }
    case EMR_CREATEPEN:
      {
	DWORD obj = mr->dParm[0];
	(handletable->objectHandle)[obj] = 
	  CreatePenIndirect32((LOGPEN32 *) &(mr->dParm[1]));
	break;
      }
    case EMR_EXTCREATEPEN:
      {
	DWORD obj = mr->dParm[0];
	DWORD style = mr->dParm[1], brush = mr->dParm[2];
	LOGBRUSH32 *b = (LOGBRUSH32 *) &mr->dParm[3];
	FIXME(metafile, "Some ExtCreatePen args not handled\n");
	(handletable->objectHandle)[obj] = 
	  ExtCreatePen32(style, brush, b, 0, NULL);
	break;
      }
    case EMR_CREATEBRUSHINDIRECT:
      {
	DWORD obj = mr->dParm[0];
	(handletable->objectHandle)[obj] = 
	  CreateBrushIndirect32((LOGBRUSH32 *) &(mr->dParm[1]));
	break;
      }
    case EMR_EXTCREATEFONTINDIRECTW:
	{
	DWORD obj = mr->dParm[0];
	(handletable->objectHandle)[obj] = 
	  CreateFontIndirect32W((LOGFONT32W *) &(mr->dParm[1]));
	break;
	}
    case EMR_MOVETOEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	MoveToEx32(hdc, x, y, NULL);
	break;
      }
    case EMR_LINETO:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
        LineTo32(hdc, x, y);
	break;
      }
    case EMR_RECTANGLE:
      {
	INT32 left = mr->dParm[0], top = mr->dParm[1], right = mr->dParm[2],
	      bottom = mr->dParm[3];
	Rectangle32(hdc, left, top, right, bottom);
	break;
      }
    case EMR_ELLIPSE:
      {
	INT32 left = mr->dParm[0], top = mr->dParm[1], right = mr->dParm[2],
	      bottom = mr->dParm[3];
	Ellipse32(hdc, left, top, right, bottom);
	break;
      }
    case EMR_POLYGON16:
      {
	/* 0-3 : a bounding rectangle? */
	INT32 count = mr->dParm[4];
	FIXME(metafile, "Some Polygon16 args not handled\n");
	Polygon16(hdc, (POINT16 *)&mr->dParm[5], count);
	break;
      }
    case EMR_POLYLINE16:
      {
	/* 0-3 : a bounding rectangle? */
	INT32 count = mr->dParm[4];
	FIXME(metafile, "Some Polyline16 args not handled\n");
	Polyline16(hdc, (POINT16 *)&mr->dParm[5], count);
	break;
      }

#if 0
    case EMR_POLYPOLYGON16:
      {
	INT32 polygons = mr->dParm[z];
	LPPOINT16 pts = (LPPOINT16) &mr->dParm[x];
	LPINT16 counts = (LPINT16) &mr->dParm[y];
	PolyPolygon16(hdc, pts, counts, polygons);
	break;
      }
#endif
    case EMR_EXTTEXTOUTW:
      {
	/* 0-3: ??? */
	DWORD flags = mr->dParm[4];
	/* 5, 6: ??? */
	DWORD x = mr->dParm[7], y = mr->dParm[8];
	DWORD count = mr->dParm[9];
	/* 10-16: ??? */
	LPWSTR str = (LPWSTR)& mr->dParm[17];
	/* trailing info: dx array? */
	FIXME(metafile, "Many ExtTextOut args not handled\n");
	ExtTextOut32W(hdc, x, y, flags, /* lpRect */ NULL, 
		      str, count, /* lpDx */ NULL); 
	break;
      }

    default:
      FIXME(metafile, "type %d is unimplemented\n", type);
      /*  SetLastError(E_NOTIMPL); */
      break;
    }
  return TRUE;
}


/*****************************************************************************
 *
 *        EnumEnhMetaFile32  (GDI32.79)
 *
 *  Walk an enhanced metafile, calling a user-specified function _EnhMetaFunc_
 *  for each
 *  record. Returns when either every record has been used or 
 *  when _EnhMetaFunc_ returns FALSE.
 *
 *
 * RETURNS
 *  TRUE if every record is used, FALSE if any invocation of _EnhMetaFunc_
 *  returns FALSE.
 *
 * BUGS
 *   Ignores rect.
 */
BOOL32 WINAPI EnumEnhMetaFile32( 
     HDC32 hdc, /* device context to pass to _EnhMetaFunc_ */
     HENHMETAFILE32 hmf, /* EMF to walk */
     ENHMFENUMPROC32 callback, /* callback function */ 
     LPVOID data, /* optional data for callback function */
     const RECT32 *rect  /* bounding rectangle for rendered metafile */
    )
{
  BOOL32 ret = TRUE;
  LPENHMETARECORD p = GlobalLock32(hmf);
  INT32 count = ((LPENHMETAHEADER) p)->nHandles;
  HANDLETABLE32 *ht = (HANDLETABLE32 *)GlobalAlloc32(GPTR, sizeof(HANDLETABLE32)*count);
  ht->objectHandle[0] = hmf;
  while (ret) {
    ret = (*callback)(hdc, ht, p, count, data); 
    if (p->iType == EMR_EOF) break;
    p = (void *) p + p->nSize;
  }
  GlobalFree32(ht);
  GlobalUnlock32(hmf);
  return ret;
}


/**************************************************************************
 *    PlayEnhMetaFile  (GDI32.263)
 *
 *    Renders an enhanced metafile into a specified rectangle *lpRect
 *    in device context hdc.
 *
 * BUGS
 *    Almost entirely unimplemented
 *
 */
BOOL32 WINAPI PlayEnhMetaFile( 
       HDC32 hdc, /* DC to render into */
       HENHMETAFILE32 hmf, /* metafile to render */
       const RECT32 *lpRect  /* rectangle to place metafile inside */
      )
{
  LPENHMETARECORD p = GlobalLock32(hmf);
  INT32 count = ((LPENHMETAHEADER) p)->nHandles;
  HANDLETABLE32 *ht = (HANDLETABLE32 *)GlobalAlloc32(GPTR, 
				    sizeof(HANDLETABLE32)*count);
  BOOL32 ret = FALSE;
  if (lpRect) {
    LPENHMETAHEADER h = (LPENHMETAHEADER) p;
    FLOAT xscale = (h->rclBounds.right-h->rclBounds.left)/(lpRect->right-lpRect->left);
    FLOAT yscale = (h->rclBounds.bottom-h->rclBounds.top)/(lpRect->bottom-lpRect->top);
    XFORM xform = {xscale, 0, 0, yscale, 0, 0};
    /*    xform.eDx = lpRect->left;
	  xform.eDy = lpRect->top; */
    FIXME(metafile, "play into rect doesn't work\n");
    if (!SetWorldTransform(hdc, &xform)) {
      WARN(metafile, "World transform failed!\n");
    }
  }

  ht->objectHandle[0] = hmf;
  while (1) {
    PlayEnhMetaFileRecord(hdc, ht, p, count);
    if (p->iType == EMR_EOF) break;
    p = (void *) p + p->nSize; /* casted so that arithmetic is in bytes */
  }
  GlobalUnlock32(hmf);
  return TRUE;
}

/*****************************************************************************
 *  DeleteEnhMetaFile (GDI32.68)
 *
 *  Deletes an enhanced metafile and frees the associated storage.
 */
BOOL32 WINAPI DeleteEnhMetaFile(HENHMETAFILE32 hmf) {
  return !GlobalFree32(hmf);
}

/*****************************************************************************
 *  CopyEnhMetaFileA (GDI32.21)  Duplicate an enhanced metafile
 *
 *   
 */
HENHMETAFILE32 WINAPI CopyEnhMetaFile32A(
    HENHMETAFILE32 hmf, 
    LPCSTR file)
{
  if (!file) {
    LPENHMETAHEADER h = GlobalLock32(hmf);
    HENHMETAFILE32 hmf2 = GlobalAlloc32(GPTR, h->nBytes);
    LPENHMETAHEADER h2 = GlobalLock32(hmf2);
    if (!h2) return 0;
    memmove(h2, h, h->nBytes);
    GlobalUnlock32(hmf2);
    GlobalUnlock32(hmf);
    return hmf2;
  } else {
    FIXME(metafile, "write to file not implemented\n");
    return 0;
  }
}


