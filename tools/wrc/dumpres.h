/*
 * Dump resource prototypes
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_DUMPRES_H
#define __WRC_DUMPRES_H

#ifndef __WRC_WRCTYPES_H
#include "wrctypes.h"
#endif

char *get_typename(resource_t* r);
void dump_resources(resource_t *top);
char *get_nameid_str(name_id_t *n);

#endif
