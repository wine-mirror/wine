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

#include <time.h>	/* For time_t */

#define WRC_MAJOR_VERSION	1
#define WRC_MINOR_VERSION	1
#define WRC_MICRO_VERSION	9
#define WRC_RELEASEDATE		"(31-Dec-2000)"

#define WRC_STRINGIZE(a)	#a
#define WRC_EXP_STRINGIZE(a)	WRC_STRINGIZE(a)
#define WRC_VERSIONIZE(a,b,c)	WRC_STRINGIZE(a) "." WRC_STRINGIZE(b) "." WRC_STRINGIZE(c)  
#define WRC_VERSION		WRC_VERSIONIZE(WRC_MAJOR_VERSION, WRC_MINOR_VERSION, WRC_MICRO_VERSION)
#define WRC_FULLVERSION 	WRC_VERSION " " WRC_RELEASEDATE

#ifndef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif

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
extern int byteorder;
extern int preprocess_only;
extern int no_preprocess;
extern int remap;

extern char *prefix;
extern char *output_name;
extern char *input_name;
extern char *header_name;
extern char *cmdline;			
extern time_t now;

extern int line_number;
extern int char_number;

extern resource_t *resource_top;
extern language_t *currentlanguage;

#endif
