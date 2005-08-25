/*
 * main default entry point for exe files
 *
 * Copyright 2005 Alexandre Julliard
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
#include "winuser.h"

int main( int argc, char *argv[] )
{
    STARTUPINFOA info;
    char *cmdline = GetCommandLineA();
    int bcount = 0, in_quotes = 0;

    while (*cmdline)
    {
        if ((*cmdline == '\t' || *cmdline == ' ') && !in_quotes) break;
        else if (*cmdline == '\\') bcount++;
        else if (*cmdline == '\"')
        {
            if (!(bcount & 1)) in_quotes = !in_quotes;
            bcount = 0;
        }
        else bcount = 0;
        cmdline++;
    }
    while (*cmdline == '\t' || *cmdline == ' ') cmdline++;

    GetStartupInfoA( &info );
    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = SW_SHOWNORMAL;
    return WinMain( GetModuleHandleA(0), 0, cmdline, info.wShowWindow );
}
