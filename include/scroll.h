/*
 *   Scroll Bar definitions
 */


typedef struct tagHEADSSCROLL {
    short	CurVal;
    short	MinVal;
    short	MaxVal;
    short	MaxPix;
    short	CurPix;
    short      ThumbVal;
    RECT	rect;
    RECT	rectUp;
    RECT	rectDown;
    BOOL	ThumbActive;
    BOOL	TimerPending;
    WORD	ButtonDown;
    WORD	Direction;
    DWORD	dwStyle;
    HWND	hWndOwner;
} HEADSCROLL;
typedef HEADSCROLL FAR* LPHEADSCROLL;



void ScrollBarButtonDown(HWND hWnd, int nBar, int x, int y);
void ScrollBarButtonUp(HWND hWnd, int nBar, int x, int y);
void ScrollBarMouseMove(HWND hWnd, int nBar, WORD wParam, int x, int y);
void StdDrawScrollBar(HWND hWnd, HDC hDC, int nBar, LPRECT lprect, LPHEADSCROLL lphs);
int CreateScrollBarStruct(HWND hWnd);
void NC_CreateScrollBars(HWND hWnd);


