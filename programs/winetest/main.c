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

struct rev_info
{
    const char* file;
    const char* rev;
};

static struct wine_test *wine_tests;
static struct rev_info *rev_infos;

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

const char* get_test_source_file(const char* test, const char* subtest)
{
    static const char* special_dirs[][2] = {
	{ "gdi32", "gdi"}, { "kernel32", "kernel" },
	{ "user32", "user" }, { "winspool.drv", "winspool" },
	{ "ws2_32", "winsock" }, { 0, 0 }
    };
    static char buffer[MAX_PATH];
    int i;

    for (i = 0; special_dirs[i][0]; i++) {
	if (strcmp(test, special_dirs[i][0]) == 0) {
	    test = special_dirs[i][1];
	    break;
	}
    }

    snprintf(buffer, sizeof(buffer), "dlls/%s/tests/%s.c", test, subtest);
    return buffer;
}

const char* get_file_rev(const char* file)
{
    const struct rev_info* rev;
 
    for(rev = rev_infos; rev->file; rev++) {
	if (strcmp(rev->file, file) == 0) return rev->rev;
    }

    return "-";
}

void extract_rev_infos ()
{
    char revinfo[256], *p;
    int size = 0, i, len;
    HMODULE module = GetModuleHandle (NULL);

    for (i = 0; TRUE; i++) {
	if (i >= size) {
	    size += 100;
	    rev_infos = xrealloc (rev_infos, size * sizeof (*rev_infos));
	}
	memset(rev_infos + i, 0, sizeof(rev_infos[i]));

        len = LoadStringA (module, i + 30000, revinfo, sizeof(revinfo));
        if (len == 0) break; /* end of revision info */
	if (len >= sizeof(revinfo) - 1) 
	    report (R_FATAL, "Revision info too long.");
	if(!(p = strrchr(revinfo, ':')))
	    report (R_FATAL, "Revision info malformed (i=%d)", i);
	*p = 0;
	rev_infos[i].file = strdup(revinfo);
	rev_infos[i].rev = strdup(p + 1);
    }
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

/* Fills in the name, is_elf and exename fields */
void
extract_test (struct wine_test *test, const char *dir, int id)
{
    BYTE* code;
    DWORD size;
    FILE* fout;
    int strlen, bufflen = 128;
    char *exepos;

    code = extract_rcdata (id, &size);
    test->name = xmalloc (bufflen);
    while ((strlen = LoadStringA (NULL, id, test->name, bufflen))
           == bufflen - 1) {
        bufflen *= 2;
        test->name = xrealloc (test->name, bufflen);
    }
    if (!strlen) report (R_FATAL, "Can't read name of test %d.", id);
    test->exename = strmake (NULL, "%s/%s", dir, test->name);
    exepos = strstr (test->name, "_test.exe");
    if (!exepos) report (R_FATAL, "Not an .exe file: %s", test->name);
    *exepos = 0;
    test->name = xrealloc (test->name, exepos - test->name + 1);
    report (R_STEP, "Extracting: %s", test->name);
    test->is_elf = !memcmp (code+1, "ELF", 3);

    if (!(fout = fopen (test->exename, "wb")) ||
        (fwrite (code, size, 1, fout) != 1) ||
        fclose (fout)) report (R_FATAL, "Failed to write file %s.",
                               test->exename);
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
    const char* argv[] = {"wine", test->exename, subtest, NULL};
    const char* file = get_test_source_file(test->name, subtest);
    const char* rev = get_file_rev(file);

    xprintf ("%s:%s start %s %s\n", test->name, subtest, file, rev);
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

char *
run_tests (char *logname, const char *tag)
{
    int nr_of_files = 0, nr_of_tests = 0, i;
    char *tempdir;
    FILE *logfile;
    char build_tag[128];

    SetErrorMode (SEM_NOGPFAULTERRORBOX);

    if (!(wineloader = getenv("WINELOADER"))) wineloader = "wine";

    tempdir = tempnam (0, "wct");
    if (!tempdir)
        report (R_FATAL, "Can't name temporary dir (check %%TEMP%%).");
    report (R_DIR, tempdir);
    if (!CreateDirectory (tempdir, NULL))
        report (R_FATAL, "Could not create directory: %s", tempdir);

    if (!logname) {
        logname = tempnam (0, "res");
        if (!logname) report (R_FATAL, "Can't name logfile.");
    }
    report (R_OUT, logname);

    logfile = fopen (logname, "a");
    if (!logfile) report (R_FATAL, "Could not open logfile.");
    if (-1 == dup2 (fileno (logfile), 1))
        report (R_FATAL, "Can't redirect stdout.");
    fclose (logfile);

    xprintf ("Version 2\n");
    i = LoadStringA (GetModuleHandle (NULL), 0,
                     build_tag, sizeof build_tag);
    if (i == 0) report (R_FATAL, "Build descriptor not found: %d",
                        GetLastError ());
    if (i >= sizeof build_tag)
        report (R_FATAL, "Build descriptor too long.");
    xprintf ("Tests from build %s\n", build_tag);
    xprintf ("Tag: %s\n", tag?tag:"");
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
    report (R_PROGRESS, 0, nr_of_files);
    for (i = 0; i < nr_of_files; i++) {
        get_subtests (tempdir, wine_tests+i, i+1);
        nr_of_tests += wine_tests[i].subtest_count;
    }
    report (R_DELTA, 0, "Extracting: Done");

    report (R_STATUS, "Running tests");
    report (R_PROGRESS, 1, nr_of_tests);
    for (i = 0; i < nr_of_files; i++) {
        struct wine_test *test = wine_tests + i;
        int j;

	for (j = 0; j < test->subtest_count; j++) {
            report (R_STEP, "Running: %s:%s", test->name,
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

    return logname;
}

void
usage ()
{
    fprintf (stderr, "\
Usage: winetest [OPTION]...\n\n\
  -c       console mode, no GUI\n\
  -h       print this message and exit\n\
  -q       quiet mode, no output at all\n\
  -o FILE  put report into FILE, do not submit\n\
  -s FILE  submit FILE, do not run tests\n\
  -t TAG   include TAG of characters [-.0-9a-zA-Z] in the report\n");
}

/* One can't nest strtok()-s, so here is a replacement. */
char *
mystrtok (char *newstr)
{
    static char *start, *end;
    static int finish = 1;

    if (newstr) {
        start = newstr;
        finish = 0;
    } else start = end;
    if (finish) return NULL;
    while (*start == ' ') start++;
    if (*start == 0) return NULL;
    end = start;
    while (*end != ' ')
        if (*end == 0) {
            finish = 1;
            return start;
        } else end++;
    *end++ = 0;
    return start;
}

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst,
                    LPSTR cmdLine, int cmdShow)
{
    char *logname = NULL;
    char *tag = NULL, *cp;
    const char *submit = NULL;

    /* initialize the revision information first */
    extract_rev_infos();

    cmdLine = mystrtok (cmdLine);
    while (cmdLine) {
        if (cmdLine[0] != '-' || cmdLine[2]) {
            report (R_ERROR, "Not a single letter option: %s", cmdLine);
            usage ();
            exit (2);
        }
        switch (cmdLine[1]) {
        case 'c':
            report (R_TEXTMODE);
            break;
        case 'h':
            usage ();
            exit (0);
        case 'q':
            report (R_QUIET);
            break;
        case 's':
            submit = mystrtok (NULL);
            if (tag)
                report (R_WARNING, "ignoring tag for submit");
            send_file (submit);
            break;
        case 'o':
            logname = mystrtok (NULL);
            run_tests (logname, tag);
            break;
        case 't':
            tag = mystrtok (NULL);
            cp = badtagchar (tag);
            if (cp) {
                report (R_ERROR, "invalid char in tag: %c", *cp);
                usage ();
                exit (2);
            }
            break;
        default:
            report (R_ERROR, "invalid option: -%c", cmdLine[1]);
            usage ();
            exit (2);
        }
        cmdLine = mystrtok (NULL);
    }
    if (!logname && !submit) {
        report (R_STATUS, "Starting up");
        logname = run_tests (NULL, tag);
        if (report (R_ASK, MB_YESNO, "Do you want to submit the "
                    "test results?") == IDYES)
            if (!send_file (logname) && remove (logname))
                report (R_WARNING, "Can't remove logfile: %d.", errno);
        free (logname);
        report (R_STATUS, "Finished");
    }
    exit (0);
}
