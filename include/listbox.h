/*
 *   List Box definitions
 */


typedef struct tagLISTSTRUCT {
    DRAWITEMSTRUCT 	dis;
    HANDLE		hMem;
    HANDLE		hData;
    void		*lpNext;
} LISTSTRUCT;
typedef LISTSTRUCT FAR* LPLISTSTRUCT;


typedef struct tagHEADLIST {
    short	FirstVisible;
    short	ItemsCount;
    short	ItemsVisible;
    short	ColumnsVisible;
    short	ItemsPerColumn;
    short	ItemFocused;
    short	PrevFocused;
    short 	StdItemHeight;
    short	ColumnsWidth;
    short	DrawCtlType;
    void	*lpFirst; 
    DWORD	dwStyle;
    HWND	hWndLogicParent;
	HFONT	hFont;
} HEADLIST;
typedef HEADLIST FAR* LPHEADLIST;


