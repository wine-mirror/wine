/*
 * Unit test suite for debug functions.
 *
 * Copyright 2004 Patrik Stridvall
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "wine/test.h"

/**********************************************************************/

static void * (*pMSVCRTD_operator_new_dbg)(unsigned long, int, const char *, int) = NULL;

/* Some exports are only available in later versions */
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hModule,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)

static int init_functions(void)
{
  HMODULE hModule = LoadLibraryA("msvcrtd.dll");
  ok(hModule != NULL, "LoadLibraryA failed\n");

  if (!hModule) 
    return FALSE;

  SET(pMSVCRTD_operator_new_dbg, "??2@YAPAXIHPBDH@Z");
  if (pMSVCRTD_operator_new_dbg == NULL)
    return FALSE;

  return TRUE;
}

/**********************************************************************/

static void test_new(void)
{
  void *mem;

  mem = pMSVCRTD_operator_new_dbg(42, 0, __FILE__, __LINE__);
  ok(mem != NULL, "memory not allocated\n");
}

/**********************************************************************/

START_TEST(debug)
{
  if (!init_functions()) 
    return;

  test_new();
}
