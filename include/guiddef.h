/*
 * Copyright (C) 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef GUID_DEFINED
#define GUID_DEFINED

#ifdef __WIDL__
typedef struct
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    byte           Data4[ 8 ];
} GUID;
#else
typedef struct _GUID
{
#ifndef __LP64__
    unsigned long  Data1;
#else
    unsigned int   Data1;
#endif
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif

/* Macros for __uuidof emulation */
#ifdef __cplusplus
# if defined(__MINGW32__)
#  if !defined(__uuidof)  /* Mingw64 can provide support for __uuidof and __CRT_UUID_DECL */
#   define __WINE_UUID_ATTR __attribute__((selectany))
#   undef __CRT_UUID_DECL
#  endif
# elif defined(__GNUC__)
#  define __WINE_UUID_ATTR __attribute__((visibility("hidden"),weak))
# endif
#endif

#ifdef __WINE_UUID_ATTR

extern "C++" {
    template<typename T> struct __wine_uuidof;

    template<typename T> struct __wine_uuidof_type {
        typedef __wine_uuidof<T> inst;
    };
    template<typename T> struct __wine_uuidof_type<T *> {
        typedef __wine_uuidof<T> inst;
    };
    template<typename T> struct __wine_uuidof_type<T * const> {
        typedef __wine_uuidof<T> inst;
    };
}

#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)           \
    extern "C++" {                                                      \
        template<> struct __wine_uuidof<type> {                         \
            static const GUID uuid;                                     \
        };                                                              \
        __WINE_UUID_ATTR const GUID __wine_uuidof<type>::uuid = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}; \
    }

#define __uuidof(type) __wine_uuidof_type<__typeof__(type)>::inst::uuid

#elif !defined(__CRT_UUID_DECL)

#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#endif /* __WINE_UUID_ATTR */

#endif

#undef DEFINE_GUID

#ifdef INITGUID
#ifdef __cplusplus
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name
#endif

#define DEFINE_OLEGUID(name, l, w1, w2) \
	DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#ifndef _GUIDDEF_H_
#define _GUIDDEF_H_

#ifndef __LPGUID_DEFINED__
#define __LPGUID_DEFINED__
typedef GUID *LPGUID;
#endif

#ifndef __LPCGUID_DEFINED__
#define __LPCGUID_DEFINED__
typedef const GUID *LPCGUID;
#endif

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef GUID IID,*LPIID;
typedef GUID CLSID,*LPCLSID;
typedef GUID FMTID,*LPFMTID;
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)
#define IsEqualFMTID(rfmtid1, rfmtid2) IsEqualGUID(rfmtid1, rfmtid2)
#define IID_NULL   GUID_NULL
#define CLSID_NULL GUID_NULL
#define FMTID_NULL GUID_NULL

#ifdef __midl_proxy
#define __MIDL_CONST
#else
#define __MIDL_CONST const
#endif

#endif /* ndef __IID_DEFINED__ */

#ifdef __cplusplus
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else
#define REFGUID             const GUID* __MIDL_CONST
#define REFCLSID            const CLSID* __MIDL_CONST
#define REFIID              const IID* __MIDL_CONST
#define REFFMTID            const FMTID* __MIDL_CONST
#endif

#ifdef __cplusplus
#define IsEqualGUID(rguid1, rguid2) (!memcmp(&(rguid1), &(rguid2), sizeof(GUID)))
inline int InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (((unsigned int *)&rguid1)[0] == ((unsigned int *)&rguid2)[0] &&
           ((unsigned int *)&rguid1)[1] == ((unsigned int *)&rguid2)[1] &&
           ((unsigned int *)&rguid1)[2] == ((unsigned int *)&rguid2)[2] &&
           ((unsigned int *)&rguid1)[3] == ((unsigned int *)&rguid2)[3]);
}
#else
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define InlineIsEqualGUID(rguid1, rguid2)  \
        (((unsigned int *)rguid1)[0] == ((unsigned int *)rguid2)[0] && \
         ((unsigned int *)rguid1)[1] == ((unsigned int *)rguid2)[1] && \
         ((unsigned int *)rguid1)[2] == ((unsigned int *)rguid2)[2] && \
         ((unsigned int *)rguid1)[3] == ((unsigned int *)rguid2)[3])
#endif

#ifdef __cplusplus
#include <string.h>
inline bool operator==(const GUID& guidOne, const GUID& guidOther)
{
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
}
inline bool operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}
#endif

#endif /* _GUIDDEF_H_ */
