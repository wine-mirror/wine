/*
 * Format String Generator for IDL Compiler
 *
 * Copyright 2005 Eric Kohl
 * Copyright 2005 Robert Shearman
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


enum pass
{
    PASS_IN,
    PASS_OUT,
    PASS_RETURN
};

enum remoting_phase
{
    PHASE_BUFFERSIZE,
    PHASE_MARSHAL,
    PHASE_UNMARSHAL,
    PHASE_FREE
};

void write_procformatstring(FILE *file, type_t *iface);
void write_typeformatstring(FILE *file, type_t *iface);
unsigned int get_required_buffer_size(const var_t *var, unsigned int *alignment);
void write_remoting_arguments(FILE *file, int indent, const func_t *func, unsigned int *type_offset, enum pass pass, enum remoting_phase phase);
size_t get_size_procformatstring_var(const var_t *var);
size_t get_size_typeformatstring_var(const var_t *var);
int write_expr_eval_routines(FILE *file, const char *iface);
void write_expr_eval_routine_list(FILE *file, const char *iface);
