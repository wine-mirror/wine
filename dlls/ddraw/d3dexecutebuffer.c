/* Direct3D ExecuteBuffer
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains the implementation of Direct3DExecuteBuffer.
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

#include "config.h"

#include <string.h>

#include "windef.h"
#include "winerror.h"
#include "objbase.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"
#include "mesa_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* Structure to store the 'semi transformed' vertices */
typedef struct {
    D3DVALUE x;
    D3DVALUE y;
    D3DVALUE z;
    D3DVALUE w;

    D3DVALUE nx;
    D3DVALUE ny;
    D3DVALUE nz;

    D3DVALUE u;
    D3DVALUE v;
} OGL_Vertex;

typedef struct {
    D3DVALUE x;
    D3DVALUE y;
    D3DVALUE z;
    D3DVALUE w;

    D3DCOLOR c;
    D3DCOLOR sc;

    D3DVALUE u;
    D3DVALUE v;
} OGL_LVertex;

static void _dump_d3dstatus(LPD3DSTATUS lpStatus) {

}

static void _dump_executedata(LPD3DEXECUTEDATA lpData) {
    DPRINTF("dwSize : %ld\n", lpData->dwSize);
    DPRINTF("Vertex      Offset : %ld  Count  : %ld\n", lpData->dwVertexOffset, lpData->dwVertexCount);
    DPRINTF("Instruction Offset : %ld  Length : %ld\n", lpData->dwInstructionOffset, lpData->dwInstructionLength);
    DPRINTF("HVertex     Offset : %ld\n", lpData->dwHVertexOffset);
    _dump_d3dstatus(&(lpData->dsStatus));
}

static void _dump_D3DEXECUTEBUFFERDESC(LPD3DEXECUTEBUFFERDESC lpDesc) {

}

#define DO_VERTEX(index) 			                                \
{										\
    glTexCoord2f(vx[index].u,							\
   	         vx[index].v);							\
    glNormal3f(vx[index].nx,							\
	       vx[index].ny,							\
	       vx[index].nz);							\
    glVertex4f(vx[index].x,						     	\
	       vx[index].y,							\
	       vx[index].z,							\
	       vx[index].w);							\
										\
    TRACE("   V: %f %f %f %f (%f %f %f) (%f %f)\n",			        \
	  vx[index].x, vx[index].y, vx[index].z, vx[index].w,			\
	  vx[index].nx, vx[index].ny, vx[index].nz,				\
	  vx[index].u, vx[index].v);						\
}

#define DO_LVERTEX(index)							\
{										\
    DWORD col = l_vx[index].c;							\
  										\
    glColor3f(((col >> 16) & 0xFF) / 255.0,					\
	      ((col >>  8) & 0xFF) / 255.0,					\
	      ((col >>  0) & 0xFF) / 255.0);					\
    glTexCoord2f(l_vx[index].u,							\
	         l_vx[index].v);						\
    glVertex4f(l_vx[index].x,							\
	       l_vx[index].y,							\
	       l_vx[index].z,							\
	       l_vx[index].w);							\
  										\
    TRACE("  LV: %f %f %f %f (%02lx %02lx %02lx) (%f %f)\n",		        \
	  l_vx[index].x, l_vx[index].y, l_vx[index].z, l_vx[index].w,		\
	  ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),	\
	  l_vx[index].u, l_vx[index].v);					\
}

#define DO_TLVERTEX(index)							\
{										\
    D3DTLVERTEX *vx = &(tl_vx[index]);						\
    DWORD col = vx->u5.color;							\
										\
    glColor3f(((col >> 16) & 0xFF) / 255.0,					\
              ((col >>  8) & 0xFF) / 255.0,					\
              ((col >>  0) & 0xFF) / 255.0);					\
    glTexCoord2f(vx->u7.tu, vx->u8.tv);						\
    if (vx->u4.rhw < 1e-8)							\
        glVertex3f(vx->u1.sx,							\
                   vx->u2.sy,							\
                   vx->u3.sz);							\
    else									\
        glVertex4f(vx->u1.sx / vx->u4.rhw,					\
                   vx->u2.sy / vx->u4.rhw,					\
                   vx->u3.sz / vx->u4.rhw,					\
                   1.0 / vx->u4.rhw);						\
    TRACE(" TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",		        \
          vx->u1.sx, vx->u2.sy, vx->u3.sz,					\
          ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),	\
          vx->u7.tu, vx->u8.tv, vx->u4.rhw);					\
}

