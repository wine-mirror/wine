/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 * Copyright 1998 Marcus Meissner
 * Copyright 2001 Eric Pouech
 * Copyright 2001 Alexandre Julliard
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

/* Reference applications:
 * -  IDA (interactive disassembler) full version 3.75. Works.
 * -  LYNX/W32. Works mostly, some keys crash it.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wincon.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);


static UINT console_input_codepage;
static UINT console_output_codepage;

/* map input records to ASCII */
static void input_records_WtoA( INPUT_RECORD *buffer, int count )
{
    int i;
    char ch;

    for (i = 0; i < count; i++)
    {
        if (buffer[i].EventType != KEY_EVENT) continue;
        WideCharToMultiByte( GetConsoleCP(), 0,
                             &buffer[i].Event.KeyEvent.uChar.UnicodeChar, 1, &ch, 1, NULL, NULL );
        buffer[i].Event.KeyEvent.uChar.AsciiChar = ch;
    }
}

/* map input records to Unicode */
static void input_records_AtoW( INPUT_RECORD *buffer, int count )
{
    int i;
    WCHAR ch;

    for (i = 0; i < count; i++)
    {
        if (buffer[i].EventType != KEY_EVENT) continue;
        MultiByteToWideChar( GetConsoleCP(), 0,
                             &buffer[i].Event.KeyEvent.uChar.AsciiChar, 1, &ch, 1 );
        buffer[i].Event.KeyEvent.uChar.UnicodeChar = ch;
    }
}

/* map char infos to ASCII */
static void char_info_WtoA( CHAR_INFO *buffer, int count )
{
    char ch;

    while (count-- > 0)
    {
        WideCharToMultiByte( GetConsoleOutputCP(), 0, &buffer->Char.UnicodeChar, 1,
                             &ch, 1, NULL, NULL );
        buffer->Char.AsciiChar = ch;
        buffer++;
    }
}

/* map char infos to Unicode */
static void char_info_AtoW( CHAR_INFO *buffer, int count )
{
    WCHAR ch;

    while (count-- > 0)
    {
        MultiByteToWideChar( GetConsoleOutputCP(), 0, &buffer->Char.AsciiChar, 1, &ch, 1 );
        buffer->Char.UnicodeChar = ch;
        buffer++;
    }
}


/******************************************************************************
 * GetConsoleCP [KERNEL32.@]  Returns the OEM code page for the console
 *
 * RETURNS
 *    Code page code
 */
UINT WINAPI GetConsoleCP(VOID)
{
    if (!console_input_codepage) console_input_codepage = GetOEMCP();
    return console_input_codepage;
}


/******************************************************************************
 *  SetConsoleCP	 [KERNEL32.@]
 */
BOOL WINAPI SetConsoleCP(UINT cp)
{
    if (!IsValidCodePage( cp )) return FALSE;
    console_input_codepage = cp;
    return TRUE;
}


/***********************************************************************
 *            GetConsoleOutputCP   (KERNEL32.@)
 */
UINT WINAPI GetConsoleOutputCP(VOID)
{
    if (!console_output_codepage) console_output_codepage = GetOEMCP();
    return console_output_codepage;
}


/******************************************************************************
 * SetConsoleOutputCP [KERNEL32.@]  Set the output codepage used by the console
 *
 * PARAMS
 *    cp [I] code page to set
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetConsoleOutputCP(UINT cp)
{
    if (!IsValidCodePage( cp )) return FALSE;
    console_output_codepage = cp;
    return TRUE;
}


/******************************************************************************
 * WriteConsoleInputA [KERNEL32.@]
 */
BOOL WINAPI WriteConsoleInputA( HANDLE handle, const INPUT_RECORD *buffer,
                                DWORD count, LPDWORD written )
{
    INPUT_RECORD *recW;
    BOOL ret;

    if (!(recW = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*recW) ))) return FALSE;
    memcpy( recW, buffer, count*sizeof(*recW) );
    input_records_AtoW( recW, count );
    ret = WriteConsoleInputW( handle, recW, count, written );
    HeapFree( GetProcessHeap(), 0, recW );
    return ret;
}


/******************************************************************************
 * WriteConsoleInputW [KERNEL32.@]
 */
