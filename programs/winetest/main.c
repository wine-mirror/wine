/*
 * Wine Conformance Test EXE
 *
 * Copyright 2003 Jakob Eriksson   (for Solid Form Sweden AB)
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Ferenc Wagner
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
 *
 * This program is dedicated to Anna Lindh,
 * Swedish Minister of Foreign Affairs.
 * Anna was murdered September 11, 2003.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <windows.h>

#include "winetest.h"

struct wine_test
{
    char *name;
    int resource;
    int subtest_count;
    char **subtests;
    int is_elf;
    char *exename;
};

static struct wine_test wine_tests[32];

static const char *wineloader;

void print_version ()
{
    OSVERSIONINFOEX ver;
    BOOL ext;

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!(ext = GetVersionEx ((OSVERSIONINFO *) &ver)))
    {
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx ((OSVERSIONINFO *) &ver))
	    fatal("Can't get OS version.");
    }

    xprintf ("    dwMajorVersion=%ld\n    dwMinorVersion=%ld\n"
             "    dwBuildNumber=%ld\n    PlatformId=%ld\n    szCSDVersion=%s\n",
             ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
             ver.dwPlatformId, ver.szCSDVersion);

    if (!ext) return;

    xprintf ("    wServicePackMajor=%d\n    wServicePackMinor=%d\n"
             "    wSuiteMask=%d\n    wProductType=%d\n    wReserved=%d\n",
             ver.wServicePackMajor, ver.wServicePackMinor, ver.wSuiteMask,
             ver.wProductType, ver.wReserved);
}

static inline int is_dot_dir(const char* x)
{
    return ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))));
}

void remove_dir (const char *dir)
{
    HANDLE  hFind;
    WIN32_FIND_DATA wfd;
    char path[MAX_PATH];
    size_t dirlen = strlen (dir);

    /* Make sure the directory exists before going further */
    memcpy (path, dir, dirlen);
    strcpy (path + dirlen++, "\\*");
    hFind = FindFirstFile (path, &wfd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        char *lp = wfd.cFileName;

        if (!lp[0]) lp = wfd.cAlternateFileName; /* ? FIXME not (!lp) ? */
        if (is_dot_dir (lp)) continue;
        strcpy (path + dirlen, lp);
        if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
            remove_dir(path);
        else if (!DeleteFile (path))
            warning (strmake (NULL, "Can't delete file %s: error %d", path, GetLastError ()));
    } while (FindNextFile (hFind, &wfd));
    FindClose (hFind);
    if (!RemoveDirectory (dir))
        warning (strmake (NULL, "Can't remove directory %s: error %d", dir, GetLastError ()));
}

void* extract_rcdata (int id, DWORD* size)
{
    HRSRC rsrc;
    HGLOBAL hdl;

    rsrc = FindResource (0, (LPTSTR)(id + 1), "USERDATA");
    if (!rsrc) return 0;
    *size = SizeofResource (0, rsrc);
    if (!*size) return 0;
    hdl = LoadResource (0, rsrc);
    if (!hdl) return 0;
    return LockResource (hdl);
}

int extract_test (const char *dir, int id)
{
    BYTE* code;
    DWORD size;
    FILE* fout;
    char buffer[128];
    int len;
    struct wine_test *test;

    if (id >= sizeof(wine_tests)/sizeof(wine_tests[0])-1) fatal("Too many tests\n");

    code = extract_rcdata (id, &size);
    if (!code) return 0;

    test = &wine_tests[id];
    len = LoadStringA(0, id + 1, buffer, sizeof(buffer) );
    test->name = xmalloc( len + 1 );
    memcpy( test->name, buffer, len + 1 );
    test->is_elf = (code[1] == 'E' && code[2] == 'L' && code[3] == 'F');
    test->exename = strmake(NULL, "%s/%s", dir, test->name);

    if (!(fout = fopen(test->exename, "wb")) ||
        (fwrite (code, size, 1, fout) != 1) ||
        fclose (fout)) fatal (strmake (NULL, "Failed to write file %s.", test->name));
    return 1;
}

