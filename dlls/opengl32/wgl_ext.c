/* Support for window-specific OpenGL extensions.
 *
 * Copyright (c) 2004 Lionel Ulmer
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
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"

#include "wgl.h"
#include "wgl_ext.h"
#include "opengl_ext.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

/* Some WGL extensions... */
static const char *WGL_extensions_base = "WGL_ARB_extensions_string WGL_EXT_extensions_string";
static char *WGL_extensions = NULL;

/* Extensions-query functions */
BOOL query_function_pbuffers(const char *gl_version, const char *gl_extensions, const char *glx_extensions,
			     const char *server_glx_extensions, const char *client_glx_extensions)
{
    return FALSE;
}

/***********************************************************************
 *              wglGetExtensionsStringEXT(OPENGL32.@)
 */
const char * WINAPI wglGetExtensionsStringEXT(void) {
    TRACE("() returning \"%s\"\n", WGL_extensions);

    return WGL_extensions;
}

/***********************************************************************
 *              wglGetExtensionsStringARB(OPENGL32.@)
 */
const char * WINAPI wglGetExtensionsStringARB(HDC hdc) {
    TRACE("() returning \"%s\"\n", WGL_extensions);

    return WGL_extensions;    
}

static const struct {
    const char *name;
    BOOL (*query_function)(const char *gl_version, const char *gl_extensions, const char *glx_extensions,
			   const char *server_glx_extensions, const char *client_glx_extensions);
} extension_list[] = {
    { "WGL_ARB_pbuffer", query_function_pbuffers }
};

/* Used to initialize the WGL extension string at DLL loading */
void wgl_ext_initialize_extensions(Display *display, int screen)
{
    int size = strlen(WGL_extensions_base);
    const char *glx_extensions = glXQueryExtensionsString(display, screen);
    const char *server_glx_extensions = glXQueryServerString(display, screen, GLX_EXTENSIONS);
    const char *client_glx_extensions = glXGetClientString(display, GLX_EXTENSIONS);
    const char *gl_extensions = (const char *) glGetString(GL_EXTENSIONS);
    const char *gl_version = (const char *) glGetString(GL_VERSION);
    int i;

    TRACE("GL version      : %s.\n", debugstr_a(gl_version));
    TRACE("GL exts         : %s.\n", debugstr_a(gl_extensions));
    TRACE("GLX exts        : %s.\n", debugstr_a(glx_extensions));
    TRACE("Server GLX exts : %s.\n", debugstr_a(server_glx_extensions));
    TRACE("Client GLX exts : %s.\n", debugstr_a(client_glx_extensions));

    for (i = 0; i < (sizeof(extension_list) / sizeof(extension_list[0])); i++) {
	if (extension_list[i].query_function(gl_version, gl_extensions, glx_extensions,
					     server_glx_extensions, client_glx_extensions)) {
	    size += strlen(extension_list[i].name) + 1;
	}
    }

    /* For the moment, only 'base' extensions are supported. */
    WGL_extensions = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
    if (WGL_extensions == NULL) {
	WGL_extensions = (char *) WGL_extensions_base;
    } else {
	strcpy(WGL_extensions, WGL_extensions_base);
	for (i = 0; i < (sizeof(extension_list) / sizeof(extension_list[0])); i++) {
	    if (extension_list[i].query_function(gl_version, gl_extensions, glx_extensions,
						 server_glx_extensions, client_glx_extensions)) {
		strcat(WGL_extensions, " ");
		strcat(WGL_extensions, extension_list[i].name);
	    }
	}
    }

    TRACE("Supporting following WGL extensions : %s.\n", debugstr_a(WGL_extensions));
}

void wgl_ext_finalize_extensions(void)
{
    if (WGL_extensions != WGL_extensions_base) {
	HeapFree(GetProcessHeap(), 0, WGL_extensions);
    }
}

/* Putting this at the end to prevent having to write the prototypes :-) */
WGL_extension wgl_extension_registry[] = {
    { "wglGetExtensionsStringARB", (void *) wglGetExtensionsStringARB, NULL, NULL},
    { "wglGetExtensionsStringEXT", (void *) wglGetExtensionsStringEXT, NULL, NULL}
};
int wgl_extension_registry_size = sizeof(wgl_extension_registry) / sizeof(wgl_extension_registry[0]);
