#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>

#include "wineconsole_res.h"

struct inner_data {
    unsigned		sb_width;	/* active screen buffer width */
    unsigned		sb_height;	/* active screen buffer height */
    CHAR_INFO*		cells;		/* local copy of cells (sb_width * sb_height) */
    COORD		win_pos;	/* position (in cells) of visible part of screen buffer in window */
    unsigned		win_width;	/* size (in cells) of visible part of window (width & height) */
    unsigned		win_height;

    COORD		cursor;		/* position in cells of cursor */
    int			cursor_visible;
    int			cursor_size;	/* in % of cell height */

    HANDLE		hConIn;		/* console input handle */
    HANDLE		hConOut;	/* screen buffer handle: has to be changed when active sb changes */
    HANDLE		hSynchro;	/* waitable handle signalled by server when something in server has been modified */

    int			(*fnMainLoop)(struct inner_data* data);
    void		(*fnPosCursor)(const struct inner_data* data);
    void		(*fnShapeCursor)(struct inner_data* data, int size, int vis, BOOL force);
    void		(*fnComputePositions)(struct inner_data* data);
    void		(*fnRefresh)(const struct inner_data* data, int tp, int bm);
    void		(*fnResizeScreenBuffer)(struct inner_data* data);
    void		(*fnSetTitle)(const struct inner_data* data);
    void		(*fnScroll)(struct inner_data* data, int pos, BOOL horz);
    void		(*fnDeleteBackend)(struct inner_data* data);

    /* the following fields are only user by the USER backend (should be hidden in user) */
    HWND		hWnd;		/* handle to windows for rendering */
    HFONT		hFont;		/* font used for rendering, usually fixed */
    LOGFONT		logFont;	/* logFont dscription for used hFont */
    unsigned		cell_width;	/* width in pixels of a character */	
    unsigned		cell_height;	/* height in pixels of a character */
    HDC			hMemDC;		/* memory DC holding the bitmap below */
    HBITMAP		hBitmap;	/* bitmap of display window content */

    HBITMAP		cursor_bitmap;  /* bitmap used for the caret */
    BOOL		hasSelection;	/* a rectangular mouse selection has taken place */
    COORD		selectPt1;	/* start (and end) point of a mouse selection */
    COORD		selectPt2;
};

#  ifdef __GNUC__
extern void  XTracer(int level, const char* format, ...) __attribute__((format (printf,2,3)));
#  else
extern void  XTracer(int level, const char* format, ...);
#  endif
#if 0
/* Trace mode */
#  define Trace XTracer
#else
/* non trace mode */
#  define Trace (1) ? (void)0 : XTracer
#endif

extern void WINECON_NotifyWindowChange(struct inner_data* data);
extern int  WINECON_GetHistorySize(HANDLE hConIn);
extern BOOL WINECON_SetHistorySize(HANDLE hConIn, int size);
extern int  WINECON_GetHistoryMode(HANDLE hConIn);
extern BOOL WINECON_SetHistoryMode(HANDLE hConIn, int mode);
extern BOOL WINECON_GetConsoleTitle(HANDLE hConIn, WCHAR* buffer, size_t len);
extern void WINECON_FetchCells(struct inner_data* data, int upd_tp, int upd_bm);
extern int  WINECON_GrabChanges(struct inner_data* data);

extern BOOL WCUSER_GetProperties(struct inner_data*);
extern BOOL WCUSER_SetFont(struct inner_data* data, const LOGFONT* font, const TEXTMETRIC* tm);
extern BOOL WCUSER_ValidateFont(const struct inner_data* data, const LOGFONT* lf);
extern BOOL WCUSER_ValidateFontMetric(const struct inner_data* data, const TEXTMETRIC* tm);
extern BOOL WCUSER_InitBackend(struct inner_data* data);
