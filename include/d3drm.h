/*
 * Copyright (C) 2005 Peter Berg Larsen
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

#ifndef __D3DRM_H__
#define __D3DRM_H__

#include <ddraw.h>
#include <d3drmobj.h>


/* Direct3DRM Object CLSID */
DEFINE_GUID(CLSID_CDirect3DRM,              0x4516ec41, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

/* Direct3DRM Interface GUIDs */
DEFINE_GUID(IID_IDirect3DRM,                0x2bc49361, 0x8327, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRM2,               0x4516ecc8, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRM3,               0x4516ec83, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

typedef void *LPDIRECT3DRM;

HRESULT WINAPI Direct3DRMCreate(LPDIRECT3DRM* ppDirect3DRM);

#define D3DRM_OK                        DD_OK
#define D3DRMERR_BADOBJECT              MAKE_DDHRESULT(781)
#define D3DRMERR_BADTYPE                MAKE_DDHRESULT(782)
#define D3DRMERR_BADALLOC               MAKE_DDHRESULT(783)
#define D3DRMERR_FACEUSED               MAKE_DDHRESULT(784)
#define D3DRMERR_NOTFOUND               MAKE_DDHRESULT(785)
#define D3DRMERR_NOTDONEYET             MAKE_DDHRESULT(786)
#define D3DRMERR_FILENOTFOUND           MAKE_DDHRESULT(787)
#define D3DRMERR_BADFILE                MAKE_DDHRESULT(788)
#define D3DRMERR_BADDEVICE              MAKE_DDHRESULT(789)
#define D3DRMERR_BADVALUE               MAKE_DDHRESULT(790)
#define D3DRMERR_BADMAJORVERSION        MAKE_DDHRESULT(791)
#define D3DRMERR_BADMINORVERSION        MAKE_DDHRESULT(792)
#define D3DRMERR_UNABLETOEXECUTE        MAKE_DDHRESULT(793)
#define D3DRMERR_LIBRARYNOTFOUND        MAKE_DDHRESULT(794)
#define D3DRMERR_INVALIDLIBRARY         MAKE_DDHRESULT(795)
#define D3DRMERR_PENDING                MAKE_DDHRESULT(796)
#define D3DRMERR_NOTENOUGHDATA          MAKE_DDHRESULT(797)
#define D3DRMERR_REQUESTTOOLARGE        MAKE_DDHRESULT(798)
#define D3DRMERR_REQUESTTOOSMALL        MAKE_DDHRESULT(799)
#define D3DRMERR_CONNECTIONLOST         MAKE_DDHRESULT(800)
#define D3DRMERR_LOADABORTED            MAKE_DDHRESULT(801)
#define D3DRMERR_NOINTERNET             MAKE_DDHRESULT(802)
#define D3DRMERR_BADCACHEFILE           MAKE_DDHRESULT(803)
#define D3DRMERR_BOXNOTSET              MAKE_DDHRESULT(804)
#define D3DRMERR_BADPMDATA              MAKE_DDHRESULT(805)
#define D3DRMERR_CLIENTNOTREGISTERED    MAKE_DDHRESULT(806)
#define D3DRMERR_NOTCREATEDFROMDDS      MAKE_DDHRESULT(807)
#define D3DRMERR_NOSUCHKEY              MAKE_DDHRESULT(808)
#define D3DRMERR_INCOMPATABLEKEY        MAKE_DDHRESULT(809)
#define D3DRMERR_ELEMENTINUSE           MAKE_DDHRESULT(810)
#define D3DRMERR_TEXTUREFORMATNOTFOUND  MAKE_DDHRESULT(811)
#define D3DRMERR_NOTAGGREGATED          MAKE_DDHRESULT(812)

#endif /* __D3DRM_H__ */
