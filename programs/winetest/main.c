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

#define TESTRESOURCE "USERDATA"

struct wine_test
{
    char *name;
    int resource;
    int subtest_count;
    char **subtests;
    int is_elf;
    char *exename;
};

static struct wine_test *wine_tests;

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
	    report (R_FATAL, "Can't get OS version.");
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
            report (R_WARNING, "Can't delete file %s: error %d",
                    path, GetLastError ());
    } while (FindNextFile (hFind, &wfd));
    FindClose (hFind);
    if (!RemoveDirectory (dir))
        report (R_WARNING, "Can't remove directory %s: error %d",
                dir, GetLastError ());
}

void* extract_rcdata (int id, DWORD* size)
{
    HRSRC rsrc;
    HGLOBAL hdl;
    LPVOID addr = NULL;
    
    if (!(rsrc = FindResource (0, (LPTSTR)id, TESTRESOURCE)) ||
        !(*size = SizeofResource (0, rsrc)) ||
        !(hdl = LoadResource (0, rsrc)) ||
        !(addr = LockResource (hdl)))
        report (R_FATAL, "Can't extract test file of id %d: %d",
                id, GetLastError ());
    return addr;
}

/* Fills out the name, is_elf and exename fields */
void
extract_test (struct wine_test *test, const char *dir, int id)
{
    BYTE* code;
    DWORD size;
    FILE* fout;
    int strlen, bufflen = 128;

    code = extract_rcdata (id, &size);
    test->name = xmalloc (bufflen);
    while ((strlen = LoadStringA (NULL, id, test->name, bufflen))
           == bufflen - 1) {
        bufflen *= 2;
        test->name = xrealloc (test->name, bufflen);
    }
    if (!strlen) report (R_FATAL, "Can't read name of test %d.", id);
    test->name = xrealloc (test->name, strlen+1);
    report (R_STEP, "Extracting: %s", test->name);
    test->is_elf = !memcmp (code+1, "ELF", 3);
    test->exename = strmake (NULL, "%s/%s", dir, test->name);

    if (!(fout = fopen (test->exename, "wb")) ||
        (fwrite (code, size, 1, fout) != 1) ||
        fclose (fout)) report (R_FATAL, "Failed to write file %s.",
                               test->name);
}

void
get_subtests (const char *tempdir, struct wine_test *test, int id)
{
    char *subname;
    FILE *subfile;
    size_t subsize, bytes_read, total;
    char *buffer, *index;
    const char header[] = "Valid test names:", seps[] = " \r\n";
    int oldstdout;
    const char *argv[] = {"wine", NULL, NULL};
    int allocated;

    subname = tempnam (0, "sub");
    if (!subname) report (R_FATAL, "Can't name subtests file.");
    oldstdout = dup (1);
    if (-1 == oldstdout) report (R_FATAL, "Can't preserve stdout.");
    subfile = fopen (subname, "w+b");
    if (!subfile) report (R_FATAL, "Can't open subtests file.");
    if (-1 == dup2 (fileno (subfile), 1))
        report (R_FATAL, "Can't redirect output to subtests.");
    fclose (subfile);

    extract_test (test, tempdir, id);
    argv[1] = test->exename;
    if (test->is_elf)
        spawnvp (_P_WAIT, wineloader, argv);
    else
        spawnvp (_P_WAIT, test->exename, argv+1);
    subsize = lseek (1, 0, SEEK_CUR);
    buffer = xmalloc (subsize+1);

    lseek (1, 0, SEEK_SET);
    total = 0;
    while ((bytes_read = read (1, buffer + total, subsize - total))
               && (signed)bytes_read != -1)
            total += bytes_read;
    if (bytes_read)
        report (R_FATAL, "Can't get subtests of %s", test->name);
    buffer[total] = 0;
    index = strstr (buffer, header);
    if (!index)
        report (R_FATAL, "Can't parse subtests output of %s",
                test->name);
    index += sizeof header;

    allocated = 10;
    test->subtests = xmalloc (allocated * sizeof(char*));
    test->subtest_count = 0;
    index = strtok (index, seps);
    while (index) {
        if (test->subtest_count == allocated) {
            allocated *= 2;
            test->subtests = xrealloc (test->subtests,
                                       allocated * sizeof(char*));
        }
        test->subtests[test->subtest_count++] = strdup (index);
        index = strtok (NULL, seps);
    }
    test->subtests = xrealloc (test->subtests,
                               test->subtest_count * sizeof(char*));
    free (buffer);
    close (1);
    if (-1 == dup2 (oldstdout, 1))
        report (R_FATAL, "Can't recover old stdout.");
    close (oldstdout);
    if (remove (subname))
        report (R_FATAL, "Can't remove subtests file.");
    free (subname);
}

