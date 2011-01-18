/*
 * Interface to terminfo and termcap libraries
 *
 * Copyright 2010 Eric Pouech
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
#include "wine/port.h"

#ifdef HAVE_NCURSES_H
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#endif
/* avoid redefinition warnings */
#undef KEY_EVENT
#undef MOUSE_MOVED

#include <term.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wincon.h>
#include "console_private.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

#if defined(SONAME_LIBCURSES) || defined(SONAME_LIBNCURSES)

#ifdef HAVE_NCURSES_H
# define CURSES_NAME "ncurses"
#else
# define CURSES_NAME "curses"
#endif

static void *nc_handle = NULL;

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f;

MAKE_FUNCPTR(setupterm)

#undef MAKE_FUNCPTR

/**********************************************************************/

static BOOL TERM_bind_libcurses(void)
{
#ifdef SONAME_LIBNCURSES
    static const char ncname[] = SONAME_LIBNCURSES;
#else
    static const char ncname[] = SONAME_LIBCURSES;
#endif

    if (!(nc_handle = wine_dlopen(ncname, RTLD_NOW, NULL, 0)))
    {
        WINE_MESSAGE("Wine cannot find the " CURSES_NAME " library (%s).\n",
                     ncname);
        return FALSE;
    }

#define LOAD_FUNCPTR(f)                                      \
    if((p_##f = wine_dlsym(nc_handle, #f, NULL, 0)) == NULL) \
    {                                                        \
        WINE_WARN("Can't find symbol %s\n", #f);             \
        goto sym_not_found;                                  \
    }

    LOAD_FUNCPTR(setupterm)

#undef LOAD_FUNCPTR

    return TRUE;

sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the "
       CURSES_NAME "\nlibrary.  To enable Wine to use " CURSES_NAME
      " please upgrade your " CURSES_NAME "\nlibraries\n");
    wine_dlclose(nc_handle, NULL, 0);
    nc_handle = NULL;
    return FALSE;
}

#define setupterm p_setupterm

BOOL TERM_Init(void)
{
    if (!TERM_bind_libcurses()) return FALSE;
    if (setupterm(NULL, 1 /* really ?? */, NULL) == -1) return FALSE;
    return TRUE;
}

BOOL TERM_Exit(void)
{
    return TRUE;
}
#else
BOOL     TERM_Init(void) {return FALSE;}
BOOL     TERM_Exit(void) {return FALSE;}
#endif
