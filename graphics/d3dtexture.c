/* Direct3D Texture
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of interface Direct3DTexture2. */


#include "config.h"
#include "wintypes.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"
#include "objbase.h"

#include "d3d_private.h"

#ifdef HAVE_MESAGL

/* Define this if you want to save to a file all the textures used by a game
   (can be funny to see how they managed to cram all the pictures in
   texture memory) */
#undef TEXTURE_SNOOP

static IDirect3DTexture2_VTable texture2_vtable;
static IDirect3DTexture_VTable texture_vtable;

/*******************************************************************************
 *				Texture2 Creation functions
 */
LPDIRECT3DTEXTURE2 d3dtexture2_create(LPDIRECTDRAWSURFACE4 surf)
{
  LPDIRECT3DTEXTURE2 mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DTexture2));
  mat->ref = 1;
  mat->lpvtbl = &texture2_vtable;
  mat->surface = surf;
  
  return mat;
}

/*******************************************************************************
 *				Texture Creation functions
 */
LPDIRECT3DTEXTURE d3dtexture_create(LPDIRECTDRAWSURFACE4 surf)
{
  LPDIRECT3DTEXTURE mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DTexture));
  mat->ref = 1;
  mat->lpvtbl = (IDirect3DTexture2_VTable*) &texture_vtable;
  mat->surface = surf;
  
  return mat;
}


/*******************************************************************************
 *				IDirect3DTexture2 methods
 */

