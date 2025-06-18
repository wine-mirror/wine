/*
	layer2.c: the layer 2 decoder, root of mpg123

	copyright 1994-2021 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	mpg123 started as mp2 decoder a long time ago...
	part of this file is required for layer 1, too.
*/


#include "mpg123lib_intern.h"

#ifndef NO_LAYER2
#include "l2tables.h"
#endif

#include "getbits.h"

#ifndef NO_LAYER12 /* Stuff  needed for layer I and II. */

#include "l12tabs.h"

#ifdef RUNTIME_TABLES
#include "init_layer12.h"
#endif

// The layer12_table is already in real format (fixed or float), just needs
// a little scaling in the MMX/SSE case.

void INT123_init_layer12_stuff(mpg123_handle *fr, real* (*init_table)(mpg123_handle *fr, real *table, int m))
{
	int k;
	real *table;
	for(k=0;k<27;k++)
	{
		table = init_table(fr, fr->muls[k], k);
		*table++ = 0.0;
	}
}

real* INT123_init_layer12_table(mpg123_handle *fr, real *table, int m)
{
	int i;
	for(i=0;i<63;i++)
		*table++ = layer12_table[m][i];
	return table;
}

#ifdef OPT_MMXORSSE
real* INT123_init_layer12_table_mmx(mpg123_handle *fr, real *table, int m)
{
	int i,j;
	if(!fr->p.down_sample) 
	{
		for(j=3,i=0;i<63;i++,j--)
			*table++ = 16384 * layer12_table[m][i];
	}
	else
	{
		for(j=3,i=0;i<63;i++,j--)
			*table++ = layer12_table[m][i];
	}
	return table;
}
#endif

#endif /* NO_LAYER12 */

/* The rest is the actual decoding of layer II data. */

#ifndef NO_LAYER2

static int II_step_one(unsigned int *bit_alloc,int *scale,mpg123_handle *fr)
{
	int stereo = fr->stereo-1;
	int sblimit = fr->II_sblimit;
	int jsbound = fr->jsbound;
	int sblimit2 = fr->II_sblimit<<stereo;
	const struct al_table *alloc1 = fr->alloc;
	int i;
	unsigned int scfsi_buf[64];
	unsigned int *scfsi,*bita;
	int sc,step;
	/* Count the bits needed for getbits_fast(). */
	unsigned int needbits = 0;
	unsigned int scale_bits[4] = { 18, 12, 6, 12 };

	bita = bit_alloc;
	if(stereo)
	{
		for(i=jsbound;i;i--,alloc1+=(1<<step))
		{
			step=alloc1->bits;
			bita[0] = (char) getbits(fr, step);
			bita[1] = (char) getbits(fr, step);
			needbits += ((bita[0]?1:0)+(bita[1]?1:0))*2;
			bita+=2;
		}
		for(i=sblimit-jsbound;i;i--,alloc1+=(1<<step))
		{
			step=alloc1->bits;
			bita[0] = (char) getbits(fr, step);
			bita[1] = bita[0];
			needbits += (bita[0]?1:0)*2*2;
			bita+=2;
		}
		bita = bit_alloc;
		scfsi=scfsi_buf;

		if(fr->bits_avail < needbits)
		{
			if(NOQUIET)
				error2("need %u bits, have %li", needbits, fr->bits_avail);
			return -1;
		}
		for(i=sblimit2;i;i--)
		if(*bita++) *scfsi++ = (char) getbits_fast(fr, 2);
	}
	else /* mono */
	{
		for(i=sblimit;i;i--,alloc1+=(1<<step))
		{
			step=alloc1->bits;
			*bita = (char) getbits(fr, step);
			if(*bita)
				needbits += 2;
			++bita;
		}
		bita = bit_alloc;
		scfsi=scfsi_buf;
		if(fr->bits_avail < needbits)
		{
			if(NOQUIET)
				error2("need %u bits, have %li", needbits, fr->bits_avail);
			return -1;
		}
		for(i=sblimit;i;i--)
		if(*bita++) *scfsi++ = (char) getbits_fast(fr, 2);
	}

	needbits = 0;
	bita = bit_alloc;
	scfsi=scfsi_buf;
	for(i=sblimit2;i;--i)
		if(*bita++)
			needbits += scale_bits[*scfsi++];
	if(fr->bits_avail < needbits)
	{
		if(NOQUIET)
			error2("need %u bits, have %li", needbits, fr->bits_avail);
		return -1;
	}

	bita = bit_alloc;
	scfsi=scfsi_buf;
	for(i=sblimit2;i;--i)
	if(*bita++)
	switch(*scfsi++)
	{
		case 0: 
			*scale++ = getbits_fast(fr, 6);
			*scale++ = getbits_fast(fr, 6);
			*scale++ = getbits_fast(fr, 6);
		break;
		case 1 : 
			*scale++ = sc = getbits_fast(fr, 6);
			*scale++ = sc;
			*scale++ = getbits_fast(fr, 6);
		break;
		case 2: 
			*scale++ = sc = getbits_fast(fr, 6);
			*scale++ = sc;
			*scale++ = sc;
		break;
		default:              /* case 3 */
			*scale++ = getbits_fast(fr, 6);
			*scale++ = sc = getbits_fast(fr, 6);
			*scale++ = sc;
		break;
	}

	return 0;
}


