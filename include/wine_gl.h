/* Wrapper for OpenGL includes...
     Copyright 1998 - Lionel Ulmer

   This wrapper is needed because Mesa uses also the CALLBACK / WINAPI
   constants. */

#ifndef __WINE_WINE_GL_H
#define __WINE_WINE_GL_H

#include "config.h"

#if defined(HAVE_LIBMESAGL) && defined(HAVE_GL_GLX_H)

#define HAVE_MESAGL

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#include <GL/gl.h>
#include <GL/glx.h>

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI

#else /* HAVE_LIBMESAGL */

#undef HAVE_MESAGL

#endif /* HAVE_LIBMESAGL */

#endif /* __WINE_WINE_GL_H */
