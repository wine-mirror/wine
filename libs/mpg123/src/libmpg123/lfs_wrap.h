/*
	lfs_wrap: I/O wrapper code

	copyright 2010-2023 by the mpg123 project
	free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis (after code from Michael Hipp)

	This is the interface to the implementation of internal reading/seeking
	as well as wrapping of client callbacks to the one and only 64 bit
	callback API on opaque handles.

	Code outside of this shall not be concerned with I/O details, and
	code inside of this shall not be concerned with other
	libmpg123 internals. Just the public portable API interface.
*/


// This is to be offered by libmpg123 code that has access to frame struct
// details. It returns the address to load/store the pointer to the
// wrapper handle from/into. A little hack to keep things disentangled.
void ** INT123_wrap_handle(mpg123_handle *mh);
// Set error code in the mpg123 handle and return MPG123_ERR.
int INT123_set_err(mpg123_handle *mh, int err);

// These are offered by the source associated with this header.

// This is one open routine for all ways. One of the given resource arguments is active:
// 1. handle: if path == NULL && fd < 0
// 2. path: if path != NULL
// 3. fd: if fd >= 0
// In case of 64 bit handle setup, this does nothing.
// Return values:
//  0: setup for wrapped I/O successful.
//  1: use user-supplied 64 bit I/O handle directly, no internal wrappery
// <0: error
#define LFS_WRAP_NONE 1
int INT123_wrap_open(mpg123_handle *mh, void *handle, const char *path, int fd, long timeout, int quiet);
// Deallocate all associated resources and handle memory itself.
void INT123_wrap_destroy(void * handle);

// The bulk of functions are implementations of the non-portable
// libmpg123 API declared or implied in the main header.
