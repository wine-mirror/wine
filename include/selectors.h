/*
 * Selector definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_SELECTORS_H
#define __WINE_SELECTORS_H

#include "windows.h"
#include "ldt.h"

extern WORD SELECTOR_AllocBlock( void *base, DWORD size, enum seg_type type,
                                 BOOL is32bit, BOOL readonly );
extern WORD SELECTOR_ReallocBlock( WORD sel, void *base, DWORD size,
                                   enum seg_type type, BOOL is32bit,
                                   BOOL readonly );

#include "dlls.h"

extern WORD *CreateSelectors( struct w_files * wpnt );

extern unsigned int GetEntryDLLName(char *dll_name, char *function,
                                    WORD *sel, WORD *offset);
extern unsigned int GetEntryDLLOrdinal(char *dll_name, int ordinal,
                                       WORD *sel, WORD *offset);
extern unsigned int GetEntryPointFromOrdinal(struct w_files * wpnt,
                                             int ordinal);
extern void InitSelectors(void);

extern WNDPROC GetWndProcEntry16( char *name );

#endif /* __WINE_SELECTORS_H */
