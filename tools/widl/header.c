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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "proxy.h"
#include "y.tab.h"

static int indentation = 0;

static void indent(int delta)
{
  int c;
  if (delta < 0) indentation += delta;
  for (c=0; c<indentation; c++) fprintf(header, "    ");
  if (delta > 0) indentation += delta;
}

int is_void(type_t *t, var_t *v)
{
  if (v && v->ptr_level) return 0;
  if (!t->type && !t->ref) return 1;
  return 0;
}

static void write_pident(var_t *v)
{
  int c;
  for (c=0; c<v->ptr_level; c++) {
    fprintf(header, "*");
  }
  if (v->name) fprintf(header, "%s", v->name);
}

void write_name(FILE *h, var_t *v)
{
  fprintf(h, "%s", v->name);
}

char* get_name(var_t *v)
{
  return v->name;
}

static void write_fields(FILE *h, var_t *v)
{
  if (!v) return;
  while (NEXT_LINK(v)) v = NEXT_LINK(v);
  while (v) {
    if (v->type) {
      indent(0);
      write_type(h, v->type, NULL, v->tname);
      if (get_name(v)) {
        fprintf(header, " ");
        write_pident(v);
      }
      fprintf(header, ";\n");
    }
    v = PREV_LINK(v);
  }
}

void write_type(FILE *h, type_t *t, var_t *v, char *n)
{
  int c;

  if (n) fprintf(h, "%s", n);
  else {
    if (t->is_const) fprintf(h, "const ");
    if (t->type) {
      switch (t->type) {
      case RPC_FC_BYTE:
        fprintf(h, "byte");
        break;
      case RPC_FC_CHAR:
        fprintf(h, "char");
        break;
      case RPC_FC_USHORT:
        fprintf(h, "unsigned ");
      case RPC_FC_SHORT:
        if (t->ref) fprintf(h, t->ref->name);
        fprintf(h, "short");
        break;
      case RPC_FC_ULONG:
        fprintf(h, "unsigned ");
      case RPC_FC_LONG:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "long");
        break;
      case RPC_FC_STRUCT:
        if (t->defined && !t->written) {
          if (t->name) fprintf(h, "struct %s {\n", t->name);
          else fprintf(h, "struct {\n");
          indentation++;
          write_fields(h, t->fields);
          indent(-1);
          fprintf(h, "}");
        }
        else fprintf(h, "struct %s", t->name);
        break;
      case RPC_FC_NON_ENCAPSULATED_UNION:
        if (t->defined && !t->written) {
          if (t->name) fprintf(h, "union %s {\n", t->name);
          else fprintf(h, "union {\n");
          indentation++;
          write_fields(h, t->fields);
          indent(-1);
          fprintf(h, "}");
        }
        else fprintf(h, "union %s", t->name);
        break;
      default:
        fprintf(h, "(unknown-type:%d)", t->type);
      }
    }
    else {
      if (t->ref) {
        write_type(h, t->ref, NULL, t->name);
      }
      else fprintf(h, "void");
    }
  }
  if (v) {
    for (c=0; c<v->ptr_level; c++) {
      fprintf(h, "*");
    }
  }
}

void write_typedef(type_t *type, var_t *names)
{
  char *tname = names->tname;
  while (NEXT_LINK(names)) names = NEXT_LINK(names);
  fprintf(header, "typedef ");
  write_type(header, type, NULL, tname);
  fprintf(header, " ");
  while (names) {
    write_pident(names);
    if (PREV_LINK(names))
      fprintf(header, ", ");
    names = PREV_LINK(names);
  }
  fprintf(header, ";\n");
}

/********** INTERFACES **********/

int is_object(attr_t *a)
{
  while (a) {
    if (a->type == tOBJECT) return 1;
    a = NEXT_LINK(a);
  }
  return 0;
}

int is_local(attr_t *a)
{
  while (a) {
    if (a->type == tLOCAL) return 1;
    a = NEXT_LINK(a);
  }
  return 0;
}

var_t *is_callas(attr_t *a)
{
  while (a) {
    if (a->type == tCALLAS) return a->u.pval;
    a = NEXT_LINK(a);
  }
  return NULL;
}

static void write_method_def(type_t *iface)
{
  func_t *cur = iface->funcs;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
  fprintf(header, "#define %s_METHODS", iface->name);
  while (cur) {
    var_t *def = cur->def;
    if (!is_callas(def->attrs)) {
      var_t *arg = cur->args;
      int argc = 0;
      if (arg) {
	argc++;
	while (NEXT_LINK(arg)) {
	  arg = NEXT_LINK(arg);
	  argc++;
	}
      }
      fprintf(header, " \\\n");
      if (!is_void(def->type, def)) {
	if (argc)
	  fprintf(header, "    ICOM_METHOD%d (", argc);
	else
	  fprintf(header, "    ICOM_METHOD  (");
	write_type(header, def->type, def, def->tname);
	fprintf(header, ",");
      } else
	if (argc)
	  fprintf(header, "    ICOM_VMETHOD%d(", argc);
	else
	  fprintf(header, "    ICOM_VMETHOD (");
      write_name(header, def);
      while (arg) {
	fprintf(header, ",");
	write_type(header, arg->type, arg, arg->tname);
	fprintf(header, ",");
	write_name(header,arg);
	arg = PREV_LINK(arg);
      }
      fprintf(header, ")");
    }
    cur = PREV_LINK(cur);
  }
  fprintf(header, "\n");
}

