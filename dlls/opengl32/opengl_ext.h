/* Typedefs for extensions loading

     Copyright (c) 2000 Lionel Ulmer
*/
#ifndef __DLLS_OPENGL32_OPENGL_EXT_H
#define __DLLS_OPENGL32_OPENGL_EXT_H

typedef struct {
  char  *name;     /* name of the extension */
  void  *func;     /* pointer to the Wine function for this extension */
  void **func_ptr; /* where to store the value of glXGetProcAddressARB */
} OpenGL_extension;

extern OpenGL_extension extension_registry[];
extern int extension_registry_size;

#endif /* __DLLS_OPENGL32_OPENGL_EXT_H */