static void II_step_two(unsigned int *bit_alloc,real fraction[2][4][SBLIMIT],int *scale,mpg123_handle *fr,int x1)
{
	int i,j,k,ba;
	int stereo = fr->stereo;
	int sblimit = fr->II_sblimit;
	int jsbound = fr->jsbound;
	const struct al_table *alloc2,*alloc1 = fr->alloc;
	unsigned int *bita=bit_alloc;
	int d1,step;

	for(i=0;i<jsbound;i++,alloc1+=(1<<step))
	{
		step = alloc1->bits;
		for(j=0;j<stereo;j++)
		{
			if( (ba=*bita++) ) 
			{
				k=(alloc2 = alloc1+ba)->bits;
				if( (d1=alloc2->d) < 0) 
				{
					real cm=fr->muls[k][scale[x1]];
					fraction[j][0][i] = REAL_MUL_SCALE_LAYER12(DOUBLE_TO_REAL_15((int)getbits(fr, k) + d1), cm);
					fraction[j][1][i] = REAL_MUL_SCALE_LAYER12(DOUBLE_TO_REAL_15((int)getbits(fr, k) + d1), cm);
					fraction[j][2][i] = REAL_MUL_SCALE_LAYER12(DOUBLE_TO_REAL_15((int)getbits(fr, k) + d1), cm);
				}        
				else 
				{
					const unsigned char *table[] = { 0,0,0,grp_3tab,0,grp_5tab,0,0,0,grp_9tab };
					unsigned int m=scale[x1];
					unsigned int idx = (unsigned int) getbits(fr, k);
					const unsigned char *tab = table[d1] + idx + idx + idx;
					fraction[j][0][i] = REAL_SCALE_LAYER12(fr->muls[*tab++][m]);
					fraction[j][1][i] = REAL_SCALE_LAYER12(fr->muls[*tab++][m]);
					fraction[j][2][i] = REAL_SCALE_LAYER12(fr->muls[*tab][m]);  
				}
				scale+=3;
			}
			else
			fraction[j][0][i] = fraction[j][1][i] = fraction[j][2][i] = DOUBLE_TO_REAL(0.0);
			if(fr->bits_avail < 0)
				return; /* Caller checks that again. */
		}
	}

	for(i=jsbound;i<sblimit;i++,alloc1+=(1<<step))
	{
		step = alloc1->bits;
		bita++;	/* channel 1 and channel 2 bitalloc are the same */
		if( (ba=*bita++) )
		{
			k=(alloc2 = alloc1+ba)->bits;
			if( (d1=alloc2->d) < 0)
			{
				real cm;
				cm=fr->muls[k][scale[x1+3]];
				fraction[0][0][i] = DOUBLE_TO_REAL_15((int)getbits(fr, k) + d1);
				fraction[0][1][i] = DOUBLE_TO_REAL_15((int)getbits(fr, k) + d1);
				fraction[0][2][i] = DOUBLE_TO_REAL_15((int)getbits(fr, k) + d1);
				fraction[1][0][i] = REAL_MUL_SCALE_LAYER12(fraction[0][0][i], cm);
				fraction[1][1][i] = REAL_MUL_SCALE_LAYER12(fraction[0][1][i], cm);
				fraction[1][2][i] = REAL_MUL_SCALE_LAYER12(fraction[0][2][i], cm);
				cm=fr->muls[k][scale[x1]];
				fraction[0][0][i] = REAL_MUL_SCALE_LAYER12(fraction[0][0][i], cm);
				fraction[0][1][i] = REAL_MUL_SCALE_LAYER12(fraction[0][1][i], cm);
				fraction[0][2][i] = REAL_MUL_SCALE_LAYER12(fraction[0][2][i], cm);
			}
			else
			{
				const unsigned char *table[] = { 0,0,0,grp_3tab,0,grp_5tab,0,0,0,grp_9tab };
				unsigned int m1 = scale[x1];
				unsigned int m2 = scale[x1+3];
				unsigned int idx = (unsigned int) getbits(fr, k);
				const unsigned char *tab = table[d1] + idx + idx + idx;
				fraction[0][0][i] = REAL_SCALE_LAYER12(fr->muls[*tab][m1]); fraction[1][0][i] = REAL_SCALE_LAYER12(fr->muls[*tab++][m2]);
				fraction[0][1][i] = REAL_SCALE_LAYER12(fr->muls[*tab][m1]); fraction[1][1][i] = REAL_SCALE_LAYER12(fr->muls[*tab++][m2]);
				fraction[0][2][i] = REAL_SCALE_LAYER12(fr->muls[*tab][m1]); fraction[1][2][i] = REAL_SCALE_LAYER12(fr->muls[*tab][m2]);
			}
			scale+=6;
			if(fr->bits_avail < 0)
				return; /* Caller checks that again. */
		}
		else
		{
			fraction[0][0][i] = fraction[0][1][i] = fraction[0][2][i] =
			fraction[1][0][i] = fraction[1][1][i] = fraction[1][2][i] = DOUBLE_TO_REAL(0.0);
		}
/*
	Historic comment...
	should we use individual scalefac for channel 2 or
	is the current way the right one , where we just copy channel 1 to
	channel 2 ?? 
	The current 'strange' thing is, that we throw away the scalefac
	values for the second channel ...!!
	-> changed .. now we use the scalefac values of channel one !! 
*/
	}

	if(sblimit > (fr->down_sample_sblimit) )
	sblimit = fr->down_sample_sblimit;

	for(i=sblimit;i<SBLIMIT;i++)
	for (j=0;j<stereo;j++)
	fraction[j][0][i] = fraction[j][1][i] = fraction[j][2][i] = DOUBLE_TO_REAL(0.0);
}


