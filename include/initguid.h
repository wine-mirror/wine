/*
 * defines a minimum set of macros create GUID's to keep the size 
 * small
 *
 * this file should be included into "only GUID definition *.h" like
 * shlguid.h
 */

#ifndef __INIT_GUID_H
#define __INIT_GUID_H

#include "wtypes.h"

/*****************************************************************************
 * Macros to declare the GUIDs
 */
#ifdef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern const GUID name
#endif

#define DEFINE_OLEGUID(name, l, w1, w2) \
	DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#define DEFINE_SHLGUID(name, l, w1, w2) DEFINE_OLEGUID(name,l,w1,w2)


#endif 
