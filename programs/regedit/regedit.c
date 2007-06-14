/*
 * Windows regedit.exe registry editor implementation.
 *
 * Copyright 2002 Andriy Palamarchuk
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

#include <ctype.h>
#include <stdio.h>
#include <windows.h>
#include "regproc.h"

static const char *usage =
    "Usage:\n"
    "    regedit filename\n"
    "    regedit /E filename [regpath]\n"
    "    regedit /D regpath\n"
    "\n"
    "filename - registry file name\n"
    "regpath - name of the registry key\n"
    "\n"
    "When called without any switches, adds the content of the specified\n"
    "file to the registry\n"
    "\n"
    "Switches:\n"
    "    /E - exports contents of the specified registry key to the specified\n"
    "	file. Exports the whole registry if no key is specified.\n"
    "    /D - deletes specified registry key\n"
    "    /S - silent execution, can be used with any other switch.\n"
    "	Default. The only existing mode, exists for compatibility with Windows regedit.\n"
    "    /V - advanced mode, can be used with any other switch.\n"
    "	Ignored, exists for compatibility with Windows regedit.\n"
    "    /L - location of system.dat file. Can be used with any other switch.\n"
    "	Ignored. Exists for compatibility with Windows regedit.\n"
    "    /R - location of user.dat file. Can be used with any other switch.\n"
    "	Ignored. Exists for compatibility with Windows regedit.\n"
    "    /? - print this help. Any other switches are ignored.\n"
    "    /C - create registry from file. Not implemented.\n"
    "\n"
    "The switches are case-insensitive, can be prefixed either by '-' or '/'.\n"
    "This program is command-line compatible with Microsoft Windows\n"
    "regedit.\n";

typedef enum {
    ACTION_UNDEF, ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;


const CHAR *getAppName(void)
{
    return "regedit";
}

/******************************************************************************
 * Copies file name from command line string to the buffer.
 * Rewinds the command line string pointer to the next non-space character
 * after the file name.
 * Buffer contains an empty string if no filename was found;
 *
 * params:
 * command_line - command line current position pointer
 *      where *s[0] is the first symbol of the file name.
 * file_name - buffer to write the file name to.
 */
static void get_file_name(CHAR **command_line, CHAR *file_name)
{
    CHAR *s = *command_line;
    int pos = 0;                /* position of pointer "s" in *command_line */
    file_name[0] = 0;

    if (!s[0]) {
        return;
    }

    if (s[0] == '"') {
        s++;
        (*command_line)++;
        while(s[0] != '"') {
            if (!s[0]) {
                fprintf(stderr,"%s: Unexpected end of file name!\n",
                        getAppName());
                exit(1);
            }
            s++;
            pos++;
        }
    } else {
        while(s[0] && !isspace(s[0])) {
            s++;
            pos++;
        }
    }
    memcpy(file_name, *command_line, pos * sizeof((*command_line)[0]));
    /* remove the last backslash */
    if (file_name[pos - 1] == '\\') {
        file_name[pos - 1] = '\0';
    } else {
        file_name[pos] = '\0';
    }

    if (s[0]) {
        s++;
        pos++;
    }
    while(s[0] && isspace(s[0])) {
        s++;
        pos++;
    }
    (*command_line) += pos;
}

