/*
 * XCOPY - Wine-compatible xcopy program
 *
 * Copyright (C) 2007 J. Edmeades
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

/*
 * Notes:
 * Apparently, valid return codes are:
 *   0 - OK
 *   1 - No files found to copy
 *   2 - CTRL+C during copy
 *   4 - Initialization error, or invalid source specification
 *   5 - Disk write error
 */


#include <stdio.h>
#include <windows.h>
#include <wine/debug.h>

/* Local #defines */
#define RC_OK         0
#define RC_NOFILES    1
#define RC_CTRLC      2
#define RC_INITERROR  4
#define RC_WRITEERROR 5

#define OPT_ASSUMEDIR    0x00000001
#define OPT_RECURSIVE    0x00000002
#define OPT_EMPTYDIR     0x00000004
#define OPT_QUIET        0x00000008
#define OPT_FULL         0x00000010
#define OPT_SIMULATE     0x00000020
#define OPT_PAUSE        0x00000040
#define OPT_NOCOPY       0x00000080
#define OPT_NOPROMPT     0x00000100
#define OPT_SHORTNAME    0x00000200
#define OPT_MUSTEXIST    0x00000400

#define MAXSTRING 8192

WINE_DEFAULT_DEBUG_CHANNEL(xcopy);

/* Prototypes */
static int XCOPY_ProcessSourceParm(WCHAR *suppliedsource, WCHAR *stem, WCHAR *spec);
static int XCOPY_ProcessDestParm(WCHAR *supplieddestination, WCHAR *stem,
                                 WCHAR *spec, WCHAR *srcspec, DWORD flags);
static int XCOPY_DoCopy(WCHAR *srcstem, WCHAR *srcspec,
                        WCHAR *deststem, WCHAR *destspec,
                        DWORD flags);
static BOOL XCOPY_CreateDirectory(const WCHAR* path);

/* Global variables */
static ULONG filesCopied           = 0;              /* Number of files copied  */
static const WCHAR wchr_slash[]   = {'\\', 0};
static const WCHAR wchr_star[]    = {'*', 0};
static const WCHAR wchr_dot[]     = {'.', 0};
static const WCHAR wchr_dotdot[]  = {'.', '.', 0};

/* Constants (Mostly for widechars) */


/* To minimize stack usage during recursion, some temporary variables
   made global                                                        */
static WCHAR copyFrom[MAX_PATH];
static WCHAR copyTo[MAX_PATH];


/* =========================================================================
   main - Main entrypoint for the xcopy command

     Processes the args, and drives the actual copying
   ========================================================================= */
