/*
 *	OLE2NLS library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "ole.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           GetUserDefaultLCID       (OLE2NLS.1)
 */
DWORD WINAPI GetUserDefaultLCID()
{
/* Default sorting, neutral sublanguage */
#if #LANG(En)
	return 9;
#elif #LANG(De)
	return 7;
#elif #LANG(No)
	return 0x14;
#else
	/* Neutral language */
	return 0;
#endif
}

/***********************************************************************
 *         GetSystemDefaultLCID       (OLE2NLS.2)
 */
DWORD WINAPI GetSystemDefaultLCID()
{
	return GetUserDefaultLCID();
}

/***********************************************************************
 *         GetUserDefaultLangID       (OLE2NLS.3)
 */
WORD WINAPI GetUserDefaultLangID()
{
	return (WORD)GetUserDefaultLCID();
}

/***********************************************************************
 *         GetSystemDefaultLangID     (OLE2NLS.4)
 */
WORD WINAPI GetSystemDefaultLangID()
{
	return GetUserDefaultLangID();
}
