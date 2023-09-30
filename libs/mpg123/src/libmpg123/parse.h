/*
	parse: spawned from common; clustering around stream/frame parsing

	copyright ?-2023 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp & Thomas Orgis
*/

#ifndef MPG123_PARSE_H
#define MPG123_PARSE_H

#include "frame.h"

int INT123_read_frame_init(mpg123_handle* fr);
int INT123_frame_bitrate(mpg123_handle *fr);
long INT123_frame_freq(mpg123_handle *fr);
int INT123_read_frame_recover(mpg123_handle* fr); /* dead? */
int INT123_read_frame(mpg123_handle *fr);
void INT123_set_pointer(mpg123_handle *fr, int part2, long backstep);
double INT123_compute_bpf(mpg123_handle *fr);

#endif
