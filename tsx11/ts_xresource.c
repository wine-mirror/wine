/*
 * Thread safe wrappers around Xresource calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"


#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "ts_xresource.h"


XrmQuark  TSXrmUniqueQuark(void)
{
  XrmQuark  r;
  wine_tsx11_lock();
  r = XrmUniqueQuark();
  wine_tsx11_unlock();
  return r;
}

int   TSXrmGetResource(XrmDatabase a0, const  char* a1, const  char* a2, char** a3, XrmValue* a4)
{
  int   r;
  wine_tsx11_lock();
  r = XrmGetResource(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

XrmDatabase  TSXrmGetFileDatabase(const  char* a0)
{
  XrmDatabase  r;
  wine_tsx11_lock();
  r = XrmGetFileDatabase(a0);
  wine_tsx11_unlock();
  return r;
}

XrmDatabase  TSXrmGetStringDatabase(const  char* a0)
{
  XrmDatabase  r;
  wine_tsx11_lock();
  r = XrmGetStringDatabase(a0);
  wine_tsx11_unlock();
  return r;
}

void  TSXrmMergeDatabases(XrmDatabase a0, XrmDatabase* a1)
{
  wine_tsx11_lock();
  XrmMergeDatabases(a0, a1);
  wine_tsx11_unlock();
}

void  TSXrmParseCommand(XrmDatabase* a0, XrmOptionDescList a1, int a2, const  char* a3, int* a4, char** a5)
{
  wine_tsx11_lock();
  XrmParseCommand(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
}


