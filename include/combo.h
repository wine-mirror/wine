/*
 * Combo box definitions
 */


typedef struct tagHEADCOMBO {
    DWORD	dwStyle;
    DWORD	dwState;
    HWND	hWndEdit;
    HWND	hWndLBox;
	short	LastSel;
	RECT	RectEdit;
	BOOL	bRedrawFlag;
} HEADCOMBO;
typedef HEADCOMBO FAR* LPHEADCOMBO;


