/*
 * Main definitions and externals
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_WRC_H
#define __WRC_WRC_H

#ifndef __WRC_WRCTYPES_H
#include "wrctypes.h"
#endif

#define WRC_VERSION	"1.0.12"
#define WRC_RELEASEDATE	"(18-Jul-1999)"
#define WRC_FULLVERSION WRC_VERSION " " WRC_RELEASEDATE

/* Only used in heavy debugging sessions */
#define HEAPCHECK()

/* From wrc.c */
extern int debuglevel;
#define DEBUGLEVEL_NONE		0x0000
#define DEBUGLEVEL_CHAT		0x0001
#define DEBUGLEVEL_DUMP		0x0002
#define DEBUGLEVEL_TRACE	0x0004

extern int win32;
extern int constant;
extern int create_res;
extern int extensions;
extern int binary;
extern int create_header;
extern int create_dir;
extern int global;
extern int indirect;
extern int indirect_only;
extern int alignment;
extern int alignment_pwr;
extern int create_s;
extern DWORD codepage;
extern int pedantic;
extern int auto_register;
extern int leave_case;

extern char *prefix;
extern char *output_name;
extern char *input_name;
extern char *header_name;
extern char *cmdline;			

extern resource_t *resource_top;
extern language_t *currentlanguage;

#endif
