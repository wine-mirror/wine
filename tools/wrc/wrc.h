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

#define WRC_VERSION	"1.0.3"
#define WRC_RELEASEDATE	"(02-Nov-1998)"
#define WRC_FULLVERSION WRC_VERSION " " WRC_RELEASEDATE

/* Only used in heavy debugging sessions */
#define HEAPCHECK()

/* Memory/load flags */
#define WRC_MO_MOVEABLE		0x0010
#define WRC_MO_PURE		0x0020
#define WRC_MO_PRELOAD		0x0040
#define WRC_MO_DISCARDABLE	0x1000

/* Resource type IDs */
#define WRC_RT_CURSOR		(1)
#define WRC_RT_BITMAP		(2)
#define WRC_RT_ICON		(3)
#define WRC_RT_MENU		(4)
#define WRC_RT_DIALOG		(5)
#define WRC_RT_STRING		(6)
#define WRC_RT_FONTDIR		(7)
#define WRC_RT_FONT		(8)
#define WRC_RT_ACCELERATOR	(9)
#define WRC_RT_RCDATA		(10)
#define WRC_RT_MESSAGETABLE	(11)
#define WRC_RT_GROUP_CURSOR	(12)
#define WRC_RT_GROUP_ICON	(14)
#define WRC_RT_VERSION		(16)
#define WRC_RT_DLGINCLUDE	(17)
#define WRC_RT_PLUGPLAY		(19)
#define WRC_RT_VXD		(20)
#define WRC_RT_ANICURSOR	(21)
#define WRC_RT_ANIICON		(22)

/* Default class type IDs */
#define CT_BUTTON	0x80
#define CT_EDIT		0x81
#define CT_STATIC 	0x82
#define CT_LISTBOX	0x83
#define CT_SCROLLBAR	0x84
#define CT_COMBOBOX	0x85

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
