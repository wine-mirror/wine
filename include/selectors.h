/*
 * Selector definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_SELECTORS_H
#define __WINE_SELECTORS_H

#include "windows.h"
#include "ldt.h"

extern WORD SELECTOR_AllocBlock( const void *base, DWORD size,
				 enum seg_type type, BOOL is32bit,
				 BOOL readonly );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size,
                                   enum seg_type type, BOOL is32bit,
                                   BOOL readonly );

extern void CreateSelectors(void);

extern WNDPROC GetWndProcEntry16( char *name );

#endif /* __WINE_SELECTORS_H */