static HRESULT WINAPI IDirect3DTexture2_QueryInterface(LPDIRECT3DTEXTURE2 this,
							REFIID riid,
							LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DTexture2_AddRef(LPDIRECT3DTEXTURE2 this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DTexture2_Release(LPDIRECT3DTEXTURE2 this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    /* Delete texture from OpenGL */
    glDeleteTextures(1, &(this->tex_name));
    
    /* Release surface */
    this->surface->lpvtbl->fnRelease(this->surface);
    
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

/*** IDirect3DTexture methods ***/
static HRESULT WINAPI IDirect3DTexture_GetHandle(LPDIRECT3DTEXTURE this,
						 LPDIRECT3DDEVICE lpD3DDevice,
						 LPD3DTEXTUREHANDLE lpHandle)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lpD3DDevice, lpHandle);

  *lpHandle = (DWORD) this;
  
  /* Now, bind a new texture */
  lpD3DDevice->set_context(lpD3DDevice);
  this->D3Ddevice = (void *) lpD3DDevice;
  if (this->tex_name == 0)
    glGenTextures(1, &(this->tex_name));

  TRACE(ddraw, "OpenGL texture handle is : %d\n", this->tex_name);
  
  return D3D_OK;
}

static HRESULT WINAPI IDirect3DTexture_Initialize(LPDIRECT3DTEXTURE this,
						  LPDIRECT3DDEVICE lpD3DDevice,
						  LPDIRECTDRAWSURFACE lpSurface)
{
  TRACE(ddraw, "(%p)->(%p,%p)\n", this, lpD3DDevice, lpSurface);

  return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirect3DTexture_Unload(LPDIRECT3DTEXTURE this)
{
  FIXME(ddraw, "(%p)->(): stub\n", this);

  return D3D_OK;
}

/*** IDirect3DTexture2 methods ***/
static HRESULT WINAPI IDirect3DTexture2_GetHandle(LPDIRECT3DTEXTURE2 this,
						  LPDIRECT3DDEVICE2 lpD3DDevice2,
						  LPD3DTEXTUREHANDLE lpHandle)
{
  TRACE(ddraw, "(%p)->(%p,%p)\n", this, lpD3DDevice2, lpHandle);

  /* For 32 bits OSes, handles = pointers */
  *lpHandle = (DWORD) this;
  
  /* Now, bind a new texture */
  lpD3DDevice2->set_context(lpD3DDevice2);
  this->D3Ddevice = (void *) lpD3DDevice2;
  if (this->tex_name == 0)
  glGenTextures(1, &(this->tex_name));

  TRACE(ddraw, "OpenGL texture handle is : %d\n", this->tex_name);
  
  return D3D_OK;
}

/* Common methods */
static HRESULT WINAPI IDirect3DTexture2_PaletteChanged(LPDIRECT3DTEXTURE2 this,
						       DWORD dwStart,
						       DWORD dwCount)
{
  FIXME(ddraw, "(%p)->(%8ld,%8ld): stub\n", this, dwStart, dwCount);

  return D3D_OK;
}

/* NOTE : if you experience crashes in this function, you must have a buggy
          version of Mesa. See the file d3dtexture.c for a cure */
static HRESULT WINAPI IDirect3DTexture2_Load(LPDIRECT3DTEXTURE2 this,
					     LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
  DDSURFACEDESC	*src_d, *dst_d;
  TRACE(ddraw, "(%p)->(%p)\n", this, lpD3DTexture2);

  TRACE(ddraw, "Copied to surface %p, surface %p\n", this->surface, lpD3DTexture2->surface);

  /* Suppress the ALLOCONLOAD flag */
  this->surface->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

  /* Copy one surface on the other */
  dst_d = &(this->surface->s.surface_desc);
  src_d = &(lpD3DTexture2->surface->s.surface_desc);

  if ((src_d->dwWidth != dst_d->dwWidth) || (src_d->dwHeight != dst_d->dwHeight)) {
    /* Should also check for same pixel format, lPitch, ... */
    ERR(ddraw, "Error in surface sizes\n");
    return D3DERR_TEXTURE_LOAD_FAILED;
  } else {
    /* LPDIRECT3DDEVICE2 d3dd = (LPDIRECT3DDEVICE2) this->D3Ddevice; */
    /* I should put a macro for the calculus of bpp */
    int bpp = (src_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8 ?
	       1 /* 8 bit of palette index */:
	       src_d->ddpfPixelFormat.x.dwRGBBitCount / 8 /* RGB bits for each colors */ );
    GLuint current_texture;

    /* Not sure if this is usefull ! */
    memcpy(dst_d->y.lpSurface, src_d->y.lpSurface, src_d->dwWidth * src_d->dwHeight * bpp);

    /* Now, load the texture */
    /* d3dd->set_context(d3dd); We need to set the context somehow.... */
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    /* If the GetHandle was not done, get the texture name here */
    if (this->tex_name == 0)
      glGenTextures(1, &(this->tex_name));
    glBindTexture(GL_TEXTURE_2D, this->tex_name);

    if (src_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
      /* ****************
	 Paletted Texture
	 **************** */
      LPDIRECTDRAWPALETTE pal = this->surface->s.palette;
      BYTE table[256][4];
      int i;
      
      if (pal == NULL) {
	ERR(ddraw, "Palettized texture Loading with a NULL palette !\n");
	return D3DERR_TEXTURE_LOAD_FAILED;
      }

      /* Get the surface's palette */
      for (i = 0; i < 256; i++) {
	table[i][0] = pal->palents[i].peRed;
	table[i][1] = pal->palents[i].peGreen;
	table[i][2] = pal->palents[i].peBlue;
	if ((this->surface->s.surface_desc.dwFlags & DDSD_CKSRCBLT) &&
	    (i >= this->surface->s.surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue) &&
	    (i <= this->surface->s.surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue))
	  table[i][3] = 0x00;
	else
	table[i][3] = 0xFF;
      }
      
#ifdef TEXTURE_SNOOP
      {
	FILE *f;
	char buf[32];
	int x, y;
	
	sprintf(buf, "%d.pnm", this->tex_name);
	f = fopen(buf, "wb");
	fprintf(f, "P6\n%d %d\n255\n", src_d->dwWidth, src_d->dwHeight);
	for (y = 0; y < src_d->dwHeight; y++) {
	  for (x = 0; x < src_d->dwWidth; x++) {
	    unsigned char c = ((unsigned char *) src_d->y.lpSurface)[y * src_d->dwWidth + x];
	    fputc(table[c][0], f);
	    fputc(table[c][1], f);
	    fputc(table[c][2], f);
	  }
	}
	fclose(f);
      }
#endif 
      /* Use Paletted Texture Extension */
      glColorTableEXT(GL_TEXTURE_2D,    /* target */
		      GL_RGBA,          /* internal format */
		      256,              /* table size */
		      GL_RGBA,          /* table format */
		      GL_UNSIGNED_BYTE, /* table type */
		      table);           /* the color table */

      glTexImage2D(GL_TEXTURE_2D,       /* target */
		   0,                   /* level */
		   GL_COLOR_INDEX8_EXT, /* internal format */
		   src_d->dwWidth, src_d->dwHeight, /* width, height */
		   0,                   /* border */
		   GL_COLOR_INDEX,      /* texture format */
		   GL_UNSIGNED_BYTE,    /* texture type */
		   src_d->y.lpSurface); /* the texture */
    } else if (src_d->ddpfPixelFormat.dwFlags & DDPF_RGB) {
      /* ************
	 RGB Textures
	 ************ */
      if (src_d->ddpfPixelFormat.x.dwRGBBitCount == 8) {
	/* **********************
	   GL_UNSIGNED_BYTE_3_3_2 
	   ********************** */
	glTexImage2D(GL_TEXTURE_2D,
		     0,
		     GL_RGB,
		     src_d->dwWidth, src_d->dwHeight,
		     0,
		     GL_RGB,
		     GL_UNSIGNED_BYTE_3_3_2,
		     src_d->y.lpSurface);
      } else if (src_d->ddpfPixelFormat.x.dwRGBBitCount == 16) {
	if (src_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x00000000) {
#ifdef TEXTURE_SNOOP
	  {
	    FILE *f;
	    char buf[32];
	    int x, y;
	    
	    sprintf(buf, "%d.pnm", this->tex_name);
	    f = fopen(buf, "wb");
	    fprintf(f, "P6\n%d %d\n255\n", src_d->dwWidth, src_d->dwHeight);
	    for (y = 0; y < src_d->dwHeight; y++) {
	      for (x = 0; x < src_d->dwWidth; x++) {
		unsigned short c = ((unsigned short *) src_d->y.lpSurface)[y * src_d->dwWidth + x];
		fputc((c & 0xF800) >> 8, f);
		fputc((c & 0x07E0) >> 3, f);
		fputc((c & 0x001F) << 3, f);
	      }
	    }
	    fclose(f);
	  }
#endif
	  glTexImage2D(GL_TEXTURE_2D,
		       0,
		       GL_RGB,
		       src_d->dwWidth, src_d->dwHeight,
		       0,
		       GL_RGB,
		       GL_UNSIGNED_SHORT_5_6_5,
		       src_d->y.lpSurface);
	} else if (src_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x00000001) {
#ifdef TEXTURE_SNOOP
	  {
	    FILE *f;
	    char buf[32];
	    int x, y;
	    
	    sprintf(buf, "%d.pnm", this->tex_name);
	    f = fopen(buf, "wb");
	    fprintf(f, "P6\n%d %d\n255\n", src_d->dwWidth, src_d->dwHeight);
	    for (y = 0; y < src_d->dwHeight; y++) {
	      for (x = 0; x < src_d->dwWidth; x++) {
		unsigned short c = ((unsigned short *) src_d->y.lpSurface)[y * src_d->dwWidth + x];
		fputc((c & 0xF800) >> 8, f);
		fputc((c & 0x07C0) >> 3, f);
		fputc((c & 0x003E) << 2, f);
	      }
	    }
	    fclose(f);
	  }
#endif
	  
	  glTexImage2D(GL_TEXTURE_2D,
		       0,
		       GL_RGBA,
		       src_d->dwWidth, src_d->dwHeight,
		       0,
		       GL_RGBA,
		       GL_UNSIGNED_SHORT_5_5_5_1,
		       src_d->y.lpSurface);
	} else if (src_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x0000000F) {
	  glTexImage2D(GL_TEXTURE_2D,
		       0,
		       GL_RGBA,
		       src_d->dwWidth, src_d->dwHeight,
		       0,
		       GL_RGBA,
		       GL_UNSIGNED_SHORT_4_4_4_4,
		       src_d->y.lpSurface);
	} else {
	  ERR(ddraw, "Unhandled texture format (bad Aplha channel for a 16 bit texture)\n");
	}
      } else if (src_d->ddpfPixelFormat.x.dwRGBBitCount == 24) {
	glTexImage2D(GL_TEXTURE_2D,
		     0,
		     GL_RGB,
		     src_d->dwWidth, src_d->dwHeight,
		     0,
		     GL_RGB,
		     GL_UNSIGNED_BYTE,
		     src_d->y.lpSurface);
      } else if (src_d->ddpfPixelFormat.x.dwRGBBitCount == 32) {
	glTexImage2D(GL_TEXTURE_2D,
		     0,
		     GL_RGBA,
		     src_d->dwWidth, src_d->dwHeight,
		     0,
		     GL_RGBA,
		     GL_UNSIGNED_BYTE,
		     src_d->y.lpSurface);
      } else {
	ERR(ddraw, "Unhandled texture format (bad RGB count)\n");
      }
    } else {
      ERR(ddraw, "Unhandled texture format (neither RGB nor INDEX)\n");
    }

    glBindTexture(GL_TEXTURE_2D, current_texture);
  }
  
  return D3D_OK;
}


/*******************************************************************************
 *				IDirect3DTexture2 VTable
 */
static IDirect3DTexture2_VTable texture2_vtable = {
  /*** IUnknown methods ***/
  IDirect3DTexture2_QueryInterface,
  IDirect3DTexture2_AddRef,
  IDirect3DTexture2_Release,
  /*** IDirect3DTexture methods ***/
  IDirect3DTexture2_GetHandle,
  IDirect3DTexture2_PaletteChanged,
  IDirect3DTexture2_Load
};

/*******************************************************************************
 *				IDirect3DTexture VTable
 */
static IDirect3DTexture_VTable texture_vtable = {
  /*** IUnknown methods ***/
  IDirect3DTexture2_QueryInterface,
  IDirect3DTexture2_AddRef,
  IDirect3DTexture2_Release,
  /*** IDirect3DTexture methods ***/
  IDirect3DTexture_Initialize,
  IDirect3DTexture_GetHandle,
  IDirect3DTexture2_PaletteChanged,
  IDirect3DTexture2_Load,
  IDirect3DTexture_Unload
};

#else /* HAVE_MESAGL */

/* These function should never be called if MesaGL is not present */
LPDIRECT3DTEXTURE2 d3dtexture2_create(LPDIRECTDRAWSURFACE4 surf) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DTEXTURE d3dtexture_create(LPDIRECTDRAWSURFACE4 surf) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
