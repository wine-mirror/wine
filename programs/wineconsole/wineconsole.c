/*
 * an application for displaying Win32 console
 *
 * Copyright 2001 Eric Pouech
 */

#include <stdio.h>
#include <wine/server.h>
#include "winecon_private.h"

static int trace_level = 1;
void      XTracer(int level, const char* format, ...)
{
    char        buf[1024];
    va_list     valist;
    int         len;

    if (level > trace_level) return;

    va_start(valist, format);
    len = wvsnprintfA(buf, sizeof(buf), format, valist);
    va_end(valist);
 
    if (len <= -1) 
    {
        len = sizeof(buf) - 1;
        buf[len] = 0;
        buf[len - 1] = buf[len - 2] = buf[len - 3] = '.';
    }
    fprintf(stderr, buf);
}

/******************************************************************
 *		WINECON_FetchCells
 *
 * updates the local copy of cells (band to update)
 */
void	WINECON_FetchCells(struct inner_data* data, int upd_tp, int upd_bm)
{
    int		step;
    int		j, nr;

    step = REQUEST_MAX_VAR_SIZE / (data->sb_width * 4);

    for (j = upd_tp; j <= upd_bm; j += step)
    {
	nr = min(step, upd_bm - j + 1);
	SERVER_START_VAR_REQ( read_console_output, 4 * nr * data->sb_width )
	{
	    req->handle       = (handle_t)data->hConOut;
	    req->x            = 0;
	    req->y            = j;
	    req->w            = data->sb_width;
	    req->h            = nr;
	    if (!SERVER_CALL_ERR())
	    {
		if (data->sb_width != req->eff_w || nr != req->eff_h) 
		    Trace(0, "pb here... wrong eff_w %d/%d or eff_h %d/%d\n", 
			  req->eff_w, data->sb_width, req->eff_h, nr);
		memcpy(&data->cells[j * data->sb_width], server_data_ptr(req), 
		       4 * nr * data->sb_width);
	    }
	}
	SERVER_END_VAR_REQ;
    }
    data->fnRefresh(data, upd_tp, upd_bm);
}

/******************************************************************
 *		WINECON_NotifyWindowChange
 *
 * Inform server that visible window on sb has changed
 */
