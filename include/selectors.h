/*
 * Selector definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_SELECTORS_H
#define __WINE_SELECTORS_H

#include "windef.h"
#include "ldt.h"

extern WORD SELECTOR_AllocBlock( const void *base, DWORD size,
				 enum seg_type type, BOOL is32bit,
				 BOOL readonly );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size,
                                   enum seg_type type, BOOL is32bit,
                                   BOOL readonly );
extern void SELECTOR_MoveBlock( WORD sel, const void *new_base );
extern void SELECTOR_FreeBlock( WORD sel, WORD count );

#ifdef __i386__
# define __GET_SEG(seg,res) __asm__( "movw %%" seg ",%w0" : "=r" (res) )
# define __SET_SEG(seg,val) __asm__( "movw %w0,%%" seg : : "r" (val) )
#else  /* __i386__ */
# define __GET_SEG(seg,res) ((res) = 0)
# define __SET_SEG(seg,val) /* nothing */
#endif  /* __i386__ */

#define GET_CS(cs) __GET_SEG("cs",cs)
#define GET_DS(ds) __GET_SEG("ds",ds)
#define GET_ES(es) __GET_SEG("es",es)
#define GET_FS(fs) __GET_SEG("fs",fs)
#define GET_GS(gs) __GET_SEG("gs",gs)
#define GET_SS(ss) __GET_SEG("ss",ss)

#define SET_CS(cs) __SET_SEG("cs",cs)
#define SET_DS(ds) __SET_SEG("ds",ds)
#define SET_ES(es) __SET_SEG("es",es)
#define SET_FS(fs) __SET_SEG("fs",fs)
#define SET_GS(gs) __SET_SEG("gs",gs)
#define SET_SS(ss) __SET_SEG("ss",ss)

#endif /* __WINE_SELECTORS_H */
