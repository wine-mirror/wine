/* Direct3D ExecuteBuffer
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DExecuteBuffer. */


#include <string.h>
#include "config.h"
#include "wintypes.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"

#include "d3d_private.h"

#ifdef HAVE_MESAGL

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

static IDirect3DExecuteBuffer_VTable executebuffer_vtable;

/*******************************************************************************
 *				ExecuteBuffer static functions
 */
void _dump_d3dstatus(LPD3DSTATUS lpStatus) {
  
}

void _dump_executedata(LPD3DEXECUTEDATA lpData) {
  DUMP("dwSize : %ld\n", lpData->dwSize);
  DUMP("Vertex      Offset : %ld  Count  : %ld\n", lpData->dwVertexOffset, lpData->dwVertexCount);
  DUMP("Instruction Offset : %ld  Length : %ld\n", lpData->dwInstructionOffset, lpData->dwInstructionLength);
  DUMP("HVertex     Offset : %ld\n", lpData->dwHVertexOffset);
  _dump_d3dstatus(&(lpData->dsStatus));
}

#define DO_VERTEX(index) 			\
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
  TRACE(ddraw, "   V: %f %f %f %f (%f %f %f) (%f %f)\n",			\
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
	       l_vx[index].v);							\
  glVertex4f(l_vx[index].x,							\
	     l_vx[index].y,							\
	     l_vx[index].z,							\
	     l_vx[index].w);							\
  										\
  TRACE(ddraw, "  LV: %f %f %f %f (%02lx %02lx %02lx) (%f %f)\n",		\
	l_vx[index].x, l_vx[index].y, l_vx[index].z, l_vx[index].w,		\
	((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),	\
	l_vx[index].u, l_vx[index].v);						\
}

#define DO_TLVERTEX(index)							\
{										\
  D3DTLVERTEX *vx = &(tl_vx[index]);						\
  DWORD col = vx->c.color;							\
										\
  glColor3f(((col >> 16) & 0xFF) / 255.0,					\
            ((col >>  8) & 0xFF) / 255.0,					\
            ((col >>  0) & 0xFF) / 255.0);					\
  glTexCoord2f(vx->u.tu, vx->v.tv);						\
  if (vx->r.rhw < 0.01)								\
    glVertex3f(vx->x.sx,							\
               vx->y.sy,							\
               vx->z.sz);							\
  else										\
    glVertex4f(vx->x.sx / vx->r.rhw,						\
               vx->y.sy / vx->r.rhw,						\
               vx->z.sz / vx->r.rhw,						\
               1.0 / vx->r.rhw);						\
  TRACE(ddraw, " TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",		\
        vx->x.sx, vx->y.sy, vx->z.sz,						\
        ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),	\
        vx->u.tu, vx->v.tv, vx->r.rhw);						\
}

#define TRIANGLE_LOOP(macro)				\
{							\
  glBegin(GL_TRIANGLES); {				\
    for (i = 0; i < count; i++) {			\
      LPD3DTRIANGLE ci = (LPD3DTRIANGLE) instr;		\
      							\
      TRACE(ddraw, "  v1: %d  v2: %d  v3: %d\n",	\
	    ci->v1.v1, ci->v2.v2, ci->v3.v3);		\
      TRACE(ddraw, "  Flags : ");			\
      if (TRACE_ON(ddraw)) {				\
	/* Wireframe */					\
	if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)	\
	  DUMP("EDGEENABLE1 ");				\
	if (ci->wFlags & D3DTRIFLAG_EDGEENABLE2)	\
	  DUMP("EDGEENABLE2 ");				\
	if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)	\
	  DUMP("EDGEENABLE3 ");				\
							\
	/* Strips / Fans */				\
	if (ci->wFlags == D3DTRIFLAG_EVEN)		\
	  DUMP("EVEN ");				\
	if (ci->wFlags == D3DTRIFLAG_ODD)		\
	  DUMP("ODD ");					\
	if (ci->wFlags == D3DTRIFLAG_START)		\
	  DUMP("START ");				\
	if ((ci->wFlags > 0) && (ci->wFlags < 30))	\
	  DUMP("STARTFLAT(%d) ", ci->wFlags);		\
	DUMP("\n");					\
      }							\
							\
      /* Draw the triangle */				\
      macro(ci->v1.v1);					\
      macro(ci->v2.v2);					\
      macro(ci->v3.v3);					\
							\
      instr += size;					\
    }							\
  } glEnd();						\
}


