/*
 * Copyright 2026 Nikolay Sivov for CodeWeavers
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
 *
 */

#include <stdio.h>

struct xmlregex;
struct xmlregex_state;
struct xmlregex_parser_context;
struct xmlregex_exec_context;

extern struct xmlregex_parser_context * xmlregex_new_fa(void);
extern void xmlregex_free_parser_context(struct xmlregex_parser_context *ctxt);

extern struct xmlregex_state * xmlregex_parser_get_init_state(struct xmlregex_parser_context *ctxt);
extern int xmlregex_parser_set_final_state(struct xmlregex_parser_context *ctxt, struct xmlregex_state *state);

extern struct xmlregex_state * xmlregex_parser_push_state(struct xmlregex_parser_context *ctxt);

extern struct xmlregex_state * xmlregex_parser_new_transition(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state * from, struct xmlregex_state * to, const WCHAR *token, void *data);

extern struct xmlregex_state * xmlregex_parser_new_epsilon(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state * from, struct xmlregex_state * to);

extern struct xmlregex * xmlregex_parser_compile(struct xmlregex_parser_context *ctxt);

extern void xmlregex_free(struct xmlregex *regexp);
extern int xmlregex_is_determinist(struct xmlregex *comp);

typedef void (*xmlregex_exec_callbacks)(struct xmlregex_exec_context *exec, const WCHAR *token,
        void *transdata, void *inputdata);

extern struct xmlregex_exec_context * xmlregex_new_exec_context(struct xmlregex *comp,
        xmlregex_exec_callbacks callback, void *data);
extern void xmlregex_free_exec_context(struct xmlregex_exec_context *exec);
extern int xmlregex_exec_push_string(struct xmlregex_exec_context *exec, const WCHAR *value, void *data);
