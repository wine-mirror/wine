/* Wrapper for OpenGL includes...
     Copyright 1998 - Lionel Ulmer

   This wrapper is needed because Mesa uses also the CALLBACK / WINAPI
   constants. */

#ifndef __WINE_WINE_GL_H
#define __WINE_WINE_GL_H

#include "config.h"

#if defined(HAVE_OPENGL)

#include "x11drv.h"

/* As GLX relies on X, this is needed */
#define ENTER_GL() EnterCriticalSection( &X11DRV_CritSection )
#define LEAVE_GL() LeaveCriticalSection( &X11DRV_CritSection )

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#define XMD_H /* This is to prevent the Xmd.h inclusion bug to happen :-/ */
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#undef  XMD_H

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI

#endif /* HAVE_OPENGL */

#endif /* __WINE_WINE_GL_H */