int main (int argc, char *argv[])
{
    int     rc = 0;
    WCHAR   suppliedsource[MAX_PATH] = {0};   /* As supplied on the cmd line */
    WCHAR   supplieddestination[MAX_PATH] = {0};
    WCHAR   sourcestem[MAX_PATH] = {0};       /* Stem of source          */
    WCHAR   sourcespec[MAX_PATH] = {0};       /* Filespec of source      */
    WCHAR   destinationstem[MAX_PATH] = {0};  /* Stem of destination     */
    WCHAR   destinationspec[MAX_PATH] = {0};  /* Filespec of destination */
    WCHAR   copyCmd[MAXSTRING];               /* COPYCMD env var         */
    DWORD   flags = 0;                        /* Option flags            */
    LPWSTR *argvW = NULL;
    const WCHAR PROMPTSTR1[]  = {'/', 'Y', 0};
    const WCHAR PROMPTSTR2[]  = {'/', 'y', 0};
    const WCHAR COPYCMD[]  = {'C', 'O', 'P', 'Y', 'C', 'M', 'D', 0};

    /*
     * Parse the command line
     */

    /* overwrite the command line */
    argvW = CommandLineToArgvW( GetCommandLineW(), &argc );

    /* Confirm at least one parameter */
    if (argc < 2) {
        printf("Invalid number of parameters - Use xcopy /? for help\n");
        return RC_INITERROR;
    }

    /* Preinitialize flags based on COPYCMD */
    if (GetEnvironmentVariable(COPYCMD, copyCmd, MAXSTRING)) {
        if (wcsstr(copyCmd, PROMPTSTR1) != NULL ||
            wcsstr(copyCmd, PROMPTSTR2) != NULL) {
            flags |= OPT_NOPROMPT;
        }
    }

    /* Skip first arg, which is the program name */
    argvW++;

    while (argc > 1)
    {
        argc--;
        WINE_TRACE("Processing Arg: '%s'\n", wine_dbgstr_w(*argvW));

        /* First non-switch parameter is source, second is destination */
        if (*argvW[0] != '/') {
            if (suppliedsource[0] == 0x00) {
                lstrcpyW(suppliedsource, *argvW);
            } else if (supplieddestination[0] == 0x00) {
                lstrcpyW(supplieddestination, *argvW);
            } else {
                printf("Invalid number of parameters - Use xcopy /? for help\n");
                return RC_INITERROR;
            }
        } else {
            /* Process all the switch options */
            switch (toupper(argvW[0][1])) {
            case 'I': flags |= OPT_ASSUMEDIR;     break;
            case 'S': flags |= OPT_RECURSIVE;     break;
            case 'E': flags |= OPT_EMPTYDIR;      break;
            case 'Q': flags |= OPT_QUIET;         break;
            case 'F': flags |= OPT_FULL;          break;
            case 'L': flags |= OPT_SIMULATE;      break;
            case 'W': flags |= OPT_PAUSE;         break;
            case 'T': flags |= OPT_NOCOPY | OPT_RECURSIVE; break;
            case 'Y': flags |= OPT_NOPROMPT;      break;
            case 'N': flags |= OPT_SHORTNAME;     break;
            case 'U': flags |= OPT_MUSTEXIST;     break;
            case '-': if (toupper(argvW[0][2])=='Y')
                          flags &= ~OPT_NOPROMPT; break;
            default:
              WINE_FIXME("Unhandled parameter '%s'\n", wine_dbgstr_w(*argvW));
            }
        }
        argvW++;
    }

    /* Default the destination if not supplied */
    if (supplieddestination[0] == 0x00)
        lstrcpyW(supplieddestination, wchr_dot);

    /* Trace out the supplied information */
    WINE_TRACE("Supplied parameters:\n");
    WINE_TRACE("Source      : '%s'\n", wine_dbgstr_w(suppliedsource));
    WINE_TRACE("Destination : '%s'\n", wine_dbgstr_w(supplieddestination));

    /* Extract required information from source specification */
    rc = XCOPY_ProcessSourceParm(suppliedsource, sourcestem, sourcespec);

    /* Extract required information from destination specification */
    rc = XCOPY_ProcessDestParm(supplieddestination, destinationstem,
                               destinationspec, sourcespec, flags);

    /* Trace out the resulting information */
    WINE_TRACE("Resolved parameters:\n");
    WINE_TRACE("Source Stem : '%s'\n", wine_dbgstr_w(sourcestem));
    WINE_TRACE("Source Spec : '%s'\n", wine_dbgstr_w(sourcespec));
    WINE_TRACE("Dest   Stem : '%s'\n", wine_dbgstr_w(destinationstem));
    WINE_TRACE("Dest   Spec : '%s'\n", wine_dbgstr_w(destinationspec));

    /* Pause if necessary */
    if (flags & OPT_PAUSE) {
        DWORD count;
        char pausestr[10];

        printf("Press <enter> to begin copying\n");
        ReadFile (GetStdHandle(STD_INPUT_HANDLE), pausestr, sizeof(pausestr),
                  &count, NULL);
    }

    /* Now do the hard work... */
    rc = XCOPY_DoCopy(sourcestem, sourcespec,
                destinationstem, destinationspec,
                flags);

    /* Finished - print trailer and exit */
    if (flags & OPT_SIMULATE) {
        printf("%d file(s) would be copied\n", filesCopied);
    } else if (!(flags & OPT_NOCOPY)) {
        printf("%d file(s) copied\n", filesCopied);
    }
    if (rc == RC_OK && filesCopied == 0) rc = RC_NOFILES;
    return rc;

}


