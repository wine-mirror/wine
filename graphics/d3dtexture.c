/* Direct3D Texture
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of interface Direct3DTexture2. */


#include <string.h>
#include "config.h"
#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"

#include "d3d_private.h"

#ifdef HAVE_MESAGL

/* Define this if you want to save to a file all the textures used by a game
   (can be funny to see how they managed to cram all the pictures in
   texture memory) */
#undef TEXTURE_SNOOP

#ifdef TEXTURE_SNOOP
#define SNOOP_PALETTED() 									\
      {												\
	FILE *f;										\
	char buf[32];										\
	int x, y;										\
												\
	sprintf(buf, "%d.pnm", This->tex_name);							\
	f = fopen(buf, "wb");									\
	fprintf(f, "P6\n%ld %ld\n255\n", src_d->dwWidth, src_d->dwHeight);			\
	for (y = 0; y < src_d->dwHeight; y++) {							\
	  for (x = 0; x < src_d->dwWidth; x++) {						\
	    unsigned char c = ((unsigned char *) src_d->y.lpSurface)[y * src_d->dwWidth + x];	\
	    fputc(table[c][0], f);								\
	    fputc(table[c][1], f);								\
	    fputc(table[c][2], f);								\
	  }											\
	}											\
	fclose(f);										\
      }

#define SNOOP_5650()											\
	  {												\
	    FILE *f;											\
	    char buf[32];										\
	    int x, y;											\
	    												\
	    sprintf(buf, "%d.pnm", This->tex_name);							\
	    f = fopen(buf, "wb");									\
	    fprintf(f, "P6\n%ld %ld\n255\n", src_d->dwWidth, src_d->dwHeight);				\
	    for (y = 0; y < src_d->dwHeight; y++) {							\
	      for (x = 0; x < src_d->dwWidth; x++) {							\
		unsigned short c = ((unsigned short *) src_d->y.lpSurface)[y * src_d->dwWidth + x];	\
		fputc((c & 0xF800) >> 8, f);								\
		fputc((c & 0x07E0) >> 3, f);								\
		fputc((c & 0x001F) << 3, f);								\
	      }												\
	    }												\
	    fclose(f);											\
	  }

#define SNOOP_5551()											\
	  {												\
	    FILE *f;											\
	    char buf[32];										\
	    int x, y;											\
	    												\
	    sprintf(buf, "%d.pnm", This->tex_name);							\
	    f = fopen(buf, "wb");									\
	    fprintf(f, "P6\n%ld %ld\n255\n", src_d->dwWidth, src_d->dwHeight);				\
	    for (y = 0; y < src_d->dwHeight; y++) {							\
	      for (x = 0; x < src_d->dwWidth; x++) {							\
		unsigned short c = ((unsigned short *) src_d->y.lpSurface)[y * src_d->dwWidth + x];	\
		fputc((c & 0xF800) >> 8, f);								\
		fputc((c & 0x07C0) >> 3, f);								\
		fputc((c & 0x003E) << 2, f);								\
	      }												\
	    }												\
	    fclose(f);											\
	  }
#else
#define SNOOP_PALETTED()
#define SNOOP_5650()
#define SNOOP_5551()
#endif

static ICOM_VTABLE(IDirect3DTexture2) texture2_vtable;
static ICOM_VTABLE(IDirect3DTexture) texture_vtable;

/*******************************************************************************
 *				Texture2 Creation functions
 */
LPDIRECT3DTEXTURE2 d3dtexture2_create(IDirectDrawSurface4Impl* surf)
{
  IDirect3DTexture2Impl* tex;
  
  tex = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DTexture2Impl));
  tex->ref = 1;
  tex->lpvtbl = &texture2_vtable;
  tex->surface = surf;
  
  return (LPDIRECT3DTEXTURE2)tex;
}

/*******************************************************************************
 *				Texture Creation functions
 */
LPDIRECT3DTEXTURE d3dtexture_create(IDirectDrawSurface4Impl* surf)
{
  IDirect3DTexture2Impl* tex;
  
  tex = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DTexture2Impl));
  tex->ref = 1;
  tex->lpvtbl = (ICOM_VTABLE(IDirect3DTexture2)*)&texture_vtable;
  tex->surface = surf;
  
  return (LPDIRECT3DTEXTURE)tex;
}

