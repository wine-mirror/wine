#ifndef __WINE_OLECTL_H
#define __WINE_OLECTL_H

#include "wintypes.h"
#include "ole.h"

#define WINOLECTLAPI INT WINAPI

/*
 * FONTDESC is used as an OLE encapsulation of the GDI fonts
 */
typedef struct tagFONTDESC {
  UINT     cbSizeofstruct;
  LPOLESTR lpstrName;
  CY         cySize;
  SHORT      sWeight;
  SHORT      sCharset;
  BOOL     fItalic;
  BOOL     fUnderline;
  BOOL     fStrikeThrough;
} FONTDESC, *LPFONTDESC;

WINOLECTLAPI OleCreateFontIndirect(LPFONTDESC lpFontDesc, REFIID riid, VOID** ppvObj);

#endif /*  __WINE_OLECTL_H */


