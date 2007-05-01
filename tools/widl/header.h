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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WIDL_HEADER_H
#define __WIDL_HEADER_H

extern int is_attr(const attr_list_t *list, enum attr_type t);
extern void *get_attrp(const attr_list_t *list, enum attr_type t);
extern unsigned long get_attrv(const attr_list_t *list, enum attr_type t);
extern int is_void(const type_t *t, const var_t *v);
extern int is_conformant_array( const array_dims_t *array );
extern int is_non_void(const expr_list_t *list);
extern void write_name(FILE *h, const var_t *v);
extern void write_prefix_name(FILE *h, const char *prefix, const var_t *v);
extern const char* get_name(const var_t *v);
extern void write_type(FILE *h, type_t *t, const var_t *v);
extern int is_object(const attr_list_t *list);
extern int is_local(const attr_list_t *list);
extern const var_t *is_callas(const attr_list_t *list);
extern void write_args(FILE *h, const var_list_t *arg, const char *name, int obj, int do_indent);
extern void write_array(FILE *h, array_dims_t *v, int field);
extern void write_forward(type_t *iface);
extern void write_interface(type_t *iface);
extern void write_dispinterface(type_t *iface);
extern void write_coclass(type_t *cocl);
extern void write_coclass_forward(type_t *cocl);
extern void write_typedef(type_t *type);
extern void write_expr(FILE *h, const expr_t *e, int brackets);
extern void write_constdef(const var_t *v);
extern void write_externdef(const var_t *v);
extern void write_library(const char *name, const attr_list_t *attr);
extern void write_user_types(void);
extern const var_t* get_explicit_handle_var(const func_t* func);
extern int has_out_arg_or_return(const func_t *func);
extern void write_guid(FILE *f, const char *guid_prefix, const char *name,
                       const UUID *uuid);

static inline int is_string_type(const attr_list_t *attrs, int ptr_level, const array_dims_t *array)
{
    return (is_attr(attrs, ATTR_STRING) &&
            ((ptr_level == 1 && !array) || (ptr_level == 0 && array)));
}

static inline int is_array_type(const attr_list_t *attrs, int ptr_level, const array_dims_t *array)
{
    return ((ptr_level == 1 && !array && is_attr(attrs, ATTR_SIZEIS)) ||
            (ptr_level == 0 && array));
}

#endif