static void II_select_table(mpg123_handle *fr)
{
	const int translate[3][2][16] =
	{
		{
			{ 0,2,2,2,2,2,2,0,0,0,1,1,1,1,1,0 },
			{ 0,2,2,0,0,0,1,1,1,1,1,1,1,1,1,0 }
		},
		{
			{ 0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0 },
			{ 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 }
		},
		{
			{ 0,3,3,3,3,3,3,0,0,0,1,1,1,1,1,0 },
			{ 0,3,3,0,0,0,1,1,1,1,1,1,1,1,1,0 }
		}
	};

	int table,sblim;
	const struct al_table *tables[5] = { alloc_0, alloc_1, alloc_2, alloc_3 , alloc_4 };
	const int sblims[5] = { 27 , 30 , 8, 12 , 30 };

	if(fr->hdr.sampling_frequency >= 3)	/* Or equivalent: (fr->lsf == 1) */
	table = 4;
	else
	table = translate[fr->hdr.sampling_frequency][2-fr->stereo][fr->hdr.bitrate_index];

	sblim = sblims[table];
	fr->alloc      = tables[table];
	fr->II_sblimit = sblim;
}


int INT123_do_layer2(mpg123_handle *fr)
{
	int clip=0;
	int i,j;
	int stereo = fr->stereo;
	/* pick_table clears unused subbands */
	/* replacement for real fraction[2][4][SBLIMIT], needs alignment. */
	real (*fraction)[4][SBLIMIT] = fr->layer2.fraction;
	unsigned int bit_alloc[64];
	int scale[192];
	int single = fr->single;

	II_select_table(fr);
	fr->jsbound = (fr->hdr.mode == MPG_MD_JOINT_STEREO) ? (fr->hdr.mode_ext<<2)+4 : fr->II_sblimit;

	if(fr->jsbound > fr->II_sblimit)
	{
		if(NOQUIET)
			error("Truncating stereo boundary to sideband limit.");
		fr->jsbound=fr->II_sblimit;
	}

	/* TODO: What happens with mono mixing, actually? */
	if(stereo == 1 || single == SINGLE_MIX) /* also, mix not really handled */
	single = SINGLE_LEFT;

	if(II_step_one(bit_alloc, scale, fr))
	{
		if(NOQUIET)
			error("first step of layer I decoding failed");
		return clip;
	}

	for(i=0;i<SCALE_BLOCK;i++)
	{
		II_step_two(bit_alloc,fraction,scale,fr,i>>2);
		if(fr->bits_avail < 0)
		{
			if(NOQUIET)
				error("missing bits in layer II step two");
			return clip;
		}
		for(j=0;j<3;j++) 
		{
			if(single != SINGLE_STEREO)
			clip += (fr->synth_mono)(fraction[single][j], fr);
			else
			clip += (fr->synth_stereo)(fraction[0][j], fraction[1][j], fr);
		}
	}

	return clip;
}

#endif /* NO_LAYER2 */
