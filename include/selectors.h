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
				 enum seg_type type, BOOL32 is32bit,
				 BOOL32 readonly );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size,
                                   enum seg_type type, BOOL32 is32bit,
                                   BOOL32 readonly );
extern void SELECTOR_FreeBlock( WORD sel, WORD count );

#if defined(linux)
# define WINE_DATA_SELECTOR 0x2b
# define WINE_CODE_SELECTOR 0x23
#elif defined(__NetBSD__)
# define WINE_DATA_SELECTOR 0x1f
# define WINE_CODE_SELECTOR 0x17
#elif defined(__FreeBSD__)
# define WINE_DATA_SELECTOR 0x27
# define WINE_CODE_SELECTOR 0x1f
#elif defined(__svr4__) || defined(_SCO_DS)
# define WINE_DATA_SELECTOR 0x1f
# define WINE_CODE_SELECTOR 0x17
#elif defined(__EMX__)
# define WINE_DATA_SELECTOR 0x53 /* Is this always true? */
# define WINE_CODE_SELECTOR 0x5b
#else
# define WINE_DATA_SELECTOR 0x00
# define WINE_CODE_SELECTOR 0x00
#endif

#endif /* __WINE_SELECTORS_H */
