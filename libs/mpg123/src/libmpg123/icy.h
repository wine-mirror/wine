/*
	icy: support for SHOUTcast ICY meta info, an attempt to keep it organized

	copyright 2006-2023 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis and modelled after patch by Honza
*/
#ifndef MPG123_ICY_H
#define MPG123_ICY_H

#ifndef NO_ICY

#include "compat.h"
#include "mpg123.h"

struct icy_meta
{
	char* data;
	int64_t interval;
	int64_t next;
};

void INT123_init_icy(struct icy_meta *);
void INT123_clear_icy(struct icy_meta *);
void INT123_reset_icy(struct icy_meta *);

#else

#undef INT123_init_icy
#define INT123_init_icy(a)
#undef INT123_clear_icy
#define INT123_clear_icy(a)
#undef INT123_reset_icy
#define INT123_reset_icy(a)

#endif /* NO_ICY */

#endif
