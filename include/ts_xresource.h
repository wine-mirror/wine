/*
 * Thread safe wrappers around Xresource calls.
 * Always include this file instead of <X11/Xresource.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSXRESOURCE_H
#define __WINE_TSXRESOURCE_H

#include "config.h"

#ifndef X_DISPLAY_MISSING


#include <X11/Xlib.h>
#include <X11/Xresource.h>

extern XrmQuark  TSXrmUniqueQuark(void);
extern int   TSXrmGetResource(XrmDatabase, const  char*, const  char*, char**, XrmValue*);
extern XrmDatabase  TSXrmGetFileDatabase(const  char*);
extern XrmDatabase  TSXrmGetStringDatabase(const  char*);
extern void  TSXrmMergeDatabases(XrmDatabase, XrmDatabase*);
extern void  TSXrmParseCommand(XrmDatabase*, XrmOptionDescList, int, const  char*, int*, char**);


#endif /* !defined(X_DISPLAY_MISSING) */

#endif /* __WINE_TSXRESOURCE_H */
