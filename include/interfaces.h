#ifndef _WINE_INTERFACES_H
#define _WINE_INTERFACES_H

/* FIXME: This is not a standard Windows header. Move the contents of this file to the right place and then delete it. */

#include "ole.h"
#include "ole2.h"
#include "compobj.h"

/* FIXME: move to the right place. Some belong to aoidl.h some to oleauto.h */
DEFINE_OLEGUID(IID_IDispatch,       0x00020400,0,0);
DEFINE_OLEGUID(IID_ITypeInfo,       0x00020401,0,0);
DEFINE_OLEGUID(IID_ITypeLib,        0x00020402,0,0);
DEFINE_OLEGUID(IID_ITypeComp,       0x00020403,0,0);
DEFINE_OLEGUID(IID_IEnumVariant,    0x00020404,0,0);
DEFINE_OLEGUID(IID_ICreateTypeInfo, 0x00020405,0,0);
DEFINE_OLEGUID(IID_ICreateTypeLib,  0x00020406,0,0);
DEFINE_OLEGUID(IID_ICreateTypeInfo2,0x0002040E,0,0);
DEFINE_OLEGUID(IID_ICreateTypeLib2, 0x0002040F,0,0);
DEFINE_OLEGUID(IID_ITypeChangeEvents,0x00020410,0,0);
DEFINE_OLEGUID(IID_ITypeLib2,       0x00020411,0,0);
DEFINE_OLEGUID(IID_ITypeInfo2,      0x00020412,0,0);
DEFINE_GUID(IID_IErrorInfo,         0x1CF2B120,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
DEFINE_GUID(IID_ICreateErrorInfo,   0x22F03340,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
DEFINE_GUID(IID_ISupportErrorInfo,  0xDF0B3D60,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);

#include "objbase.h"


#endif /*_WINE_INTERFACES_H*/
