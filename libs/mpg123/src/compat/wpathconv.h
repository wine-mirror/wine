/*
	wpathconv: static functions for windows file path conversions

	This file is intended to be included in libcompat sources for internal use.
	It is separated out to be able to split off the dlopen functions into a
	separate libcompat. It is just a code fragment.

	copyright 2007-2019 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by JonY
*/

#ifndef WINDOWS_UWP

#ifdef WANT_WIN32_UNICODE

/* Convert unix UTF-8 (or ASCII) paths to Windows wide character paths. */
static wchar_t* u2wpath(const char *upath)
{
	wchar_t* wpath, *p;
	if(!upath || win32_utf8_wide(upath, &wpath, NULL) < 1)
		return NULL;
	for(p=wpath; *p; ++p)
		if(*p == L'/')
			*p = L'\\';
	return wpath;
}

/* Convert Windows wide character paths to unix UTF-8. */
static char* w2upath(const wchar_t *wpath)
{
	char* upath, *p;
	if(!wpath || win32_wide_utf8(wpath, &upath, NULL) < 1)
		return NULL;
	for(p=upath; *p; ++p)
		if(*p == '\\')
			*p = '/';
	return upath;
}

/* An absolute path that is too long and not already marked with
   \\?\ can be marked as a long one and still work. */
static int wpath_need_elongation(wchar_t *wpath)
{
	if( wpath && !PathIsRelativeW(wpath)
	&&	wcslen(wpath) > MAX_PATH-1
	&&	wcsncmp(L"\\\\?\\", wpath, 4) )
		return 1;
	else
		return 0;
}

/* Take any wide windows path and turn it into a path that is allowed
   to be longer than MAX_PATH, if it is not already. */
static wchar_t* wlongpath(wchar_t *wpath)
{
	size_t len, plen;
	const wchar_t *prefix = L"";
	wchar_t *wlpath = NULL;
	if(!wpath)
		return NULL;

	/* Absolute paths that do not start with \\?\ get that prepended
	   to allow them being long. */
	if(!PathIsRelativeW(wpath) && wcsncmp(L"\\\\?\\", wpath, 4))
	{
		if(wcslen(wpath) >= 2 && PathIsUNCW(wpath))
		{
			/* \\server\path -> \\?\UNC\server\path */
			prefix = L"\\\\?\\UNC";
			++wpath; /* Skip the first \. */
		}
		else /* c:\some/path -> \\?\c:\some\path */
			prefix = L"\\\\?\\";
	}
	plen = wcslen(prefix);
	len = plen + wcslen(wpath);
	wlpath = malloc(len+1*sizeof(wchar_t));
	if(wlpath)
	{
		/* Brute force memory copying, swprintf is too dandy. */
		memcpy(wlpath, prefix, sizeof(wchar_t)*plen);
		memcpy(wlpath+plen, wpath, sizeof(wchar_t)*(len-plen));
		wlpath[len] = 0;
	}
	return wlpath;
}

/* Convert unix path to wide windows path, optionally marking
   it as long path if necessary. */
static wchar_t* u2wlongpath(const char *upath)
{
	wchar_t *wpath  = NULL;
	wchar_t *wlpath = NULL;
	wpath = u2wpath(upath);
	if(wpath_need_elongation(wpath))
	{
		wlpath = wlongpath(wpath);
		free(wpath);
		wpath = wlpath;
	}
	return wpath;
}

#endif

#else

static wchar_t* u2wlongpath(const char *upath)
{
	wchar_t* wpath, *p;
	if (!upath || win32_utf8_wide(upath, &wpath, NULL) < 1)
		return NULL;
	for (p = wpath; *p; ++p)
		if (*p == L'/')
			*p = L'\\';
	return wpath;
}

#endif