static int write_method_macro(type_t *iface, char *name)
{
  int idx;
  func_t *cur = iface->funcs;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);

  if (iface->ref) idx = write_method_macro(iface->ref, name);
  else idx = 0;
  fprintf(header, "/*** %s methods ***/\n", iface->name);
  while (cur) {
    var_t *def = cur->def;
    if (!is_callas(def->attrs)) {
      var_t *arg = cur->args;
      int argc = 0;
      int c;
      while (arg) {
	arg = NEXT_LINK(arg);
	argc++;
      }

      fprintf(header, "#define %s_", name);
      write_name(header,def);
      fprintf(header, "(p");
      for (c=0; c<argc; c++)
	fprintf(header, ",%c", c+'a');
      fprintf(header, ") ");

      if (argc)
	fprintf(header, "ICOM_CALL%d(", argc);
      else
	fprintf(header, "ICOM_CALL(");
      write_name(header,def);
      fprintf(header, ",p");
      for (c=0; c<argc; c++)
	fprintf(header, ",%c", c+'a');
      fprintf(header, ")\n");
      if (cur->idx == -1) cur->idx = idx;
      else if (cur->idx != idx) yyerror("BUG: method index mismatch in write_method_macro");
      idx++;
    }
    cur = PREV_LINK(cur);
  }
  return idx;
}

void write_method_args(FILE *h, var_t *arg, char *name)
{
  if (arg) {
    while (NEXT_LINK(arg))
      arg = NEXT_LINK(arg);
  }
  fprintf(h, "    %s* This", name);
  while (arg) {
    fprintf(h, ",\n    ");
    write_type(h, arg->type, arg, arg->tname);
    fprintf(h, " ");
    write_name(h,arg);
    arg = PREV_LINK(arg);
  }
}

static void write_method_proto(type_t *iface)
{
  func_t *cur = iface->funcs;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
  while (cur) {
    var_t *def = cur->def;
    var_t *cas = is_callas(def->attrs);
    if (!is_local(def->attrs)) {
      /* proxy prototype */
      write_type(header, def->type, def, def->tname);
      fprintf(header, " CALLBACK %s_", iface->name);
      write_name(header,def);
      fprintf(header, "_Proxy(\n");
      write_method_args(header, cur->args, iface->name);
      fprintf(header, ");\n");
      /* stub prototype */
      fprintf(header, "void __RPC_STUB %s_", iface->name);
      write_name(header,def);
      fprintf(header, "_Stub(\n");
      fprintf(header, "    IRpcStubBuffer* This,\n");
      fprintf(header, "    IRpcChannelBuffer* pRpcChannelBuffer,\n");
      fprintf(header, "    PRPC_MESSAGE pRpcMessage,\n");
      fprintf(header, "    DWORD* pdwStubPhase);\n");
    }
    if (cas) {
      func_t *m = iface->funcs;
      while (m && strcmp(get_name(m->def), cas->name))
        m = NEXT_LINK(m);
      if (m) {
        var_t *mdef = m->def;
        /* proxy prototype - use local prototype */
        write_type(header, mdef->type, mdef, mdef->tname);
        fprintf(header, " CALLBACK %s_", iface->name);
        write_name(header, mdef);
        fprintf(header, "_Proxy(\n");
        write_method_args(header, m->args, iface->name);
        fprintf(header, ");\n");
        /* stub prototype - use remotable prototype */
        write_type(header, def->type, def, def->tname);
        fprintf(header, " __RPC_STUB %s_", iface->name);
        write_name(header, mdef);
        fprintf(header, "_Stub(\n");
        write_method_args(header, cur->args, iface->name);
        fprintf(header, ");\n");
      }
      else {
        yywarning("invalid call_as attribute (%s -> %s)\n", get_name(def), cas->name);
      }
    }

    cur = PREV_LINK(cur);
  }
}

void write_forward(type_t *iface)
{
  if (!iface->written) {
    fprintf(header, "typedef struct %s %s;\n", iface->name, iface->name);
    iface->written = TRUE;
  }
}

void write_interface(type_t *iface)
{
  if (!is_object(iface->attrs)) {
    if (!iface->funcs) return;
    yywarning("RPC interfaces not supported yet\n");
    return;
  }

  if (!iface->funcs) {
    yywarning("%s has no methods", iface->name);
    return;
  }

  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s interface\n", iface->name);
  fprintf(header, " */\n");
  write_forward(iface);
  if (iface->ref)
    fprintf(header, "#define ICOM_INTERFACE %s\n", iface->name);
  write_method_def(iface);
  fprintf(header, "#define %s_IMETHODS \\\n", iface->name);
  if (iface->ref)
    fprintf(header, "    %s_IMETHODS \\\n", iface->ref->name);
  fprintf(header, "    %s_METHODS \\\n", iface->name);
  if (iface->ref) {
    fprintf(header, "ICOM_DEFINE(%s,%s)\n", iface->name, iface->ref->name);
    fprintf(header, "#undef ICOM_INTERFACE\n");
  }
  fprintf(header, "\n");
  write_method_macro(iface, iface->name);
  fprintf(header, "\n");
  write_method_proto(iface);
  fprintf(header, "\n");

  if (!is_local(iface->attrs))
    write_proxy(iface);
}
