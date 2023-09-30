/*
	ntom.c: N->M down/up sampling; the setup code.

	copyright 1995-2023 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#define SAFE_NTOM /* Do not depend on int64_t*int64_t with big values still being in the range... */
#include "mpg123lib_intern.h"
#include "debug.h"

int INT123_synth_ntom_set_step(mpg123_handle *fr)
{
	long m,n;
	m = INT123_frame_freq(fr);
	n = fr->af.rate;
	if(VERBOSE2)
		fprintf(stderr,"Init rate converter: %ld->%ld\n",m,n);

	if(n > NTOM_MAX_FREQ || m > NTOM_MAX_FREQ || m <= 0 || n <= 0) {
		if(NOQUIET) error("NtoM converter: illegal rates");
		fr->err = MPG123_BAD_RATE;
		return -1;
	}

	n *= NTOM_MUL;
	fr->ntom_step = (unsigned long) n / m;

	if(fr->ntom_step > (unsigned long)NTOM_MAX*NTOM_MUL) {
		if(NOQUIET) error3("max. 1:%i conversion allowed (%lu vs %lu)!", NTOM_MAX, fr->ntom_step, (unsigned long)8*NTOM_MUL);
		fr->err = MPG123_BAD_RATE;
		return -1;
	}

	fr->INT123_ntom_val[0] = fr->INT123_ntom_val[1] = INT123_ntom_val(fr, fr->num);
	return 0;
}

/*
	The SAFE_NTOM does iterative loops instead of straight multiplication.
	The safety is not just about the algorithm closely mimicking the decoder instead of applying some formula,
	it is more about avoiding multiplication of possibly big sample offsets (a 32bit int64_t could overflow too easily).
*/

unsigned long INT123_ntom_val(mpg123_handle *fr, int64_t frame)
{
	int64_t ntm;
#ifdef SAFE_NTOM /* Carry out the loop, without the threatening integer overflow. */
	int64_t f;
	ntm = NTOM_MUL>>1; /* for frame 0 */
	for(f=0; f<frame; ++f)   /* for frame > 0 */
	{
		ntm += fr->spf*fr->ntom_step;
		ntm -= (ntm/NTOM_MUL)*NTOM_MUL;
	}
#else /* Just make one computation with overall sample offset. */
	ntm  = (NTOM_MUL>>1) + fr->spf*frame*fr->ntom_step;
	ntm -= (ntm/NTOM_MUL)*NTOM_MUL;
#endif
	return (unsigned long) ntm;
}

/* Set the ntom value for next expected frame to be decoded.
   This is for keeping output consistent across seeks. */
void INT123_ntom_set_ntom(mpg123_handle *fr, int64_t num)
{
	fr->INT123_ntom_val[1] = fr->INT123_ntom_val[0] = INT123_ntom_val(fr, num);
}

/* Carry out the ntom sample count operation for this one frame. 
   No fear of integer overflow here. */
int64_t INT123_ntom_frame_outsamples(mpg123_handle *fr)
{
	/* The do this before decoding the separate channels, so there is only one common ntom value. */
	int ntm = fr->INT123_ntom_val[0];
	ntm += fr->spf*fr->ntom_step;
	return ntm/NTOM_MUL;
}

/* Convert frame offset to unadjusted output sample offset. */
int64_t INT123_ntom_frmouts(mpg123_handle *fr, int64_t frame)
{
#ifdef SAFE_NTOM
	int64_t f;
#endif
	int64_t soff = 0;
	int64_t ntm = INT123_ntom_val(fr,0);
#ifdef SAFE_NTOM
	if(frame <= 0) return 0;
	for(f=0; f<frame; ++f)
	{
		ntm  += fr->spf*fr->ntom_step;
		soff += ntm/NTOM_MUL;
		ntm  -= (ntm/NTOM_MUL)*NTOM_MUL;
	}
#else
	soff = (ntm + frame*(int64_t)fr->spf*(int64_t)fr->ntom_step)/(int64_t)NTOM_MUL;
#endif
	return soff;
}

/* Convert input samples to unadjusted output samples. */
int64_t INT123_ntom_ins2outs(mpg123_handle *fr, int64_t ins)
{
	int64_t soff = 0;
	int64_t ntm = INT123_ntom_val(fr,0);
#ifdef SAFE_NTOM
	{
		int64_t block = fr->spf;
		if(ins <= 0) return 0;
		do
		{
			int64_t nowblock = ins > block ? block : ins;
			ntm  += nowblock*fr->ntom_step;
			soff += ntm/NTOM_MUL;
			ntm  -= (ntm/NTOM_MUL)*NTOM_MUL;
			ins -= nowblock;
		} while(ins > 0);
	}
#else
	/* Beware of overflows: when int64_t is 32bits, the multiplication blows too easily.
	   Of course, it blows for 64bits, too, in theory, but that's for _really_ large files. */
	soff = ((int64_t)ntm + (int64_t)ins*(int64_t)fr->ntom_step)/(int64_t)NTOM_MUL;
#endif
	return soff;
}

/* Determine frame offset from unadjusted output sample offset. */
int64_t INT123_ntom_frameoff(mpg123_handle *fr, int64_t soff)
{
	int64_t ioff = 0; /* frames or samples */
	int64_t ntm = INT123_ntom_val(fr,0);
#ifdef SAFE_NTOM
	if(soff <= 0) return 0;
	for(ioff=0; 1; ++ioff)
	{
		ntm  += fr->spf*fr->ntom_step;
		if(ntm/NTOM_MUL > soff) break;
		soff -= ntm/NTOM_MUL;
		ntm  -= (ntm/NTOM_MUL)*NTOM_MUL;
	}
	return ioff;
#else
	ioff = (soff*(int64_t)NTOM_MUL-ntm)/(int64_t)fr->ntom_step;
	return ioff/(int64_t)fr->spf;
#endif
}