/*******************************************************************************
 *			   IDirectSurface callback methods
 */
HRESULT WINAPI  SetColorKey_cb(IDirect3DTexture2Impl *texture, DWORD dwFlags, LPDDCOLORKEY ckey )
{
  DDSURFACEDESC	*tex_d;
  int bpp;
  GLuint current_texture;
  
  TRACE(ddraw, "(%p) : colorkey callback\n", texture);

  /* Get the texture description */
  tex_d = &(texture->surface->s.surface_desc);
  bpp = (tex_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8 ?
	 1 /* 8 bit of palette index */:
	 tex_d->ddpfPixelFormat.x.dwRGBBitCount / 8 /* RGB bits for each colors */ );
  
  /* Now, save the current texture */
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

  /* If the GetHandle was not done yet, it's an error */
  if (texture->tex_name == 0) {
    ERR(ddraw, "Unloaded texture !\n");
    return DD_OK;
  }
  glBindTexture(GL_TEXTURE_2D, texture->tex_name);

  if (tex_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
    FIXME(ddraw, "Todo Paletted\n");
  } else if (tex_d->ddpfPixelFormat.dwFlags & DDPF_RGB) {
    if (tex_d->ddpfPixelFormat.x.dwRGBBitCount == 8) {
      FIXME(ddraw, "Todo 3_3_2_0\n");
    } else if (tex_d->ddpfPixelFormat.x.dwRGBBitCount == 16) {
      if (tex_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x00000000) {
	/* Now transform the 5_6_5 into a 5_5_5_1 surface to support color keying */
	unsigned short *dest = (unsigned short *) HeapAlloc(GetProcessHeap(),
							    HEAP_ZERO_MEMORY,
							    tex_d->dwWidth * tex_d->dwHeight * bpp);
	unsigned short *src = (unsigned short *) tex_d->y.lpSurface;
	int x, y;

	for (y = 0; y < tex_d->dwHeight; y++) {
	  for (x = 0; x < tex_d->dwWidth; x++) {
	    unsigned short cpixel = src[x + y * tex_d->dwWidth];
	    
	    if ((dwFlags & DDCKEY_SRCBLT) &&
		(cpixel >= ckey->dwColorSpaceLowValue) &&
		(cpixel <= ckey->dwColorSpaceHighValue)) /* No alpha bit => this pixel is transparent */
	      dest[x + y * tex_d->dwWidth] = (cpixel & ~0x003F) | ((cpixel & 0x001F) << 1) | 0x0000;
	    else                                         /* Alpha bit is set => this pixel will be seen */
	      dest[x + y * tex_d->dwWidth] = (cpixel & ~0x003F) | ((cpixel & 0x001F) << 1) | 0x0001; 
	  }
	}

	glTexImage2D(GL_TEXTURE_2D,
		     0,
		     GL_RGBA,
		     tex_d->dwWidth, tex_d->dwHeight,
		     0,
		     GL_RGBA,
		     GL_UNSIGNED_SHORT_5_5_5_1,
		     dest);

	/* Frees the temporary surface */
	HeapFree(GetProcessHeap(),0,dest);
      } else if (tex_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x00000001) {
	FIXME(ddraw, "Todo 5_5_5_1\n");
      } else if (tex_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x0000000F) {
	FIXME(ddraw, "Todo 4_4_4_4\n");
      } else {
	ERR(ddraw, "Unhandled texture format (bad Aplha channel for a 16 bit texture)\n");
      }
    } else if (tex_d->ddpfPixelFormat.x.dwRGBBitCount == 24) {
      FIXME(ddraw, "Todo 8_8_8_0\n");
    } else if (tex_d->ddpfPixelFormat.x.dwRGBBitCount == 32) {
      FIXME(ddraw, "Todo 8_8_8_8\n");
    } else {
      ERR(ddraw, "Unhandled texture format (bad RGB count)\n");
    }
  } else {
    ERR(ddraw, "Unhandled texture format (neither RGB nor INDEX)\n");
  }

  return DD_OK;
}

/*******************************************************************************
 *				IDirect3DTexture2 methods
 */

