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

extern HLOCAL16 LOCAL_Alloc( HANDLE16 ds, UINT16 flags, WORD size );
extern HLOCAL16 LOCAL_ReAlloc( HANDLE16 ds, HLOCAL16 handle,
                               WORD size, UINT16 flags );
extern HLOCAL16 LOCAL_Free( HANDLE16 ds, HLOCAL16 handle );
extern HLOCAL16 LOCAL_Handle( HANDLE16 ds, WORD addr );
extern UINT16 LOCAL_Size( HANDLE16 ds, HLOCAL16 handle );
extern UINT16 LOCAL_Flags( HANDLE16 ds, HLOCAL16 handle );
extern UINT16 LOCAL_HeapSize( HANDLE16 ds );
extern UINT16 LOCAL_CountFree( HANDLE16 ds );
extern LPSTR LOCAL_Lock( HANDLE16 ds, HLOCAL16 handle );
extern SEGPTR LOCAL_LockSegptr( HANDLE16 ds, HLOCAL16 handle );
extern BOOL16 LOCAL_Unlock( HANDLE16 ds, HLOCAL16 handle );

#endif  /* __WINE_LOCAL_H */
