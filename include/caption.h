/*
 *   Caption Bar definitions
 */


typedef struct tagHEADCAPTION {
    HBITMAP	hClose;
    HBITMAP	hMinim;
    HBITMAP	hMaxim;
    HMENU	hSysMenu;
    RECT	rectClose;
    RECT	rectMinim;
    RECT	rectMaxim;
} HEADCAPTION;
typedef HEADCAPTION FAR* LPHEADCAPTION;