/* =========================================================================
   XCOPY_ProcessSourceParm - Takes the supplied source parameter, and
     converts it into a stem and a filespec
   ========================================================================= */
static int XCOPY_ProcessSourceParm(WCHAR *suppliedsource, WCHAR *stem, WCHAR *spec)
{
    WCHAR             actualsource[MAX_PATH];
    WCHAR            *starPos;
    WCHAR            *questPos;

    /*
     * Validate the source, expanding to full path ensuring it exists
     */
    if (GetFullPathName(suppliedsource, MAX_PATH, actualsource, NULL) == 0) {
        WINE_FIXME("Unexpected failure expanding source path (%d)\n", GetLastError());
        return RC_INITERROR;
    }

    /*
     * Work out the stem of the source
     */

    /* If no wildcard were supplied then the source is either a single
       file or a directory - in which case thats the stem of the search,
       otherwise split off the wildcards and use the higher level as the
       stem                                                              */
    lstrcpyW(stem, actualsource);
    starPos = wcschr(stem, '*');
    questPos = wcschr(stem, '?');
    if (starPos || questPos) {
        WCHAR *lastDir;

        if (starPos) *starPos = 0x00;
        if (questPos) *questPos = 0x00;

        lastDir = wcsrchr(stem, '\\');
        if (lastDir) *(lastDir+1) = 0x00;
        else {
            WINE_FIXME("Unexpected syntax error in source parameter\n");
            return RC_INITERROR;
        }
        lstrcpyW(spec, actualsource + (lastDir - stem)+1);
    } else {

        DWORD attribs = GetFileAttributes(actualsource);

        if (attribs == INVALID_FILE_ATTRIBUTES) {
            LPWSTR lpMsgBuf;
            DWORD lastError = GetLastError();
            int status;
            status = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL, lastError, 0, (LPWSTR) &lpMsgBuf, 0, NULL);
            printf("%S\n", lpMsgBuf);
            return RC_INITERROR;

        /* Directory: */
        } else if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
            lstrcatW(stem, wchr_slash);
            lstrcpyW(spec, wchr_star);

        /* File: */
        } else {
            WCHAR drive[MAX_PATH];
            WCHAR dir[MAX_PATH];
            WCHAR fname[MAX_PATH];
            WCHAR ext[MAX_PATH];
            _wsplitpath(actualsource, drive, dir, fname, ext);
            lstrcpyW(stem, drive);
            lstrcatW(stem, dir);
            lstrcpyW(spec, fname);
            lstrcatW(spec, ext);
        }
    }
    return RC_OK;
}

/* =========================================================================
   XCOPY_ProcessDestParm - Takes the supplied destination parameter, and
     converts it into a stem
   ========================================================================= */
