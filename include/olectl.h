#ifndef __WINE_OLECTL_H
#define __WINE_OLECTL_H

#include "wintypes.h"
#include "ole.h"

#define WINOLECTLAPI INT32 WINAPI

/*
 * FONTDESC is used as an OLE encapsulation of the GDI fonts
 */
typedef struct tagFONTDESC {
  UINT32     cbSizeofstruct;
  LPOLESTR32 lpstrName;
  CY         cySize;
  SHORT      sWeight;
  SHORT      sCharset;
  BOOL32     fItalic;
  BOOL32     fUnderline;
  BOOL32     fStrikeThrough;
} FONTDESC, *LPFONTDESC;

WINOLECTLAPI OleCreateFontIndirect(LPFONTDESC lpFontDesc, REFIID riid, VOID** ppvObj);

#endif /*  __WINE_OLECTL_H */