#define TRIANGLE_LOOP(macro)				\
{							\
    glBegin(GL_TRIANGLES); 				\
    for (i = 0; i < count; i++) {			\
        LPD3DTRIANGLE ci = (LPD3DTRIANGLE) instr;	\
      							\
        TRACE("  v1: %d  v2: %d  v3: %d\n",	        \
	      ci->u1.v1, ci->u2.v2, ci->u3.v3);		\
        TRACE("  Flags : ");			        \
        if (TRACE_ON(ddraw)) {				\
	    /* Wireframe */				\
	    if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)	\
	        DPRINTF("EDGEENABLE1 ");		\
	    if (ci->wFlags & D3DTRIFLAG_EDGEENABLE2)	\
	        DPRINTF("EDGEENABLE2 ");		\
	    if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)	\
	        DPRINTF("EDGEENABLE3 ");		\
							\
	    /* Strips / Fans */				\
	    if (ci->wFlags == D3DTRIFLAG_EVEN)		\
	        DPRINTF("EVEN ");			\
	    if (ci->wFlags == D3DTRIFLAG_ODD)		\
	        DPRINTF("ODD ");			\
	    if (ci->wFlags == D3DTRIFLAG_START)		\
	        DPRINTF("START ");			\
	    if ((ci->wFlags > 0) && (ci->wFlags < 30))	\
	        DPRINTF("STARTFLAT(%d) ", ci->wFlags);	\
	    DPRINTF("\n");				\
        }						\
							\
        /* Draw the triangle */				\
        macro(ci->u1.v1);				\
        macro(ci->u2.v2);				\
        macro(ci->u3.v3);				\
							\
        instr += size;					\
    }							\
    glEnd();						\
}


