/*
 * Module/Library loadorder
 *
 * Copyright 1999 Bertho Stultiens
 */

#ifndef __WINE_LOADORDER_H
#define __WINE_LOADORDER_H

#include "windef.h"

#define MODULE_LOADORDER_INVALID	0	/* Must be 0 */
#define MODULE_LOADORDER_DLL		1	/* Native DLLs */
#define MODULE_LOADORDER_ELFDLL		2	/* Elf-dlls */
#define MODULE_LOADORDER_SO		3	/* Native .so libraries */
#define MODULE_LOADORDER_BI		4	/* Built-in modules */
#define MODULE_LOADORDER_NTYPES		4

typedef struct module_loadorder {
	char	*modulename;
	char	loadorder[MODULE_LOADORDER_NTYPES];
} module_loadorder_t;

BOOL MODULE_InitLoadOrder(void);
module_loadorder_t *MODULE_GetLoadOrder(const char *path);

#endif

