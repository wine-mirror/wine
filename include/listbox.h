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
    short	ItemSelect;
    short	ItemsCount;
    short	ItemsVisible;
    short	ItemSelected;
    short	PrevSelected;
    short 	StdItemHeight;
    short	DrawCtlType;
    void	*lpFirst; 
    DWORD	dwStyle;
    HWND	hWndScroll;
    HWND	hWndLogicParent;
} HEADLIST;
typedef HEADLIST FAR* LPHEADLIST;


