#ifndef __WINE_OAIDL_H
#define __WINE_OAIDL_H


#include "wine/obj_base.h"

/* the following depend only on obj_base.h */
#include "wine/obj_oleaut.h"

typedef enum tagCALLCONV {
    CC_CDECL    = 1,
    CC_MSCPASCAL  = CC_CDECL + 1,
    CC_PASCAL   = CC_MSCPASCAL,
    CC_MACPASCAL  = CC_PASCAL + 1,
    CC_STDCALL    = CC_MACPASCAL + 1,
    CC_RESERVED   = CC_STDCALL + 1,
    CC_SYSCALL    = CC_RESERVED + 1,
    CC_MPWCDECL   = CC_SYSCALL + 1,
    CC_MPWPASCAL  = CC_MPWCDECL + 1,
    CC_MAX    = CC_MPWPASCAL + 1
} CALLCONV;

#endif /* _WINE_OAIDL_H */
