#ifndef __WINE_OLE32_MAIN_H
#define __WINE_OLE32_MAIN_H

#include "windef.h"

extern HINSTANCE OLE32_hInstance;

void COMPOBJ_InitProcess( void );
void COMPOBJ_UninitProcess( void );

#endif /* __WINE_OLE32_MAIN_H */
