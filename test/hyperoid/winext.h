#if !defined(WINEXT_H)
#define WINEXT_H

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

//
// WINEXT.H - additional windows definitions
//
// Version 1.0  03/21/89  Copyright (C) 1989,90,91 Lantern Coroporation.
// Author: Edward Hutchins
// Status: Freeware
// Revisions:
// 06/06/90 modified HUGE to LARGE to preserve compatibility with math.h
//          also nested windows.h include to prevent modifications on the
//          actual source file - Ed.
// 10/01/90 added CONST and CONSTP,NP,LP,HP and a few comments - Ed.
// 08/28/91 added EXPORT and SEGMENT - Ed.
// 10/28/91 added DLLEXP - Ed.
// 11/02/91 posted on Compuserve - Ed.
//

//
// include WINDOWS.H, if needed
//

#if !defined(WINDOWS_H)
	#if defined(NULL)
		#undef NULL
	#endif
	#include <windows.h>
	#if !defined(NULL)
		#define NULL 0
	#endif
	#define WINDOWS_H
#endif // WINDOWS_H

//
// extra data types and defines
//

#define LARGE huge
#define CDECL cdecl
#define CONST const
#define HNULL (0)
#define LPNULL (0L)

typedef char        CHAR;
typedef int         INT;
typedef float       FLOAT;
typedef double      DOUBLE;
typedef long double LDOUBLE;

// extend the string type
typedef CHAR LARGE  *HPSTR;

// useful macros for typedefing pointers to objects //
#define npointerdef(o) typedef o NEAR * NP ## o
#define lpointerdef(o) typedef o FAR * LP ## o
#define hpointerdef(o) typedef o LARGE * HP ## o
#define pointerdef(o) npointerdef(o); lpointerdef(o); hpointerdef(o)

// define the different kinds of pointers to things //
pointerdef( BOOL );
npointerdef( BYTE ); hptrdef( BYTE );
pointerdef( CHAR );
npointerdef( INT ); hpointerdef( INT );
npointerdef( WORD ); hpointerdef( WORD );
npointerdef( LONG ); hpointerdef( LONG );
npointerdef( DWORD ); hpointerdef( DWORD );
pointerdef( FLOAT );
pointerdef( DOUBLE );
pointerdef( LDOUBLE );
npointerdef( HANDLE ); hpointerdef( HANDLE );
npointerdef( VOID ); hpointerdef( VOID );

// these are here for compatibility - use NPVOID etc...
typedef VOID NEAR *NPMEM;
typedef VOID FAR *LPMEM;

// window proc function pointer
typedef LONG (FAR PASCAL *WNDPROC)( HWND, unsigned, WORD, LONG );

//
// scope protocol definitions
//

#define GLOBAL      // GLOBAL
#define LOCAL       static
#define IMPORT      extern
#define FROM(where) // FROM where
#define PROTO       // PROTOTYPE

#define EXPORT _export
#define SEGMENT _segment

#if defined(__cplusplus)
}
#endif // __cplusplus

// c++ class export type
#if defined(__DLL__)
#define DLLEXP EXPORT
#else
#define DLLEXP LARGE
#endif

#endif // WINEXT_H