int get_subtests (struct wine_test tests[])
{
    char *subname;
    FILE *subfile;
    size_t subsize, bytes_read, total;
    char buffer[8000], *index;
    const char header[] = "Valid test names:", seps[] = " \r\n";
    int oldstdout;
    const char *argv[] = {"wine", NULL, NULL};
    struct wine_test* test;
    int allocated, all_subtests = 0;

    subname = tempnam (0, "sub");
    if (!subname) fatal ("Can't name subtests file.");
    oldstdout = dup (1);
    if (-1 == oldstdout) fatal ("Can't preserve stdout.");
    subfile = fopen (subname, "w+b");
    if (!subfile) fatal ("Can't open subtests file.");
    if (-1 == dup2 (fileno (subfile), 1))
        fatal ("Can't redirect output to subtests.");
    fclose (subfile);

    for (test = tests; test->name; test++) {
        lseek (1, 0, SEEK_SET);
        argv[1] = test->exename;
        if (test->is_elf)
            spawnvp (_P_WAIT, wineloader, argv);
        else
            spawnvp (_P_WAIT, test->exename, argv+1);
        subsize = lseek (1, 0, SEEK_CUR);
        if (subsize >= sizeof buffer) {
            fprintf (stderr, "Subtests output too big: %s.\n",
                     test->name);
            continue;
        }

        lseek (1, 0, SEEK_SET);
        total = 0;
        while ((bytes_read = read (1, buffer + total, subsize - total))
               && (signed)bytes_read != -1)
            total += bytes_read;
        if (bytes_read) {
            fprintf (stderr, "Error reading %s.\n", test->name);
            continue;
        }
        buffer[total] = 0;
        index = strstr (buffer, header);
        if (!index) {
            fprintf (stderr, "Can't parse subtests output of %s.\n",
                     test->name);
            continue;
        }
        index += sizeof(header);
        allocated = 10;
        test->subtests = xmalloc (allocated * sizeof (char*));
        test->subtest_count = 0;
        index = strtok (index, seps);
        while (index) {
            if (test->subtest_count == allocated) {
                allocated *= 2;
                test->subtests = xrealloc (test->subtests,
                                           allocated * sizeof (char*));
            }
            test->subtests[test->subtest_count++] = strdup (index);
            index = strtok (NULL, seps);
        }
        test->subtests = xrealloc (test->subtests,
                                   test->subtest_count * sizeof (char*));
        all_subtests += test->subtest_count;
    }
    close (1);

    if (-1 == dup2 (oldstdout, 1)) fatal ("Can't recover old stdout.");
    close (oldstdout);

    if (remove (subname)) fatal ("Can't remove subtests file.");
    free (subname);

    return all_subtests;
}

void run_test (struct wine_test* test, const char* subtest)
{
    int status;
    const char *argv[] = {"wine", test->exename, subtest, NULL};

    fprintf (stderr, "Running %s:%s\n", test->name, subtest);
    xprintf ("%s:%s start\n", test->name, subtest);
    if (test->is_elf)
        status = spawnvp (_P_WAIT, wineloader, argv);
    else
        status = spawnvp (_P_WAIT, test->exename, argv+1);
    if (status == -1)
        xprintf ("Can't run: %d, errno=%d: %s\n", status, errno, strerror (errno));
    xprintf ("%s:%s done (%x)\n", test->name, subtest, status);
}

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow)
{
    struct wine_test* test;
    int nr_of_tests, subtest, i;
    char *tempdir, *logname;
    FILE *logfile;
    char build_tag[128];

    SetErrorMode (SEM_FAILCRITICALERRORS);

    if (!(wineloader = getenv("WINELOADER"))) wineloader = "wine";
    if (setvbuf (stdout, NULL, _IONBF, 0)) fatal ("Can't unbuffer output.");

    tempdir = tempnam (0, "wct");
    if (!tempdir) fatal ("Can't name temporary dir (check TMP).");
    fprintf (stderr, "tempdir=%s\n", tempdir);
    if (!CreateDirectory (tempdir, NULL)) fatal (strmake (NULL, "Could not create directory: %s", tempdir));

    logname = tempnam (0, "res");
    if (!logname) fatal ("Can't name logfile.");
    fprintf (stderr, "logname=%s\n", logname);

    logfile = fopen (logname, "ab");
    if (!logfile) fatal ("Could not open logfile.");
    if (-1 == dup2 (fileno (logfile), 1)) fatal ("Can't redirect stdout.");
    fclose (logfile);

    LoadStringA( 0, 0, build_tag, sizeof(build_tag) );
    xprintf ("Tests from build %s\n", build_tag);
    xprintf ("Operating system version:\n");
    print_version ();
    xprintf ("Test output:\n" );

    i = 0;
    while (extract_test (tempdir, i)) i++;

    nr_of_tests = get_subtests (wine_tests);

    for (test = wine_tests; test->name; test++)
	for (subtest = 0; subtest < test->subtest_count; subtest++)
	    run_test (test, test->subtests[subtest]);

    close (1);

    remove_dir (tempdir);

    /* FIXME: add an explanation of what is going on */
    if (MessageBoxA( 0, "Do you want to submit the test results?", "Confirmation",
                     MB_YESNO | MB_ICONQUESTION ) == IDYES)
    {
        if (send_file (logname))
            fatal ("Can't submit logfile (network of file error).");
    }

    if (remove (logname))
        warning (strmake (NULL, "Can't remove logfile: %d.", errno));

    return 0;
}