void	WINECON_NotifyWindowChange(struct inner_data* data)
{
    SERVER_START_REQ( set_console_output_info )
    {
	req->handle       = (handle_t)data->hConOut;
	req->win_left     = data->win_pos.X;
	req->win_top      = data->win_pos.Y;
	req->win_right    = data->win_pos.X + data->win_width - 1;
	req->win_bottom   = data->win_pos.Y + data->win_height - 1;
	req->mask         = SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW;
	if (!SERVER_CALL_ERR())
	{
	}
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
	if (!SERVER_CALL_ERR()) ret = req->history_size;
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
	ret = !SERVER_CALL_ERR();
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
	if (!SERVER_CALL_ERR()) ret = req->history_mode;
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
	ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *		WINECON_GetConsoleTitle
 *
 *
 */
BOOL	WINECON_GetConsoleTitle(HANDLE hConIn, WCHAR* buffer, size_t len)
{
    BOOL	ret;
    DWORD 	size = 0;

    SERVER_START_VAR_REQ(get_console_input_info, sizeof(buffer))
    {
	req->handle = (handle_t)hConIn;
        if ((ret = !SERVER_CALL_ERR()))
        {
            size = min(len - sizeof(WCHAR), server_data_size(req));
            memcpy(buffer, server_data_ptr(req), size);
            buffer[size / sizeof(WCHAR)] = 0;
        }
    }
    SERVER_END_VAR_REQ;
    return ret;
}

/******************************************************************
 *		WINECON_GrabChanges
 *
 * A change occurs, try to figure out which
 */
int	WINECON_GrabChanges(struct inner_data* data)
{
    struct console_renderer_event	evts[16];
    int	i, num;
    HANDLE h;

    SERVER_START_VAR_REQ( get_console_renderer_events, sizeof(evts) )
    {
	req->handle       = (handle_t)data->hSynchro;
	if (!SERVER_CALL_ERR())
	{
	    num = server_data_size(req);
	    memcpy(evts, server_data_ptr(req), num);
	    num /= sizeof(evts[0]);
	}
	else num = 0;
    }
    SERVER_END_VAR_REQ;
    if (!num) {Trace(0, "hmm renderer signaled but no events available\n"); return 1;}
    
    /* FIXME: should do some event compression here (cursor pos, update) */
    Trace(1, "Change notification:");
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
		h = SERVER_CALL_ERR() ? 0 : (HANDLE)req->handle;
	    }
	    SERVER_END_REQ;
	    Trace(1, " active(%d)", (int)h);
	    if (h)
	    {
		CloseHandle(data->hConOut);
		data->hConOut = h;
	    }
	    break;
	case CONSOLE_RENDERER_SB_RESIZE_EVENT:
	    if (data->sb_width != evts[i].u.resize.width || 
		data->sb_height != evts[i].u.resize.height)
	    {
		Trace(1, " resize(%d,%d)", evts[i].u.resize.width, evts[i].u.resize.height);
		data->sb_width  = evts[i].u.resize.width;
		data->sb_height = evts[i].u.resize.height;
		
		data->cells = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, data->cells,
					  data->sb_width * data->sb_height * sizeof(CHAR_INFO));
		if (!data->cells) {Trace(0, "OOM\n"); exit(0);}
		data->fnResizeScreenBuffer(data);
		data->fnComputePositions(data);
	    }
	    break;
	case CONSOLE_RENDERER_UPDATE_EVENT:
	    Trace(1, " update(%d,%d)", evts[i].u.update.top, evts[i].u.update.bottom);
	    WINECON_FetchCells(data, evts[i].u.update.top, evts[i].u.update.bottom);
	    break;
	case CONSOLE_RENDERER_CURSOR_POS_EVENT:
	    if (evts[i].u.cursor_pos.x != data->cursor.X || evts[i].u.cursor_pos.y != data->cursor.Y)
	    {	
		data->cursor.X = evts[i].u.cursor_pos.x;
		data->cursor.Y = evts[i].u.cursor_pos.y;
		data->fnPosCursor(data);
		Trace(1, " curs-pos(%d,%d)",evts[i].u.cursor_pos.x, evts[i].u.cursor_pos.y);
	    }
	    break;
	case CONSOLE_RENDERER_CURSOR_GEOM_EVENT:
	    if (evts[i].u.cursor_geom.size != data->cursor_size || 
		evts[i].u.cursor_geom.visible != data->cursor_visible)
	    {
		data->fnShapeCursor(data, evts[i].u.cursor_geom.size, 
				    evts[i].u.cursor_geom.visible, FALSE);
		Trace(1, " curs-geom(%d,%d)", 
		      evts[i].u.cursor_geom.size, evts[i].u.cursor_geom.visible);
	    }
	    break;
	case CONSOLE_RENDERER_DISPLAY_EVENT:
	    if (evts[i].u.display.left != data->win_pos.X)
	    {
		data->fnScroll(data, evts[i].u.display.left, TRUE);
		data->fnPosCursor(data);
		Trace(1, " h-scroll(%d)", evts[i].u.display.left);
	    }
	    if (evts[i].u.display.top != data->win_pos.Y)
	    {
		data->fnScroll(data, evts[i].u.display.top, FALSE);
		data->fnPosCursor(data);
		Trace(1, " v-scroll(%d)", evts[i].u.display.top);
	    }
	    if (evts[i].u.display.width != data->win_width || 
		evts[i].u.display.height != data->win_height)
	    {
		Trace(1, " win-size(%d,%d)", evts[i].u.display.width, evts[i].u.display.height);
		data->win_width = evts[i].u.display.width;
		data->win_height = evts[i].u.display.height;
		data->fnComputePositions(data);
	    }
	    break;
	case CONSOLE_RENDERER_EXIT_EVENT:
	    Trace(1, ". Exit!!\n");
	    return 0;
	default:
	    Trace(0, "Unknown event type (%d)\n", evts[i].event);
	}
    }

    Trace(1, ". Done\n");
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
    data->fnDeleteBackend(data);
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
    size_t		len;

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
    if (!data) return 0;

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
        ret = !SERVER_CALL_ERR();
        data->hConIn = (HANDLE)req->handle_in;
	data->hSynchro = (HANDLE)req->event;
    }
    SERVER_END_REQ;
    if (!ret) goto error;

    len = lstrlenW(szTitle) * sizeof(WCHAR);
    len = min(len, REQUEST_MAX_VAR_SIZE);

    SERVER_START_VAR_REQ(set_console_input_info, len)
    {
	req->handle = (handle_t)data->hConIn;
        req->mask = SET_CONSOLE_INPUT_INFO_TITLE;
        memcpy(server_data_ptr(req), szTitle, len);
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_VAR_REQ;

    if (ret) 
    {    
	SERVER_START_REQ(create_console_output)
	{
	    req->handle_in = (handle_t)data->hConIn;
	    req->access    = GENERIC_WRITE|GENERIC_READ;
	    req->share     = FILE_SHARE_READ|FILE_SHARE_WRITE;
	    req->inherit   = FALSE;
	    data->hConOut  = (HANDLE)(SERVER_CALL_ERR() ? 0 : req->handle_out);
	}
	SERVER_END_REQ;
	if (data->hConOut) return data;
    }

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
    startup.dwFlags     = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_SHOWNORMAL;

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
	Trace(0, "can't dup handles\n");
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

static BOOL WINECON_HasEvent(LPCSTR ptr, unsigned *evt)
{
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (strncmp(ptr, "--use-event=", 12)) return FALSE;
    return sscanf(ptr + 12, "%d", evt) == 1;
}

/******************************************************************
 *		WINECON_WinMain
 *
 * wineconsole can either be started as:
 *	wineconsole --use-event=<int>	used when a new console is created (AllocConsole)
 *	wineconsole <pgm> <arguments>	used to start the program <pgm> from the command line in
 *					a freshly created console
 */
int PASCAL WINECON_WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPCSTR lpCmdLine, UINT nCmdShow)
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