static BOOL PerformRegAction(REGEDIT_ACTION action, LPSTR s)
{
    switch (action) {
    case ACTION_ADD: {
            CHAR filename[MAX_PATH];
            FILE *reg_file;

            get_file_name(&s, filename);
            if (!filename[0]) {
                fprintf(stderr,"%s: No file name was specified\n", getAppName());
                fprintf(stderr,usage);
                exit(1);
            }

            while(filename[0]) {
                char* realname = NULL;
                int size;
                size=SearchPath(NULL,filename,NULL,0,NULL,NULL);
                if (size>0)
                {
                    realname=HeapAlloc(GetProcessHeap(),0,size);
                    size=SearchPath(NULL,filename,NULL,size,realname,NULL);
                }
                if (size==0)
                {
                    fprintf(stderr,"%s: File not found \"%s\" (%d)\n",
                            getAppName(),filename,GetLastError());
                    exit(1);
                }
                reg_file = fopen(realname, "r");
                if (reg_file==NULL)
                {
                    perror("");
                    fprintf(stderr,"%s: Can't open file \"%s\"\n", getAppName(), filename);
                    exit(1);
                }
                processRegLines(reg_file, doSetValue);
                if (realname)
                {
                    HeapFree(GetProcessHeap(),0,realname);
                    fclose(reg_file);
                }
                get_file_name(&s, filename);
            }
            break;
        }
    case ACTION_DELETE: {
            CHAR reg_key_name[KEY_MAX_LEN];

            get_file_name(&s, reg_key_name);
            if (!reg_key_name[0]) {
                fprintf(stderr,"%s: No registry key was specified for removal\n",
                        getAppName());
                fprintf(stderr,usage);
                exit(1);
            }
            delete_registry_key(reg_key_name);
            break;
        }
    case ACTION_EXPORT: {
            CHAR filename[MAX_PATH];

            filename[0] = '\0';
            get_file_name(&s, filename);
            if (!filename[0]) {
                fprintf(stderr,"%s: No file name was specified\n", getAppName());
                fprintf(stderr,usage);
                exit(1);
            }

            if (s[0]) {
                CHAR reg_key_name[KEY_MAX_LEN];

                get_file_name(&s, reg_key_name);
                export_registry_key(filename, reg_key_name);
            } else {
                export_registry_key(filename, NULL);
            }
            break;
        }
    default:
        fprintf(stderr,"%s: Unhandled action!\n", getAppName());
        exit(1);
        break;
    }
    return TRUE;
}

/**
 * Process unknown switch.
 *
 * Params:
 *   chu - the switch character in upper-case.
 *   s - the command line string where s points to the switch character.
 */
static void error_unknown_switch(char chu, char *s)
{
    if (isalpha(chu)) {
        fprintf(stderr,"%s: Undefined switch /%c!\n", getAppName(), chu);
    } else {
        fprintf(stderr,"%s: Alphabetic character is expected after '%c' "
                "in switch specification\n", getAppName(), *(s - 1));
    }
    exit(1);
}

BOOL ProcessCmdLine(LPSTR lpCmdLine)
{
    REGEDIT_ACTION action = ACTION_UNDEF;
    LPSTR s = lpCmdLine;        /* command line pointer */
    CHAR ch = *s;               /* current character */

    while (ch && ((ch == '-') || (ch == '/'))) {
        char chu;
        char ch2;

        s++;
        ch = *s;
        ch2 = *(s+1);
        chu = toupper(ch);
        if (!ch2 || isspace(ch2)) {
            if (chu == 'S' || chu == 'V') {
                /* ignore these switches */
            } else {
                switch (chu) {
                case 'D':
                    action = ACTION_DELETE;
                    break;
                case 'E':
                    action = ACTION_EXPORT;
                    break;
                case '?':
                    fprintf(stderr,usage);
                    exit(0);
                    break;
                default:
                    error_unknown_switch(chu, s);
                    break;
                }
            }
            s++;
        } else {
            if (ch2 == ':') {
                switch (chu) {
                case 'L':
                    /* fall through */
                case 'R':
                    s += 2;
                    while (*s && !isspace(*s)) {
                        s++;
                    }
                    break;
                default:
                    error_unknown_switch(chu, s);
                    break;
                }
            } else {
                /* this is a file name, starting from '/' */
                s--;
                break;
            }
        }
        /* skip spaces to the next parameter */
        ch = *s;
        while (ch && isspace(ch)) {
            s++;
            ch = *s;
        }
    }

    if (*s && action == ACTION_UNDEF)
        action = ACTION_ADD;

    if (action == ACTION_UNDEF)
        return FALSE;

    return PerformRegAction(action, s);
}
