/* Direct3D ExecuteBuffer
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DExecuteBuffer. */


#include <string.h>
#include "config.h"
#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"

#include "d3d_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw)

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

static ICOM_VTABLE(IDirect3DExecuteBuffer) executebuffer_vtable;

/*******************************************************************************
 *				ExecuteBuffer static functions
 */
void _dump_d3dstatus(LPD3DSTATUS lpStatus) {
  
}

void _dump_executedata(LPD3DEXECUTEDATA lpData) {
  DPRINTF("dwSize : %ld\n", lpData->dwSize);
  DPRINTF("Vertex      Offset : %ld  Count  : %ld\n", lpData->dwVertexOffset, lpData->dwVertexCount);
  DPRINTF("Instruction Offset : %ld  Length : %ld\n", lpData->dwInstructionOffset, lpData->dwInstructionLength);
  DPRINTF("HVertex     Offset : %ld\n", lpData->dwHVertexOffset);
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
  TRACE("   V: %f %f %f %f (%f %f %f) (%f %f)\n",			\
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
  TRACE("  LV: %f %f %f %f (%02lx %02lx %02lx) (%f %f)\n",		\
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
  TRACE(" TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",		\
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
      TRACE("  v1: %d  v2: %d  v3: %d\n",	\
	    ci->v1.v1, ci->v2.v2, ci->v3.v3);		\
      TRACE("  Flags : ");			\
      if (TRACE_ON(ddraw)) {				\
	/* Wireframe */					\
	if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)	\
	  DPRINTF("EDGEENABLE1 ");				\
	if (ci->wFlags & D3DTRIFLAG_EDGEENABLE2)	\
	  DPRINTF("EDGEENABLE2 ");				\
	if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)	\
	  DPRINTF("EDGEENABLE3 ");				\
							\
	/* Strips / Fans */				\
	if (ci->wFlags == D3DTRIFLAG_EVEN)		\
	  DPRINTF("EVEN ");				\
	if (ci->wFlags == D3DTRIFLAG_ODD)		\
	  DPRINTF("ODD ");					\
	if (ci->wFlags == D3DTRIFLAG_START)		\
	  DPRINTF("START ");				\
	if ((ci->wFlags > 0) && (ci->wFlags < 30))	\
	  DPRINTF("STARTFLAT(%d) ", ci->wFlags);		\
	DPRINTF("\n");					\
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
  IDirect3DExecuteBufferImpl* ilpBuff=(IDirect3DExecuteBufferImpl*)lpBuff;
  IDirect3DViewport2Impl* ivp=(IDirect3DViewport2Impl*)vp;
  /* DWORD bs = ilpBuff->desc.dwBufferSize; */
  DWORD vs = ilpBuff->data.dwVertexOffset;
  /* DWORD vc = ilpBuff->data.dwVertexCount; */
  DWORD is = ilpBuff->data.dwInstructionOffset;
  /* DWORD il = ilpBuff->data.dwInstructionLength; */
  
  void *instr = ilpBuff->desc.lpData + is;
  OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) dev;
  
  TRACE("ExecuteData : \n");
  if (TRACE_ON(ddraw))
  _dump_executedata(&(ilpBuff->data));
  
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
      TRACE("POINT-s          (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_LINE: {
      TRACE("LINE-s           (%d)\n", count);

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
      
      OGL_Vertex  *vx    = (OGL_Vertex  *) ilpBuff->vertex_data;
      OGL_LVertex *l_vx  = (OGL_LVertex *) ilpBuff->vertex_data;
      D3DTLVERTEX *tl_vx = (D3DTLVERTEX *) ilpBuff->vertex_data;
      
      TRACE("TRIANGLE         (%d)\n", count);

      switch (ilpBuff->vertex_type) {
      case D3DVT_VERTEX:
	/* This time, there is lighting */
	glEnable(GL_LIGHTING);
	
      /* Use given matrixes */
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity(); /* The model transformation was done during the
			   transformation phase */
      glMatrixMode(GL_PROJECTION);
	TRACE("  Projection Matrix : (%p)\n", odev->proj_mat);
	dump_mat(odev->proj_mat);
	TRACE("  View       Matrix : (%p)\n", odev->view_mat);
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

	TRACE("  Projection Matrix : (%p)\n", odev->proj_mat);
	dump_mat(odev->proj_mat);
	TRACE("  View       Matrix : (%p)\n", odev->view_mat);
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
        
        if (ivp == NULL) {
          ERR("No current viewport !\n");
          /* Using standard values */
          height = 640.0;
          width = 480.0;
          minZ = -10.0;
          maxZ = 10.0;
        } else {
          height = (GLdouble) ivp->viewport.vp1.dwHeight;
          width  = (GLdouble) ivp->viewport.vp1.dwWidth;
          minZ   = (GLdouble) ivp->viewport.vp1.dvMinZ;
          maxZ   = (GLdouble) ivp->viewport.vp1.dvMaxZ;

	  if (minZ == maxZ) {
	    /* I do not know why, but many Dx 3.0 games have minZ = maxZ = 0.0 */
	    minZ = 0.0;
	    maxZ = 1.0;
	}
        }

        glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);
      } break;
	
      default:
	ERR("Unhandled vertex type !\n");
	break;
      }

      switch (ilpBuff->vertex_type) {
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
    
    case D3DOP_MATRIXLOAD: {
      TRACE("MATRIXLOAD-s     (%d)\n", count);

      instr += count * size;
    } break;
      
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
	switch (ci->t.dtstTransformStateType) {
	case D3DTRANSFORMSTATE_WORLD: {
	  TRACE("  WORLD (%p)\n", (D3DMATRIX*) ci->v.dwArg[0]);
	  odev->world_mat = (D3DMATRIX*) ci->v.dwArg[0];
	} break;

	case D3DTRANSFORMSTATE_VIEW: {
	  TRACE("  VIEW (%p)\n", (D3DMATRIX*) ci->v.dwArg[0]);
	  odev->view_mat = (D3DMATRIX*) ci->v.dwArg[0];
	} break;

	case D3DTRANSFORMSTATE_PROJECTION: {
    	  TRACE("  PROJECTION (%p)\n", (D3DMATRIX*) ci->v.dwArg[0]);
	  odev->proj_mat = (D3DMATRIX*) ci->v.dwArg[0];
	} break;

	default:
	  ERR("  Unhandled state transformation !! (%d)\n", (int) ci->t.dtstTransformStateType);
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
	switch (ci->t.dlstLightStateType) {
	case D3DLIGHTSTATE_MATERIAL: {
	  IDirect3DMaterial2Impl* mat = (IDirect3DMaterial2Impl*) ci->v.dwArg[0];
	  TRACE("  MATERIAL\n");
	  
	  if (mat != NULL) {
	    mat->activate(mat);
	  } else {
	    TRACE("    bad Material Handle\n");
	  }
	} break ;
	  
	case D3DLIGHTSTATE_AMBIENT: {
	  float light[4];
	  DWORD dwLightState = ci->v.dwArg[0];
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
	  TRACE("  COLORMODEL\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGMODE: {
	  TRACE("  FOGMODE\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGSTART: {
	  TRACE("  FOGSTART\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGEND: {
	  TRACE("  FOGEND\n");
	} break ;
	  
	case D3DLIGHTSTATE_FOGDENSITY: {
	  TRACE("  FOGDENSITY\n");
	} break ;
	  
	default:
	  ERR("  Unhandled light state !! (%d)\n", (int) ci->t.dlstLightStateType);
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
	set_render_state(ci->t.drstRenderStateType, ci->v.dwArg[0], &(odev->rs));

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
	  D3DVERTEX  *src = ((LPD3DVERTEX)  (ilpBuff->desc.lpData + vs)) + ci->wStart;
	  OGL_Vertex *dst = ((OGL_Vertex *) (ilpBuff->vertex_data)) + ci->wDest;
	  D3DMATRIX *mat = odev->world_mat;

	  TRACE("  World Matrix : (%p)\n", mat);
	  dump_mat(mat);

	  ilpBuff->vertex_type = D3DVT_VERTEX;
	  
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
	  D3DLVERTEX *src  = ((LPD3DLVERTEX) (ilpBuff->desc.lpData + vs)) + ci->wStart;
	  OGL_LVertex *dst = ((OGL_LVertex *) (ilpBuff->vertex_data)) + ci->wDest;
	  D3DMATRIX *mat = odev->world_mat;

	  TRACE("  World Matrix : (%p)\n", mat);
	  dump_mat(mat);

	  ilpBuff->vertex_type = D3DVT_LVERTEX;
	  
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
	  D3DTLVERTEX *src = ((LPD3DTLVERTEX) (ilpBuff->desc.lpData + vs)) + ci->wStart;
	  D3DTLVERTEX *dst = ((LPD3DTLVERTEX) (ilpBuff->vertex_data)) + ci->wDest;

	  ilpBuff->vertex_type = D3DVT_TLVERTEX;
	  
	  memcpy(dst, src, ci->dwCount * sizeof(D3DTLVERTEX));
	} else {
	  ERR("Unhandled vertex processing !\n");
	}
	
	instr += size;
      }
    } break;
      
    case D3DOP_TEXTURELOAD: {
      TRACE("TEXTURELOAD-s    (%d)\n", count);

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
	
	if ((ilpBuff->data.dsStatus.dwStatus & ci->dwMask) == ci->dwValue) {
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
      TRACE("SPAN-s           (%d)\n", count);

      instr += count * size;
    } break;
      
    case D3DOP_SETSTATUS: {
      int i;
      TRACE("SETSTATUS        (%d)\n", count);

      for (i = 0; i < count; i++) {
	LPD3DSTATUS ci = (LPD3DSTATUS) instr;
	
	ilpBuff->data.dsStatus = *ci;
	
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

/*******************************************************************************
 *				ExecuteBuffer Creation functions
 */
LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(IDirect3DDeviceImpl* d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc)
{
  IDirect3DExecuteBufferImpl* eb;
  
  eb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DExecuteBufferImpl));
  eb->ref = 1;
  ICOM_VTBL(eb) = &executebuffer_vtable;
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
  
  return (LPDIRECT3DEXECUTEBUFFER)eb;
}

/*******************************************************************************
 *				IDirect3ExecuteBuffer methods
 */

static HRESULT WINAPI IDirect3DExecuteBufferImpl_QueryInterface(LPDIRECT3DEXECUTEBUFFER iface,
							    REFIID riid,
							    LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME("(%p)->(%s,%p): stub\n", This, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DExecuteBufferImpl_AddRef(LPDIRECT3DEXECUTEBUFFER iface)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DExecuteBufferImpl_Release(LPDIRECT3DEXECUTEBUFFER iface)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );
  
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

static HRESULT WINAPI IDirect3DExecuteBufferImpl_Initialize(LPDIRECT3DEXECUTEBUFFER iface,
							LPDIRECT3DDEVICE lpDirect3DDevice,
							LPD3DEXECUTEBUFFERDESC lpDesc)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lpDirect3DDevice, lpDesc);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBufferImpl_Lock(LPDIRECT3DEXECUTEBUFFER iface,
						  LPD3DEXECUTEBUFFERDESC lpDesc)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  TRACE("(%p)->(%p)\n", This, lpDesc);

  /* Copies the buffer description */
  *lpDesc = This->desc;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBufferImpl_Unlock(LPDIRECT3DEXECUTEBUFFER iface)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  TRACE("(%p)->()\n", This);

  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBufferImpl_SetExecuteData(LPDIRECT3DEXECUTEBUFFER iface,
							    LPD3DEXECUTEDATA lpData)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  DWORD nbvert;

  TRACE("(%p)->(%p)\n", This, lpData);

  This->data = *lpData;

  /* Get the number of vertices in the execute buffer */
  nbvert = This->data.dwVertexCount;
    
  /* Prepares the transformed vertex buffer */
  if (This->vertex_data != NULL)
    HeapFree(GetProcessHeap(), 0, This->vertex_data);
  This->vertex_data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,nbvert * sizeof(OGL_Vertex));


  if (TRACE_ON(ddraw)) {
    _dump_executedata(lpData);
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBufferImpl_GetExecuteData(LPDIRECT3DEXECUTEBUFFER iface,
							    LPD3DEXECUTEDATA lpData)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  TRACE("(%p)->(%p): stub\n", This, lpData);

  *lpData = This->data;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBufferImpl_Validate(LPDIRECT3DEXECUTEBUFFER iface,
						      LPDWORD lpdwOffset,
						      LPD3DVALIDATECALLBACK lpFunc,
						      LPVOID lpUserArg,
						      DWORD dwReserved)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  TRACE("(%p)->(%p,%p,%p,%lu)\n", This, lpdwOffset, lpFunc, lpUserArg, dwReserved);

  return DD_OK;
}

static HRESULT WINAPI IDirect3DExecuteBufferImpl_Optimize(LPDIRECT3DEXECUTEBUFFER iface,
						      DWORD dwReserved)
{
  ICOM_THIS(IDirect3DExecuteBufferImpl,iface);
  TRACE("(%p)->(%lu)\n", This, dwReserved);

  return DD_OK;
}


/*******************************************************************************
 *				IDirect3DLight VTable
 */
static ICOM_VTABLE(IDirect3DExecuteBuffer) executebuffer_vtable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  /*** IUnknown methods ***/
  IDirect3DExecuteBufferImpl_QueryInterface,
  IDirect3DExecuteBufferImpl_AddRef,
  IDirect3DExecuteBufferImpl_Release,
  /*** IDirect3DExecuteBuffer methods ***/
  IDirect3DExecuteBufferImpl_Initialize,
  IDirect3DExecuteBufferImpl_Lock,
  IDirect3DExecuteBufferImpl_Unlock,
  IDirect3DExecuteBufferImpl_SetExecuteData,
  IDirect3DExecuteBufferImpl_GetExecuteData,
  IDirect3DExecuteBufferImpl_Validate,
  IDirect3DExecuteBufferImpl_Optimize
};

#endif /* HAVE_MESAGL */
