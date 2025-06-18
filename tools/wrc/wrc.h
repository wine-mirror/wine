/*
 * Main definitions and externals
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
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

#ifndef __WRC_WRC_H
#define __WRC_WRC_H

#include "wrctypes.h"

/* From wrc.c */
extern int debuglevel;
#define DEBUGLEVEL_NONE		0x0000
#define DEBUGLEVEL_CHAT		0x0001
#define DEBUGLEVEL_DUMP		0x0002
#define DEBUGLEVEL_TRACE	0x0004
#define DEBUGLEVEL_PPMSG	0x0008
#define DEBUGLEVEL_PPLEX	0x0010
#define DEBUGLEVEL_PPTRACE	0x0020

extern int win32;
extern int extensions;
extern int pedantic;
extern int preprocess_only;
extern int no_preprocess;
extern int utf8_input;
extern int check_utf8;

extern const char *input_name;
extern const char *nlsdirs[];

extern int line_number;
extern int char_number;

extern resource_t *resource_top;
extern language_t currentlanguage;

void write_pot_file( const char *outname );
void write_po_files( const char *outname );
void add_translations( const char *po_dir );
void write_resfile(char *outname, resource_t *top);

static inline void set_location( location_t *loc )
{
    loc->file = input_name;
    loc->line = line_number;
    loc->col  = char_number;
}

static inline void print_location( const location_t *loc )
{
    if (loc->file) fprintf(stderr, "%s:%d:%d: ", loc->file, loc->line, loc->col );
}

#endif
