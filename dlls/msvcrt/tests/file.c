/*
 * Unit test suite for file functions
 *
 * Copyright 2002 Bill Currie
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

#include <windef.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "wine/test.h"

static void test_fdopen( void )
{
    static char buffer[] = {0,1,2,3,4,5,6,7,8,9};
	int fd;
	FILE *file;

	fd = open ("fdopen.tst", O_WRONLY | O_CREAT | O_BINARY);
	write (fd, buffer, sizeof (buffer));
	close (fd);

	fd = open ("fdopen.tst", O_RDONLY | O_BINARY);
	lseek (fd, 5, SEEK_SET);
	file = fdopen (fd, "rb");
	ok (fread (buffer, 1, sizeof (buffer), file) == 5, "read wrong byte count");
	ok (memcmp (buffer, buffer + 5, 5) == 0, "read wrong bytes");
	fclose (file);
	unlink ("fdopen.tst");
}


START_TEST(file)
{
    test_fdopen();
}
