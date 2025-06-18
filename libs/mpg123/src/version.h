#ifndef MPG123_VERSION_H
#define MPG123_VERSION_H
/*
	version: mpg123 distribution version

	This is the main source of mpg123 distribution version information, parsed
	by the build system for packaging.

	copyright 2023 by the mpg123 project,
	free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis
*/

// only single spaces as separator to ease parsing by build scripts
#define MPG123_MAJOR 1
#define MPG123_MINOR 33
#define MPG123_PATCH 0
// Don't get too wild with that to avoid confusing m4. No brackets.
// Also, it should fit well into a sane file name for the tarball.
#define MPG123_SUFFIX ""

#define MPG123_VERSION_CAT_REALLY(a, b, c) #a "."  #b  "." #c
#define MPG123_VERSION_CAT(a, b, c) MPG123_VERSION_CAT_REALLY(a, b, c)

#define MPG123_VERSION \
  MPG123_VERSION_CAT(MPG123_MAJOR, MPG123_MINOR, MPG123_PATCH) MPG123_SUFFIX

#endif
