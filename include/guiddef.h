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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif

#ifndef __LPCGUID_DEFINED__
#define __LPCGUID_DEFINED__
typedef const GUID *LPCGUID;
#endif

#undef DEFINE_GUID

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

#ifndef _GUIDDEF_H_
#define _GUIDDEF_H_

typedef GUID *LPGUID;
typedef GUID CLSID,*LPCLSID;
#ifndef __IID_DEFINED__
#define __IID_DEFINED__
typedef GUID IID,*LPIID;
#endif /* ndef __IID_DEFINED__ */
typedef GUID FMTID,*LPFMTID;

#ifdef __midl_proxy
#define __MIDL_CONST
#else
#define __MIDL_CONST const
#endif

#if defined(__cplusplus) && !defined(CINTERFACE)
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else /* !defined(__cplusplus) && !defined(CINTERFACE) */
#define REFGUID             const GUID* __MIDL_CONST
#define REFCLSID            const CLSID* __MIDL_CONST
#define REFIID              const IID* __MIDL_CONST
#define REFFMTID            const FMTID* __MIDL_CONST
#endif /* !defined(__cplusplus) && !defined(CINTERFACE) */

#if defined(__cplusplus) && !defined(CINTERFACE)
#define IsEqualGUID(rguid1, rguid2) (!memcmp(&(rguid1), &(rguid2), sizeof(GUID)))
#else /* defined(__cplusplus) && !defined(CINTERFACE) */
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif /* defined(__cplusplus) && !defined(CINTERFACE) */
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#if defined(__cplusplus) && !defined(CINTERFACE)
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

extern const IID GUID_NULL;
#define IID_NULL            GUID_NULL
#define CLSID_NULL GUID_NULL
#define FMTID_NULL          GUID_NULL

#endif /* _GUIDDEF_H_ */