static HRESULT WINAPI IDirect3DTexture2Impl_QueryInterface(LPDIRECT3DTEXTURE2 iface,
							REFIID riid,
							LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", This, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DTexture2Impl_AddRef(LPDIRECT3DTEXTURE2 iface)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DTexture2Impl_Release(LPDIRECT3DTEXTURE2 iface)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );
  
  if (!--(This->ref)) {
    /* Delete texture from OpenGL */
    glDeleteTextures(1, &(This->tex_name));
    
    /* Release surface */
    IDirectDrawSurface4_Release((IDirectDrawSurface4*)This->surface);
    
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  
  return This->ref;
}

/*** IDirect3DTexture methods ***/
static HRESULT WINAPI IDirect3DTextureImpl_GetHandle(LPDIRECT3DTEXTURE iface,
						 LPDIRECT3DDEVICE lpD3DDevice,
						 LPD3DTEXTUREHANDLE lpHandle)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  IDirect3DDeviceImpl* ilpD3DDevice=(IDirect3DDeviceImpl*)lpD3DDevice;
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", This, ilpD3DDevice, lpHandle);

  *lpHandle = (D3DTEXTUREHANDLE) This;
  
  /* Now, bind a new texture */
  ilpD3DDevice->set_context(ilpD3DDevice);
  This->D3Ddevice = (void *) ilpD3DDevice;
  if (This->tex_name == 0)
    glGenTextures(1, &(This->tex_name));

  TRACE(ddraw, "OpenGL texture handle is : %d\n", This->tex_name);
  
  return D3D_OK;
}

static HRESULT WINAPI IDirect3DTextureImpl_Initialize(LPDIRECT3DTEXTURE iface,
						  LPDIRECT3DDEVICE lpD3DDevice,
						  LPDIRECTDRAWSURFACE lpSurface)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  TRACE(ddraw, "(%p)->(%p,%p)\n", This, lpD3DDevice, lpSurface);

  return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirect3DTextureImpl_Unload(LPDIRECT3DTEXTURE iface)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  FIXME(ddraw, "(%p)->(): stub\n", This);

  return D3D_OK;
}

/*** IDirect3DTexture2 methods ***/
static HRESULT WINAPI IDirect3DTexture2Impl_GetHandle(LPDIRECT3DTEXTURE2 iface,
						  LPDIRECT3DDEVICE2 lpD3DDevice2,
						  LPD3DTEXTUREHANDLE lpHandle)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  IDirect3DDevice2Impl* ilpD3DDevice2=(IDirect3DDevice2Impl*)lpD3DDevice2;
  TRACE(ddraw, "(%p)->(%p,%p)\n", This, ilpD3DDevice2, lpHandle);

  /* For 32 bits OSes, handles = pointers */
  *lpHandle = (D3DTEXTUREHANDLE) This;
  
  /* Now, bind a new texture */
  ilpD3DDevice2->set_context(ilpD3DDevice2);
  This->D3Ddevice = (void *) ilpD3DDevice2;
  if (This->tex_name == 0)
  glGenTextures(1, &(This->tex_name));

  TRACE(ddraw, "OpenGL texture handle is : %d\n", This->tex_name);
  
  return D3D_OK;
}

/* Common methods */
static HRESULT WINAPI IDirect3DTexture2Impl_PaletteChanged(LPDIRECT3DTEXTURE2 iface,
						       DWORD dwStart,
						       DWORD dwCount)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  FIXME(ddraw, "(%p)->(%8ld,%8ld): stub\n", This, dwStart, dwCount);

  return D3D_OK;
}

/* NOTE : if you experience crashes in this function, you must have a buggy
          version of Mesa. See the file d3dtexture.c for a cure */
