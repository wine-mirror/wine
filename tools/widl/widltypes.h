/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WIDL_WIDLTYPES_H
#define __WIDL_WIDLTYPES_H

#include "windef.h"
#include "oleauto.h"
#include "wine/rpcfc.h"

typedef struct _attr_t attr_t;
typedef struct _type_t type_t;
typedef struct _typeref_t typeref_t;
typedef struct _var_t var_t;
typedef struct _func_t func_t;

#define DECL_LINK(type) \
  type *l_next; \
  type *l_prev;

#define LINK(x,y) do { x->l_next = y; if (y) y->l_prev = x; } while (0)
#define LINK_LAST(x,y) do { if (y) { typeof(x) _c = x; while (_c->l_next) _c = _c->l_next; LINK(_c, y); } } while (0)
#define LINK_SAFE(x,y) do { if (x) LINK_LAST(x,y); else { x = y; } } while (0)

#define INIT_LINK(x) do { x->l_next = NULL; x->l_prev = NULL; } while (0)
#define NEXT_LINK(x) ((x)->l_next)
#define PREV_LINK(x) ((x)->l_prev)

struct _attr_t {
  int type;
  union {
    DWORD ival;
    void *pval;
  } u;
  /* parser-internal */
  DECL_LINK(attr_t)
};

struct _type_t {
  char *name;
  BYTE type;
  struct _type_t *ref;
  char *rname;
  attr_t *attrs;
  func_t *funcs;
  var_t *fields;
  int ignore, is_const;
  int defined, written;

  /* parser-internal */
  DECL_LINK(type_t)
};

struct _typeref_t {
  char *name;
  type_t *ref;
  int uniq;
};

struct _var_t {
  char *name;
  int ptr_level;
  type_t *type;
  char *tname;
  attr_t *attrs;

  /* parser-internal */
  DECL_LINK(var_t)
};

struct _func_t {
  var_t *def;
  var_t *args;
  int ignore, idx;

  /* parser-internal */
  DECL_LINK(func_t)
};

#endif
