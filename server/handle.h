/*
 * Server-side handle definitions
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#ifndef __WINE_SERVER_HANDLE_H
#define __WINE_SERVER_HANDLE_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include <stdlib.h>
#include "windef.h"

struct process;
struct object_ops;

/* handle functions */

/* alloc_handle takes a void *obj for convenience, but you better make sure */
/* that the thing pointed to starts with a struct object... */
extern int alloc_handle( struct process *process, void *obj,
                         unsigned int access, int inherit );
extern int close_handle( struct process *process, int handle );
extern struct object *get_handle_obj( struct process *process, int handle,
                                      unsigned int access, const struct object_ops *ops );
extern int duplicate_handle( struct process *src, int src_handle, struct process *dst,
                             unsigned int access, int inherit, int options );
extern int open_object( const WCHAR *name, size_t len, const struct object_ops *ops,
                        unsigned int access, int inherit );
extern struct object *alloc_handle_table( struct process *process, int count );
extern struct object *copy_handle_table( struct process *process, struct process *parent );
extern void close_global_handles(void);

#endif  /* __WINE_SERVER_HANDLE_H */
