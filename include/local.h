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

extern HLOCAL LOCAL_Alloc( WORD ds, WORD flags, WORD size );
extern HLOCAL LOCAL_ReAlloc( WORD ds, HLOCAL handle, WORD size, WORD flags );
extern HLOCAL LOCAL_Free( WORD ds, HLOCAL handle );
extern HLOCAL LOCAL_Handle( WORD ds, WORD addr );
extern WORD LOCAL_Size( WORD ds, HLOCAL handle );
extern WORD LOCAL_Flags( WORD ds, HLOCAL handle );
extern WORD LOCAL_HeapSize( WORD ds );

#endif  /* __WINE_LOCAL_H */
