/*
 * an application for displaying Win32 console
 *
 * Copyright 2001 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include "wine/server.h"
#include "wine/unicode.h"
#include "winecon_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineconsole);

/******************************************************************
 *		WINECON_FetchCells
 *
 * updates the local copy of cells (band to update)
 */
void WINECON_FetchCells(struct inner_data* data, int upd_tp, int upd_bm)
{
    SERVER_START_REQ( read_console_output )
    {
        req->handle = (handle_t)data->hConOut;
        req->x      = 0;
        req->y      = upd_tp;
        req->mode   = CHAR_INFO_MODE_TEXTATTR;
        req->wrap   = TRUE;
        wine_server_set_reply( req, &data->cells[upd_tp * data->curcfg.sb_width],
                               (upd_bm-upd_tp+1) * data->curcfg.sb_width * sizeof(CHAR_INFO) );
        wine_server_call( req );
    }
    SERVER_END_REQ;
    data->fnRefresh(data, upd_tp, upd_bm);
}

/******************************************************************
 *		WINECON_NotifyWindowChange
 *
 * Inform server that visible window on sb has changed
 */
void WINECON_NotifyWindowChange(struct inner_data* data)
{
    SERVER_START_REQ( set_console_output_info )
    {
        req->handle       = (handle_t)data->hConOut;
        req->win_left     = data->curcfg.win_pos.X;
        req->win_top      = data->curcfg.win_pos.Y;
        req->win_right    = data->curcfg.win_pos.X + data->curcfg.win_width - 1;
        req->win_bottom   = data->curcfg.win_pos.Y + data->curcfg.win_height - 1;
        req->mask         = SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

/******************************************************************
 *		WINECON_GetHistorySize
 *
 *
 */
int	WINECON_GetHistorySize(HANDLE hConIn)
{
    int	ret = 0;

    SERVER_START_REQ(get_console_input_info)
    {
	req->handle = (handle_t)hConIn;
	if (!wine_server_call_err( req )) ret = reply->history_size;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		WINECON_SetHistorySize
 *
 *
 */
BOOL	WINECON_SetHistorySize(HANDLE hConIn, int size)
{
    BOOL	ret;

    SERVER_START_REQ(set_console_input_info)
    {
	req->handle = (handle_t)hConIn;
	req->mask = SET_CONSOLE_INPUT_INFO_HISTORY_SIZE;
	req->history_size = size;
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************
 *		WINECON_GetHistoryMode
 *
 *
 */
int	WINECON_GetHistoryMode(HANDLE hConIn)
{
    int	ret = 0;

    SERVER_START_REQ(get_console_input_info)
    {
	req->handle = (handle_t)hConIn;
	if (!wine_server_call_err( req )) ret = reply->history_mode;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		WINECON_SetHistoryMode
 *
 *
 */
BOOL	WINECON_SetHistoryMode(HANDLE hConIn, int mode)
{
    BOOL	ret;

    SERVER_START_REQ(set_console_input_info)
    {
	req->handle = (handle_t)hConIn;
	req->mask = SET_CONSOLE_INPUT_INFO_HISTORY_MODE;
	req->history_mode = mode;
	ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		WINECON_GetConsoleTitle
 *
 *
 */
BOOL WINECON_GetConsoleTitle(HANDLE hConIn, WCHAR* buffer, size_t len)
{
    BOOL ret;

    if (len < sizeof(WCHAR)) return FALSE;

    SERVER_START_REQ( get_console_input_info )
    {
        req->handle = (handle_t)hConIn;
        wine_server_set_reply( req, buffer, len - sizeof(WCHAR) );
        if ((ret = !wine_server_call_err( req )))
        {
            len = wine_server_reply_size( reply );
            buffer[len / sizeof(WCHAR)] = 0;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		WINECON_GrabChanges
 *
 * A change occurs, try to figure out which
 */
int	WINECON_GrabChanges(struct inner_data* data)
{
    struct console_renderer_event	evts[256];
    int	i, num, curs = -1;
    HANDLE h;

    SERVER_START_REQ( get_console_renderer_events )
    {
        wine_server_set_reply( req, evts, sizeof(evts) );
        req->handle = (handle_t)data->hSynchro;
        if (!wine_server_call_err( req )) num = wine_server_reply_size(reply) / sizeof(evts[0]);
        else num = 0;
    }
    SERVER_END_REQ;
    if (!num) {WINE_WARN("hmm renderer signaled but no events available\n"); return 1;}
    
    /* FIXME: should do some event compression here (cursor pos, update) */
    /* step 1: keep only last cursor pos event */
    for (i = num - 1; i >= 0; i--)
    {
        if (evts[i].event == CONSOLE_RENDERER_CURSOR_POS_EVENT)
        {
            if (curs == -1)
                curs = i;
            else
            {
                memmove(&evts[i], &evts[i+1], (num - i - 1) * sizeof(evts[0]));
                num--;
            }
        }
    }
    /* step 2: manage update events */
    for (i = 0; i < num - 1; i++)
    {
        if (evts[i].event == CONSOLE_RENDERER_UPDATE_EVENT &&
            evts[i+1].event == CONSOLE_RENDERER_UPDATE_EVENT)
        {
            /* contiguous */
            if (evts[i].u.update.bottom + 1 == evts[i+1].u.update.top)
            {
                evts[i].u.update.bottom = evts[i+1].u.update.bottom;
                memmove(&evts[i+1], &evts[i+2], (num - i - 2) * sizeof(evts[0]));
                num--; i--;
            }
            /* already handled cases */
            else if (evts[i].u.update.top <= evts[i+1].u.update.top &&
                     evts[i].u.update.bottom >= evts[i+1].u.update.bottom)
            {
                memmove(&evts[i+1], &evts[i+2], (num - i - 2) * sizeof(evts[0]));
                num--; i--;
            }
        }
    }
   
    WINE_TRACE("Events:");
    for (i = 0; i < num; i++)
    {
	switch (evts[i].event)
	{
	case CONSOLE_RENDERER_TITLE_EVENT:
	    data->fnSetTitle(data);
	    break;
	case CONSOLE_RENDERER_ACTIVE_SB_EVENT:
	    SERVER_START_REQ( open_console )
	    {
		req->from    = (int)data->hConIn;
		req->access  = GENERIC_READ | GENERIC_WRITE;
		req->share   = FILE_SHARE_READ | FILE_SHARE_WRITE;
		req->inherit = FALSE;
		h = wine_server_call_err( req ) ? 0 : (HANDLE)reply->handle;
	    }
	    SERVER_END_REQ;
	    if (WINE_TRACE_ON(wineconsole)) 
                WINE_DPRINTF(" active(%d)", (int)h);
	    if (h)
	    {
		CloseHandle(data->hConOut);
		data->hConOut = h;
	    }
	    break;
	case CONSOLE_RENDERER_SB_RESIZE_EVENT:
	    if (data->curcfg.sb_width != evts[i].u.resize.width || 
		data->curcfg.sb_height != evts[i].u.resize.height)
	    {
		if (WINE_TRACE_ON(wineconsole)) 
                    WINE_DPRINTF(" resize(%d,%d)", evts[i].u.resize.width, evts[i].u.resize.height);
		data->curcfg.sb_width  = evts[i].u.resize.width;
		data->curcfg.sb_height = evts[i].u.resize.height;
		
		data->cells = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, data->cells,
					  data->curcfg.sb_width * data->curcfg.sb_height * sizeof(CHAR_INFO));
		if (!data->cells) {WINE_ERR("OOM\n"); exit(0);}
		data->fnResizeScreenBuffer(data);
		data->fnComputePositions(data);
	    }
	    break;
	case CONSOLE_RENDERER_UPDATE_EVENT:
	    if (WINE_TRACE_ON(wineconsole)) 
                WINE_DPRINTF(" update(%d,%d)", evts[i].u.update.top, evts[i].u.update.bottom);
	    WINECON_FetchCells(data, evts[i].u.update.top, evts[i].u.update.bottom);
	    break;
	case CONSOLE_RENDERER_CURSOR_POS_EVENT:
	    if (evts[i].u.cursor_pos.x != data->cursor.X || evts[i].u.cursor_pos.y != data->cursor.Y)
	    {	
		data->cursor.X = evts[i].u.cursor_pos.x;
		data->cursor.Y = evts[i].u.cursor_pos.y;
		data->fnPosCursor(data);
		if (WINE_TRACE_ON(wineconsole)) 
                    WINE_DPRINTF(" curs-pos(%d,%d)",evts[i].u.cursor_pos.x, evts[i].u.cursor_pos.y);
	    }
	    break;
	case CONSOLE_RENDERER_CURSOR_GEOM_EVENT:
	    if (evts[i].u.cursor_geom.size != data->curcfg.cursor_size || 
		evts[i].u.cursor_geom.visible != data->curcfg.cursor_visible)
	    {
		data->fnShapeCursor(data, evts[i].u.cursor_geom.size, 
				    evts[i].u.cursor_geom.visible, FALSE);
		if (WINE_TRACE_ON(wineconsole)) 
                    WINE_DPRINTF(" curs-geom(%d,%d)", 
                                 evts[i].u.cursor_geom.size, evts[i].u.cursor_geom.visible);
	    }
	    break;
	case CONSOLE_RENDERER_DISPLAY_EVENT:
	    if (evts[i].u.display.left != data->curcfg.win_pos.X)
	    {
		data->fnScroll(data, evts[i].u.display.left, TRUE);
		data->fnPosCursor(data);
		if (WINE_TRACE_ON(wineconsole)) 
                    WINE_DPRINTF(" h-scroll(%d)", evts[i].u.display.left);
	    }
	    if (evts[i].u.display.top != data->curcfg.win_pos.Y)
	    {
		data->fnScroll(data, evts[i].u.display.top, FALSE);
		data->fnPosCursor(data);
		if (WINE_TRACE_ON(wineconsole)) 
                    WINE_DPRINTF(" v-scroll(%d)", evts[i].u.display.top);
	    }
	    if (evts[i].u.display.width != data->curcfg.win_width || 
		evts[i].u.display.height != data->curcfg.win_height)
	    {
		if (WINE_TRACE_ON(wineconsole)) 
                    WINE_DPRINTF(" win-size(%d,%d)", evts[i].u.display.width, evts[i].u.display.height);
		data->curcfg.win_width = evts[i].u.display.width;
		data->curcfg.win_height = evts[i].u.display.height;
		data->fnComputePositions(data);
	    }
	    break;
	case CONSOLE_RENDERER_EXIT_EVENT:
	    if (WINE_TRACE_ON(wineconsole)) WINE_DPRINTF(". Exit!!\n");
	    return 0;
	default:
	    WINE_FIXME("Unknown event type (%d)\n", evts[i].event);
	}
    }

    if (WINE_TRACE_ON(wineconsole)) WINE_DPRINTF(".\n");
    return 1;
}

/******************************************************************
 *		WINECON_Delete
 *
 * Destroy wineconsole internal data
 */
static void WINECON_Delete(struct inner_data* data)
{
    if (!data) return;

    if (data->hConIn)		CloseHandle(data->hConIn);
    if (data->hConOut)		CloseHandle(data->hConOut);
    if (data->hSynchro)		CloseHandle(data->hSynchro);
    if (data->cells)		HeapFree(GetProcessHeap(), 0, data->cells);
    if (data->fnDeleteBackend)  data->fnDeleteBackend(data);
    HeapFree(GetProcessHeap(), 0, data);
}

/******************************************************************
 *		WINECON_Init
 *
 * Initialisation part I. Creation of server object (console input and
 * active screen buffer)
 */
static struct inner_data* WINECON_Init(HINSTANCE hInst, void* pid)
{
    struct inner_data*	data = NULL;
    DWORD		ret;
    WCHAR		szTitle[] = {'W','i','n','e',' ','c','o','n','s','o','l','e',0};

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
    if (!data) return 0;

    /* load default registry settings, and copy them into our current configuration */
    WINECON_RegLoad(&data->defcfg);
    data->curcfg = data->defcfg;

    /* the handles here are created without the whistles and bells required by console
     * (mainly because wineconsole doesn't need it)
     * - there are not inheritable
     * - hConIn is not synchronizable
     */
    SERVER_START_REQ(alloc_console)
    {
        req->access  = GENERIC_READ | GENERIC_WRITE;
        req->inherit = FALSE;
	req->pid     = pid;
        ret = !wine_server_call_err( req );
        data->hConIn = (HANDLE)reply->handle_in;
	data->hSynchro = (HANDLE)reply->event;
    }
    SERVER_END_REQ;
    if (!ret) goto error;

    SERVER_START_REQ( set_console_input_info )
    {
        req->handle = (handle_t)data->hConIn;
        req->mask = SET_CONSOLE_INPUT_INFO_TITLE;
        wine_server_add_data( req, szTitle, strlenW(szTitle) * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    if (!ret) goto error;

    SERVER_START_REQ(create_console_output)
    {
        req->handle_in = (handle_t)data->hConIn;
        req->access    = GENERIC_WRITE|GENERIC_READ;
        req->share     = FILE_SHARE_READ|FILE_SHARE_WRITE;
        req->inherit   = FALSE;
        data->hConOut  = (HANDLE)(wine_server_call_err( req ) ? 0 : reply->handle_out);
    }
    SERVER_END_REQ;
    if (data->hConOut) return data;

 error:
    WINECON_Delete(data);
    return NULL;
}

/******************************************************************
 *		WINECON_Spawn
 *
 * Spawn the child processus when invoked with wineconsole foo bar
 */
static BOOL WINECON_Spawn(struct inner_data* data, LPCSTR lpCmdLine)
{
    PROCESS_INFORMATION	info;
    STARTUPINFO		startup;
    LPWSTR		ptr = GetCommandLine(); /* we're unicode... */
    BOOL		done;

    /* we're in the case wineconsole <exe> <options>... spawn the new process */
    memset(&startup, 0, sizeof(startup));
    startup.cb          = sizeof(startup);
    startup.dwFlags     = STARTF_USESTDHANDLES;

    /* the attributes of wineconsole's handles are not adequate for inheritance, so
     * get them with the correct attributes before process creation
     */
    if (!DuplicateHandle(GetCurrentProcess(), data->hConIn,  GetCurrentProcess(),
			 &startup.hStdInput, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE, TRUE, 0) ||
	!DuplicateHandle(GetCurrentProcess(), data->hConOut, GetCurrentProcess(), 
			 &startup.hStdOutput, GENERIC_READ|GENERIC_WRITE, TRUE, 0) ||
	!DuplicateHandle(GetCurrentProcess(), data->hConOut, GetCurrentProcess(), 
			     &startup.hStdError, GENERIC_READ|GENERIC_WRITE, TRUE, 0))
    {
	WINE_ERR("Can't dup handles\n");
	/* no need to delete handles, we're exiting the programm anyway */
	return FALSE;
    }

    /* we could have several ' ' in process command line... so try first space...
     * FIXME:
     * the correct way would be to check the existence of the left part of ptr
     * (to be a file)
     */
    while (*ptr && *ptr++ != ' ');

    done = *ptr && CreateProcess(NULL, ptr, NULL, NULL, TRUE, 0L, NULL, NULL, &startup, &info);
    
    /* we no longer need the handles passed to the child for the console */
    CloseHandle(startup.hStdInput);
    CloseHandle(startup.hStdOutput);
    CloseHandle(startup.hStdError);

    return done;
}

/******************************************************************
 *		 WINECON_HasEvent
 *
 *
 */
static BOOL WINECON_HasEvent(LPCSTR ptr, unsigned *evt)
{
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (strncmp(ptr, "--use-event=", 12)) return FALSE;
    return sscanf(ptr + 12, "%d", evt) == 1;
}

/******************************************************************
 *		WinMain
 *
 * wineconsole can either be started as:
 *	wineconsole --use-event=<int>	used when a new console is created (AllocConsole)
 *	wineconsole <pgm> <arguments>	used to start the program <pgm> from the command line in
 *					a freshly created console
 */
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, INT nCmdShow)
{
    struct inner_data*	data;
    int			ret = 1;
    unsigned		evt;

    /* case of wineconsole <evt>, signal process that created us that we're up and running */
    if (WINECON_HasEvent(lpCmdLine, &evt))
    {
        if (!(data = WINECON_Init(hInst, 0))) return 0;
	ret = SetEvent((HANDLE)evt);
    }
    else
    {
        if (!(data = WINECON_Init(hInst, (void*)GetCurrentProcessId()))) return 0;
	ret = WINECON_Spawn(data, lpCmdLine);
    }

    if (ret && WCUSER_InitBackend(data))
    {
	ret = data->fnMainLoop(data);
    }
    WINECON_Delete(data);

    return ret;
}
