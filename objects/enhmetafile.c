/*
  Enhanced metafile functions
  Copyright 1998, Douglas Ridgway
*/

#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "winnt.h"

/*****************************************************************************
 *          GetEnhMetaFile32A (GDI32.174)
 *
 *
 */
HENHMETAFILE32 GetEnhMetaFile32A( 
	     LPCSTR lpszMetaFile  /* filename of enhanced metafile */
    )
{
  HENHMETAFILE32 hmf = NULL;
  ENHMETAHEADER h;
  char *p;
  DWORD read;
  HFILE32 hf = CreateFile32A(lpszMetaFile, GENERIC_READ, 0, 0, 
			     OPEN_EXISTING, 0, NULL);
  if (!ReadFile(hf, &h, sizeof(ENHMETAHEADER), &read, NULL)) 
    return NULL;
  if (read!=sizeof(ENHMETAHEADER)) return NULL;
  SetFilePointer(hf, 0, NULL, FILE_BEGIN); 
  /*  hmf = CreateFileMapping32A( hf, NULL, NULL, NULL, NULL, "temp"); */
  hmf = GlobalAlloc32(GHND, h.nBytes);
  p = GlobalLock32(hmf);
  if (!ReadFile(hf, p, h.nBytes, &read, NULL)) return NULL;
  GlobalUnlock32(hmf);
  return hmf;
}

/*****************************************************************************
 *        GetEnhMetaFileHeader32  (GDI32.178)
 *
 *  If _buf_ is NULL, returns the size of buffer required.
 *  Otherwise, copy up to _bufsize_ bytes of enhanced metafile header into 
 *  _buf.
 */
UINT32 GetEnhMetaFileHeader32( 
       HENHMETAFILE32 hmf, /* enhanced metafile */
       UINT32 bufsize,     /* size of buffer */
       LPENHMETAHEADER buf /* buffer */ 
    )
{
  LPENHMETAHEADER p = GlobalLock32(hmf);
  if (!buf) return sizeof(ENHMETAHEADER);
  memmove(buf, p, MIN(sizeof(ENHMETAHEADER), bufsize));
  return MIN(sizeof(ENHMETAHEADER), bufsize);
}


/*****************************************************************************
 *          GetEnhMetaFileDescription32A  (GDI32.176)
 *
 *  Copies the description string of an enhanced metafile into a buffer 
 *  _buf_.
 *
 * FIXME
 *   doesn't work. description is wide, with substructure
 */
UINT32 GetEnhMetaFileDescription32A( 
       HENHMETAFILE32 hmf, /* enhanced metafile */
       UINT32 size, /* size of buf */ 
       LPCSTR buf /* buffer to receive description */
    )
{
  LPENHMETAHEADER p = GlobalLock32(hmf);
  
  if (!buf || !size) return p->nDescription;
  lstrcpynWtoA(buf, (void *)p+p->offDescription, size);
  /*  memmove(buf, (void *)p+p->offDescription, MIN(size,p->nDescription));*/
  return MIN(size,p->nDescription);
}

/*****************************************************************************
 *           PlayEnhMetaFileRecord32  (GDI32.264)
 *
 *  Render a single enhanced metafile record in the device context hdc.
 *
 *  RETURNS
 *    TRUE on success, FALSE on error.
 *  BUGS
 *    Unimplemented
 */
BOOL32 PlayEnhMetaFileRecord32( 
     HDC32 hdc, 
     /* device context in which to render EMF record */
     LPHANDLETABLE32 lpHandletable, 
     /* array of handles to be used in rendering record */
     const ENHMETARECORD *lpEnhMetaRecord, /* EMF record to render */
     UINT32 nHandles  /* size of handle array */
     ) 
{
  int type;
  fprintf(stdout, 
  "PlayEnhMetaFileRecord(hdc = %08x, handletable = %p, record = %p, numHandles = %d\n", 
	  hdc, lpHandletable, lpEnhMetaRecord, nHandles);
  /*  SetLastError(E_NOTIMPL); */
  if (!lpEnhMetaRecord) return FALSE;

  type = lpEnhMetaRecord->iType;

  switch(type) 
    {
    case EMR_HEADER:
      printf("Header ok!\n");
      return TRUE;
      break;
    case EMR_EOF:
      printf("Eof ok!\n");
      return TRUE;
      break;
    }
  printf("I dunno %d\n", type);
  return FALSE;
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
 *   Doesn't free objects, ignores rect.
 */
BOOL32 EnumEnhMetaFile32( 
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
    /*   printf("EnumEnhMetaFile: type = %ld size = %ld\n", p->iType, p->nSize);*/
    ret = (*callback)(hdc, ht, p, count, data); 
    if (p->iType == EMR_EOF) break;
    p = (void *) p + p->nSize;
  }
  return ret;
}


/**************************************************************************
 *    PlayEnhMetaFile32  (GDI32.263)
 *
 *    Renders an enhanced metafile into a specified rectangle *lpRect
 *    in device context hdc.
 *
 * BUGS
 *    Almost entirely unimplemented
 *
 */
BOOL32 PlayEnhMetaFile32( 
       HDC32 hdc, /* DC to render into */
       HENHMETAFILE32 hmf, /* metafile to render */
       const RECT32 *lpRect  /* rectangle to place metafile inside */
      )
{
  LPENHMETARECORD p = GlobalLock32(hmf);
  INT32 count = ((LPENHMETAHEADER) p)->nHandles;
  HANDLETABLE32 *ht = (HANDLETABLE32 *)GlobalAlloc32(GPTR, sizeof(HANDLETABLE32)*count);
  ht->objectHandle[0] = hmf;
  while (1) {
    PlayEnhMetaFileRecord32(hdc, ht, p, count);
    if (p->iType == EMR_EOF) break;
    p = (void *) p + p->nSize; /* casted so that arithmetic is in bytes */
  }
  return FALSE;
}

/*
  need wide version as well
*/
HDC32 CreateEnhMetaFile32A( 
    HDC32 hdcRef, /* optional reference DC */
    LPCSTR lpFilename, /* optional filename for disk metafiles */
    const RECT32 *lpRect, /* optional bounding rectangle */
    LPCSTR lpDescription /* optional description */ 
    )
{
  return NULL;
}

HENHMETAFILE32 CloseEnhMetaFile32( 
               HDC32 hdc  /* metafile DC */
	       )
{
  return NULL;
}


/*****************************************************************************
 *  DeleteEnhMetaFile32 (GDI32.68)
 */
BOOL32 DeleteEnhMetaFile32(HENHMETAFILE32 hmf) {
  return FALSE;
}

/*****************************************************************************
 *  CopyEnhMetaFileA (GDI32.21)
 */