static int XCOPY_ProcessDestParm(WCHAR *supplieddestination, WCHAR *stem, WCHAR *spec,
                                 WCHAR *srcspec, DWORD flags)
{
    WCHAR  actualdestination[MAX_PATH];
    DWORD attribs;
    BOOL isDir = FALSE;

    /*
     * Validate the source, expanding to full path ensuring it exists
     */
    if (GetFullPathName(supplieddestination, MAX_PATH, actualdestination, NULL) == 0) {
        WINE_FIXME("Unexpected failure expanding source path (%d)\n", GetLastError());
        return RC_INITERROR;
    }

    /* Destination is either a directory or a file */
    attribs = GetFileAttributes(actualdestination);

    if (attribs == INVALID_FILE_ATTRIBUTES) {

        /* If /I supplied and wildcard copy, assume directory */
        if (flags & OPT_ASSUMEDIR &&
            (wcschr(srcspec, '?') || wcschr(srcspec, '*'))) {

            isDir = TRUE;

        } else {
            DWORD count;
            char  answer[10] = "";

            while (answer[0] != 'F' && answer[0] != 'D') {
                printf("Is %S a filename or directory\n"
                       "on the target?\n"
                       "(F - File, D - Directory)\n", supplieddestination);

                ReadFile(GetStdHandle(STD_INPUT_HANDLE), answer, sizeof(answer), &count, NULL);
                WINE_TRACE("User answer %c\n", answer[0]);

                answer[0] = toupper(answer[0]);
            }

            if (answer[0] == 'D') {
                isDir = TRUE;
            } else {
                isDir = FALSE;
            }
        }
    } else {
        isDir = (attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    if (isDir) {
        lstrcpyW(stem, actualdestination);
        *spec = 0x00;

        /* Ensure ends with a '\' */
        if (stem[lstrlenW(stem)-1] != '\\') {
            lstrcatW(stem, wchr_slash);
        }

    } else {
        WCHAR drive[MAX_PATH];
        WCHAR dir[MAX_PATH];
        WCHAR fname[MAX_PATH];
        WCHAR ext[MAX_PATH];
        _wsplitpath(actualdestination, drive, dir, fname, ext);
        lstrcpyW(stem, drive);
        lstrcatW(stem, dir);
        lstrcpyW(spec, fname);
        lstrcatW(spec, ext);
    }
    return RC_OK;
}

/* =========================================================================
   XCOPY_DoCopy - Recursive function to copy files based on input parms
     of a stem and a spec

      This works by using FindFirstFile supplying the source stem and spec.
      If results are found, any non-directory ones are processed
      Then, if /S or /E is supplied, another search is made just for
      directories, and this function is called again for that directory

   ========================================================================= */
static int XCOPY_DoCopy(WCHAR *srcstem, WCHAR *srcspec,
                        WCHAR *deststem, WCHAR *destspec,
                        DWORD flags)
{
    WIN32_FIND_DATA *finddata;
    HANDLE          h;
    BOOL            findres = TRUE;
    WCHAR           *inputpath, *outputpath;
    BOOL            copiedFile = FALSE;
    DWORD           destAttribs;
    BOOL            skipFile;

    /* Allocate some working memory on heap to minimize footprint */
    finddata = HeapAlloc(GetProcessHeap(), 0, sizeof(WIN32_FIND_DATA));
    inputpath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    outputpath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));

    /* Build the search info into a single parm */
    lstrcpyW(inputpath, srcstem);
    lstrcatW(inputpath, srcspec);

    /* Search 1 - Look for matching files */
    h = FindFirstFile(inputpath, finddata);
    while (h != INVALID_HANDLE_VALUE && findres) {

        skipFile = FALSE;

        /* Ignore . and .. */
        if (lstrcmpW(finddata->cFileName, wchr_dot)==0 ||
            lstrcmpW(finddata->cFileName, wchr_dotdot)==0 ||
            finddata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            WINE_TRACE("Skipping directory, . or .. (%s)\n", wine_dbgstr_w(finddata->cFileName));
        } else {

            /* Get the filename information */
            lstrcpyW(copyFrom, srcstem);
            if (flags & OPT_SHORTNAME) {
              lstrcatW(copyFrom, finddata->cAlternateFileName);
            } else {
              lstrcatW(copyFrom, finddata->cFileName);
            }

            lstrcpyW(copyTo, deststem);
            if (*destspec == 0x00) {
                if (flags & OPT_SHORTNAME) {
                    lstrcatW(copyTo, finddata->cAlternateFileName);
                } else {
                    lstrcatW(copyTo, finddata->cFileName);
                }
            } else {
                lstrcatW(copyTo, destspec);
            }

            /* Do the copy */
            WINE_TRACE("ACTION: Copy '%s' -> '%s'\n", wine_dbgstr_w(copyFrom),
                                                      wine_dbgstr_w(copyTo));
            if (!copiedFile && !(flags & OPT_SIMULATE)) XCOPY_CreateDirectory(deststem);

            /* See if file exists */
            destAttribs = GetFileAttributesW(copyTo);
            if (destAttribs != INVALID_FILE_ATTRIBUTES && !(flags & OPT_NOPROMPT)) {
                DWORD count;
                char  answer[10];
                BOOL  answered = FALSE;

                while (!answered) {
                    printf("Overwrite %S? (Yes|No|All)\n", copyTo);
                    ReadFile (GetStdHandle(STD_INPUT_HANDLE), answer, sizeof(answer),
                              &count, NULL);

                    answered = TRUE;
                    if (toupper(answer[0]) == 'A')
                        flags |= OPT_NOPROMPT;
                    else if (toupper(answer[0]) == 'N')
                        skipFile = TRUE;
                    else if (toupper(answer[0]) != 'Y')
                        answered = FALSE;
                }
            }

            /* See if it has to exist! */
            if (destAttribs == INVALID_FILE_ATTRIBUTES && (flags & OPT_MUSTEXIST)) {
                skipFile = TRUE;
            }

            /* Output a status message */
            if (!skipFile) {
                if (flags & OPT_QUIET) {
                    /* Skip message */
                } else if (flags & OPT_FULL) {
                    printf("%S -> %S\n", copyFrom, copyTo);
                } else {
                    printf("%S\n", copyFrom);
                }

                copiedFile = TRUE;
                if (flags & OPT_SIMULATE || flags & OPT_NOCOPY) {
                    /* Skip copy */
                } else if (CopyFile(copyFrom, copyTo, FALSE) == 0) {
                    printf("Copying of '%S' to '%S' failed with r/c %d\n",
                           copyFrom, copyTo, GetLastError());
                }
                filesCopied++;
            }
        }

        /* Find next file */
        findres = FindNextFile(h, finddata);
    }
    FindClose(h);

    /* Search 2 - do subdirs */
    if (flags & OPT_RECURSIVE) {
        lstrcpyW(inputpath, srcstem);
        lstrcatW(inputpath, wchr_star);
        findres = TRUE;
        WINE_TRACE("Processing subdirs with spec: %s\n", wine_dbgstr_w(inputpath));

        h = FindFirstFile(inputpath, finddata);
        while (h != INVALID_HANDLE_VALUE && findres) {

            /* Only looking for dirs */
            if ((finddata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (lstrcmpW(finddata->cFileName, wchr_dot) != 0) &&
                (lstrcmpW(finddata->cFileName, wchr_dotdot) != 0)) {

                WINE_TRACE("Handling subdir: %s\n", wine_dbgstr_w(finddata->cFileName));

                /* Make up recursive information */
                lstrcpyW(inputpath, srcstem);
                lstrcatW(inputpath, finddata->cFileName);
                lstrcatW(inputpath, wchr_slash);

                lstrcpyW(outputpath, deststem);
                if (*destspec == 0x00) {
                    lstrcatW(outputpath, finddata->cFileName);

                    /* If /E is supplied, create the directory now */
                    if ((flags & OPT_EMPTYDIR) &&
                        !(flags & OPT_SIMULATE))
                        XCOPY_CreateDirectory(outputpath);

                    lstrcatW(outputpath, wchr_slash);
                }

                XCOPY_DoCopy(inputpath, srcspec, outputpath, destspec, flags);
            }

            /* Find next one */
            findres = FindNextFile(h, finddata);
        }
    }

    /* free up memory */
    HeapFree(GetProcessHeap(), 0, finddata);
    HeapFree(GetProcessHeap(), 0, inputpath);
    HeapFree(GetProcessHeap(), 0, outputpath);

    return 0;
}

/* =========================================================================
 * Routine copied from cmd.exe md command -
 * This works recursivly. so creating dir1\dir2\dir3 will create dir1 and
 * dir2 if they do not already exist.
 * ========================================================================= */
static BOOL XCOPY_CreateDirectory(const WCHAR* path)
{
    int len;
    WCHAR *new_path;
    BOOL ret = TRUE;

    new_path = HeapAlloc(GetProcessHeap(),0, sizeof(WCHAR) * (lstrlenW(path)+1));
    lstrcpyW(new_path,path);

    while ((len = lstrlenW(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectory(new_path,NULL))
    {
        WCHAR *slash;
        DWORD last_error = GetLastError();
        if (last_error == ERROR_ALREADY_EXISTS)
            break;

        if (last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if (!(slash = wcsrchr(new_path,'\\')) && ! (slash = wcsrchr(new_path,'/')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if (!XCOPY_CreateDirectory(new_path))
        {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }
    HeapFree(GetProcessHeap(),0,new_path);
    return ret;
}