static void execute(IDirect3DExecuteBufferImpl *This,
		    IDirect3DDeviceImpl *lpDevice,
		    IDirect3DViewportImpl *lpViewport)
{
    IDirect3DDeviceGLImpl* lpDeviceGL = (IDirect3DDeviceGLImpl*) lpDevice;
    /* DWORD bs = This->desc.dwBufferSize; */
    DWORD vs = This->data.dwVertexOffset;
    /* DWORD vc = This->data.dwVertexCount; */
    DWORD is = This->data.dwInstructionOffset;
    /* DWORD il = This->data.dwInstructionLength; */
    
    void *instr = This->desc.lpData + is;

    /* Should check if the viewport was added or not to the device */

    /* Activate the viewport */
    lpViewport->active_device = lpDevice;
    lpViewport->activate(lpViewport);

    TRACE("ExecuteData : \n");
    if (TRACE_ON(ddraw))
      _dump_executedata(&(This->data));

    ENTER_GL();

    while (1) {
        LPD3DINSTRUCTION current = (LPD3DINSTRUCTION) instr;
	BYTE size;
	WORD count;
	
	count = current->wCount;
	size = current->bSize;
	instr += sizeof(D3DINSTRUCTION);
	
	switch (current->bOpcode) {
	    case D3DOP_POINT: {
	        WARN("POINT-s          (%d)\n", count);
		instr += count * size;
	    } break;

	    case D3DOP_LINE: {
	        WARN("LINE-s           (%d)\n", count);
		instr += count * size;
	    } break;

	    case D3DOP_TRIANGLE: {
	        int i;
		OGL_Vertex  *vx    = (OGL_Vertex  *) This->vertex_data;
		OGL_LVertex *l_vx  = (OGL_LVertex *) This->vertex_data;
		D3DTLVERTEX *tl_vx = (D3DTLVERTEX *) This->vertex_data;
		TRACE("TRIANGLE         (%d)\n", count);
		
		switch (This->vertex_type) {
		    case D3DVT_VERTEX:
		        /* This time, there is lighting */
		        glEnable(GL_LIGHTING);
			
			if (TRACE_ON(ddraw)) {
			    TRACE("  Projection Matrix : (%p)\n", lpDevice->proj_mat);
			    dump_D3DMATRIX(lpDevice->proj_mat);
			    TRACE("  View       Matrix : (%p)\n", lpDevice->view_mat);
			    dump_D3DMATRIX(lpDevice->view_mat);
			}

			/* Using the identity matrix as the world matrix as the world transformation was
			   already done. */
			lpDevice->set_matrices(lpDevice, VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED,
					       (D3DMATRIX *) id_mat, lpDevice->view_mat, lpDevice->proj_mat);
			break;

		    case D3DVT_LVERTEX:
			/* No lighting */
			glDisable(GL_LIGHTING);

			if (TRACE_ON(ddraw)) {
			    TRACE("  Projection Matrix : (%p)\n", lpDevice->proj_mat);
			    dump_D3DMATRIX(lpDevice->proj_mat);
			    TRACE("  View       Matrix : (%p)\n", lpDevice->view_mat);
			    dump_D3DMATRIX(lpDevice->view_mat);
			}

			/* Using the identity matrix as the world matrix as the world transformation was
			   already done. */
			lpDevice->set_matrices(lpDevice, VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED,
					       (D3DMATRIX *) id_mat, lpDevice->view_mat, lpDevice->proj_mat);
			break;

		    case D3DVT_TLVERTEX: {
		        GLdouble height, width;
			GLfloat trans_mat[16];
			
			/* First, disable lighting */
			glDisable(GL_LIGHTING);
			
			width = lpDevice->surface->surface_desc.dwWidth;
			height = lpDevice->surface->surface_desc.dwHeight;

			/* The X axis is straighforward.. For the Y axis, we need to convert 'D3D' screen coordinates
			   to OpenGL screen coordinates (ie the upper left corner is not the same).
			   For Z, the mystery is what should it be mapped to ? Ie should the resulting range be between
			   -1.0 and 1.0 (as the X and Y coordinates) or between 0.0 and 1.0 ? */
			trans_mat[ 0] = 2.0 / width;  trans_mat[ 4] = 0.0;  trans_mat[ 8] = 0.0; trans_mat[12] = -1.0;
			trans_mat[ 1] = 0.0; trans_mat[ 5] = -2.0 / height; trans_mat[ 9] = 0.0; trans_mat[13] =  1.0;
			trans_mat[ 2] = 0.0; trans_mat[ 6] = 0.0; trans_mat[10] = 1.0;           trans_mat[14] = -1.0;
			trans_mat[ 3] = 0.0; trans_mat[ 7] = 0.0; trans_mat[11] = 0.0;           trans_mat[15] =  1.0;

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(trans_mat);

			/* Remove also fogging... */
			glDisable(GL_FOG);
		    } break;

		    default:
			ERR("Unhandled vertex type !\n");
			break;
		}

		switch (This->vertex_type) {
		    case D3DVT_VERTEX:
		        TRIANGLE_LOOP(DO_VERTEX);
			break;

		    case D3DVT_LVERTEX:
			TRIANGLE_LOOP(DO_LVERTEX);
			break;

		    case D3DVT_TLVERTEX:
			TRIANGLE_LOOP(DO_TLVERTEX);
			break;

		    default:
			ERR("Unhandled vertex type !\n");
		}
	    } break;

	    case D3DOP_MATRIXLOAD:
	        WARN("MATRIXLOAD-s     (%d)\n", count);
	        instr += count * size;
	        break;

	    case D3DOP_MATRIXMULTIPLY: {
	        int i;
		TRACE("MATRIXMULTIPLY   (%d)\n", count);
		
		for (i = 0; i < count; i++) {
		    LPD3DMATRIXMULTIPLY ci = (LPD3DMATRIXMULTIPLY) instr;
		    LPD3DMATRIX a = (LPD3DMATRIX) ci->hDestMatrix;
		    LPD3DMATRIX b = (LPD3DMATRIX) ci->hSrcMatrix1;
		    LPD3DMATRIX c = (LPD3DMATRIX) ci->hSrcMatrix2;
		    
		    TRACE("  Dest : %08lx  Src1 : %08lx  Src2 : %08lx\n",
			  ci->hDestMatrix, ci->hSrcMatrix1, ci->hSrcMatrix2);
		    
		    /* Do the multiplication..
		       As I am VERY lazy, I let OpenGL do the multiplication for me */
		    glMatrixMode(GL_PROJECTION);
		    /* Save the current matrix */
		    glPushMatrix();
		    /* Load Matrix one and do the multiplication */
		    glLoadMatrixf((float *) c);
		    glMultMatrixf((float *) b);
		    glGetFloatv(GL_PROJECTION_MATRIX, (float *) a);
		    /* Restore the current matrix */
		    glPopMatrix();
		    
		    instr += size;
		}
	    } break;

	    case D3DOP_STATETRANSFORM: {
	        int i;
		TRACE("STATETRANSFORM   (%d)\n", count);
		
		for (i = 0; i < count; i++) {
		    LPD3DSTATE ci = (LPD3DSTATE) instr;
		    
		    /* Handle the state transform */
		    switch (ci->u1.dtstTransformStateType) {
		        case D3DTRANSFORMSTATE_WORLD: {
			    TRACE("  WORLD (%p)\n", (D3DMATRIX*) ci->u2.dwArg[0]);
			    lpDevice->world_mat = (D3DMATRIX*) ci->u2.dwArg[0];
			} break;

			case D3DTRANSFORMSTATE_VIEW: {
			    TRACE("  VIEW (%p)\n", (D3DMATRIX*) ci->u2.dwArg[0]);
			    lpDevice->view_mat = (D3DMATRIX*) ci->u2.dwArg[0];
			} break;

			case D3DTRANSFORMSTATE_PROJECTION: {
			    TRACE("  PROJECTION (%p)\n", (D3DMATRIX*) ci->u2.dwArg[0]);
			    lpDevice->proj_mat = (D3DMATRIX*) ci->u2.dwArg[0];
			} break;
			  
			default:
			    ERR("  Unhandled state transformation !! (%d)\n", (int) ci->u1.dtstTransformStateType);
			    break;
		    }

		    instr += size;
		}
	    } break;

	    case D3DOP_STATELIGHT: {
	        int i;
		TRACE("STATELIGHT       (%d)\n", count);
		
		for (i = 0; i < count; i++) {
		    LPD3DSTATE ci = (LPD3DSTATE) instr;
		    
		    /* Handle the state transform */
		    switch (ci->u1.dlstLightStateType) {
		        case D3DLIGHTSTATE_MATERIAL: {
			    IDirect3DMaterialImpl* mat = (IDirect3DMaterialImpl*) ci->u2.dwArg[0];
			    TRACE("  MATERIAL\n");
			    
			    if (mat != NULL) {
			        mat->activate(mat);
			    } else {
			        TRACE("    bad Material Handle\n");
			    }
			} break ;

			case D3DLIGHTSTATE_AMBIENT: {
			    float light[4];
			    DWORD dwLightState = ci->u2.dwArg[0];
			    TRACE("  AMBIENT\n");
			    
			    light[0] = ((dwLightState >> 16) & 0xFF) / 255.0;
			    light[1] = ((dwLightState >>  8) & 0xFF) / 255.0;
			    light[2] = ((dwLightState >>  0) & 0xFF) / 255.0;
			    light[3] = 1.0;
			    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);
			    
			    TRACE("    R:%02lx G:%02lx B:%02lx A:%02lx\n",
				  ((dwLightState >> 16) & 0xFF),
				  ((dwLightState >>  8) & 0xFF),
				  ((dwLightState >>  0) & 0xFF),
				  ((dwLightState >> 24) & 0xFF));
			} break ;
			  
			case D3DLIGHTSTATE_COLORMODEL: {
			    WARN("  COLORMODEL\n");
			} break ;

			case D3DLIGHTSTATE_FOGMODE: {
			    WARN("  FOGMODE\n");
			} break ;

			case D3DLIGHTSTATE_FOGSTART: {
			    WARN("  FOGSTART\n");
			} break ;

			case D3DLIGHTSTATE_FOGEND: {
			    WARN("  FOGEND\n");
			} break ;

			case D3DLIGHTSTATE_FOGDENSITY: {
			    WARN("  FOGDENSITY\n");
			} break ;

			default:
			    ERR("  Unhandled light state !! (%d)\n", (int) ci->u1.dlstLightStateType);
			    break;
		    }
		    instr += size;
	        }
	    } break;

	    case D3DOP_STATERENDER: {
	        int i;
		TRACE("STATERENDER      (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DSTATE ci = (LPD3DSTATE) instr;
		    
		    /* Handle the state transform */
		    set_render_state(ci->u1.drstRenderStateType, ci->u2.dwArg[0], &(lpDeviceGL->render_state));

		    instr += size;
		}
	    } break;

	    case D3DOP_PROCESSVERTICES: {
	        int i;
		TRACE("PROCESSVERTICES  (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DPROCESSVERTICES ci = (LPD3DPROCESSVERTICES) instr;

		    TRACE("  Start : %d Dest : %d Count : %ld\n",
			  ci->wStart, ci->wDest, ci->dwCount);
		    TRACE("  Flags : ");
		    if (TRACE_ON(ddraw)) {
		        if (ci->dwFlags & D3DPROCESSVERTICES_COPY)
			    DPRINTF("COPY ");
			if (ci->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
			    DPRINTF("NOCOLOR ");
			if (ci->dwFlags == D3DPROCESSVERTICES_OPMASK)
			    DPRINTF("OPMASK ");
			if (ci->dwFlags & D3DPROCESSVERTICES_TRANSFORM)
			    DPRINTF("TRANSFORM ");
			if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT)
			    DPRINTF("TRANSFORMLIGHT ");
			if (ci->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
			    DPRINTF("UPDATEEXTENTS ");
			DPRINTF("\n");
		    }
		    
		    /* This is where doing Direct3D on top on OpenGL is quite difficult.
		       This method transforms a set of vertices using the CURRENT state
		       (lighting, projection, ...) but does not rasterize them.
		       They will only be put on screen later (with the POINT / LINE and
		       TRIANGLE op-codes). The problem is that you can have a triangle
		       with each point having been transformed using another state...
		       
		       In this implementation, I will emulate only ONE thing : each
		       vertex can have its own "WORLD" transformation (this is used in the
		       TWIST.EXE demo of the 5.2 SDK). I suppose that all vertices of the
		       execute buffer use the same state.
		       
		       If I find applications that change other states, I will try to do a
		       more 'fine-tuned' state emulation (but I may become quite tricky if
		       it changes a light position in the middle of a triangle).
		       
		       In this case, a 'direct' approach (i.e. without using OpenGL, but
		       writing our own 3D rasterizer) would be easier. */
		    
		    /* The current method (with the hypothesis that only the WORLD matrix
		       will change between two points) is like this :
		       - I transform 'manually' all the vertices with the current WORLD
		         matrix and store them in the vertex buffer
		       - during the rasterization phase, the WORLD matrix will be set to
		         the Identity matrix */
		    
		    /* Enough for the moment */
		    if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT) {
		        int nb;
			D3DVERTEX  *src = ((LPD3DVERTEX)  (This->desc.lpData + vs)) + ci->wStart;
			OGL_Vertex *dst = ((OGL_Vertex *) (This->vertex_data)) + ci->wDest;
			D3DMATRIX *mat = lpDevice->world_mat;
			
			TRACE("  World Matrix : (%p)\n", mat);
			dump_D3DMATRIX(mat);

			This->vertex_type = D3DVT_VERTEX;
			
			for (nb = 0; nb < ci->dwCount; nb++) {
			    /* For the moment, no normal transformation... */
			    dst->nx = (src->u4.nx * mat->_11) + (src->u5.ny * mat->_21) + (src->u6.nz * mat->_31);
			    dst->ny = (src->u4.nx * mat->_12) + (src->u5.ny * mat->_22) + (src->u6.nz * mat->_32);
			    dst->nz = (src->u4.nx * mat->_13) + (src->u5.ny * mat->_23) + (src->u6.nz * mat->_33);
			    
			    dst->u  = src->u7.tu;
			    dst->v  = src->u8.tv;
			    
			    /* Now, the matrix multiplication */
			    dst->x = (src->u1.x * mat->_11) + (src->u2.y * mat->_21) + (src->u3.z * mat->_31) + (1.0 * mat->_41);
			    dst->y = (src->u1.x * mat->_12) + (src->u2.y * mat->_22) + (src->u3.z * mat->_32) + (1.0 * mat->_42);
			    dst->z = (src->u1.x * mat->_13) + (src->u2.y * mat->_23) + (src->u3.z * mat->_33) + (1.0 * mat->_43);
			    dst->w = (src->u1.x * mat->_14) + (src->u2.y * mat->_24) + (src->u3.z * mat->_34) + (1.0 * mat->_44);
			    
			    src++;
			    dst++;
			}
		    } else if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORM) {
		        int nb;
			D3DLVERTEX *src  = ((LPD3DLVERTEX) (This->desc.lpData + vs)) + ci->wStart;
			OGL_LVertex *dst = ((OGL_LVertex *) (This->vertex_data)) + ci->wDest;
			D3DMATRIX *mat = lpDevice->world_mat;
			
			TRACE("  World Matrix : (%p)\n", mat);
			dump_D3DMATRIX(mat);
			
			This->vertex_type = D3DVT_LVERTEX;
			
			for (nb = 0; nb < ci->dwCount; nb++) {
			    dst->c  = src->u4.color;
			    dst->sc = src->u5.specular;
			    dst->u  = src->u6.tu;
			    dst->v  = src->u7.tv;
			    
			    /* Now, the matrix multiplication */
			    dst->x = (src->u1.x * mat->_11) + (src->u2.y * mat->_21) + (src->u3.z * mat->_31) + (1.0 * mat->_41);
			    dst->y = (src->u1.x * mat->_12) + (src->u2.y * mat->_22) + (src->u3.z * mat->_32) + (1.0 * mat->_42);
			    dst->z = (src->u1.x * mat->_13) + (src->u2.y * mat->_23) + (src->u3.z * mat->_33) + (1.0 * mat->_43);
			    dst->w = (src->u1.x * mat->_14) + (src->u2.y * mat->_24) + (src->u3.z * mat->_34) + (1.0 * mat->_44);
			    
			    src++;
			    dst++;
			}
		    } else if (ci->dwFlags == D3DPROCESSVERTICES_COPY) {
		        D3DTLVERTEX *src = ((LPD3DTLVERTEX) (This->desc.lpData + vs)) + ci->wStart;
			D3DTLVERTEX *dst = ((LPD3DTLVERTEX) (This->vertex_data)) + ci->wDest;
			
			This->vertex_type = D3DVT_TLVERTEX;
			
			memcpy(dst, src, ci->dwCount * sizeof(D3DTLVERTEX));
		    } else {
		        ERR("Unhandled vertex processing !\n");
		    }

		    instr += size;
		}
	    } break;

	    case D3DOP_TEXTURELOAD: {
	        WARN("TEXTURELOAD-s    (%d)\n", count);

		instr += count * size;
	    } break;

	    case D3DOP_EXIT: {
	        TRACE("EXIT             (%d)\n", count);
		/* We did this instruction */
		instr += size;
		/* Exit this loop */
		goto end_of_buffer;
	    } break;

	    case D3DOP_BRANCHFORWARD: {
	        int i;
		TRACE("BRANCHFORWARD    (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DBRANCH ci = (LPD3DBRANCH) instr;

		    if ((This->data.dsStatus.dwStatus & ci->dwMask) == ci->dwValue) {
		        if (!ci->bNegate) {
			    TRACE(" Should branch to %ld\n", ci->dwOffset);
			}
		    } else {
		        if (ci->bNegate) {
			    TRACE(" Should branch to %ld\n", ci->dwOffset);
			}
		    }

		    instr += size;
		}
	    } break;

	    case D3DOP_SPAN: {
	        WARN("SPAN-s           (%d)\n", count);

		instr += count * size;
	    } break;

	    case D3DOP_SETSTATUS: {
	        int i;
		TRACE("SETSTATUS        (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DSTATUS ci = (LPD3DSTATUS) instr;
		    
		    This->data.dsStatus = *ci;

		    instr += size;
		}
	    } break;

	    default:
	        ERR("Unhandled OpCode !!!\n");
	        /* Try to save ... */
	        instr += count * size;
	        break;
	}
    }

