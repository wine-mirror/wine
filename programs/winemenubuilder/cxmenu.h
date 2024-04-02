/*
 * Header for the CrossOver menu management scripts.
 *
 * Copyright 2012 Francois Gouget
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
#ifndef _CXMENU_
#define _CXMENU_

/* Some global variables */
extern int cx_mode;
extern int cx_dump_menus;

/* Functions used by the cxmenu backend */
char* wchars_to_utf8_chars(LPCWSTR string);

/* The backend functions */
int cx_process_menu(LPCWSTR linkW, BOOL is_desktop, DWORD root_csidl,
                    LPCWSTR pathW, LPCWSTR argsW,
                    LPCWSTR icon_name, LPCWSTR description, IShellLinkW *sl);

/* Functions used by the winemenubuilder extensions */
BOOL Process_Link( LPCWSTR linkname, BOOL bWait );
BOOL Process_URL( LPCWSTR urlname, BOOL bWait );

/* The winemenubuilder extensions */
BOOL cx_process_all_menus(void);

#endif /* _CXMENU_ */
