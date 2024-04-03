/*
 * Unit test suite for file functions
 *
 * Copyright 2024 Eric Pouech for CodeWeavers
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

#include "wine/test.h"

static void test_std_stream_buffering(void)
{
    int dup_fd, ret, pos;
    FILE *file;
    char ch;

    dup_fd = _dup(STDOUT_FILENO);
    ok(dup_fd != -1, "_dup failed\n");

    file = freopen("std_stream_test.tmp", "w", stdout);
    ok(file != NULL, "freopen failed\n");

    ret = fprintf(stdout, "test");
    pos = _telli64(STDOUT_FILENO);

    fflush(stdout);
    _dup2(dup_fd, STDOUT_FILENO);
    close(dup_fd);
    setvbuf(stdout, NULL, _IONBF, 0);

    ok(ret == 4, "fprintf(stdout) returned %d\n", ret);
    ok(!pos, "expected stdout to be buffered\n");

    dup_fd = _dup(STDERR_FILENO);
    ok(dup_fd != -1, "_dup failed\n");

    file = freopen("std_stream_test.tmp", "w", stderr);
    ok(file != NULL, "freopen failed\n");

    ret = fprintf(stderr, "test");
    ok(ret == 4, "fprintf(stderr) returned %d\n", ret);
    pos = _telli64(STDERR_FILENO);
    if (broken(!GetProcAddress(GetModuleHandleA("ucrtbase"), "__CxxFrameHandler4") && !pos))
        trace("stderr is buffered\n");
    else
        ok(pos == 4, "expected stderr to be unbuffered (%d)\n", pos);

    fflush(stderr);
    _dup2(dup_fd, STDERR_FILENO);
    close(dup_fd);

    dup_fd = _dup(STDIN_FILENO);
    ok(dup_fd != -1, "_dup failed\n");

    file = freopen("std_stream_test.tmp", "r", stdin);
    ok(file != NULL, "freopen failed\n");

    ch = 0;
    ret = fscanf(stdin, "%c", &ch);
    ok(ret == 1, "fscanf returned %d\n", ret);
    ok(ch == 't', "ch = 0x%x\n", (unsigned char)ch);
    pos = _telli64(STDIN_FILENO);
    ok(pos == 4, "pos = %d\n", pos);

    fflush(stdin);
    _dup2(dup_fd, STDIN_FILENO);
    close(dup_fd);

    ok(DeleteFileA("std_stream_test.tmp"), "DeleteFile failed\n");
}

START_TEST(file)
{
    test_std_stream_buffering();
}
