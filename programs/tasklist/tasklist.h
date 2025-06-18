/*
 * Copyright 2023 Zhiyi Zhang for CodeWeavers
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

#include <windef.h>

#define MAXSTRING 8192

#define STRING_USAGE                    101
#define STRING_IMAGE_NAME               102
#define STRING_PID                      103
#define STRING_SESSION_NAME             104
#define STRING_SESSION_NUMBER           105
#define STRING_MEM_USAGE                106
#define STRING_K                        107
#define STRING_INVALID_SYNTAX           108
#define STRING_FILTER_NOT_RECOGNIZED    109

enum tasklist_format
{
    TABLE = 0,
    CSV   = 1,
    LIST  = 2,
};

enum tasklist_filter_name
{
    IMAGENAME   = 1,
    PID         = 2,
    SESSION     = 3,
    SESSIONNAME = 4,
    MEMUSAGE    = 5,
};

enum tasklist_filter_operator
{
    EQ = 0,
    NE = 1,
    GT = 2,
    LT = 3,
    GE = 4,
    LE = 5,
};

struct tasklist_filter
{
    enum tasklist_filter_name name;
    enum tasklist_filter_operator op;
    WCHAR *value;
    struct tasklist_filter *next;
};

struct tasklist_process_info
{
    DWORD pid_value;
    DWORD memory_usage_value;
    DWORD session_id_value;
    WCHAR image_name[32];
    WCHAR pid[32];
    WCHAR session_name[32];
    WCHAR session_number[32];
    WCHAR memory_usage[32];
};

struct tasklist_options
{
    BOOL no_header;
    enum tasklist_format format;
    struct tasklist_filter *filters;
};
