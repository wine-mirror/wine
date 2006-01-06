/*
 * Unit test suite for dir functions
 *
 * Copyright 2006 CodeWeavers, Aric Stewart
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <process.h>
#include <errno.h>

void test_fullpath()
{
    char full[MAX_PATH];
    char *freeme;

    SetCurrentDirectory("C:\\Windows\\System\\");

    ok(_fullpath(full,"test", MAX_PATH)>0,"_fullpath failed\n");
    ok(strcmp(full,"C:\\Windows\\System\\test")==0,"Invalid Path\n");
    ok(_fullpath(full,"\\test", MAX_PATH)>0,"_fullpath failed\n");
    ok(strcmp(full,"C:\\test")==0,"Invalid Path\n");
    ok(_fullpath(full,"..\\test", MAX_PATH)>0,"_fullpath failed\n");
    ok(strcmp(full,"C:\\Windows\\test")==0,"Invalid Path\n");
    ok(_fullpath(full,"..\\test", 10)==NULL,"_fullpath failed to generate error\n");

    freeme = _fullpath(NULL,"test", 0);
    ok(freeme!=NULL,"No path returned\n");
    ok(strcmp(freeme,"C:\\Windows\\System\\test")==0,"Invalid Path\n");
    free(freeme);
}

START_TEST(dir)
{
    test_fullpath();
}
