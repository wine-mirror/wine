/* Direct3D Common functions
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains all common miscellaneous code that spans
 * different 'objects'
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

const char *_get_renderstate(D3DRENDERSTATETYPE type) {
    static const char * const states[] = {
        "ERR",
	"D3DRENDERSTATE_TEXTUREHANDLE",
	"D3DRENDERSTATE_ANTIALIAS",
	"D3DRENDERSTATE_TEXTUREADDRESS",
	"D3DRENDERSTATE_TEXTUREPERSPECTIVE",
	"D3DRENDERSTATE_WRAPU",
	"D3DRENDERSTATE_WRAPV",
	"D3DRENDERSTATE_ZENABLE",
	"D3DRENDERSTATE_FILLMODE",
	"D3DRENDERSTATE_SHADEMODE",
	"D3DRENDERSTATE_LINEPATTERN",
	"D3DRENDERSTATE_MONOENABLE",
	"D3DRENDERSTATE_ROP2",
	"D3DRENDERSTATE_PLANEMASK",
	"D3DRENDERSTATE_ZWRITEENABLE",
	"D3DRENDERSTATE_ALPHATESTENABLE",
	"D3DRENDERSTATE_LASTPIXEL",
	"D3DRENDERSTATE_TEXTUREMAG",
	"D3DRENDERSTATE_TEXTUREMIN",
	"D3DRENDERSTATE_SRCBLEND",
	"D3DRENDERSTATE_DESTBLEND",
	"D3DRENDERSTATE_TEXTUREMAPBLEND",
	"D3DRENDERSTATE_CULLMODE",
	"D3DRENDERSTATE_ZFUNC",
	"D3DRENDERSTATE_ALPHAREF",
	"D3DRENDERSTATE_ALPHAFUNC",
	"D3DRENDERSTATE_DITHERENABLE",
	"D3DRENDERSTATE_ALPHABLENDENABLE",
	"D3DRENDERSTATE_FOGENABLE",
	"D3DRENDERSTATE_SPECULARENABLE",
	"D3DRENDERSTATE_ZVISIBLE",
	"D3DRENDERSTATE_SUBPIXEL",
	"D3DRENDERSTATE_SUBPIXELX",
	"D3DRENDERSTATE_STIPPLEDALPHA",
	"D3DRENDERSTATE_FOGCOLOR",
	"D3DRENDERSTATE_FOGTABLEMODE",
	"D3DRENDERSTATE_FOGTABLESTART",
	"D3DRENDERSTATE_FOGTABLEEND",
	"D3DRENDERSTATE_FOGTABLEDENSITY",
	"D3DRENDERSTATE_STIPPLEENABLE",
	"D3DRENDERSTATE_EDGEANTIALIAS",
	"D3DRENDERSTATE_COLORKEYENABLE",
	"ERR",
	"D3DRENDERSTATE_BORDERCOLOR",
	"D3DRENDERSTATE_TEXTUREADDRESSU",
	"D3DRENDERSTATE_TEXTUREADDRESSV",
	"D3DRENDERSTATE_MIPMAPLODBIAS",
	"D3DRENDERSTATE_ZBIAS",
	"D3DRENDERSTATE_RANGEFOGENABLE",
	"D3DRENDERSTATE_ANISOTROPY",
	"D3DRENDERSTATE_FLUSHBATCH",
	"D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT",
	"D3DRENDERSTATE_STENCILENABLE",
	"D3DRENDERSTATE_STENCILFAIL",
	"D3DRENDERSTATE_STENCILZFAIL",
	"D3DRENDERSTATE_STENCILPASS",
	"D3DRENDERSTATE_STENCILFUNC",
	"D3DRENDERSTATE_STENCILREF",
	"D3DRENDERSTATE_STENCILMASK",
	"D3DRENDERSTATE_STENCILWRITEMASK",
	"D3DRENDERSTATE_TEXTUREFACTOR",
	"ERR",
	"ERR",
	"ERR",
	"D3DRENDERSTATE_STIPPLEPATTERN00",
	"D3DRENDERSTATE_STIPPLEPATTERN01",
	"D3DRENDERSTATE_STIPPLEPATTERN02",
	"D3DRENDERSTATE_STIPPLEPATTERN03",
	"D3DRENDERSTATE_STIPPLEPATTERN04",
	"D3DRENDERSTATE_STIPPLEPATTERN05",
	"D3DRENDERSTATE_STIPPLEPATTERN06",
	"D3DRENDERSTATE_STIPPLEPATTERN07",
	"D3DRENDERSTATE_STIPPLEPATTERN08",
	"D3DRENDERSTATE_STIPPLEPATTERN09",
	"D3DRENDERSTATE_STIPPLEPATTERN10",
	"D3DRENDERSTATE_STIPPLEPATTERN11",
	"D3DRENDERSTATE_STIPPLEPATTERN12",
	"D3DRENDERSTATE_STIPPLEPATTERN13",
	"D3DRENDERSTATE_STIPPLEPATTERN14",
	"D3DRENDERSTATE_STIPPLEPATTERN15",
	"D3DRENDERSTATE_STIPPLEPATTERN16",
	"D3DRENDERSTATE_STIPPLEPATTERN17",
	"D3DRENDERSTATE_STIPPLEPATTERN18",
	"D3DRENDERSTATE_STIPPLEPATTERN19",
	"D3DRENDERSTATE_STIPPLEPATTERN20",
	"D3DRENDERSTATE_STIPPLEPATTERN21",
	"D3DRENDERSTATE_STIPPLEPATTERN22",
	"D3DRENDERSTATE_STIPPLEPATTERN23",
	"D3DRENDERSTATE_STIPPLEPATTERN24",
	"D3DRENDERSTATE_STIPPLEPATTERN25",
	"D3DRENDERSTATE_STIPPLEPATTERN26",
	"D3DRENDERSTATE_STIPPLEPATTERN27",
	"D3DRENDERSTATE_STIPPLEPATTERN28",
	"D3DRENDERSTATE_STIPPLEPATTERN29",
	"D3DRENDERSTATE_STIPPLEPATTERN30",
	"D3DRENDERSTATE_STIPPLEPATTERN31"
    };
    static const char * const states_2[] = {
        "D3DRENDERSTATE_WRAP0",
	"D3DRENDERSTATE_WRAP1",
	"D3DRENDERSTATE_WRAP2",
	"D3DRENDERSTATE_WRAP3",
	"D3DRENDERSTATE_WRAP4",
	"D3DRENDERSTATE_WRAP5",
	"D3DRENDERSTATE_WRAP6",
	"D3DRENDERSTATE_WRAP7",
	"D3DRENDERSTATE_CLIPPING",
	"D3DRENDERSTATE_LIGHTING",
	"D3DRENDERSTATE_EXTENTS",
	"D3DRENDERSTATE_AMBIENT",
	"D3DRENDERSTATE_FOGVERTEXMODE",
	"D3DRENDERSTATE_COLORVERTEX",
	"D3DRENDERSTATE_LOCALVIEWER",
	"D3DRENDERSTATE_NORMALIZENORMALS",
	"D3DRENDERSTATE_COLORKEYBLENDENABLE",
	"D3DRENDERSTATE_DIFFUSEMATERIALSOURCE",
	"D3DRENDERSTATE_SPECULARMATERIALSOURCE",
	"D3DRENDERSTATE_AMBIENTMATERIALSOURCE",
	"D3DRENDERSTATE_EMISSIVEMATERIALSOURCE",
	"ERR",
	"ERR",
	"D3DRENDERSTATE_VERTEXBLEND",
	"D3DRENDERSTATE_CLIPPLANEENABLE",
    };
    if (type >= D3DRENDERSTATE_WRAP0) {
        type -= D3DRENDERSTATE_WRAP0;
	if (type >= (sizeof(states_2) / sizeof(states_2[0]))) return "ERR";
	return states_2[type];
    }
    if (type >= (sizeof(states) / sizeof(states[0]))) return "ERR";
    return states[type];
}

void
dump_D3DCOLORVALUE(D3DCOLORVALUE *lpCol)
{
    DPRINTF("%f %f %f %f", lpCol->u1.r, lpCol->u2.g, lpCol->u3.b, lpCol->u4.a);
}

void
dump_D3DVECTOR(D3DVECTOR *lpVec)
{
    DPRINTF("%f %f %f", lpVec->u1.x, lpVec->u2.y, lpVec->u3.z);
}

void
dump_D3DMATERIAL7(LPD3DMATERIAL7 lpMat)
{
    DPRINTF(" - diffuse  : "); dump_D3DCOLORVALUE(&(lpMat->u.diffuse)); DPRINTF("\n");
    DPRINTF(" - ambient  : "); dump_D3DCOLORVALUE(&(lpMat->u1.ambient)); DPRINTF("\n");
    DPRINTF(" - specular : "); dump_D3DCOLORVALUE(&(lpMat->u2.specular)); DPRINTF("\n");
    DPRINTF(" - emissive : "); dump_D3DCOLORVALUE(&(lpMat->u3.emissive)); DPRINTF("\n");
    DPRINTF(" - power    : %f\n", lpMat->u4.power);
}

void
dump_D3DLIGHT7(LPD3DLIGHT7 lpLight)
{
    DPRINTF(" - light type : %s\n", (lpLight->dltType == D3DLIGHT_POINT ? "D3DLIGHT_POINT" : 
				     (lpLight->dltType == D3DLIGHT_SPOT ? "D3DLIGHT_SPOT" : 
				      (lpLight->dltType == D3DLIGHT_DIRECTIONAL ? "D3DLIGHT_DIRECTIONAL" : 
				       "UNSUPPORTED"))));
    DPRINTF(" - diffuse       : "); dump_D3DCOLORVALUE(&(lpLight->dcvDiffuse)); DPRINTF("\n");
    DPRINTF(" - specular      : "); dump_D3DCOLORVALUE(&(lpLight->dcvSpecular)); DPRINTF("\n");
    DPRINTF(" - ambient       : "); dump_D3DCOLORVALUE(&(lpLight->dcvAmbient)); DPRINTF("\n");
    DPRINTF(" - position      : "); dump_D3DVECTOR(&(lpLight->dvPosition)); DPRINTF("\n");
    DPRINTF(" - direction     : "); dump_D3DVECTOR(&(lpLight->dvDirection)); DPRINTF("\n");
    DPRINTF(" - dvRange       : %f\n", lpLight->dvRange);
    DPRINTF(" - dvFalloff     : %f\n", lpLight->dvFalloff);
    DPRINTF(" - dvAttenuation : %f %f %f\n", lpLight->dvAttenuation0, lpLight->dvAttenuation1, lpLight->dvAttenuation2);
    DPRINTF(" - dvTheta       : %f\n", lpLight->dvTheta);
    DPRINTF(" - dvPhi         : %f\n", lpLight->dvPhi);
}

void
dump_DPFLAGS(DWORD dwFlags)
{
    static const flag_info flags[] =
    {
        FE(D3DDP_WAIT),
	FE(D3DDP_OUTOFORDER),
	FE(D3DDP_DONOTCLIP),
	FE(D3DDP_DONOTUPDATEEXTENTS),
	FE(D3DDP_DONOTLIGHT)
    };

    DDRAW_dump_flags(dwFlags, flags, sizeof(flags)/sizeof(flags[0]));
}

void
dump_D3DMATRIX(D3DMATRIX *mat)
{
    DPRINTF("  %f %f %f %f\n", mat->_11, mat->_12, mat->_13, mat->_14);
    DPRINTF("  %f %f %f %f\n", mat->_21, mat->_22, mat->_23, mat->_24);
    DPRINTF("  %f %f %f %f\n", mat->_31, mat->_32, mat->_33, mat->_34);
    DPRINTF("  %f %f %f %f\n", mat->_41, mat->_42, mat->_43, mat->_44);
}

DWORD get_flexible_vertex_size(DWORD d3dvtVertexType)
{
    DWORD size = 0;
    int i;
    
    if (d3dvtVertexType & D3DFVF_NORMAL) size += 3 * sizeof(D3DVALUE);
    if (d3dvtVertexType & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (d3dvtVertexType & D3DFVF_SPECULAR) size += sizeof(DWORD);
    if (d3dvtVertexType & D3DFVF_RESERVED1) size += sizeof(DWORD);
    switch (d3dvtVertexType & D3DFVF_POSITION_MASK) {
        case D3DFVF_XYZ: size += 3 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZRHW: size += 4 * sizeof(D3DVALUE); break;
	default: TRACE(" matrix weighting not handled yet...\n");
    }
    for (i = 0; i < GET_TEXCOUNT_FROM_FVF(d3dvtVertexType); i++) {
	size += GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, i) * sizeof(D3DVALUE);
    }
    
    return size;
}

void dump_flexible_vertex(DWORD d3dvtVertexType)
{
    static const flag_info flags[] = {
        FE(D3DFVF_NORMAL),
	FE(D3DFVF_RESERVED1),
	FE(D3DFVF_DIFFUSE),
	FE(D3DFVF_SPECULAR)
    };
    unsigned int i;
    
    if (d3dvtVertexType & D3DFVF_RESERVED0) DPRINTF("D3DFVF_RESERVED0 ");
    switch (d3dvtVertexType & D3DFVF_POSITION_MASK) {
#define GEN_CASE(a) case a: DPRINTF(#a " "); break
        GEN_CASE(D3DFVF_XYZ);
	GEN_CASE(D3DFVF_XYZRHW);
	GEN_CASE(D3DFVF_XYZB1);
	GEN_CASE(D3DFVF_XYZB2);
	GEN_CASE(D3DFVF_XYZB3);
	GEN_CASE(D3DFVF_XYZB4);
	GEN_CASE(D3DFVF_XYZB5);
    }
    DDRAW_dump_flags_(d3dvtVertexType, flags, sizeof(flags)/sizeof(flags[0]), FALSE);
    switch (d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) {
        GEN_CASE(D3DFVF_TEX0);
	GEN_CASE(D3DFVF_TEX1);
	GEN_CASE(D3DFVF_TEX2);
	GEN_CASE(D3DFVF_TEX3);
	GEN_CASE(D3DFVF_TEX4);
	GEN_CASE(D3DFVF_TEX5);
	GEN_CASE(D3DFVF_TEX6);
	GEN_CASE(D3DFVF_TEX7);
	GEN_CASE(D3DFVF_TEX8);
    }
#undef GEN_CASE
    for (i = 0; i < GET_TEXCOUNT_FROM_FVF(d3dvtVertexType); i++) {
        DPRINTF(" T%d-s%ld", i + 1, GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, i));
    }
    DPRINTF("\n");
}

void
convert_FVF_to_strided_data(DWORD d3dvtVertexType, LPVOID lpvVertices, D3DDRAWPRIMITIVESTRIDEDDATA *strided, DWORD dwStartVertex)
{
    int current_offset = 0;
    unsigned int tex_index;
    int size = get_flexible_vertex_size(d3dvtVertexType);

    lpvVertices = ((BYTE *) lpvVertices) + (size * dwStartVertex);
    
    if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) {
        strided->position.lpvData = lpvVertices;
        current_offset += 3 * sizeof(D3DVALUE);
    } else {
        strided->position.lpvData  = lpvVertices;
        current_offset += 4 * sizeof(D3DVALUE);
    }
    if (d3dvtVertexType & D3DFVF_RESERVED1) {
        current_offset += sizeof(DWORD);
    }
    if (d3dvtVertexType & D3DFVF_NORMAL) { 
        strided->normal.lpvData  = ((char *) lpvVertices) + current_offset;
        current_offset += 3 * sizeof(D3DVALUE);
    }
    if (d3dvtVertexType & D3DFVF_DIFFUSE) { 
        strided->diffuse.lpvData  = ((char *) lpvVertices) + current_offset;
        current_offset += sizeof(DWORD);
    }
    if (d3dvtVertexType & D3DFVF_SPECULAR) {
        strided->specular.lpvData  = ((char *) lpvVertices) + current_offset;
        current_offset += sizeof(DWORD);
    }
    for (tex_index = 0; tex_index < GET_TEXCOUNT_FROM_FVF(d3dvtVertexType); tex_index++) {
        strided->textureCoords[tex_index].lpvData  = ((char *) lpvVertices) + current_offset;
        current_offset += GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_index) * sizeof(D3DVALUE);
    }
    strided->position.dwStride = current_offset;
    strided->normal.dwStride   = current_offset;
    strided->diffuse.dwStride  = current_offset;
    strided->specular.dwStride = current_offset;
    for (tex_index = 0; tex_index < ((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT); tex_index++)
        strided->textureCoords[tex_index].dwStride  = current_offset;   
}

void
dump_D3DVOP(DWORD dwVertexOp)
{
    static const flag_info flags[] =
    {
        FE(D3DVOP_LIGHT),
	FE(D3DVOP_CLIP),
	FE(D3DVOP_EXTENTS),
	FE(D3DVOP_TRANSFORM)
    };
    DDRAW_dump_flags(dwVertexOp, flags, sizeof(flags)/sizeof(flags[0]));
}

void
dump_D3DPV(DWORD dwFlags)
{
    if (dwFlags == D3DPV_DONOTCOPYDATA) DPRINTF("D3DPV_DONOTCOPYDATA\n");
    else if (dwFlags != 0) DPRINTF("Unknown !!!\n");
    else DPRINTF("\n");
}

void multiply_matrix(LPD3DMATRIX dest, LPD3DMATRIX src1, LPD3DMATRIX src2)
{
    D3DMATRIX temp;

    /* Now do the multiplication 'by hand'.
       I know that all this could be optimised, but this will be done later :-) */
    temp._11 = (src1->_11 * src2->_11) + (src1->_21 * src2->_12) + (src1->_31 * src2->_13) + (src1->_41 * src2->_14);
    temp._21 = (src1->_11 * src2->_21) + (src1->_21 * src2->_22) + (src1->_31 * src2->_23) + (src1->_41 * src2->_24);
    temp._31 = (src1->_11 * src2->_31) + (src1->_21 * src2->_32) + (src1->_31 * src2->_33) + (src1->_41 * src2->_34);
    temp._41 = (src1->_11 * src2->_41) + (src1->_21 * src2->_42) + (src1->_31 * src2->_43) + (src1->_41 * src2->_44);

    temp._12 = (src1->_12 * src2->_11) + (src1->_22 * src2->_12) + (src1->_32 * src2->_13) + (src1->_42 * src2->_14);
    temp._22 = (src1->_12 * src2->_21) + (src1->_22 * src2->_22) + (src1->_32 * src2->_23) + (src1->_42 * src2->_24);
    temp._32 = (src1->_12 * src2->_31) + (src1->_22 * src2->_32) + (src1->_32 * src2->_33) + (src1->_42 * src2->_34);
    temp._42 = (src1->_12 * src2->_41) + (src1->_22 * src2->_42) + (src1->_32 * src2->_43) + (src1->_42 * src2->_44);

    temp._13 = (src1->_13 * src2->_11) + (src1->_23 * src2->_12) + (src1->_33 * src2->_13) + (src1->_43 * src2->_14);
    temp._23 = (src1->_13 * src2->_21) + (src1->_23 * src2->_22) + (src1->_33 * src2->_23) + (src1->_43 * src2->_24);
    temp._33 = (src1->_13 * src2->_31) + (src1->_23 * src2->_32) + (src1->_33 * src2->_33) + (src1->_43 * src2->_34);
    temp._43 = (src1->_13 * src2->_41) + (src1->_23 * src2->_42) + (src1->_33 * src2->_43) + (src1->_43 * src2->_44);

    temp._14 = (src1->_14 * src2->_11) + (src1->_24 * src2->_12) + (src1->_34 * src2->_13) + (src1->_44 * src2->_14);
    temp._24 = (src1->_14 * src2->_21) + (src1->_24 * src2->_22) + (src1->_34 * src2->_23) + (src1->_44 * src2->_24);
    temp._34 = (src1->_14 * src2->_31) + (src1->_24 * src2->_32) + (src1->_34 * src2->_33) + (src1->_44 * src2->_34);
    temp._44 = (src1->_14 * src2->_41) + (src1->_24 * src2->_42) + (src1->_34 * src2->_43) + (src1->_44 * src2->_44);

    /* And copy the new matrix in the good storage.. */
    memcpy(dest, &temp, 16 * sizeof(D3DVALUE));    
}
