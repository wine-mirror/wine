/*
 * Generate resource prototypes
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_GENRES_H
#define __WRC_GENRES_H

#ifndef __WRC_WRCTYPES_H
#include "wrctypes.h"
#endif

res_t *new_res(void);
res_t *grow_res(res_t *r, int add);
void put_byte(res_t *res, unsigned c);
void put_word(res_t *res, unsigned w);
void put_dword(res_t *res, unsigned d);
void resources2res(resource_t *top);
char *get_c_typename(enum res_e type);
char *make_c_name(char *base, name_id_t *nid, language_t *lan);
char *prep_nid_for_label(name_id_t *nid);

#endif
