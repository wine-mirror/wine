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

#ifndef __WIDL_HEADER_H
#define __WIDL_HEADER_H

extern int is_attr(const attr_t *a, enum attr_type t);
extern void *get_attrp(const attr_t *a, enum attr_type t);
extern unsigned long get_attrv(const attr_t *a, enum attr_type t);
extern int is_void(const type_t *t, const var_t *v);
extern void write_name(FILE *h, const var_t *v);
extern const char* get_name(const var_t *v);
extern void write_type(FILE *h, type_t *t, const var_t *v, const char *n);
extern int is_object(attr_t *a);
extern int is_local(attr_t *a);
extern const var_t *is_callas(attr_t *a);
extern void write_args(FILE *h, var_t *arg, const char *name, int obj, int do_indent);
extern void write_array(FILE *h, const expr_t *v, int field);
extern void write_forward(type_t *iface);
extern void write_interface(type_t *iface);
extern void write_dispinterface(type_t *iface);
extern void write_coclass(class_t *iface);
extern void write_typedef(type_t *type, const var_t *names);
extern void write_expr(FILE *h, const expr_t *e, int brackets);
extern void write_constdef(const var_t *v);
extern void write_externdef(const var_t *v);
extern void write_library(const char *name, attr_t *attr);
extern void write_user_types(void);
extern const var_t* get_explicit_handle_var(const func_t* func);

static inline int is_string_type(const attr_t *attrs, int ptr_level, const expr_t *array)
{
    return (is_attr(attrs, ATTR_STRING) &&
            ((ptr_level == 1 && !array) || (ptr_level == 0 && array)));
}

static inline int is_array_type(const attr_t *attrs, int ptr_level, const expr_t *array)
{
    return ((ptr_level == 1 && !array && is_attr(attrs, ATTR_SIZEIS)) ||
            (ptr_level == 0 && array));
}

#endif
