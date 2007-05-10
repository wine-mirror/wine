/*
 * Wine server consoles
 *
 * Copyright (C) 2001 Eric Pouech
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

#ifndef __WINE_SERVER_CONSOLE_H
#define __WINE_SERVER_CONSOLE_H

#include "wincon.h"

struct screen_buffer;
struct console_input_events;

struct console_input
{
    struct object                obj;           /* object header */
    int                          num_proc;      /* number of processes attached to this console */
    struct thread               *renderer;      /* console renderer thread */
    int                          mode;          /* input mode */
    struct screen_buffer        *active;        /* active screen buffer */
    int                          recnum;        /* number of input records */
    INPUT_RECORD                *records;       /* input records */
    struct console_input_events *evt;           /* synchronization event with renderer */
    WCHAR                       *title;         /* console title */
    WCHAR                      **history;       /* lines history */
    int                          history_size;  /* number of entries in history array */
    int                          history_index; /* number of used entries in history array */
    int                          history_mode;  /* mode of history (non zero means remove doubled strings */
    int                          edition_mode;  /* index to edition mode flavors */
    int                          input_cp;      /* console input codepage */
    int                          output_cp;     /* console output codepage */
    struct event                *event;         /* event to wait on for input queue */
};

/* console functions */

extern void inherit_console(struct thread *parent_thread, struct process *process, obj_handle_t hconin);
extern int free_console( struct process *process );

#endif  /* __WINE_SERVER_CONSOLE_H */