end_of_buffer:
    LEAVE_GL();
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_QueryInterface(LPDIRECT3DEXECUTEBUFFER iface,
                                                 REFIID riid,
                                                 LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DExecuteBuffer_AddRef(ICOM_INTERFACE(This, IDirect3DExecuteBuffer));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial, riid ) ) {
        IDirect3DExecuteBuffer_AddRef(ICOM_INTERFACE(This, IDirect3DExecuteBuffer));
        *obp = ICOM_INTERFACE(This, IDirect3DExecuteBuffer);
	TRACE("  Creating IDirect3DExecuteBuffer interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DExecuteBufferImpl_1_AddRef(LPDIRECT3DEXECUTEBUFFER iface)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    FIXME("(%p/%p)->()incrementing from %lu.\n", This, iface, This->ref );
    return ++(This->ref);
}

ULONG WINAPI
Main_IDirect3DExecuteBufferImpl_1_Release(LPDIRECT3DEXECUTEBUFFER iface)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->()decrementing from %lu.\n", This, iface, This->ref);
    if (!--(This->ref)) {
        if ((This->desc.lpData != NULL) && This->need_free)
	    HeapFree(GetProcessHeap(),0,This->desc.lpData);
	if (This->vertex_data != NULL)
	    HeapFree(GetProcessHeap(),0,This->vertex_data);
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }

    return This->ref;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Initialize(LPDIRECT3DEXECUTEBUFFER iface,
                                             LPDIRECT3DDEVICE lpDirect3DDevice,
                                             LPD3DEXECUTEBUFFERDESC lpDesc)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->(%p,%p) no-op....\n", This, iface, lpDirect3DDevice, lpDesc);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Lock(LPDIRECT3DEXECUTEBUFFER iface,
                                       LPD3DEXECUTEBUFFERDESC lpDesc)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDesc);

    dwSize = lpDesc->dwSize;
    memset(lpDesc, 0, dwSize);
    memcpy(lpDesc, &This->desc, dwSize);
    
    if (TRACE_ON(ddraw)) {
        TRACE("  Returning description : \n");
	_dump_D3DEXECUTEBUFFERDESC(lpDesc);
    }
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Unlock(LPDIRECT3DEXECUTEBUFFER iface)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->() no-op...\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_SetExecuteData(LPDIRECT3DEXECUTEBUFFER iface,
                                                 LPD3DEXECUTEDATA lpData)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    DWORD nbvert;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    memcpy(&This->data, lpData, lpData->dwSize);

    /* Get the number of vertices in the execute buffer */
    nbvert = This->data.dwVertexCount;
    
    /* Prepares the transformed vertex buffer */
    if (This->vertex_data != NULL)
        HeapFree(GetProcessHeap(), 0, This->vertex_data);
    This->vertex_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nbvert * sizeof(OGL_Vertex));

    if (TRACE_ON(ddraw)) {
        _dump_executedata(lpData);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_GetExecuteData(LPDIRECT3DEXECUTEBUFFER iface,
                                                 LPD3DEXECUTEDATA lpData)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p): stub!\n", This, iface, lpData);

    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    memcpy(lpData, &This->data, dwSize);

    if (TRACE_ON(ddraw)) {
        TRACE("Returning data : \n");
	_dump_executedata(lpData);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Validate(LPDIRECT3DEXECUTEBUFFER iface,
                                           LPDWORD lpdwOffset,
                                           LPD3DVALIDATECALLBACK lpFunc,
                                           LPVOID lpUserArg,
                                           DWORD dwReserved)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    FIXME("(%p/%p)->(%p,%p,%p,%08lx): stub!\n", This, iface, lpdwOffset, lpFunc, lpUserArg, dwReserved);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Optimize(LPDIRECT3DEXECUTEBUFFER iface,
                                           DWORD dwDummy)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->(%08lx) no-op...\n", This, iface, dwDummy);
    return DD_OK;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DExecuteBuffer.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DExecuteBuffer) VTABLE_IDirect3DExecuteBuffer =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Main_IDirect3DExecuteBufferImpl_1_QueryInterface,
    XCAST(AddRef) Main_IDirect3DExecuteBufferImpl_1_AddRef,
    XCAST(Release) Main_IDirect3DExecuteBufferImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DExecuteBufferImpl_1_Initialize,
    XCAST(Lock) Main_IDirect3DExecuteBufferImpl_1_Lock,
    XCAST(Unlock) Main_IDirect3DExecuteBufferImpl_1_Unlock,
    XCAST(SetExecuteData) Main_IDirect3DExecuteBufferImpl_1_SetExecuteData,
    XCAST(GetExecuteData) Main_IDirect3DExecuteBufferImpl_1_GetExecuteData,
    XCAST(Validate) Main_IDirect3DExecuteBufferImpl_1_Validate,
    XCAST(Optimize) Main_IDirect3DExecuteBufferImpl_1_Optimize,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


