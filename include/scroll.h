/*
 *   Scroll Bar definitions
 */


typedef struct tagHEADSSCROLL {
    short	CurVal;
    short	MinVal;
    short	MaxVal;
    short	MaxPix;
    short	CurPix;
    BOOL	ThumbActive;
    WORD	Direction;
    DWORD	dwStyle;
    HWND	hWndUp;
    HWND	hWndDown;
} HEADSCROLL;
typedef HEADSCROLL FAR* LPHEADSCROLL;


