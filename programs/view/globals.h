
/* for SMALL_RECT */
#include "wincon.h" 

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


#pragma pack(1)
typedef struct
{
	DWORD		key;
	WORD		hmf;
	SMALL_RECT	bbox;
	WORD		inch;
	DWORD		reserved;
	WORD		checksum;
} APMFILEHEADER;

#define APMHEADER_KEY	0x9AC6CDD7l

#pragma pack(4)
