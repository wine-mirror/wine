/*
 * Local heap declarations
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_LOCAL_H
#define __WINE_LOCAL_H

#include "wintypes.h"

  /* These function are equivalent to the Local* API functions, */
  /* excepted that they need DS as the first parameter. This    */
  /* allows managing several heaps from the emulation library.  */

extern HLOCAL LOCAL_Alloc( HANDLE ds, WORD flags, WORD size );
extern HLOCAL LOCAL_ReAlloc( HANDLE ds, HLOCAL handle, WORD size, WORD flags );
extern HLOCAL LOCAL_Free( HANDLE ds, HLOCAL handle );
extern HLOCAL LOCAL_Handle( HANDLE ds, WORD addr );
extern WORD LOCAL_Size( HANDLE ds, HLOCAL handle );
extern WORD LOCAL_Flags( HANDLE ds, HLOCAL handle );
extern WORD LOCAL_HeapSize( HANDLE ds );
extern WORD LOCAL_CountFree( WORD ds );
extern LPSTR LOCAL_Lock( HANDLE ds, HLOCAL handle );
extern BOOL LOCAL_Unlock( HANDLE ds, HLOCAL handle );

#endif  /* __WINE_LOCAL_H */
