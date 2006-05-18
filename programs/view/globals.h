/*
 * Copyright 1998 Douglas Ridgway
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* for SMALL_RECT */
#include "wincon.h"
#include "resource.h"

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


#include "pshpack1.h"
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

#include "poppack.h"