static void execute(LPDIRECT3DEXECUTEBUFFER lpBuff,
		    LPDIRECT3DDEVICE dev,
		    LPDIRECT3DVIEWPORT vp) {
  /* DWORD bs = lpBuff->desc.dwBufferSize; */
  DWORD vs = lpBuff->data.dwVertexOffset;
  /* DWORD vc = lpBuff->data.dwVertexCount; */
  DWORD is = lpBuff->data.dwInstructionOffset;
  /* DWORD il = lpBuff->data.dwInstructionLength; */
  
  void *instr = lpBuff->desc.lpData + is;
  OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) dev;
  
  TRACE(ddraw, "ExecuteData : \n");
  if (TRACE_ON(ddraw))
  _dump_executedata(&(lpBuff->data));
  
  while (1) {
    LPD3DINSTRUCTION current = (LPD3DINSTRUCTION) instr;
    BYTE size;
    WORD count;

    count = current->wCount;
    size = current->bSize;
    instr += sizeof(D3DINSTRUCTION);
    
    switch (current->bOpcode) {
    case D3DOP_POINT: {
      TRACE(ddraw, "POINT-s          (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_LINE: {
      TRACE(ddraw, "LINE-s           (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_TRIANGLE: {
      int i;
      float z_inv_matrix[16] = {
	1.0, 0.0,  0.0, 0.0,
	0.0, 1.0,  0.0, 0.0,
	0.0, 0.0, -1.0, 0.0,
	0.0, 0.0,  1.0, 1.0
      };
      
      OGL_Vertex  *vx    = (OGL_Vertex  *) lpBuff->vertex_data;
      OGL_LVertex *l_vx  = (OGL_LVertex *) lpBuff->vertex_data;
      D3DTLVERTEX *tl_vx = (D3DTLVERTEX *) lpBuff->vertex_data;
      
      TRACE(ddraw, "TRIANGLE         (%d)\n", count);

      switch (lpBuff->vertex_type) {
      case D3DVT_VERTEX:
	/* This time, there is lighting */
	glEnable(GL_LIGHTING);
	
      /* Use given matrixes */
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity(); /* The model transformation was done during the
			   transformation phase */
      glMatrixMode(GL_PROJECTION);
	TRACE(ddraw, "  Projection Matrix : (%p)\n", odev->proj_mat);
	dump_mat(odev->proj_mat);
	TRACE(ddraw, "  View       Matrix : (%p)\n", odev->view_mat);
	dump_mat(odev->view_mat);

	glLoadMatrixf((float *) z_inv_matrix);
	glMultMatrixf((float *) odev->proj_mat);
      glMultMatrixf((float *) odev->view_mat);
	break;

      case D3DVT_LVERTEX:
	/* No lighting */
	glDisable(GL_LIGHTING);

	/* Use given matrixes */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); /* The model transformation was done during the
			     transformation phase */
	glMatrixMode(GL_PROJECTION);

	TRACE(ddraw, "  Projection Matrix : (%p)\n", odev->proj_mat);
	dump_mat(odev->proj_mat);
	TRACE(ddraw, "  View       Matrix : (%p)\n", odev->view_mat);
	dump_mat(odev->view_mat);

	glLoadMatrixf((float *) z_inv_matrix);
	glMultMatrixf((float *) odev->proj_mat);
	glMultMatrixf((float *) odev->view_mat);
	break;

      case D3DVT_TLVERTEX: {
	GLdouble height, width, minZ, maxZ;
        
        /* First, disable lighting */
        glDisable(GL_LIGHTING);
        
        /* Then do not put any transformation matrixes */
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        if (vp == NULL) {
          ERR(ddraw, "No current viewport !\n");
          /* Using standard values */
          height = 640.0;
          width = 480.0;
          minZ = -10.0;
          maxZ = 10.0;
        } else {
          height = (GLdouble) vp->viewport.vp1.dwHeight;
          width  = (GLdouble) vp->viewport.vp1.dwWidth;
          minZ   = (GLdouble) vp->viewport.vp1.dvMinZ;
          maxZ   = (GLdouble) vp->viewport.vp1.dvMaxZ;

	  if (minZ == maxZ) {
	    /* I do not know why, but many Dx 3.0 games have minZ = maxZ = 0.0 */
	    minZ = 0.0;
	    maxZ = 1.0;
	}
        }

        glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);
      } break;
	
      default:
	ERR(ddraw, "Unhandled vertex type !\n");
	break;
      }

      switch (lpBuff->vertex_type) {
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
	ERR(ddraw, "Unhandled vertex type !\n");
      }
      
    } break;
    
    case D3DOP_MATRIXLOAD: {
      TRACE(ddraw, "MATRIXLOAD-s     (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_MATRIXMULTIPLY: {
      int i;
      TRACE(ddraw, "MATRIXMULTIPLY   (%d)\n", count);

      for (i = 0; i < count; i++) {
	LPD3DMATRIXMULTIPLY ci = (LPD3DMATRIXMULTIPLY) instr;
	LPD3DMATRIX a = (LPD3DMATRIX) ci->hDestMatrix;
	LPD3DMATRIX b = (LPD3DMATRIX) ci->hSrcMatrix1;
	LPD3DMATRIX c = (LPD3DMATRIX) ci->hSrcMatrix2;
	
	TRACE(ddraw, "  Dest : %08lx  Src1 : %08lx  Src2 : %08lx\n",
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
      TRACE(ddraw, "STATETRANSFORM   (%d)\n", count);

      for (i = 0; i < count; i++) {
	LPD3DSTATE ci = (LPD3DSTATE) instr;

	/* Handle the state transform */
	switch (ci->t.dtstTransformStateType) {
	case D3DTRANSFORMSTATE_WORLD: {
	  TRACE(ddraw, "  WORLD (%p)\n", (D3DMATRIX*) ci->v.dwArg[0]);
	  odev->world_mat = (D3DMATRIX*) ci->v.dwArg[0];
	} break;

	case D3DTRANSFORMSTATE_VIEW: {
	  TRACE(ddraw, "  VIEW (%p)\n", (D3DMATRIX*) ci->v.dwArg[0]);
	  odev->view_mat = (D3DMATRIX*) ci->v.dwArg[0];
	} break;

	case D3DTRANSFORMSTATE_PROJECTION: {
    	  TRACE(ddraw, "  PROJECTION (%p)\n", (D3DMATRIX*) ci->v.dwArg[0]);
	  odev->proj_mat = (D3DMATRIX*) ci->v.dwArg[0];
	} break;

	default:
	  ERR(ddraw, "  Unhandled state transformation !! (%d)\n", (int) ci->t.dtstTransformStateType);
	  break;
	  
	}
	
	instr += size;
      }
    } break;
      
    case D3DOP_STATELIGHT: {
      int i;
      TRACE(ddraw, "STATELIGHT       (%d)\n", count);

      for (i = 0; i < count; i++) {
	LPD3DSTATE ci = (LPD3DSTATE) instr;
	
	/* Handle the state transform */
	switch (ci->t.dlstLightStateType) {
	case D3DLIGHTSTATE_MATERIAL: {
	  LPDIRECT3DMATERIAL mat = (LPDIRECT3DMATERIAL) ci->v.dwArg[0];
	  TRACE(ddraw, "  MATERIAL\n");
	  
	  if (mat != NULL) {
	    mat->activate(mat);
	  } else {
	    TRACE(ddraw, "    bad Material Handle\n");
	  }
	} break ;
	  
	case D3DLIGHTSTATE_AMBIENT: {
	  float light[4];
	  DWORD dwLightState = ci->v.dwArg[0];
	  TRACE(ddraw, "  AMBIENT\n");
	  
	  light[0] = ((dwLightState >> 16) & 0xFF) / 255.0;
	  light[1] = ((dwLightState >>  8) & 0xFF) / 255.0;
	  light[2] = ((dwLightState >>  0) & 0xFF) / 255.0;
	  light[3] = 1.0;
	  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);

	  TRACE(ddraw, "    R:%02lx G:%02lx B:%02lx A:%02lx\n",
		((dwLightState >> 16) & 0xFF),
		((dwLightState >>  8) & 0xFF),
		((dwLightState >>  0) & 0xFF),
		((dwLightState >> 24) & 0xFF));
	} break ;
	  
	case D3DLIGHTSTATE_COLORMODEL: {
	  TRACE(ddraw, "  COLORMODEL\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGMODE: {
	  TRACE(ddraw, "  FOGMODE\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGSTART: {
	  TRACE(ddraw, "  FOGSTART\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGEND: {
	  TRACE(ddraw, "  FOGEND\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGDENSITY: {
	  TRACE(ddraw, "  FOGDENSITY\n");
	} break ;
	  
	default:
	  ERR(ddraw, "  Unhandled light state !! (%d)\n", (int) ci->t.dlstLightStateType);
	  break;
	}
	instr += size;
      }
    } break;
      
    case D3DOP_STATERENDER: {
      int i;
      TRACE(ddraw, "STATERENDER      (%d)\n", count);
      
      for (i = 0; i < count; i++) {
	LPD3DSTATE ci = (LPD3DSTATE) instr;
	
	/* Handle the state transform */
	set_render_state(ci->t.drstRenderStateType, ci->v.dwArg[0], &(odev->rs));

	instr += size;
      }
    } break;
      
    case D3DOP_PROCESSVERTICES: {
      int i;
      TRACE(ddraw, "PROCESSVERTICES  (%d)\n", count);
      
      for (i = 0; i < count; i++) {
	LPD3DPROCESSVERTICES ci = (LPD3DPROCESSVERTICES) instr;
	
	TRACE(ddraw, "  Start : %d Dest : %d Count : %ld\n",
	      ci->wStart, ci->wDest, ci->dwCount);
	TRACE(ddraw, "  Flags : ");
	if (TRACE_ON(ddraw)) {
	  if (ci->dwFlags & D3DPROCESSVERTICES_COPY)
	    DUMP("COPY ");
	  if (ci->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
	    DUMP("NOCOLOR ");
	  if (ci->dwFlags == D3DPROCESSVERTICES_OPMASK)
	    DUMP("OPMASK ");
	  if (ci->dwFlags & D3DPROCESSVERTICES_TRANSFORM)
	    DUMP("TRANSFORM ");
	  if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT)
	    DUMP("TRANSFORMLIGHT ");
	  if (ci->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
	    DUMP("UPDATEEXTENTS ");
	  DUMP("\n");
	}

	/* This is where doing Direct3D on top on OpenGL is quite difficult.
	   This method transforms a set of vertices using the CURRENT state
	   (lighting, projection, ...) but does not rasterize them.
	   They will oinly be put on screen later (with the POINT / LINE and
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
	  D3DVERTEX  *src = ((LPD3DVERTEX)  (lpBuff->desc.lpData + vs)) + ci->wStart;
	  OGL_Vertex *dst = ((OGL_Vertex *) (lpBuff->vertex_data)) + ci->wDest;
	  D3DMATRIX *mat = odev->world_mat;

	  TRACE(ddraw, "  World Matrix : (%p)\n", mat);
	  dump_mat(mat);

	  lpBuff->vertex_type = D3DVT_VERTEX;
	  
	  for (nb = 0; nb < ci->dwCount; nb++) {
	    /* For the moment, no normal transformation... */
	    dst->nx = src->nx.nx;
	    dst->ny = src->ny.ny;
	    dst->nz = src->nz.nz;
	    
	    dst->u  = src->u.tu;
	    dst->v  = src->v.tv;

	    /* Now, the matrix multiplication */
	    dst->x = (src->x.x * mat->_11) + (src->y.y * mat->_21) + (src->z.z * mat->_31) + (1.0 * mat->_41);
	    dst->y = (src->x.x * mat->_12) + (src->y.y * mat->_22) + (src->z.z * mat->_32) + (1.0 * mat->_42);
	    dst->z = (src->x.x * mat->_13) + (src->y.y * mat->_23) + (src->z.z * mat->_33) + (1.0 * mat->_43);
	    dst->w = (src->x.x * mat->_14) + (src->y.y * mat->_24) + (src->z.z * mat->_34) + (1.0 * mat->_44);
	    
	    src++;
	    dst++;
	  }
	} else if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORM) {
	  int nb;
	  D3DLVERTEX *src  = ((LPD3DLVERTEX) (lpBuff->desc.lpData + vs)) + ci->wStart;
	  OGL_LVertex *dst = ((OGL_LVertex *) (lpBuff->vertex_data)) + ci->wDest;
	  D3DMATRIX *mat = odev->world_mat;

	  TRACE(ddraw, "  World Matrix : (%p)\n", mat);
	  dump_mat(mat);

	  lpBuff->vertex_type = D3DVT_LVERTEX;
	  
	  for (nb = 0; nb < ci->dwCount; nb++) {
	    dst->c  = src->c.color;
	    dst->sc = src->s.specular;
	    dst->u  = src->u.tu;
	    dst->v  = src->v.tv;

	    /* Now, the matrix multiplication */
	    dst->x = (src->x.x * mat->_11) + (src->y.y * mat->_21) + (src->z.z * mat->_31) + (1.0 * mat->_41);
	    dst->y = (src->x.x * mat->_12) + (src->y.y * mat->_22) + (src->z.z * mat->_32) + (1.0 * mat->_42);
	    dst->z = (src->x.x * mat->_13) + (src->y.y * mat->_23) + (src->z.z * mat->_33) + (1.0 * mat->_43);
	    dst->w = (src->x.x * mat->_14) + (src->y.y * mat->_24) + (src->z.z * mat->_34) + (1.0 * mat->_44);
	    
	    src++;
	    dst++;
	  }
	} else if (ci->dwFlags == D3DPROCESSVERTICES_COPY) {
	  D3DTLVERTEX *src = ((LPD3DTLVERTEX) (lpBuff->desc.lpData + vs)) + ci->wStart;
	  D3DTLVERTEX *dst = ((LPD3DTLVERTEX) (lpBuff->vertex_data)) + ci->wDest;

	  lpBuff->vertex_type = D3DVT_TLVERTEX;
	  
	  memcpy(dst, src, ci->dwCount * sizeof(D3DTLVERTEX));
	} else {
	  ERR(ddraw, "Unhandled vertex processing !\n");
	}
	
	instr += size;
      }
    } break;
      
    case D3DOP_TEXTURELOAD: {
      TRACE(ddraw, "TEXTURELOAD-s    (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_EXIT: {
      TRACE(ddraw, "EXIT             (%d)\n", count);
      /* We did this instruction */
      instr += size;
      /* Exit this loop */
      goto end_of_buffer;
    } break;
      
    case D3DOP_BRANCHFORWARD: {
      int i;
      TRACE(ddraw, "BRANCHFORWARD    (%d)\n", count);

      for (i = 0; i < count; i++) {
	LPD3DBRANCH ci = (LPD3DBRANCH) instr;
	
	if ((lpBuff->data.dsStatus.dwStatus & ci->dwMask) == ci->dwValue) {
	  if (!ci->bNegate) {
	    TRACE(ddraw," Should branch to %ld\n", ci->dwOffset);
	  }
	} else {
	  if (ci->bNegate) {
	    TRACE(ddraw," Should branch to %ld\n", ci->dwOffset);
	  }
	}

	instr += size;
      }
    } break;
      
    case D3DOP_SPAN: {
      TRACE(ddraw, "SPAN-s           (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_SETSTATUS: {
      int i;
      TRACE(ddraw, "SETSTATUS        (%d)\n", count);

      for (i = 0; i < count; i++) {
	LPD3DSTATUS ci = (LPD3DSTATUS) instr;
	
	lpBuff->data.dsStatus = *ci;
	
	instr += size;
      }
    } break;

    default:
      ERR(ddraw, "Unhandled OpCode !!!\n");
      /* Try to save ... */
      instr += count * size;
      break;
    }
  }

 end_of_buffer:
}

/*******************************************************************************
 *				ExecuteBuffer Creation functions
 */
LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(LPDIRECT3DDEVICE d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc)
{
  LPDIRECT3DEXECUTEBUFFER eb;
  
  eb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DExecuteBuffer));
  eb->ref = 1;
  eb->lpvtbl = &executebuffer_vtable;
  eb->d3ddev = d3ddev;

  /* Initializes memory */
  eb->desc = *lpDesc;

  /* No buffer given */
  if (!(eb->desc.dwFlags & D3DDEB_LPDATA))
    eb->desc.lpData = NULL;

  /* No buffer size given */
  if (!(lpDesc->dwFlags & D3DDEB_BUFSIZE))
    eb->desc.dwBufferSize = 0;

  /* Create buffer if asked */
  if ((eb->desc.lpData == NULL) && (eb->desc.dwBufferSize > 0)) {
    eb->need_free = TRUE;
    eb->desc.lpData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,eb->desc.dwBufferSize);
  } else {
    eb->need_free = FALSE;
  }
    
  /* No vertices for the moment */
  eb->vertex_data = NULL;

  eb->desc.dwFlags |= D3DDEB_LPDATA;

  eb->execute = execute;
  
  return eb;
}

/*******************************************************************************
 *				IDirect3DLight methods
 */

static HRESULT WINAPI IDirect3DExecuteBuffer_QueryInterface(LPDIRECT3DEXECUTEBUFFER this,
							    REFIID riid,
							    LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DExecuteBuffer_AddRef(LPDIRECT3DEXECUTEBUFFER this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DExecuteBuffer_Release(LPDIRECT3DEXECUTEBUFFER this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    if ((this->desc.lpData != NULL) && this->need_free)
      HeapFree(GetProcessHeap(),0,this->desc.lpData);

    if (this->vertex_data != NULL)
      HeapFree(GetProcessHeap(),0,this->vertex_data);

    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_Initialize(LPDIRECT3DEXECUTEBUFFER this,
							LPDIRECT3DDEVICE lpDirect3DDevice,
							LPD3DEXECUTEBUFFERDESC lpDesc)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lpDirect3DDevice, lpDesc);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_Lock(LPDIRECT3DEXECUTEBUFFER this,
						  LPD3DEXECUTEBUFFERDESC lpDesc)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpDesc);

  /* Copies the buffer description */
  *lpDesc = this->desc;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_Unlock(LPDIRECT3DEXECUTEBUFFER this)
{
  TRACE(ddraw, "(%p)->()\n", this);

  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_SetExecuteData(LPDIRECT3DEXECUTEBUFFER this,
							    LPD3DEXECUTEDATA lpData)
{
  DWORD nbvert;

  TRACE(ddraw, "(%p)->(%p)\n", this, lpData);

  this->data = *lpData;

  /* Get the number of vertices in the execute buffer */
  nbvert = this->data.dwVertexCount;
    
  /* Prepares the transformed vertex buffer */
  if (this->vertex_data != NULL)
    HeapFree(GetProcessHeap(), 0, this->vertex_data);
  this->vertex_data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,nbvert * sizeof(OGL_Vertex));


  if (TRACE_ON(ddraw)) {
    _dump_executedata(lpData);
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_GetExecuteData(LPDIRECT3DEXECUTEBUFFER this,
							    LPD3DEXECUTEDATA lpData)
{
  TRACE(ddraw, "(%p)->(%p): stub\n", this, lpData);

  *lpData = this->data;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_Validate(LPDIRECT3DEXECUTEBUFFER this,
						      LPDWORD lpdwOffset,
						      LPD3DVALIDATECALLBACK lpFunc,
						      LPVOID lpUserArg,
						      DWORD dwReserved)
{
  TRACE(ddraw, "(%p)->(%p,%p,%p,%lu)\n", this, lpdwOffset, lpFunc, lpUserArg, dwReserved);

  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBuffer_Optimize(LPDIRECT3DEXECUTEBUFFER this,
						      DWORD dwReserved)
{
  TRACE(ddraw, "(%p)->(%lu)\n", this, dwReserved);

  return DD_OK;
}


/*******************************************************************************
 *				IDirect3DLight VTable
 */
static IDirect3DExecuteBuffer_VTable executebuffer_vtable = {
  /*** IUnknown methods ***/
  IDirect3DExecuteBuffer_QueryInterface,
  IDirect3DExecuteBuffer_AddRef,
  IDirect3DExecuteBuffer_Release,
  /*** IDirect3DExecuteBuffer methods ***/
  IDirect3DExecuteBuffer_Initialize,
  IDirect3DExecuteBuffer_Lock,
  IDirect3DExecuteBuffer_Unlock,
  IDirect3DExecuteBuffer_SetExecuteData,
  IDirect3DExecuteBuffer_GetExecuteData,
  IDirect3DExecuteBuffer_Validate,
  IDirect3DExecuteBuffer_Optimize
};

#endif /* HAVE_MESAGL */