static HRESULT WINAPI IDirect3DTexture2Impl_Load(LPDIRECT3DTEXTURE2 iface,
					     LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
  ICOM_THIS(IDirect3DTexture2Impl,iface);
  IDirect3DTexture2Impl* ilpD3DTexture2=(IDirect3DTexture2Impl*)lpD3DTexture2;
  DDSURFACEDESC	*src_d, *dst_d;
  TRACE(ddraw, "(%p)->(%p)\n", This, ilpD3DTexture2);

  TRACE(ddraw, "Copied surface %p to surface %p\n", ilpD3DTexture2->surface, This->surface);

  /* Suppress the ALLOCONLOAD flag */
  This->surface->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

  /* Copy one surface on the other */
  dst_d = &(This->surface->s.surface_desc);
  src_d = &(ilpD3DTexture2->surface->s.surface_desc);

  /* Install the callbacks to the destination surface */
  This->surface->s.texture = This;
  This->surface->s.SetColorKey_cb = SetColorKey_cb;
  
  if ((src_d->dwWidth != dst_d->dwWidth) || (src_d->dwHeight != dst_d->dwHeight)) {
    /* Should also check for same pixel format, lPitch, ... */
    ERR(ddraw, "Error in surface sizes\n");
    return D3DERR_TEXTURE_LOAD_FAILED;
  } else {
    /* LPDIRECT3DDEVICE2 d3dd = (LPDIRECT3DDEVICE2) This->D3Ddevice; */
    /* I should put a macro for the calculus of bpp */
    int bpp = (src_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8 ?
	       1 /* 8 bit of palette index */:
	       src_d->ddpfPixelFormat.x.dwRGBBitCount / 8 /* RGB bits for each colors */ );
    GLuint current_texture;

    /* Copy the main memry texture into the surface that corresponds to the OpenGL
       texture object. */
    memcpy(dst_d->y.lpSurface, src_d->y.lpSurface, src_d->dwWidth * src_d->dwHeight * bpp);

    /* Now, load the texture */
    /* d3dd->set_context(d3dd); We need to set the context somehow.... */
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    /* If the GetHandle was not done, get the texture name here */
    if (This->tex_name == 0)
      glGenTextures(1, &(This->tex_name));
    glBindTexture(GL_TEXTURE_2D, This->tex_name);

    if (src_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
      /* ****************
	 Paletted Texture
	 **************** */
      IDirectDrawPaletteImpl* pal = This->surface->s.palette;
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
	if ((This->surface->s.surface_desc.dwFlags & DDSD_CKSRCBLT) &&
	    (i >= This->surface->s.surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue) &&
	    (i <= This->surface->s.surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue))
	  table[i][3] = 0x00;
	else
	table[i][3] = 0xFF;
      }
      
      /* Texture snooping */
      SNOOP_PALETTED();
	
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
	    
	  /* Texture snooping */
	  SNOOP_5650();
	  
	  glTexImage2D(GL_TEXTURE_2D,
		       0,
		       GL_RGB,
		       src_d->dwWidth, src_d->dwHeight,
		       0,
		       GL_RGB,
		       GL_UNSIGNED_SHORT_5_6_5,
		       src_d->y.lpSurface);
	} else if (src_d->ddpfPixelFormat.xy.dwRGBAlphaBitMask == 0x00000001) {
	  /* Texture snooping */
	  SNOOP_5551();
	  
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
static ICOM_VTABLE(IDirect3DTexture2) texture2_vtable = {
  /*** IUnknown methods ***/
  IDirect3DTexture2Impl_QueryInterface,
  IDirect3DTexture2Impl_AddRef,
  IDirect3DTexture2Impl_Release,
  /*** IDirect3DTexture methods ***/
  IDirect3DTexture2Impl_GetHandle,
  IDirect3DTexture2Impl_PaletteChanged,
  IDirect3DTexture2Impl_Load
};

/*******************************************************************************
 *				IDirect3DTexture VTable
 */
static ICOM_VTABLE(IDirect3DTexture) texture_vtable = {
  /*** IUnknown methods ***/
  IDirect3DTexture2Impl_QueryInterface,
  IDirect3DTexture2Impl_AddRef,
  IDirect3DTexture2Impl_Release,
  /*** IDirect3DTexture methods ***/
  IDirect3DTextureImpl_Initialize,
  IDirect3DTextureImpl_GetHandle,
  IDirect3DTexture2Impl_PaletteChanged,
  IDirect3DTexture2Impl_Load,
  IDirect3DTextureImpl_Unload
};

#else /* HAVE_MESAGL */

/* These function should never be called if MesaGL is not present */
LPDIRECT3DTEXTURE2 d3dtexture2_create(IDirectDrawSurface4Impl* surf) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DTEXTURE d3dtexture_create(IDirectDrawSurface4Impl* surf) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