/* Return number of failures, -1 if couldn't spawn process. */
int run_test (struct wine_test* test, const char* subtest)
{
    int status;
    const char *argv[] = {"wine", test->exename, subtest, NULL};

    xprintf ("%s:%s start\n", test->name, subtest);
    if (test->is_elf)
        status = spawnvp (_P_WAIT, wineloader, argv);
    else
        status = spawnvp (_P_WAIT, test->exename, argv+1);
    if (status == -1)
        xprintf ("Can't run: %d, errno=%d: %s\n",
                 status, errno, strerror (errno));
    xprintf ("%s:%s done (%d)\n", test->name, subtest, status);
    return status;
}

BOOL CALLBACK
EnumTestFileProc (HMODULE hModule, LPCTSTR lpszType,
                  LPTSTR lpszName, LONG_PTR lParam)
{
    (*(int*)lParam)++;
    return TRUE;
}

void
run_tests ()
{
    int nr_of_files = 0, nr_of_tests = 0, i;
    char *tempdir, *logname;
    FILE *logfile;
    char build_tag[128];

    SetErrorMode (SEM_FAILCRITICALERRORS);

    if (!(wineloader = getenv("WINELOADER"))) wineloader = "wine";
    if (setvbuf (stdout, NULL, _IONBF, 0))
        report (R_FATAL, "Can't unbuffer output.");

    tempdir = tempnam (0, "wct");
    if (!tempdir)
        report (R_FATAL, "Can't name temporary dir (check %%TEMP%%).");
    report (R_DIR, tempdir);
    if (!CreateDirectory (tempdir, NULL))
        report (R_FATAL, "Could not create directory: %s", tempdir);

    logname = tempnam (0, "res");
    if (!logname) report (R_FATAL, "Can't name logfile.");
    report (R_OUT, logname);

    logfile = fopen (logname, "a");
    if (!logfile) report (R_FATAL, "Could not open logfile.");
    if (-1 == dup2 (fileno (logfile), 1))
        report (R_FATAL, "Can't redirect stdout.");
    fclose (logfile);

    xprintf ("Version 1\n");
    i = LoadStringA (GetModuleHandle (NULL), 0,
                     build_tag, sizeof build_tag);
    if (i == 0) report (R_FATAL, "Build descriptor not found.");
    if (i >= sizeof build_tag)
        report (R_FATAL, "Build descriptor too long.");
    xprintf ("Tests from build %s\n", build_tag);
    xprintf ("Operating system version:\n");
    print_version ();
    xprintf ("Test output:\n" );

    report (R_STATUS, "Counting tests");
    if (!EnumResourceNames (NULL, TESTRESOURCE,
                            EnumTestFileProc, (LPARAM)&nr_of_files))
        report (R_FATAL, "Can't enumerate test files: %d",
                GetLastError ());
    wine_tests = xmalloc (nr_of_files * sizeof wine_tests[0]);

    report (R_STATUS, "Extracting tests");
    report (R_PROGRESS, nr_of_files);
    for (i = 0; i < nr_of_files; i++) {
        get_subtests (tempdir, wine_tests+i, i+1);
        nr_of_tests += wine_tests[i].subtest_count;
    }
    report (R_DELTA, 0, "Extracting: Done");

    report (R_STATUS, "Running tests");
    report (R_PROGRESS, nr_of_tests);
    for (i = 0; i < nr_of_files; i++) {
        struct wine_test *test = wine_tests + i;
        int j;

	for (j = 0; j < test->subtest_count; j++) {
            report (R_STEP, "Running: %s: %s", test->name,
                    test->subtests[j]);
	    run_test (test, test->subtests[j]);
        }
    }
    report (R_DELTA, 0, "Running: Done");

    report (R_STATUS, "Cleaning up");
    close (1);
    remove_dir (tempdir);
    free (tempdir);
    free (wine_tests);

    if (report (R_ASK, MB_YESNO,
                "Do you want to submit the test results?") == IDYES)
        if (send_file (logname))
            report (R_FATAL, "Can't submit logfile '%s'", logname);

    if (remove (logname))
        report (R_WARNING, "Can't remove logfile: %d.", errno);
    free (logname);
}

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst,
                    LPSTR cmdLine, int cmdShow)
{
    report (R_STATUS, "Starting up");
    run_tests ();
    report (R_STATUS, "Finished");
    exit (0);
}
