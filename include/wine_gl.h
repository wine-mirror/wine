/* Wrapper for OpenGL includes...
     Copyright 1998 - Lionel Ulmer

   This wrapper is needed because Mesa uses also the CALLBACK / WINAPI
   constants. */

#ifndef __WINE_GL_H
#define __WINE_GL_H

#ifdef HAVE_MESAGL

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#include <GL/gl.h>
/* These will need to have some #ifdef / #endif added to support
   more than the X11 using OSMesa target */
#include <GL/osmesa.h>

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI

#endif /* HAVE_MESAGL */

#endif /* __WINE_GL_H */
