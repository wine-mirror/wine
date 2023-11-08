/*
 * Copyright 2021 Huw Davies
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

#ifndef __WINE_NETIODEF_H
#define __WINE_NETIODEF_H

typedef enum _NPI_MODULEID_TYPE
{
    MIT_GUID = 1,
    MIT_IF_LUID
} NPI_MODULEID_TYPE;

typedef struct _NPI_MODULEID
{
    USHORT Length;
    NPI_MODULEID_TYPE Type;
    union
    {
        GUID Guid;
        LUID IfLuid;
    };
} NPI_MODULEID;

typedef const NPI_MODULEID *PNPI_MODULEID;

static inline BOOLEAN NmrIsEqualNpiModuleId( const NPI_MODULEID *mod1, const NPI_MODULEID *mod2 )
{
    if (mod1->Type == mod2->Type)
    {
        if (mod1->Type == MIT_GUID)
        {
#ifdef __cplusplus
            return IsEqualGUID( mod1->Guid, mod2->Guid );
#else
            return IsEqualGUID( &mod1->Guid, &mod2->Guid );
#endif
        }
        else if (mod1->Type == MIT_IF_LUID)
            return mod1->IfLuid.HighPart == mod2->IfLuid.HighPart &&
                mod1->IfLuid.LowPart == mod2->IfLuid.LowPart;
    }
    return FALSE;
}

#ifdef __WINE_INIT_NPI_MODULEID
#ifdef __cplusplus
#define DEFINE_NPI_GUID_MODULEID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const NPI_MODULEID name = \
    { sizeof(NPI_MODULEID), MIT_GUID, { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } } }
#else
#define DEFINE_NPI_GUID_MODULEID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const NPI_MODULEID name = \
    { sizeof(NPI_MODULEID), MIT_GUID, { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } } }
#endif
#else /* __WINE_INIT_MODULEID */
#define DEFINE_NPI_GUID_MODULEID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const NPI_MODULEID name
#endif /* __WINE_INIT_MODULEID */

#define DEFINE_NPI_MS_MODULEID(name, n) DEFINE_NPI_GUID_MODULEID(name, 0xeb004a00 + (n), 0x9b1a, 0x11d4, \
                                                                 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc)

DEFINE_NPI_MS_MODULEID( NPI_MS_IPV4_MODULEID,        0x00 );
DEFINE_NPI_MS_MODULEID( NPI_MS_IPV6_MODULEID,        0x01 );
DEFINE_NPI_MS_MODULEID( NPI_MS_UDP_MODULEID,         0x02 );
DEFINE_NPI_MS_MODULEID( NPI_MS_TCP_MODULEID,         0x03 );
DEFINE_NPI_MS_MODULEID( NPI_MS_NDIS_MODULEID,        0x11 );

#undef DEFINE_NPI_MS_MODULEID
#undef DEFINE_NPI_GUID_MODULEID

#endif /* __WINE_NETIODEF_H */
