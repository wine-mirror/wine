/*
 * Machine dependent integer conversions
 *
 * Copyright Miguel de Icaza, 1994
*/

#if defined (mc68000) || defined (sparc)
#define CONV_LONG(a) (((a)&0xFF) << 24) | (((a) & 0xFF00) << 8) | (((a) & 0xFF0000) >> 8) | ((a)&0xFF000000 >> 24)
#define CONV_SHORT(a) (((a) & 0xFF) << 8) | (((a) & 0xFF00) >> 8)
#define CONV_CHAR_TO_LONG(x) ((x) >> 24)
#define CONV_SHORT_TO_LONG(x) ((x) >> 16)
#else
#define CONV_LONG(a) (a)
#define CONV_SHORT(a) (a)
#define CONV_CHAR_TO_LONG(a) (a)
#define CONV_SHORT_TO_LONG(a) (a)
#endif
