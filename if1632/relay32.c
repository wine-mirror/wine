/*
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "windows.h"
#include "callback.h"
#include "module.h"
#include "neexe.h"
#include "peexe.h"
#include "relay32.h"
#include "struct32.h"
#include "stackframe.h"
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"


LONG RELAY32_CallWindowProcConvStruct( WNDPROC func, int hwnd, int message,
	int wParam, LPARAM lParam16)
{
	WINDOWPOS32 wp;
	union {
		MINMAXINFO32 mmi;
		NCCALCSIZE_PARAMS32 nccs;
		CREATESTRUCT32 cs;
	} st;
	CREATESTRUCT *lpcs;
	LONG result;
	void *lParam;
	if(!lParam16)
		return CallWndProc32(func,hwnd,message,wParam,(int)lParam16);
	lParam = PTR_SEG_TO_LIN(lParam16);
	switch(message) {
		case WM_GETMINMAXINFO:
			STRUCT32_MINMAXINFO16to32(lParam,&st.mmi);
			result=CallWndProc32(func,hwnd,message,wParam,(int)&st.mmi);
			STRUCT32_MINMAXINFO32to16(&st.mmi,lParam);
			return result;
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			STRUCT32_WINDOWPOS16to32(lParam,&wp);
			result=CallWndProc32(func,hwnd,message,wParam,(int)&wp);
			STRUCT32_WINDOWPOS32to16(&wp,lParam);
			return result;
		case WM_NCCALCSIZE:
			STRUCT32_NCCALCSIZE16to32Flat(lParam,&st.nccs);
			if(((NCCALCSIZE_PARAMS*)lParam)->lppos) {
				STRUCT32_WINDOWPOS16to32(((NCCALCSIZE_PARAMS*)lParam)->lppos,&wp);
				st.nccs.lppos=&wp;
			} else
				st.nccs.lppos= 0;
			result=CallWndProc32(func,hwnd,message,wParam,(int)&st.nccs);
			STRUCT32_NCCALCSIZE32to16Flat(&st.nccs,lParam);
			if(((NCCALCSIZE_PARAMS*)lParam)->lppos)
				STRUCT32_WINDOWPOS32to16(&wp,((NCCALCSIZE_PARAMS*)lParam)->lppos);
			return result;
		case WM_NCCREATE:
			lpcs = (CREATESTRUCT*)lParam;
			STRUCT32_CREATESTRUCT16to32(lParam,&st.cs);
			st.cs.lpszName = HIWORD(lpcs->lpszName) ? 
				PTR_SEG_TO_LIN(lpcs->lpszName) : (char*)lpcs->lpszName;
			st.cs.lpszClass = HIWORD(lpcs->lpszClass) ? 
				PTR_SEG_TO_LIN(lpcs->lpszClass) : (char*)lpcs->lpszClass;
			result=CallWndProc32(func,hwnd,message,wParam,(int)&st.cs);
			STRUCT32_CREATESTRUCT32to16(&st.cs,lParam);
			lpcs->lpszName = HIWORD(st.cs.lpszName) ? 
				MAKE_SEGPTR(st.cs.lpszName) : (SEGPTR)st.cs.lpszName;
			lpcs->lpszClass = HIWORD(st.cs.lpszClass) ? 
				MAKE_SEGPTR(st.cs.lpszClass) : (SEGPTR)st.cs.lpszClass;
			return result;
		case WM_GETTEXT:
		case WM_SETTEXT:
			return CallWndProc32(func,hwnd,message,wParam,(int)lParam);
		default:
			fprintf(stderr,"No conversion function for message %d\n",message);
	}
	return CallWndProc32(func,hwnd,message,wParam,(int)lParam);
}
