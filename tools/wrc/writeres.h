/*
 * Write resource prototypes
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_WRITERES_H
#define __WRC_WRITERES_H

#ifndef __WRC_WRCTYPES_H
#include "wrctypes.h"
#endif

void write_resfile(char *outname, resource_t *top);
void write_s_file(char *outname, resource_t *top);
void write_h_file(char *outname, resource_t *top);

#endif
