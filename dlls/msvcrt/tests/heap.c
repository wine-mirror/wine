/*
 * Unit test suite for memory functions
 *
 * Copyright 2003 Dimitrie O. Paun
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

#include "wine/test.h"
#include <stdlib.h>

static void test_realloc( void )
{
    void *mem = NULL;

    mem = realloc(mem, 10);
    ok(mem != NULL, "memory not allocated");
    
    mem = realloc(mem, 20);
    ok(mem != NULL, "memory not reallocated");
 
    mem = realloc(mem, 0);
    ok(mem == NULL, "memory nto freed");
}

START_TEST(heap)
{
    test_realloc();
}
