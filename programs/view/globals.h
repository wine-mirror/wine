
/*  Add global function prototypes here */

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
BOOL CenterWindow(HWND, HWND);

/* Add new callback function prototypes here  */

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/* Global variable declarations */

extern HINSTANCE hInst;          /* The current instance handle */
extern char      szAppName[];    /* The name of this application */
extern char      szTitle[];      /* The title bar text */


#ifdef WINELIB
typedef struct
{
	DWORD	key WINE_PACKED;
	HANDLE16	hmf WINE_PACKED;
	RECT16	bbox WINE_PACKED;
	WORD	inch WINE_PACKED;
	DWORD	reserved WINE_PACKED;
	WORD	checksum WINE_PACKED;
} APMFILEHEADER WINE_PACKED;
#else
#pragma pack( 2 )
typedef struct
{
	DWORD		key;
	WORD		hmf;
	SMALL_RECT	bbox;
	WORD		inch;
	DWORD		reserved;
	WORD		checksum;
} APMFILEHEADER;
#endif

#define APMHEADER_KEY	0x9AC6CDD7l


