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
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size );
extern void SELECTOR_MoveBlock( WORD sel, const void *new_base );
extern void SELECTOR_FreeBlock( WORD sel, WORD count );

#ifdef __i386__
# ifdef __GNUC__
#  define __DEFINE_GET_SEG(seg) \
    extern inline unsigned short __get_##seg(void) \
    { unsigned short res; __asm__("movw %%" #seg ",%w0" : "=r"(res)); return res; }
#  define __DEFINE_SET_SEG(seg) \
    extern inline void __set_##seg(int val) { __asm__("movw %w0,%%" #seg : : "r" (val)); }
# else  /* __GNUC__ */
#  define __DEFINE_GET_SEG(seg) extern unsigned short __get_##seg(void);
#  define __DEFINE_SET_SEG(seg) extern void __set_##seg(unsigned int);
# endif /* __GNUC__ */
#else  /* __i386__ */
# define __DEFINE_GET_SEG(seg) static inline unsigned short __get_##seg(void) { return 0; }
# define __DEFINE_SET_SEG(seg) /* nothing */
#endif  /* __i386__ */

__DEFINE_GET_SEG(cs)
__DEFINE_GET_SEG(ds)
__DEFINE_GET_SEG(es)
__DEFINE_GET_SEG(fs)
__DEFINE_GET_SEG(gs)
__DEFINE_GET_SEG(ss)
__DEFINE_SET_SEG(fs)
__DEFINE_SET_SEG(gs)

#endif /* __WINE_SELECTORS_H */
