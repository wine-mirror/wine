/*
 * Useful functions for winegcc/winewrap
 *
 * Copyright 2000 Francois Gouget
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 2003 Richard Cohen
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


void error(const char *s, ...);

void *xmalloc(size_t size);
void *xrealloc(void* p, size_t size);
char *strmake(const char *fmt, ...);

typedef struct {
    size_t maximum;
    size_t size;
    const char** base;
} strarray;

strarray *strarray_alloc(void);
void strarray_free(strarray* arr);
void strarray_add(strarray* arr, const char* str);

void spawn(const strarray* arr);

extern int verbose;
