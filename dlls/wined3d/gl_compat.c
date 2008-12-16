/*
 * Compatibility functions for older GL implementations
 *
 * Copyright 2008 Stefan Dösinger for CodeWeavers
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

#include "config.h"
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gl_compat);

static void WINE_GLAPI wine_glMultiTexCoord1fARB(GLenum target, GLfloat s) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord1f(s);
}

static void WINE_GLAPI wine_glMultiTexCoord1fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord1fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord2f(s, t);
}

static void WINE_GLAPI wine_glMultiTexCoord2fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord2fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord3f(s, t, r);
}

static void WINE_GLAPI wine_glMultiTexCoord3fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord3fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord4f(s, t, r, q);
}

static void WINE_GLAPI wine_glMultiTexCoord4fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord4fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord2svARB(GLenum target, const const GLshort *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord2sv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord4svARB(GLenum target, const const GLshort *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord4sv(v);
}

static void WINE_GLAPI wine_glActiveTextureARB(GLenum texture) {
    if(texture != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
}

static void WINE_GLAPI wine_glClientActiveTextureARB(GLenum texture) {
    if(texture != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
}

static void (WINE_GLAPI *old_multitex_glGetIntegerv) (GLenum pname, GLint* params) = NULL;
static void WINE_GLAPI wine_glGetIntegerv(GLenum pname, GLint* params) {
    switch(pname) {
        case GL_ACTIVE_TEXTURE:         *params = 0;    break;
        case GL_MAX_TEXTURE_UNITS_ARB:  *params = 1;    break;
        default: old_multitex_glGetIntegerv(pname, params);
    }
}

static void (WINE_GLAPI *old_multitex_glGetFloatv) (GLenum pname, GLfloat* params) = NULL;
static void WINE_GLAPI wine_glGetFloatv(GLenum pname, GLfloat* params) {
    if(pname == GL_ACTIVE_TEXTURE) *params = 0.0;
    else old_multitex_glGetFloatv(pname, params);
}

static void (WINE_GLAPI *old_multitex_glGetDoublev) (GLenum pname, GLdouble* params) = NULL;
static void WINE_GLAPI wine_glGetDoublev(GLenum pname, GLdouble* params) {
    if(pname == GL_ACTIVE_TEXTURE) *params = 0.0;
    else old_multitex_glGetDoublev(pname, params);
}

#define GLINFO_LOCATION (*gl_info)
void add_gl_compat_wrappers(WineD3D_GL_Info *gl_info) {
    if(!GL_SUPPORT(ARB_MULTITEXTURE)) {
        gl_info->glActiveTextureARB         = wine_glActiveTextureARB;
        gl_info->glClientActiveTextureARB   = wine_glClientActiveTextureARB;
        gl_info->glMultiTexCoord1fARB       = wine_glMultiTexCoord1fARB;
        gl_info->glMultiTexCoord1fvARB      = wine_glMultiTexCoord1fvARB;
        gl_info->glMultiTexCoord2fARB       = wine_glMultiTexCoord2fARB;
        gl_info->glMultiTexCoord2fvARB      = wine_glMultiTexCoord2fvARB;
        gl_info->glMultiTexCoord3fARB       = wine_glMultiTexCoord3fARB;
        gl_info->glMultiTexCoord3fvARB      = wine_glMultiTexCoord3fvARB;
        gl_info->glMultiTexCoord4fARB       = wine_glMultiTexCoord4fARB;
        gl_info->glMultiTexCoord4fvARB      = wine_glMultiTexCoord4fvARB;
        gl_info->glMultiTexCoord2svARB      = wine_glMultiTexCoord2svARB;
        gl_info->glMultiTexCoord4svARB      = wine_glMultiTexCoord4svARB;
        if(old_multitex_glGetIntegerv) {
            FIXME("GL_ARB_multitexture glGetIntegerv hook already applied\n");
        } else {
            old_multitex_glGetIntegerv = glGetIntegerv;
            glGetIntegerv = wine_glGetIntegerv;
        }
        if(old_multitex_glGetFloatv) {
            FIXME("GL_ARB_multitexture glGetGloatv hook already applied\n");
        } else {
            old_multitex_glGetFloatv = glGetFloatv;
            glGetFloatv = wine_glGetFloatv;
        }
        if(old_multitex_glGetDoublev) {
            FIXME("GL_ARB_multitexture glGetDoublev hook already applied\n");
        } else {
            old_multitex_glGetDoublev = glGetDoublev;
            glGetDoublev = wine_glGetDoublev;
        }
        gl_info->supported[ARB_MULTITEXTURE] = TRUE;
    }
}
#undef GLINFO_LOCATION
