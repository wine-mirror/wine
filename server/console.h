/*
 * Wine server consoles
 *
 * Copyright (C) 2001 Eric Pouech
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
};

/* console functions */

extern void inherit_console(struct thread *parent_thread, struct process *process, handle_t hconin);
extern int free_console( struct process *process );

#endif  /* __WINE_SERVER_CONSOLE_H */