BOOL WINAPI WriteConsoleInputW( HANDLE handle, const INPUT_RECORD *buffer,
                                DWORD count, LPDWORD written )
{
    BOOL ret;

    TRACE("(%d,%p,%ld,%p)\n", handle, buffer, count, written);

    if (written) *written = 0;
    SERVER_START_REQ( write_console_input )
    {
        req->handle = handle;
        wine_server_add_data( req, buffer, count * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (written) *written = reply->written;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            WriteConsoleOutputA   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleOutputA( HANDLE hConsoleOutput, const CHAR_INFO *lpBuffer,
                                 COORD size, COORD coord, LPSMALL_RECT region )
{
    int y;
    BOOL ret;
    COORD new_size, new_coord;
    CHAR_INFO *ciw;

    new_size.X = min( region->Right - region->Left + 1, size.X - coord.X );
    new_size.Y = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (new_size.X <= 0 || new_size.Y <= 0)
    {
        region->Bottom = region->Top + new_size.Y - 1;
        region->Right = region->Left + new_size.X - 1;
        return TRUE;
    }

    /* only copy the useful rectangle */
    if (!(ciw = HeapAlloc( GetProcessHeap(), 0, sizeof(CHAR_INFO) * new_size.X * new_size.Y )))
        return FALSE;
    for (y = 0; y < new_size.Y; y++)
    {
        memcpy( &ciw[y * new_size.X], &lpBuffer[(y + coord.Y) * size.X + coord.X],
                new_size.X * sizeof(CHAR_INFO) );
        char_info_AtoW( ciw, new_size.X );
    }
    new_coord.X = new_coord.Y = 0;
    ret = WriteConsoleOutputW( hConsoleOutput, ciw, new_size, new_coord, region );
    if (ciw) HeapFree( GetProcessHeap(), 0, ciw );
    return ret;
}


/***********************************************************************
 *            WriteConsoleOutputW   (KERNEL32.@)
 */
BOOL WINAPI WriteConsoleOutputW( HANDLE hConsoleOutput, const CHAR_INFO *lpBuffer,
                                 COORD size, COORD coord, LPSMALL_RECT region )
{
    int width, height, y;
    BOOL ret = TRUE;

    TRACE("(%x,%p,(%d,%d),(%d,%d),(%d,%dx%d,%d)\n",
          hConsoleOutput, lpBuffer, size.X, size.Y, coord.X, coord.Y,
          region->Left, region->Top, region->Right, region->Bottom);

    width = min( region->Right - region->Left + 1, size.X - coord.X );
    height = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (width > 0 && height > 0)
    {
        for (y = 0; y < height; y++)
        {
            SERVER_START_REQ( write_console_output )
            {
                req->handle = hConsoleOutput;
                req->x      = region->Left;
                req->y      = region->Top + y;
                req->mode   = CHAR_INFO_MODE_TEXTATTR;
                req->wrap   = FALSE;
                wine_server_add_data( req, &lpBuffer[(y + coord.Y) * size.X + coord.X],
                                      width * sizeof(CHAR_INFO));
                if ((ret = !wine_server_call_err( req )))
                {
                    width  = min( width, reply->width - region->Left );
                    height = min( height, reply->height - region->Top );
                }
            }
            SERVER_END_REQ;
            if (!ret) break;
        }
    }
    region->Bottom = region->Top + height - 1;
    region->Right = region->Left + width - 1;
    return ret;
}


/******************************************************************************
 * WriteConsoleOutputCharacterA [KERNEL32.@]  Copies character to consecutive
 * 					      cells in the console screen buffer
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    str               [I] Pointer to buffer with chars to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 */
BOOL WINAPI WriteConsoleOutputCharacterA( HANDLE hConsoleOutput, LPCSTR str, DWORD length,
                                          COORD coord, LPDWORD lpNumCharsWritten )
{
    BOOL ret;
    LPWSTR strW;
    DWORD lenW;

    TRACE("(%d,%s,%ld,%dx%d,%p)\n", hConsoleOutput,
          debugstr_an(str, length), length, coord.X, coord.Y, lpNumCharsWritten);

    lenW = MultiByteToWideChar( GetConsoleOutputCP(), 0, str, length, NULL, 0 );

    if (lpNumCharsWritten) *lpNumCharsWritten = 0;

    if (!(strW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( GetConsoleOutputCP(), 0, str, length, strW, lenW );

    ret = WriteConsoleOutputCharacterW( hConsoleOutput, strW, lenW, coord, lpNumCharsWritten );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/******************************************************************************
 * WriteConsoleOutputAttribute [KERNEL32.@]  Sets attributes for some cells in
 * 					     the console screen buffer
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    attr              [I] Pointer to buffer with write attributes
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumAttrsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 * 
 */
BOOL WINAPI WriteConsoleOutputAttribute( HANDLE hConsoleOutput, CONST WORD *attr, DWORD length,
                                         COORD coord, LPDWORD lpNumAttrsWritten )
{
    BOOL ret;

    TRACE("(%d,%p,%ld,%dx%d,%p)\n", hConsoleOutput,attr,length,coord.X,coord.Y,lpNumAttrsWritten);

    SERVER_START_REQ( write_console_output )
    {
        req->handle = hConsoleOutput;
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_ATTR;
        req->wrap   = TRUE;
        wine_server_add_data( req, attr, length * sizeof(WORD) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (lpNumAttrsWritten) *lpNumAttrsWritten = reply->written;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * FillConsoleOutputCharacterA [KERNEL32.@]
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    ch                [I] Character to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputCharacterA( HANDLE hConsoleOutput, CHAR ch, DWORD length,
                                         COORD coord, LPDWORD lpNumCharsWritten )
{
    WCHAR wch;

    MultiByteToWideChar( GetConsoleOutputCP(), 0, &ch, 1, &wch, 1 );
    return FillConsoleOutputCharacterW(hConsoleOutput, wch, length, coord, lpNumCharsWritten);
}


/******************************************************************************
 * FillConsoleOutputCharacterW [KERNEL32.@]  Writes characters to console
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    ch                [I] Character to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumCharsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputCharacterW( HANDLE hConsoleOutput, WCHAR ch, DWORD length,
                                         COORD coord, LPDWORD lpNumCharsWritten)
{
    BOOL ret;

    TRACE("(%d,%s,%ld,(%dx%d),%p)\n",
          hConsoleOutput, debugstr_wn(&ch, 1), length, coord.X, coord.Y, lpNumCharsWritten);

    SERVER_START_REQ( fill_console_output )
    {
        req->handle  = hConsoleOutput;
        req->x       = coord.X;
        req->y       = coord.Y;
        req->mode    = CHAR_INFO_MODE_TEXT;
        req->wrap    = TRUE;
        req->data.ch = ch;
        req->count   = length;
        if ((ret = !wine_server_call_err( req )))
        {
            if (lpNumCharsWritten) *lpNumCharsWritten = reply->written;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * FillConsoleOutputAttribute [KERNEL32.@]  Sets attributes for console
 *
 * PARAMS
 *    hConsoleOutput    [I] Handle to screen buffer
 *    attr              [I] Color attribute to write
 *    length            [I] Number of cells to write to
 *    coord             [I] Coords of first cell
 *    lpNumAttrsWritten [O] Pointer to number of cells written
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI FillConsoleOutputAttribute( HANDLE hConsoleOutput, WORD attr, DWORD length,
                                        COORD coord, LPDWORD lpNumAttrsWritten )
{
    BOOL ret;

    TRACE("(%d,%d,%ld,(%dx%d),%p)\n",
          hConsoleOutput, attr, length, coord.X, coord.Y, lpNumAttrsWritten);

    SERVER_START_REQ( fill_console_output )
    {
        req->handle    = hConsoleOutput;
        req->x         = coord.X;
        req->y         = coord.Y;
        req->mode      = CHAR_INFO_MODE_ATTR;
        req->wrap      = TRUE;
        req->data.attr = attr;
        req->count     = length;
        if ((ret = !wine_server_call_err( req )))
        {
            if (lpNumAttrsWritten) *lpNumAttrsWritten = reply->written;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 * ReadConsoleOutputCharacterA [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputCharacterA(HANDLE hConsoleOutput, LPSTR lpstr, DWORD count,
                                        COORD coord, LPDWORD read_count)
{
    DWORD read;
    BOOL ret;
    LPWSTR wptr = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));

    if (read_count) *read_count = 0;
    if (!wptr) return FALSE;

    if ((ret = ReadConsoleOutputCharacterW( hConsoleOutput, wptr, count, coord, &read )))
    {
        read = WideCharToMultiByte( GetConsoleOutputCP(), 0, wptr, read, lpstr, count, NULL, NULL);
        if (read_count) *read_count = read;
    }
    HeapFree( GetProcessHeap(), 0, wptr );
    return ret;
}


/******************************************************************************
 * ReadConsoleOutputCharacterW [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputCharacterW( HANDLE hConsoleOutput, LPWSTR buffer, DWORD count,
                                         COORD coord, LPDWORD read_count )
{
    BOOL ret;

    TRACE( "(%d,%p,%ld,%dx%d,%p)\n", hConsoleOutput, buffer, count, coord.X, coord.Y, read_count );

    SERVER_START_REQ( read_console_output )
    {
        req->handle = hConsoleOutput;
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_TEXT;
        req->wrap   = TRUE;
        wine_server_set_reply( req, buffer, count * sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (read_count) *read_count = wine_server_reply_size(reply) / sizeof(WCHAR);
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputAttribute [KERNEL32.@]
 */
BOOL WINAPI ReadConsoleOutputAttribute(HANDLE hConsoleOutput, LPWORD lpAttribute, DWORD length,
                                       COORD coord, LPDWORD read_count)
{
    BOOL ret;

    TRACE("(%d,%p,%ld,%dx%d,%p)\n",
          hConsoleOutput, lpAttribute, length, coord.X, coord.Y, read_count);

    SERVER_START_REQ( read_console_output )
    {
        req->handle = hConsoleOutput;
        req->x      = coord.X;
        req->y      = coord.Y;
        req->mode   = CHAR_INFO_MODE_ATTR;
        req->wrap   = TRUE;
        wine_server_set_reply( req, lpAttribute, length * sizeof(WORD) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (read_count) *read_count = wine_server_reply_size(reply) / sizeof(WORD);
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputA [KERNEL32.@]
 * 
 */
BOOL WINAPI ReadConsoleOutputA( HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD size,
                                COORD coord, LPSMALL_RECT region )
{
    BOOL ret;
    int y;

    ret = ReadConsoleOutputW( hConsoleOutput, lpBuffer, size, coord, region );
    if (ret && region->Right >= region->Left)
    {
        for (y = 0; y <= region->Bottom - region->Top; y++)
        {
            char_info_WtoA( &lpBuffer[(coord.Y + y) * size.X + coord.X],
                            region->Right - region->Left + 1 );
        }
    }
    return ret;
}


/******************************************************************************
 *  ReadConsoleOutputW [KERNEL32.@]
 * 
 * NOTE: The NT4 (sp5) kernel crashes on me if size is (0,0). I don't
 * think we need to be *that* compatible.  -- AJ
 */
BOOL WINAPI ReadConsoleOutputW( HANDLE hConsoleOutput, LPCHAR_INFO lpBuffer, COORD size,
                                COORD coord, LPSMALL_RECT region )
{
    int width, height, y;
    BOOL ret = TRUE;

    width = min( region->Right - region->Left + 1, size.X - coord.X );
    height = min( region->Bottom - region->Top + 1, size.Y - coord.Y );

    if (width > 0 && height > 0)
    {
        for (y = 0; y < height; y++)
        {
            SERVER_START_REQ( read_console_output )
            {
                req->handle = hConsoleOutput;
                req->x      = region->Left;
                req->y      = region->Top + y;
                req->mode   = CHAR_INFO_MODE_TEXTATTR;
                req->wrap   = FALSE;
                wine_server_set_reply( req, &lpBuffer[(y+coord.Y) * size.X + coord.X],
                                       width * sizeof(CHAR_INFO) );
                if ((ret = !wine_server_call_err( req )))
                {
                    width  = min( width, reply->width - region->Left );
                    height = min( height, reply->height - region->Top );
                }
            }
            SERVER_END_REQ;
            if (!ret) break;
        }
    }
    region->Bottom = region->Top + height - 1;
    region->Right = region->Left + width - 1;
    return ret;
}


/******************************************************************************
 * ReadConsoleInputA [KERNEL32.@]  Reads data from a console
 *
 * PARAMS
 *    handle   [I] Handle to console input buffer
 *    buffer   [O] Address of buffer for read data
 *    count    [I] Number of records to read
 *    pRead    [O] Address of number of records read
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ReadConsoleInputA( HANDLE handle, LPINPUT_RECORD buffer, DWORD count, LPDWORD pRead )
{
    DWORD read;

    if (!ReadConsoleInputW( handle, buffer, count, &read )) return FALSE;
    input_records_WtoA( buffer, read );
    if (pRead) *pRead = read;
    return TRUE;
}


/***********************************************************************
 *            PeekConsoleInputA   (KERNEL32.@)
 *
 * Gets 'count' first events (or less) from input queue.
 */
BOOL WINAPI PeekConsoleInputA( HANDLE handle, LPINPUT_RECORD buffer, DWORD count, LPDWORD pRead )
{
    DWORD read;

    if (!PeekConsoleInputW( handle, buffer, count, &read )) return FALSE;
    input_records_WtoA( buffer, read );
    if (pRead) *pRead = read;
    return TRUE;
}


/***********************************************************************
 *            PeekConsoleInputW   (KERNEL32.@)
 */
BOOL WINAPI PeekConsoleInputW( HANDLE handle, LPINPUT_RECORD buffer, DWORD count, LPDWORD read )
{
    BOOL ret;
    SERVER_START_REQ( read_console_input )
    {
        req->handle = handle;
        req->flush  = FALSE;
        wine_server_set_reply( req, buffer, count * sizeof(INPUT_RECORD) );
        if ((ret = !wine_server_call_err( req )))
        {
            if (read) *read = count ? reply->read : 0;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            GetNumberOfConsoleInputEvents   (KERNEL32.@)
 */
BOOL WINAPI GetNumberOfConsoleInputEvents( HANDLE handle, LPDWORD nrofevents )
{
    BOOL ret;
    SERVER_START_REQ( read_console_input )
    {
        req->handle = handle;
        req->flush  = FALSE;
        if ((ret = !wine_server_call_err( req )))
        {
            if (nrofevents) *nrofevents = reply->read;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            FlushConsoleInputBuffer   (KERNEL32.@)
 */
BOOL WINAPI FlushConsoleInputBuffer( HANDLE handle )
{
    BOOL ret;
    SERVER_START_REQ( read_console_input )
    {
        req->handle = handle;
        req->flush  = TRUE;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            SetConsoleTitleA   (KERNEL32.@)
 */
BOOL WINAPI SetConsoleTitleA( LPCSTR title )
{
    LPWSTR titleW;
    BOOL ret;

    DWORD len = MultiByteToWideChar( GetConsoleOutputCP(), 0, title, -1, NULL, 0 );
    if (!(titleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)))) return FALSE;
    MultiByteToWideChar( GetConsoleOutputCP(), 0, title, -1, titleW, len );
    ret = SetConsoleTitleW(titleW);
    HeapFree(GetProcessHeap(), 0, titleW);
    return ret;
}


/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.@)
 */
DWORD WINAPI GetConsoleTitleA(LPSTR title, DWORD size)
{
    WCHAR *ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * size);
    DWORD ret;

    if (!ptr) return 0;
    ret = GetConsoleTitleW( ptr, size );
    if (ret)
    {
        WideCharToMultiByte( GetConsoleOutputCP(), 0, ptr, ret + 1, title, size, NULL, NULL);
        ret = strlen(title);
    }
    return ret;
}


/******************************************************************************
 * GetConsoleTitleW [KERNEL32.@]  Retrieves title string for console
 *
 * PARAMS
 *    title [O] Address of buffer for title
 *    size  [I] Size of buffer
 *
 * RETURNS
 *    Success: Length of string copied
 *    Failure: 0
 */
DWORD WINAPI GetConsoleTitleW(LPWSTR title, DWORD size)
{
    DWORD ret = 0;

    SERVER_START_REQ( get_console_input_info )
    {
        req->handle = 0;
        wine_server_set_reply( req, title, (size-1) * sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            ret = wine_server_reply_size(reply) / sizeof(WCHAR);
            title[ret] = 0;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *            GetLargestConsoleWindowSize   (KERNEL32.@)
 *
 * NOTE
 *	This should return a COORD, but calling convention for returning
 *      structures is different between Windows and gcc on i386.
 *
 * VERSION: [i386]
 */
#ifdef __i386__
#undef GetLargestConsoleWindowSize
DWORD WINAPI GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
    COORD c;
    c.X = 80;
    c.Y = 24;
    return *(DWORD *)&c;
}
#endif /* defined(__i386__) */


/***********************************************************************
 *            GetLargestConsoleWindowSize   (KERNEL32.@)
 *
 * NOTE
 *	This should return a COORD, but calling convention for returning
 *      structures is different between Windows and gcc on i386.
 *
 * VERSION: [!i386]
 */
#ifndef __i386__
COORD WINAPI GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
    COORD c;
    c.X = 80;
    c.Y = 24;
    return c;
}
#endif /* defined(__i386__) */