HRESULT d3dexecutebuffer_create(IDirect3DExecuteBufferImpl **obj, IDirect3DImpl *d3d, IDirect3DDeviceImpl *d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc)
{
    IDirect3DExecuteBufferImpl* object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DExecuteBufferImpl));

    ICOM_INIT_INTERFACE(object, IDirect3DExecuteBuffer, VTABLE_IDirect3DExecuteBuffer);
    
    object->ref = 1;
    object->d3d = d3d;
    object->d3ddev = d3ddev;

    /* Initializes memory */
    memcpy(&object->desc, lpDesc, lpDesc->dwSize);

    /* No buffer given */
    if ((object->desc.dwFlags & D3DDEB_LPDATA) == 0)
        object->desc.lpData = NULL;

    /* No buffer size given */
    if ((lpDesc->dwFlags & D3DDEB_BUFSIZE) == 0)
        object->desc.dwBufferSize = 0;

    /* Create buffer if asked */
    if ((object->desc.lpData == NULL) && (object->desc.dwBufferSize > 0)) {
        object->need_free = TRUE;
	object->desc.lpData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,object->desc.dwBufferSize);
    } else {
        object->need_free = FALSE;
    }

    /* No vertices for the moment */
    object->vertex_data = NULL;

    object->desc.dwFlags |= D3DDEB_LPDATA;

    object->execute = execute;

    *obj = object;
    
    TRACE(" creating implementation at %p.\n", *obj);
    
    return DD_OK;
}
