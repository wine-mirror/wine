/*
	libmpg123: MPEG Audio Decoder library

	copyright 1995-2023 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

*/

#include "mpg123lib_intern.h"
#include "../version.h"
#include "icy2utf8.h"

#include "gapless.h"
/* Want accurate rounding function regardless of decoder setup. */
#define FORCE_ACCURATE
#include "../common/sample.h"
#include "parse.h"
#include "lfs_wrap.h"

#include "../common/debug.h"

#define SEEKFRAME(mh) ((mh)->ignoreframe < 0 ? 0 : (mh)->ignoreframe)


const char * attribute_align_arg mpg123_distversion(unsigned int *major, unsigned int *minor, unsigned int *patch)
{
	if(major)
		*major = MPG123_MAJOR;
	if(minor)
		*minor = MPG123_MINOR;
	if(patch)
		*patch = MPG123_PATCH;
	return MPG123_VERSION;
}

unsigned int attribute_align_arg mpg123_libversion(unsigned int *patch)
{
	if(patch)
		*patch = MPG123_PATCHLEVEL;
	return MPG123_API_VERSION;
}

int attribute_align_arg mpg123_init(void)
{
	// Since 1.27.0, this is a no-op and shall stay that way.
	return MPG123_OK;
}

void attribute_align_arg mpg123_exit(void)
{
	// Nothing to ever happen here.
}

/* create a new handle with specified decoder, decoder can be "", "auto" or NULL for auto-detection */
mpg123_handle attribute_align_arg *mpg123_new(const char* decoder, int *error)
{
	return mpg123_parnew(NULL, decoder, error);
}

// Runtime table calculation is back for specific uses that want minimal
// binary size. It's hidden in handle initialization, not in mpg123_init(),
// any user of such a minimal build should ensure that there is no trouble
// from concurrency or just call mpg123_new() once in single-threaded context.

/* ...the full routine with optional initial parameters to override defaults. */
mpg123_handle attribute_align_arg *mpg123_parnew(mpg123_pars *mp, const char* decoder, int *error)
{
	mpg123_handle *fr = NULL;
	int err = MPG123_OK;

#ifdef RUNTIME_TABLES
	static char tables_initialized = 0;
	if(!tables_initialized)
	{
#ifndef NO_LAYER12
		INT123_init_layer12(); /* inits also shared tables with layer1 */
#endif
#ifndef NO_LAYER3
		INT123_init_layer3();
#endif
		INT123_init_costabs();
		tables_initialized = 1;
	}
#endif

	// Silly paranoia checks. Really silly, but libmpg123 has been ported to strange
	// platforms in the past.
	if((sizeof(short) != 2) || (sizeof(long) < 4))
	{
		if(error != NULL) *error = MPG123_BAD_TYPES;
		return NULL;
	}

#if (defined REAL_IS_FLOAT) && (defined IEEE_FLOAT)
	/* This is rather pointless but it eases my mind to check that we did
	   not enable the special rounding on a VAX or something. */
	if(12346 != REAL_TO_SHORT_ACCURATE(12345.67f))
	{
		if(error != NULL) *error = MPG123_BAD_FLOAT;
		return NULL;
	}
#endif

	fr = (mpg123_handle*) malloc(sizeof(mpg123_handle));
	if(fr != NULL)
	{
		INT123_frame_init_par(fr, mp);
		debug("cpu opt setting");
		if(INT123_frame_cpu_opt(fr, decoder) != 1)
		{
			err = MPG123_BAD_DECODER;
			INT123_frame_exit(fr);
			free(fr);
			fr = NULL;
		}
	}
	if(fr != NULL)
	{
		fr->decoder_change = 1;
	}
	else if(err == MPG123_OK) err = MPG123_OUT_OF_MEM;

	if(error != NULL) *error = err;
	return fr;
}

int attribute_align_arg mpg123_decoder(mpg123_handle *mh, const char* decoder)
{
	enum optdec dt = INT123_dectype(decoder);

	if(mh == NULL) return MPG123_BAD_HANDLE;

	if(dt == nodec)
	{
		mh->err = MPG123_BAD_DECODER;
		return MPG123_ERR;
	}
	if(dt == mh->cpu_opts.type) return MPG123_OK;

	/* Now really change. */
	/* INT123_frame_exit(mh);
	INT123_frame_init(mh); */
	debug("cpu opt setting");
	if(INT123_frame_cpu_opt(mh, decoder) != 1)
	{
		mh->err = MPG123_BAD_DECODER;
		INT123_frame_exit(mh);
		return MPG123_ERR;
	}
	/* New buffers for decoder are created in INT123_frame_buffers() */
	if((INT123_frame_outbuffer(mh) != 0))
	{
		mh->err = MPG123_NO_BUFFERS;
		INT123_frame_exit(mh);
		return MPG123_ERR;
	}
	/* Do _not_ call INT123_decode_update here! That is only allowed after a first MPEG frame has been met. */
	mh->decoder_change = 1;
	return MPG123_OK;
}

int attribute_align_arg mpg123_param(mpg123_handle *mh, enum mpg123_parms key, long val, double fval)
{
	int r;

	if(mh == NULL) return MPG123_BAD_HANDLE;
	r = mpg123_par(&mh->p, key, val, fval);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }
	else
	{ /* Special treatment for some settings. */
#ifdef FRAME_INDEX
		if(key == MPG123_INDEX_SIZE)
		{ /* Apply frame index size and grow property on the fly. */
			r = INT123_frame_index_setup(mh);
			if(r != MPG123_OK) mh->err = MPG123_INDEX_FAIL;
		}
#endif
#ifndef NO_FEEDER
		/* Feeder pool size is applied right away, reader will react to that. */
		if(key == MPG123_FEEDPOOL || key == MPG123_FEEDBUFFER)
		INT123_bc_poolsize(&mh->rdat.buffer, mh->p.feedpool, mh->p.feedbuffer);
#endif
	}
	return r;
}

int attribute_align_arg mpg123_param2(mpg123_handle *mh,  int key, long val, double fval)
{
	return mpg123_param(mh, key, val, fval);
}

int attribute_align_arg mpg123_par(mpg123_pars *mp, enum mpg123_parms key, long val, double fval)
{
	int ret = MPG123_OK;

	if(mp == NULL) return MPG123_BAD_PARS;
	switch(key)
	{
		case MPG123_VERBOSE:
			mp->verbose = val;
		break;
		case MPG123_FLAGS:
#ifndef GAPLESS
			if(val & MPG123_GAPLESS) ret = MPG123_NO_GAPLESS;
#endif
			if(ret == MPG123_OK) mp->flags = val;
			debug1("set flags to 0x%lx", (unsigned long) mp->flags);
		break;
		case MPG123_ADD_FLAGS:
#ifndef GAPLESS
			/* Enabling of gapless mode doesn't work when it's not there, but disabling (below) is no problem. */
			if(val & MPG123_GAPLESS) ret = MPG123_NO_GAPLESS;
			else
#endif
			mp->flags |= val;
			debug1("set flags to 0x%lx", (unsigned long) mp->flags);
		break;
		case MPG123_REMOVE_FLAGS:
			mp->flags &= ~val;
			debug1("set flags to 0x%lx", (unsigned long) mp->flags);
		break;
		case MPG123_FORCE_RATE: /* should this trigger something? */
#ifdef NO_NTOM
			if(val > 0)
			ret = MPG123_BAD_RATE;
#else
			if(val > 96000) ret = MPG123_BAD_RATE;
			else mp->force_rate = val < 0 ? 0 : val; /* >0 means enable, 0 disable */
#endif
		break;
		case MPG123_DOWN_SAMPLE:
#ifdef NO_DOWNSAMPLE
			if(val != 0) ret = MPG123_BAD_RATE;
#else
			if(val < 0 || val > 2) ret = MPG123_BAD_RATE;
			else mp->down_sample = (int)val;
#endif
		break;
		case MPG123_RVA:
			if(val < 0 || val > MPG123_RVA_MAX) ret = MPG123_BAD_RVA;
			else mp->rva = (int)val;
		break;
		case MPG123_DOWNSPEED:
			mp->halfspeed = val < 0 ? 0 : val;
		break;
		case MPG123_UPSPEED:
			mp->doublespeed = val < 0 ? 0 : val;
		break;
		case MPG123_ICY_INTERVAL:
#ifndef NO_ICY
			mp->icy_interval = val > 0 ? val : 0;
#else
			if(val > 0) ret = MPG123_BAD_PARAM;
#endif
		break;
		case MPG123_OUTSCALE:
			/* Choose the value that is non-zero, if any.
			   Downscaling integers to 1.0 . */
			mp->outscale = val == 0 ? fval : (double)val/SHORT_SCALE;
		break;
		case MPG123_TIMEOUT:
#ifdef TIMEOUT_READ
			mp->timeout = val >= 0 ? val : 0;
#else
			if(val > 0) ret = MPG123_NO_TIMEOUT;
#endif
		break;
		case MPG123_RESYNC_LIMIT:
			mp->resync_limit = val;
		break;
		case MPG123_INDEX_SIZE:
#ifdef FRAME_INDEX
			mp->index_size = val;
#else
			if(val) // It is only an eror if you want to enable the index.
				ret = MPG123_NO_INDEX;
#endif
		break;
		case MPG123_PREFRAMES:
			if(val >= 0) mp->preframes = val;
			else ret = MPG123_BAD_VALUE;
		break;
		case MPG123_FEEDPOOL:
#ifndef NO_FEEDER
			if(val >= 0) mp->feedpool = val;
			else ret = MPG123_BAD_VALUE;
#else
			ret = MPG123_MISSING_FEATURE;
#endif
		break;
		case MPG123_FEEDBUFFER:
#ifndef NO_FEEDER
			if(val > 0) mp->feedbuffer = val;
			else ret = MPG123_BAD_VALUE;
#else
			ret = MPG123_MISSING_FEATURE;
#endif
		break;
		case MPG123_FREEFORMAT_SIZE:
			mp->freeformat_framesize = val;
		break; 
		default:
			ret = MPG123_BAD_PARAM;
	}
	return ret;
}

int attribute_align_arg mpg123_par2(mpg123_pars *mp, int key, long val, double fval)
{
	return mpg123_par(mp, key, val, fval);
}

int attribute_align_arg mpg123_getparam(mpg123_handle *mh, enum mpg123_parms key, long *val, double *fval)
{
	int r;

	if(mh == NULL) return MPG123_BAD_HANDLE;
	r = mpg123_getpar(&mh->p, key, val, fval);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }
	return r;
}

int attribute_align_arg mpg123_getparam2(mpg123_handle *mh, int key, long *val, double *fval)
{
	return mpg123_getparam(mh, key, val, fval);
}

int attribute_align_arg mpg123_getpar(mpg123_pars *mp, enum mpg123_parms key, long *val, double *fval)
{
	int ret = 0;

	if(mp == NULL) return MPG123_BAD_PARS;
	switch(key)
	{
		case MPG123_VERBOSE:
			if(val) *val = mp->verbose;
		break;
		case MPG123_FLAGS:
		case MPG123_ADD_FLAGS:
			if(val) *val = mp->flags;
		break;
		case MPG123_FORCE_RATE:
			if(val) 
#ifdef NO_NTOM
			*val = 0;
#else
			*val = mp->force_rate;
#endif
		break;
		case MPG123_DOWN_SAMPLE:
			if(val) *val = mp->down_sample;
		break;
		case MPG123_RVA:
			if(val) *val = mp->rva;
		break;
		case MPG123_DOWNSPEED:
			if(val) *val = mp->halfspeed;
		break;
		case MPG123_UPSPEED:
			if(val) *val = mp->doublespeed;
		break;
		case MPG123_ICY_INTERVAL:
#ifndef NO_ICY
			if(val) *val = (long)mp->icy_interval;
#else
			if(val) *val = 0;
#endif
		break;
		case MPG123_OUTSCALE:
			if(fval) *fval = mp->outscale;
			if(val) *val = (long)(mp->outscale*SHORT_SCALE);
		break;
		case MPG123_RESYNC_LIMIT:
			if(val) *val = mp->resync_limit;
		break;
		case MPG123_INDEX_SIZE:
			if(val)
#ifdef FRAME_INDEX
			*val = mp->index_size;
#else
			*val = 0; /* graceful fallback: no index is index of zero size */
#endif
		break;
		case MPG123_PREFRAMES:
			*val = mp->preframes;
		break;
		case MPG123_FEEDPOOL:
#ifndef NO_FEEDER
			*val = mp->feedpool;
#else
			ret = MPG123_MISSING_FEATURE;
#endif
		break;
		case MPG123_FEEDBUFFER:
#ifndef NO_FEEDER
			*val = mp->feedbuffer;
#else
			ret = MPG123_MISSING_FEATURE;
#endif
		break;
		case MPG123_FREEFORMAT_SIZE:
			*val = mp->freeformat_framesize;
		break; 
		default:
			ret = MPG123_BAD_PARAM;
	}
	return ret;
}

int attribute_align_arg mpg123_getpar2(mpg123_pars *mp, int key, long *val, double *fval)
{
	return mpg123_getpar(mp, key, val, fval);
}

int attribute_align_arg mpg123_getstate(mpg123_handle *mh, enum mpg123_state key, long *val, double *fval)
{
	int ret = MPG123_OK;
	long theval = 0;
	double thefval = 0.;

	if(mh == NULL) return MPG123_BAD_HANDLE;

	switch(key)
	{
		case MPG123_ACCURATE:
			theval = mh->state_flags & FRAME_ACCURATE;
		break;
		case MPG123_FRANKENSTEIN:
			theval = mh->state_flags & FRAME_FRANKENSTEIN;
		break;
		case MPG123_BUFFERFILL:
#ifndef NO_FEEDER
		{
			size_t sval = INT123_bc_fill(&mh->rdat.buffer);
			theval = (long)sval;
			if(theval < 0 || (size_t)theval != sval)
			{
				mh->err = MPG123_INT_OVERFLOW;
				ret = MPG123_ERR;
			}
		}
#else
			mh->err = MPG123_MISSING_FEATURE;
			ret = MPG123_ERR;
#endif
		break;
		case MPG123_FRESH_DECODER:
			theval = mh->state_flags & FRAME_FRESH_DECODER;
			mh->state_flags &= ~FRAME_FRESH_DECODER;
		break;
		case MPG123_ENC_DELAY:
			theval = mh->enc_delay;
		break;
		case MPG123_ENC_PADDING:
			theval = mh->enc_padding;
		break;
		case MPG123_DEC_DELAY:
			theval = mh->hdr.lay == 3 ? GAPLESS_DELAY : -1;
		break;
		default:
			mh->err = MPG123_BAD_KEY;
			ret = MPG123_ERR;
	}

	if(val  != NULL) *val  = theval;
	if(fval != NULL) *fval = thefval;

	return ret;
}

int attribute_align_arg mpg123_getstate2(mpg123_handle *mh, int key, long *val, double *fval)
{
	return mpg123_getstate(mh, key, val, fval);
}

int attribute_align_arg mpg123_eq(mpg123_handle *mh, enum mpg123_channels channel, int band, double val)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
#ifndef NO_EQUALIZER
	if(band < 0 || band > 31){ mh->err = MPG123_BAD_BAND; return MPG123_ERR; }
	switch(channel)
	{
		case MPG123_LEFT|MPG123_RIGHT:
			mh->equalizer[0][band] = mh->equalizer[1][band] = DOUBLE_TO_REAL(val);
		break;
		case MPG123_LEFT:  mh->equalizer[0][band] = DOUBLE_TO_REAL(val); break;
		case MPG123_RIGHT: mh->equalizer[1][band] = DOUBLE_TO_REAL(val); break;
		default:
			mh->err=MPG123_BAD_CHANNEL;
			return MPG123_ERR;
	}
	mh->have_eq_settings = TRUE;
#endif
	return MPG123_OK;
}

int attribute_align_arg mpg123_eq2(mpg123_handle *mh, int channel, int band, double val)
{
	return mpg123_eq(mh, channel, band, val);
}

int attribute_align_arg mpg123_eq_bands(mpg123_handle *mh, int channel, int a, int b, double factor)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
#ifndef NO_EQUALIZER
	int ret;
	// Always count up.
	if(a>b){ int s=a; a=b; b=s; }
	for(int n=a; n<=b; ++n)
		if( (ret=mpg123_eq(mh, channel, n, factor)) != MPG123_OK )
			return ret;
#endif
	return MPG123_OK;
}

int attribute_align_arg mpg123_eq_change(mpg123_handle *mh, int channel, int a, int b, double db)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
#ifndef NO_EQUALIZER
	// Always count up.
	if(a>b){ int s=a; a=b; b=s; }
	for(int band=a; band<=b; ++band)
	{
		if(band < 0 || band > 31){ mh->err = MPG123_BAD_BAND; return MPG123_ERR; }
		if(channel & MPG123_LEFT)
			mh->equalizer[0][band] = DOUBLE_TO_REAL(dbchange(REAL_TO_DOUBLE(mh->equalizer[0][band]), db));
		if(channel & MPG123_RIGHT)
			mh->equalizer[1][band] = DOUBLE_TO_REAL(dbchange(REAL_TO_DOUBLE(mh->equalizer[1][band]), db));
		mh->have_eq_settings = TRUE;
	}
#endif
	return MPG123_OK;
}

double attribute_align_arg mpg123_geteq(mpg123_handle *mh, enum mpg123_channels channel, int band)
{
	double ret = 1.;
#ifndef NO_EQUALIZER

	/* Handle this gracefully. When there is no band, it has no volume. */
	if(mh != NULL && band > -1 && band < 32)
	switch(channel)
	{
		case MPG123_LEFT|MPG123_RIGHT:
			ret = 0.5*(REAL_TO_DOUBLE(mh->equalizer[0][band])+REAL_TO_DOUBLE(mh->equalizer[1][band]));
		break;
		case MPG123_LEFT:  ret = REAL_TO_DOUBLE(mh->equalizer[0][band]); break;
		case MPG123_RIGHT: ret = REAL_TO_DOUBLE(mh->equalizer[1][band]); break;
		/* Default case is already handled: ret = 0 */
	}
#endif
	return ret;
}

double attribute_align_arg mpg123_geteq2(mpg123_handle *mh, int channel, int band)
{
	return mpg123_geteq(mh, channel, band);
}

// LFS wrapper code is so agnostic to it all now that internal I/O is portable
// as long as you do not mix in off_t API.
int attribute_align_arg mpg123_open64(mpg123_handle *mh, const char *path)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mpg123_close(mh);
	if(!path)
		return MPG123_ERR;
	// sets callbacks, only allocating wrapperdata handle if internal callbacks involved
	int ret = INT123_wrap_open( mh, NULL, path, -1
	,	mh->p.timeout, mh->p.flags & MPG123_QUIET );
	if(!ret)
		ret = INT123_open_stream_handle(mh, mh->wrapperdata);
	return ret;
}

#ifndef PORTABLE_API

#ifdef FORCED_OFF_64
// Only _64 symbols for a system-wide enforced _FILE_OFFSET_BITS=64.
#define mpg123_open mpg123_open_64
#define mpg123_open_fixed mpg123_open_fixed_64
#define mpg123_open_fd mpg123_open_fd_64
#define mpg123_open_handle mpg123_open_handle_64
#endif

// This now is agnostic to off_t choice, but still subject to renaming
// for legacy reasons.
int attribute_align_arg mpg123_open(mpg123_handle *mh, const char *path)
{
	return mpg123_open64(mh, path);
}
#endif // PORTABLE_API

// The convenience function mpg123_open_fixed() wraps over acual mpg123_open
// and hence needs to have the exact same code in lfs_wrap.c. The flesh is
// in INT123_open_fixed_pre() and INT123_open_fixed_post(), wich are only defined here.
// Update: The open routines are just alias calls now, since the conversion to
// int64_t internally.
static int INT123_open_fixed_pre(mpg123_handle *mh, int channels, int encoding)
{
	if(!mh)
		return MPG123_BAD_HANDLE;
	mh->p.flags |= MPG123_NO_FRANKENSTEIN;
	int err = mpg123_format_none(mh);
	if(err == MPG123_OK)
		err = mpg123_format2(mh, 0, channels, encoding);
	return err;
}

static int INT123_open_fixed_post(mpg123_handle *mh, int channels, int encoding)
{
	if(!mh)
		return MPG123_BAD_HANDLE;
	long rate;
	int err = mpg123_getformat(mh, &rate, &channels, &encoding);
	if(err == MPG123_OK)
		err = mpg123_format_none(mh);
	if(err == MPG123_OK)
		err = mpg123_format(mh, rate, channels, encoding);
	if(err == MPG123_OK)
	{
		if(mh->track_frames < 1 && (mh->rdat.flags & READER_SEEKABLE))
		{
			debug("INT123_open_fixed_post: scan because we can seek and do not know track_frames");
			err = mpg123_scan(mh);
		}
	}
	if(err != MPG123_OK)
		mpg123_close(mh);
	return err;
}

int attribute_align_arg mpg123_open_fixed64( mpg123_handle *mh, const char *path
,	int channels, int encoding )
{
	int err = INT123_open_fixed_pre(mh, channels, encoding);
	if(err == MPG123_OK)
		err = mpg123_open64(mh, path);
	if(err == MPG123_OK)
		err = INT123_open_fixed_post(mh, channels, encoding);
	return err;
}

#ifndef PORTABLE_API
// Only to have the modern offset-agnostic open under a fixed name.
int attribute_align_arg mpg123_open_fixed( mpg123_handle *mh, const char *path
,	int channels, int encoding )
{
	return mpg123_open_fixed64(mh, path, channels, encoding);
}

// Won't define a 'portable' variant of this, as I cannot guess
// properties of the handed-in fd, which in theory, on specific platforms,
// could not support large files.
int attribute_align_arg mpg123_open_fd(mpg123_handle *mh, int fd)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mpg123_close(mh);
	if(fd < 0)
		return MPG123_ERR;
	int ret = INT123_wrap_open( mh, NULL, NULL, fd
	,	mh->p.timeout, mh->p.flags & MPG123_QUIET );
	if(!ret)
		ret = INT123_open_stream_handle(mh, mh->wrapperdata);
	return ret;
}
#endif // PORTABLE_API

// Only works with int64 reader setup.
int attribute_align_arg mpg123_open_handle64(mpg123_handle *mh, void *iohandle)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mpg123_close(mh);
	return INT123_open_stream_handle(mh, iohandle);
}

#ifndef PORTABLE_API
// Change from 1.32: No largefile-renamed symbols in a library with strict
// portable API.
// I allow that breaking change since this is far from a standard libmpg123 build.
int attribute_align_arg mpg123_open_handle(mpg123_handle *mh, void *iohandle)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mpg123_close(mh);
	int ret;
	ret = INT123_wrap_open( mh, iohandle, NULL, -1
	,	mh->p.timeout, mh->p.flags & MPG123_QUIET );
	iohandle = ret == LFS_WRAP_NONE ? iohandle : mh->wrapperdata;
	if(ret >= 0)
		ret = INT123_open_stream_handle(mh, iohandle);
	return ret;
}
#endif

int attribute_align_arg mpg123_open_feed(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mpg123_close(mh);
	return INT123_open_feed(mh);
}

static int64_t no_lseek64(void *handle, int64_t off, int whence)
{
	return -1;
}

// The simplest direct wrapper, actually no wrapping at all.
int attribute_align_arg mpg123_reader64( mpg123_handle *mh
,       int (*r_read) (void *, void *, size_t, size_t *)
,       int64_t (*r_lseek)(void *, int64_t, int)
,       void (*cleanup)(void*) )
{
	if(mh == NULL)
		return MPG123_BAD_HANDLE;

	mpg123_close(mh);

	if(!r_read)
		return MPG123_NULL_POINTER;

	mh->rdat.r_read64 = r_read;
	mh->rdat.r_lseek64 = r_lseek ? r_lseek : no_lseek64;
	mh->rdat.cleanup_handle = cleanup;
	return MPG123_OK;
}

// All other I/O gets routed through predefined wrapper.

/* Update decoding engine for
   a) a new choice of decoder
   b) a changed native format of the MPEG stream
   ... calls are only valid after parsing some MPEG frame! */
int INT123_decode_update(mpg123_handle *mh)
{
	long native_rate;
	int b;

	mh->state_flags &= ~FRAME_DECODER_LIVE;
	if(mh->num < 0)
	{
		if(!(mh->p.flags & MPG123_QUIET)) error("INT123_decode_update() has been called before reading the first MPEG frame! Internal programming error.");

		mh->err = MPG123_BAD_DECODER_SETUP;
		return MPG123_ERR;
	}

	mh->state_flags |= FRAME_FRESH_DECODER;
	native_rate = INT123_frame_freq(mh);

	b = INT123_frame_output_format(mh); /* Select the new output format based on given constraints. */
	if(b < 0) return MPG123_ERR;

	if(b == 1) mh->new_format = 1; /* Store for later... */

	debug3("updating decoder structure with native rate %li and af.rate %li (new format: %i)", native_rate, mh->af.rate, mh->new_format);
	if(mh->af.rate == native_rate) mh->down_sample = 0;
	else if(mh->af.rate == native_rate>>1) mh->down_sample = 1;
	else if(mh->af.rate == native_rate>>2) mh->down_sample = 2;
	else mh->down_sample = 3; /* flexible (fixed) rate */
	switch(mh->down_sample)
	{
		case 0:
		case 1:
		case 2:
			mh->down_sample_sblimit = SBLIMIT>>(mh->down_sample);
			/* With downsampling I get less samples per frame */
			mh->outblock = INT123_outblock_bytes(mh, (mh->spf>>mh->down_sample));
		break;
#ifndef NO_NTOM
		case 3:
		{
			if(INT123_synth_ntom_set_step(mh) != 0) return -1;
			if(INT123_frame_freq(mh) > mh->af.rate)
			{
				mh->down_sample_sblimit = SBLIMIT * mh->af.rate;
				mh->down_sample_sblimit /= INT123_frame_freq(mh);
				if(mh->down_sample_sblimit < 1)
					mh->down_sample_sblimit = 1;
			}
			else mh->down_sample_sblimit = SBLIMIT;
			mh->outblock = INT123_outblock_bytes(mh,
			                 ( ( NTOM_MUL-1+mh->spf
			                   * (((size_t)NTOM_MUL*mh->af.rate)/INT123_frame_freq(mh))
			                 )/NTOM_MUL ));
		}
		break;
#endif
	}

	if(!(mh->p.flags & MPG123_FORCE_MONO))
	{
		if(mh->af.channels == 1) mh->single = SINGLE_MIX;
		else mh->single = SINGLE_STEREO;
	}
	else mh->single = (mh->p.flags & MPG123_FORCE_MONO)-1;
	if(INT123_set_synth_functions(mh) != 0) return -1;

	/* The needed size of output buffer may have changed. */
	if(INT123_frame_outbuffer(mh) != MPG123_OK) return -1;

	INT123_do_rva(mh);
	debug3("done updating decoder structure with native rate %li and af.rate %li and down_sample %i", INT123_frame_freq(mh), mh->af.rate, mh->down_sample);

	mh->decoder_change = 0;
	mh->state_flags |= FRAME_DECODER_LIVE;
	return 0;
}

size_t attribute_align_arg mpg123_safe_buffer(void)
{
	/* real is the largest possible output (it's 32bit float, 32bit int or 64bit double). */
	return sizeof(real)*2*1152*NTOM_MAX;
}

size_t attribute_align_arg mpg123_outblock(mpg123_handle *mh)
{
	/* Try to be helpful and never return zero output block size. */
	if(mh != NULL && mh->outblock > 0) return mh->outblock;
	else return mpg123_safe_buffer();
}

/* Read in the next frame we actually want for decoding.
   This includes skipping/ignoring frames, in additon to skipping junk in the parser. */
static int get_next_frame(mpg123_handle *mh)
{
	int change = mh->decoder_change;
	/* Ensure we got proper decoder for ignoring frames.
	   Header can be changed from seeking around. But be careful: Only after at
	   least one frame got read, decoder update makes sense. */
	if(mh->header_change > 1 && mh->num >= 0)
	{
		change = 1;
		mh->header_change = 0;
		debug("starting with big header change");
		if(INT123_decode_update(mh) < 0)
		return MPG123_ERR;
	}

	do
	{
		int b;
		/* Decode & discard some frame(s) before beginning. */
		if(mh->to_ignore && mh->num < mh->firstframe && mh->num >= mh->ignoreframe)
		{
			debug1("ignoring frame %li", (long)mh->num);
			/* Decoder structure must be current! INT123_decode_update has been called before... */
			(mh->do_layer)(mh); mh->buffer.fill = 0;
#ifndef NO_NTOM
			/* The ignored decoding may have failed. Make sure ntom stays consistent. */
			if(mh->down_sample == 3) INT123_ntom_set_ntom(mh, mh->num+1);
#endif
			mh->to_ignore = mh->to_decode = FALSE;
		}
		/* Read new frame data; possibly breaking out here for MPG123_NEED_MORE. */
		debug("read frame");
		mh->to_decode = FALSE;
		b = INT123_read_frame(mh); /* That sets to_decode only if a full frame was read. */
		debug4("read of frame %"PRIi64" returned %i (to_decode=%i) at sample %"PRIi64, mh->num, b, mh->to_decode, mpg123_tell64(mh));
		if(b == MPG123_NEED_MORE) return MPG123_NEED_MORE; /* need another call with data */
		else if(b <= 0)
		{
			/* More sophisticated error control? */
			if(b==0 || (mh->rdat.filelen >= 0 && mh->rdat.filepos == mh->rdat.filelen))
			{ /* We simply reached the end. */
				mh->track_frames = mh->num + 1;
				debug("What about updating/checking gapless sample count here?");
				return MPG123_DONE;
			}
			else return MPG123_ERR; /* Some real error. */
		}
		/* Now, there should be new data to decode ... and also possibly new stream properties */
		if(mh->header_change > 1 || mh->decoder_change)
		{
			debug("big header or decoder change");
			change = 1;
			mh->header_change = 0;
			/* Need to update decoder structure right away since frame might need to
			   be decoded on next loop iteration for properly ignoring its output. */
			if(INT123_decode_update(mh) < 0)
			return MPG123_ERR;
		}
		/* Now some accounting: Look at the numbers and decide if we want this frame. */
		++mh->playnum;
		/* Plain skipping without decoding, only when frame is not ignored on next cycle. */
		if(mh->num < mh->firstframe || (mh->p.doublespeed && (mh->playnum % mh->p.doublespeed)))
		{
			if(!(mh->to_ignore && mh->num < mh->firstframe && mh->num >= mh->ignoreframe))
			{
				INT123_frame_skip(mh);
				/* Should one fix NtoM here or not?
				   It is not work the trouble for doublespeed, but what with leading frames? */
			}
		}
		/* Or, we are finally done and have a new frame. */
		else break;
	} while(1);

	/* If we reach this point, we got a new frame ready to be decoded.
	   All other situations resulted in returns from the loop. */
	if(change)
	{
		if(mh->fresh)
		{
#ifdef GAPLESS
			int b=0;
			/* Prepare offsets for gapless decoding. */
			debug1("preparing gapless stuff with native rate %li", INT123_frame_freq(mh));
			INT123_frame_gapless_realinit(mh);
			INT123_frame_set_frameseek(mh, mh->num);
#endif
			mh->fresh = 0;
#ifdef GAPLESS
			/* Could this possibly happen? With a real big gapless offset... */
			if(mh->num < mh->firstframe) b = get_next_frame(mh);
			if(b < 0) return b; /* Could be error, need for more, new format... */
#endif
		}
	}
	return MPG123_OK;
}

/* Assumption: A buffer full of zero samples can be constructed by repetition of this byte.
   Oh, and it handles some format conversion.
   Only to be used by decode_the_frame() ... */
static int zero_byte(mpg123_handle *fr)
{
#ifndef NO_8BIT
	return fr->af.encoding & MPG123_ENC_8 ? fr->conv16to8[0] : 0;
#else
	return 0; /* All normal signed formats have the zero here (even in byte form -- that may be an assumption for your funny machine...). */
#endif
}

/*
	Not part of the api. This just decodes the frame and fills missing bits with zeroes.
	There can be frames that are broken and thus make do_layer() fail.
*/
static void decode_the_frame(mpg123_handle *fr)
{
	size_t needed_bytes = INT123_decoder_synth_bytes(fr, INT123_frame_expect_outsamples(fr));
	fr->clip += (fr->do_layer)(fr);
	/* There could be less data than promised.
	   Also, then debugging, we look out for coding errors that could result in _more_ data than expected. */
#ifdef DEBUG
	if(fr->buffer.fill != needed_bytes)
	{
#endif
		if(fr->buffer.fill < needed_bytes)
		{
			if(VERBOSE2)
				fprintf( stderr, "Note: broken frame %li, filling up with %zu zeroes, from %zu\n"
				,	(long)fr->num, (needed_bytes-fr->buffer.fill), fr->buffer.fill );

			/*
				One could do a loop with individual samples instead... but zero is zero
				Actually, that is wrong: zero is mostly a series of null bytes,
				but we have funny 8bit formats that have a different opinion on zero...
				Unsigned 16 or 32 bit formats are handled later.
			*/
			memset( fr->buffer.data + fr->buffer.fill, zero_byte(fr), needed_bytes - fr->buffer.fill );

			fr->buffer.fill = needed_bytes;
#ifndef NO_NTOM
			/* INT123_ntom_val will be wrong when the decoding wasn't carried out completely */
			INT123_ntom_set_ntom(fr, fr->num+1);
#endif
		}
#ifdef DEBUG
		else
		{
			if(NOQUIET)
			error2("I got _more_ bytes than expected (%zu / %zu), that should not be possible!", fr->buffer.fill, needed_bytes);
		}
	}
#endif
	INT123_postprocess_buffer(fr);
}

/*
	Decode the current frame into the frame structure's buffer, accessible at the location stored in <audio>, with <bytes> bytes available.
	<num> will contain the last decoded frame number. This function should be called after mpg123_framebyframe_next positioned the stream at a
	valid mp3 frame. The buffer contents will get lost on the next call to mpg123_framebyframe_next or mpg123_framebyframe_decode.
	returns
	MPG123_OK -- successfully decoded or ignored the frame, you get your output data or in case of ignored frames 0 bytes
	MPG123_DONE -- decoding finished, should not happen
	MPG123_ERR -- some error occured.
	MPG123_ERR_NULL -- audio or bytes are not pointing to valid storage addresses
	MPG123_BAD_HANDLE -- mh has not been initialized
	MPG123_NO_SPACE -- not enough space in buffer for safe decoding, should not happen
*/
int attribute_align_arg mpg123_framebyframe_decode64(mpg123_handle *mh, int64_t *num, unsigned char **audio, size_t *bytes)
{
	if(bytes == NULL) return MPG123_ERR_NULL;
	if(audio == NULL) return MPG123_ERR_NULL;
	if(mh == NULL)    return MPG123_BAD_HANDLE;
	if(mh->buffer.size < mh->outblock) return MPG123_NO_SPACE;

	*audio = NULL;
	*bytes = 0;
	mh->buffer.fill = 0; /* always start fresh */
	if(!mh->to_decode) return MPG123_OK;

	if(num != NULL) *num = mh->num;
	debug("decoding");
	if(!(mh->state_flags & FRAME_DECODER_LIVE))
		return MPG123_ERR;
	decode_the_frame(mh);
	mh->to_decode = mh->to_ignore = FALSE;
	mh->buffer.p = mh->buffer.data;
	FRAME_BUFFERCHECK(mh);
	*audio = mh->buffer.p;
	*bytes = mh->buffer.fill;
	return MPG123_OK;
}

/*
	Find, read and parse the next mp3 frame while skipping junk and parsing id3 tags, lame headers, etc.
	Prepares everything for decoding using mpg123_framebyframe_decode.
	returns
	MPG123_OK -- new frame was read and parsed, call mpg123_framebyframe_decode to actually decode
	MPG123_NEW_FORMAT -- new frame was read, it results in changed output format, call mpg123_framebyframe_decode to actually decode
	MPG123_BAD_HANDLE -- mh has not been initialized
	MPG123_NEED_MORE  -- more input data is needed to advance to the next frame. supply more input data using mpg123_feed
*/
int attribute_align_arg mpg123_framebyframe_next(mpg123_handle *mh)
{
	int b;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mh->to_decode = mh->to_ignore = FALSE;
	mh->buffer.fill = 0;

	b = get_next_frame(mh);
	if(b < 0) return b;
	debug1("got next frame, %i", mh->to_decode);

	/* mpg123_framebyframe_decode will return MPG123_OK with 0 bytes decoded if mh->to_decode is 0 */
	if(!mh->to_decode)
		return MPG123_OK;

	if(mh->new_format)
	{
		debug("notifiying new format");
		mh->new_format = 0;
		return MPG123_NEW_FORMAT;
	}

	return MPG123_OK;
}

/*
	Put _one_ decoded frame into the frame structure's buffer, accessible at the location stored in <audio>, with <bytes> bytes available.
	The buffer contents will be lost on next call to mpg123_decode_frame.
	MPG123_OK -- successfully decoded the frame, you get your output data
	MPg123_DONE -- This is it. End.
	MPG123_ERR -- some error occured...
	MPG123_NEW_FORMAT -- new frame was read, it results in changed output format -> will be decoded on next call
	MPG123_NEED_MORE  -- that should not happen as this function is intended for in-library stream reader but if you force it...
	MPG123_NO_SPACE   -- not enough space in buffer for safe decoding, also should not happen

	num will be updated to the last decoded frame number (may possibly _not_ increase, p.ex. when format changed).
*/
int attribute_align_arg mpg123_decode_frame64(mpg123_handle *mh, int64_t *num, unsigned char **audio, size_t *bytes)
{
	if(bytes != NULL) *bytes = 0;
	if(mh == NULL) return MPG123_BAD_HANDLE;
	if(mh->buffer.size < mh->outblock) return MPG123_NO_SPACE;
	mh->buffer.fill = 0; /* always start fresh */
	/* Be nice: Set these also for sensible values in case of error. */
	if(audio) *audio = NULL;
	if(bytes) *bytes = 0;
	while(TRUE)
	{
		/* decode if possible */
		if(mh->to_decode)
		{
			if(num != NULL) *num = mh->num;
			if(mh->new_format)
			{
				debug("notifiying new format");
				mh->new_format = 0;
				return MPG123_NEW_FORMAT;
			}
			debug("decoding");

			if( (mh->decoder_change && INT123_decode_update(mh) < 0)
			||	!(mh->state_flags & FRAME_DECODER_LIVE) )
				return MPG123_ERR;
			decode_the_frame(mh);

			mh->to_decode = mh->to_ignore = FALSE;
			mh->buffer.p = mh->buffer.data;
			FRAME_BUFFERCHECK(mh);
			if(audio != NULL) *audio = mh->buffer.p;
			if(bytes != NULL) *bytes = mh->buffer.fill;

			return MPG123_OK;
		}
		else
		{
			int b = get_next_frame(mh);
			if(b < 0) return b;
			debug1("got next frame, %i", mh->to_decode);
		}
	}
}


int attribute_align_arg mpg123_read(mpg123_handle *mh, void *out, size_t size, size_t *done)
{
	return mpg123_decode(mh, NULL, 0, out, size, done);
}

int attribute_align_arg mpg123_feed(mpg123_handle *mh, const unsigned char *in, size_t size)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
#ifndef NO_FEEDER
	if(size > 0)
	{
		if(in != NULL)
		{
			if(INT123_feed_more(mh, in, size) != 0) return MPG123_ERR;
			else
			{
				/* The need for more data might have triggered an error.
				   This one is outdated now with the new data. */
				if(mh->err == MPG123_ERR_READER) mh->err = MPG123_OK;

				return MPG123_OK;
			}
		}
		else
		{
			mh->err = MPG123_NULL_BUFFER;
			return MPG123_ERR;
		}
	}
	return MPG123_OK;
#else
	mh->err = MPG123_MISSING_FEATURE;
	return MPG123_ERR;
#endif
}

/*
	The old picture:
	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;
		ret = decodeMP3(&mp,buf,len,out,8192,&size);
		while(ret == MP3_OK) {
			write(1,out,size);
			ret = decodeMP3(&mp,NULL,0,out,8192,&size);
		}
	}
*/

int attribute_align_arg mpg123_decode(mpg123_handle *mh, const unsigned char *inmemory, size_t inmemsize, void *outmem, size_t outmemsize, size_t *done)
{
	int ret = MPG123_OK;
	size_t mdone = 0;
	unsigned char *outmemory = outmem;

	if(done != NULL) *done = 0;
	if(mh == NULL) return MPG123_BAD_HANDLE;
	if(inmemsize > 0 && mpg123_feed(mh, inmemory, inmemsize) != MPG123_OK)
	{
		ret = MPG123_ERR;
		goto decodeend;
	}
	if(outmemory == NULL) outmemsize = 0; /* Not just give error, give chance to get a status message. */

	while(ret == MPG123_OK)
	{
		debug4("decode loop, fill %i (%li vs. %li); to_decode: %i", (int)mh->buffer.fill, (long)mh->num, (long)mh->firstframe, mh->to_decode);
		/* Decode a frame that has been read before.
		   This only happens when buffer is empty! */
		if(mh->to_decode)
		{
			if(mh->new_format)
			{
				debug("notifiying new format");
				mh->new_format = 0;
				ret = MPG123_NEW_FORMAT;
				goto decodeend;
			}
			if(mh->buffer.size - mh->buffer.fill < mh->outblock)
			{
				ret = MPG123_NO_SPACE;
				goto decodeend;
			}
			if( (mh->decoder_change && INT123_decode_update(mh) < 0)
			||	! (mh->state_flags & FRAME_DECODER_LIVE) )
			{
				ret = MPG123_ERR;
				goto decodeend;
			}
			decode_the_frame(mh);
			mh->to_decode = mh->to_ignore = FALSE;
			mh->buffer.p = mh->buffer.data;
			debug2("decoded frame %li, got %li samples in buffer", (long)mh->num, (long)(mh->buffer.fill / (INT123_samples_to_bytes(mh, 1))));
			FRAME_BUFFERCHECK(mh);
		}
		if(mh->buffer.fill) /* Copy (part of) the decoded data to the caller's buffer. */
		{
			/* get what is needed - or just what is there */
			int a = mh->buffer.fill > (outmemsize - mdone) ? outmemsize - mdone : mh->buffer.fill;
			debug4("buffer fill: %i; copying %i (%i - %li)", (int)mh->buffer.fill, a, (int)outmemsize, (long)mdone);
			memcpy(outmemory, mh->buffer.p, a);
			/* less data in frame buffer, less needed, output pointer increase, more data given... */
			mh->buffer.fill -= a;
			outmemory  += a;
			mdone += a;
			mh->buffer.p += a;
			if(!(outmemsize > mdone)) goto decodeend;
		}
		else /* If we didn't have data, get a new frame. */
		{
			int b = get_next_frame(mh);
			if(b < 0){ ret = b; goto decodeend; }
		}
	}
decodeend:
	if(done != NULL) *done = mdone;
	return ret;
}

long attribute_align_arg mpg123_clip(mpg123_handle *mh)
{
	long ret = 0;

	if(mh != NULL)
	{
		ret = mh->clip;
		mh->clip = 0;
	}
	return ret;
}

/* Simples: Track needs initializtion if no initial frame has been read yet. */
#define track_need_init(mh) ((mh)->num < 0)

static int init_track(mpg123_handle *mh)
{
	if(track_need_init(mh))
	{
		/* Fresh track, need first frame for basic info. */
		int b = get_next_frame(mh);
		if(b < 0) return b;
	}
	return 0;
}

// Duplicating the code for the changed member types in struct mpg123_frameinfo2.
#define MPG123_INFO_FUNC \
{ \
	int b; \
 \
	if(mh == NULL) return MPG123_BAD_HANDLE; \
	if(mi == NULL) \
	{ \
		mh->err = MPG123_ERR_NULL; \
		return MPG123_ERR; \
	} \
	b = init_track(mh); \
	if(b < 0) return b; \
 \
	mi->version = mh->hdr.mpeg25 ? MPG123_2_5 : (mh->hdr.lsf ? MPG123_2_0 : MPG123_1_0); \
	mi->layer = mh->hdr.lay; \
	mi->rate = INT123_frame_freq(mh); \
	switch(mh->hdr.mode) \
	{ \
		case 0: mi->mode = MPG123_M_STEREO; break; \
		case 1: mi->mode = MPG123_M_JOINT;  break; \
		case 2: mi->mode = MPG123_M_DUAL;   break; \
		case 3: mi->mode = MPG123_M_MONO;   break; \
		default: mi->mode = 0; /* Nothing good to do here. */ \
	} \
	mi->mode_ext = mh->hdr.mode_ext; \
	mi->framesize = mh->hdr.framesize+4; /* Include header. */ \
	mi->flags = 0; \
	if(mh->hdr.error_protection) mi->flags |= MPG123_CRC; \
	if(mh->hdr.copyright)        mi->flags |= MPG123_COPYRIGHT; \
	if(mh->hdr.extension)        mi->flags |= MPG123_PRIVATE; \
	if(mh->hdr.original)         mi->flags |= MPG123_ORIGINAL; \
	mi->emphasis = mh->hdr.emphasis; \
	mi->bitrate  = INT123_frame_bitrate(mh); \
	mi->abr_rate = mh->abr_rate; \
	mi->vbr = mh->vbr; \
	return MPG123_OK; \
}

int attribute_align_arg mpg123_info(mpg123_handle *mh, struct mpg123_frameinfo *mi)
MPG123_INFO_FUNC

int attribute_align_arg mpg123_info2(mpg123_handle *mh, struct mpg123_frameinfo2 *mi)
MPG123_INFO_FUNC

int attribute_align_arg mpg123_getformat2( mpg123_handle *mh
,	long *rate, int *channels, int *encoding, int clear_flag )
{
	int b;

	if(mh == NULL) return MPG123_BAD_HANDLE;
	b = init_track(mh);
	if(b < 0) return b;

	if(rate != NULL) *rate = mh->af.rate;
	if(channels != NULL) *channels = mh->af.channels;
	if(encoding != NULL) *encoding = mh->af.encoding;
	if(clear_flag) mh->new_format = 0;
	return MPG123_OK;
}

int attribute_align_arg mpg123_getformat(mpg123_handle *mh, long *rate, int *channels, int *encoding)
{
	return mpg123_getformat2(mh, rate, channels, encoding, 1);
}

int64_t attribute_align_arg mpg123_timeframe64(mpg123_handle *mh, double seconds)
{
	int64_t b;

	if(mh == NULL) return MPG123_ERR;
	b = init_track(mh);
	if(b<0) return b;
	// Overflow checking here would be a bit more elaborate. TODO?
	return (int64_t)(seconds/mpg123_tpf(mh));
}

/*
	Now, where are we? We need to know the last decoded frame... and what's left of it in buffer.
	The current frame number can mean the last decoded frame or the to-be-decoded frame.
	If mh->to_decode, then mh->num frames have been decoded, the frame mh->num now coming next.
	If not, we have the possibility of mh->num+1 frames being decoded or nothing at all.
	Then, there is firstframe...when we didn't reach it yet, then the next data will come from there.
	mh->num starts with -1
*/
int64_t attribute_align_arg mpg123_tell64(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;
	if(track_need_init(mh)) return 0;
	/* Now we have all the info at hand. */
	debug5("tell: %li/%i first %li buffer %lu; INT123_frame_outs=%li", (long)mh->num, mh->to_decode, (long)mh->firstframe, (unsigned long)mh->buffer.fill, (long)INT123_frame_outs(mh, mh->num));

	{ /* Funny block to keep C89 happy. */
		int64_t pos = 0;
		if((mh->num < mh->firstframe) || (mh->num == mh->firstframe && mh->to_decode))
		{ /* We are at the beginning, expect output from firstframe on. */
			pos = INT123_frame_outs(mh, mh->firstframe);
#ifdef GAPLESS
			pos += mh->firstoff;
#endif
		}
		else if(mh->to_decode)
		{ /* We start fresh with this frame. Buffer should be empty, but we make sure to count it in.  */
			pos = INT123_frame_outs(mh, mh->num) - INT123_bytes_to_samples(mh, mh->buffer.fill);
		}
		else
		{ /* We serve what we have in buffer and then the beginning of next frame... */
			pos = INT123_frame_outs(mh, mh->num+1) - INT123_bytes_to_samples(mh, mh->buffer.fill);
		}
		/* Substract padding and delay from the beginning. */
		pos = SAMPLE_ADJUST(mh,pos);
		/* Negative sample offsets are not right, less than nothing is still nothing. */
		return pos>0 ? pos : 0;
	}
}

int64_t attribute_align_arg mpg123_tellframe64(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;
	if(mh->num < mh->firstframe) return mh->firstframe;
	if(mh->to_decode) return mh->num;
	/* Consider firstoff? */
	return mh->buffer.fill ? mh->num : mh->num + 1;
}

int64_t attribute_align_arg mpg123_tell_stream64(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;
	/* mh->rd is at least a bad_reader, so no worry. */
	return mh->rd->tell(mh);
}

static int do_the_seek(mpg123_handle *mh)
{
	int b;
	int64_t fnum = SEEKFRAME(mh);
	mh->buffer.fill = 0;

	/* If we are inside the ignoreframe - firstframe window, we may get away without actual seeking. */
	if(mh->num < mh->firstframe)
	{
		mh->to_decode = FALSE; /* In any case, don't decode the current frame, perhaps ignore instead. */
		if(mh->num > fnum) return MPG123_OK;
	}

	/* If we are already there, we are fine either for decoding or for ignoring. */
	if(mh->num == fnum && (mh->to_decode || fnum < mh->firstframe)) return MPG123_OK;
	/* We have the frame before... just go ahead as normal. */
	if(mh->num == fnum-1)
	{
		mh->to_decode = FALSE;
		return MPG123_OK;
	}

	/* OK, real seeking follows... clear buffers and go for it. */
	INT123_frame_buffers_reset(mh);
#ifndef NO_NTOM
	if(mh->down_sample == 3)
	{
		INT123_ntom_set_ntom(mh, fnum);
		debug3("fixed ntom for frame %"PRIi64" to %lu, num=%"PRIi64, fnum, mh->INT123_ntom_val[0], mh->num);
	}
#endif
	b = mh->rd->seek_frame(mh, fnum);
	if(mh->header_change > 1)
	{
		if(INT123_decode_update(mh) < 0) return MPG123_ERR;
		mh->header_change = 0;
	}
	debug1("seek_frame returned: %i", b);
	if(b<0) return b;
	/* Only mh->to_ignore is TRUE. */
	if(mh->num < mh->firstframe) mh->to_decode = FALSE;

	mh->playnum = mh->num;
	return 0;
}

int64_t attribute_align_arg mpg123_seek64(mpg123_handle *mh, int64_t sampleoff, int whence)
{
	int b;
	int64_t pos;

	pos = mpg123_tell64(mh); /* adjusted samples */
	/* pos < 0 also can mean that simply a former seek failed at the lower levels.
	  In that case, we only allow absolute seeks. */
	if(pos < 0 && whence != SEEK_SET)
	{ /* Unless we got the obvious error of NULL handle, this is a special seek failure. */
		if(mh != NULL) mh->err = MPG123_NO_RELSEEK;
		return MPG123_ERR;
	}
	if((b=init_track(mh)) < 0) return b;
	switch(whence)
	{
		case SEEK_CUR: pos += sampleoff; break;
		case SEEK_SET: pos  = sampleoff; break;
		case SEEK_END:
			// Fix for a bug that existed since the beginning of libmpg123: SEEK_END offsets are
			// also pointing forward for SEEK_END in lseek(). In libmpg123, they used to interpreted
			// as positive from the end towards the beginning. Since just swapping the sign now would
			// break existing programs and seeks beyond the end just don't make sense for a
			// read-only library, we simply ignore the sign and always assumne negative offsets
			// (pointing towards the beginning). Assuming INT64_MIN <= -INT64_MAX.
			if(sampleoff > 0)
				sampleoff = -sampleoff;
			/* When we do not know the end already, we can try to find it. */
			if(mh->track_frames < 1 && (mh->rdat.flags & READER_SEEKABLE))
			mpg123_scan(mh);
			if(mh->track_frames > 0) pos = SAMPLE_ADJUST(mh,INT123_frame_outs(mh, mh->track_frames)) + sampleoff;
#ifdef GAPLESS
			else if(mh->end_os > 0) pos = SAMPLE_ADJUST(mh,mh->end_os) + sampleoff;
#endif
			else
			{
				mh->err = MPG123_NO_SEEK_FROM_END;
				return MPG123_ERR;
			}
		break;
		default: mh->err = MPG123_BAD_WHENCE; return MPG123_ERR;
	}
	if(pos < 0) pos = 0;
	/* pos now holds the wanted sample offset in adjusted samples */
	INT123_frame_set_seek(mh, SAMPLE_UNADJUST(mh,pos));
	pos = do_the_seek(mh);
	if(pos < 0) return pos;

	return mpg123_tell64(mh);
}

/*
	A bit more tricky... libmpg123 does not do the seeking itself.
	All it can do is to ignore frames until the wanted one is there.
	The caller doesn't know where a specific frame starts and mpg123 also only knows the general region after it scanned the file.
	Well, it is tricky...

	Wow, there was no input checking at all ... I'll better add it.
*/
int64_t attribute_align_arg mpg123_feedseek64(mpg123_handle *mh, int64_t sampleoff, int whence, int64_t *input_offset)
{
	int b;
	int64_t pos;
	int64_t inoff = 0;
	if(!mh)
		return MPG123_BAD_HANDLE;

	pos = mpg123_tell64(mh); /* adjusted samples */
	debug3("seek from %li to %li (whence=%i)", (long)pos, (long)sampleoff, whence);
	/* The special seek error handling does not apply here... there is no lowlevel I/O. */
	if(pos < 0) return pos; /* mh == NULL is covered in mpg123_tell() */
#ifndef NO_FEEDER
	if(input_offset == NULL)
	{
		mh->err = MPG123_NULL_POINTER;
		return MPG123_ERR;
	}

	if((b=init_track(mh)) < 0) return b; /* May need more to do anything at all. */

	switch(whence)
	{
		case SEEK_CUR: pos += sampleoff; break;
		case SEEK_SET: pos  = sampleoff; break;
		case SEEK_END:
			if(mh->track_frames > 0) pos = SAMPLE_ADJUST(mh,INT123_frame_outs(mh, mh->track_frames)) - sampleoff;
#ifdef GAPLESS
			else if(mh->end_os >= 0) pos = SAMPLE_ADJUST(mh,mh->end_os) - sampleoff;
#endif
			else
			{
				mh->err = MPG123_NO_SEEK_FROM_END;
				return MPG123_ERR;
			}
		break;
		default: mh->err = MPG123_BAD_WHENCE; return MPG123_ERR;
	}
	if(pos < 0) pos = 0;
	INT123_frame_set_seek(mh, SAMPLE_UNADJUST(mh,pos));
	pos = SEEKFRAME(mh);
	mh->buffer.fill = 0;

	/* Shortcuts without modifying input stream. */
	inoff = mh->rdat.buffer.fileoff + mh->rdat.buffer.size;
	if(mh->num < mh->firstframe) mh->to_decode = FALSE;
	if(mh->num == pos && mh->to_decode) goto feedseekend;
	if(mh->num == pos-1) goto feedseekend;
	/* Whole way. */
	inoff = INT123_feed_set_pos(mh, INT123_frame_index_find(mh, SEEKFRAME(mh), &pos));
	mh->num = pos-1; /* The next read frame will have num = pos. */
	if(input_offset)
		*input_offset = inoff;
	if(inoff < 0)
		return MPG123_ERR;

feedseekend:
	return mpg123_tell64(mh);
#else
	mh->err = MPG123_MISSING_FEATURE;
	return MPG123_ERR;
#endif
}

int64_t attribute_align_arg mpg123_seek_frame64(mpg123_handle *mh, int64_t offset, int whence)
{
	int b;
	int64_t pos = 0;

	if(mh == NULL) return MPG123_ERR;
	if((b=init_track(mh)) < 0) return b;

	/* Could play games here with to_decode... */
	pos = mh->num;
	switch(whence)
	{
		case SEEK_CUR: pos += offset; break;
		case SEEK_SET: pos  = offset; break;
		case SEEK_END:
			if(mh->track_frames > 0) pos = mh->track_frames - offset;
			else
			{
				mh->err = MPG123_NO_SEEK_FROM_END;
				return MPG123_ERR;
			}
		break;
		default:
			mh->err = MPG123_BAD_WHENCE;
			return MPG123_ERR;
	}
	if(pos < 0) pos = 0;
	/* Not limiting the possible position on end for the chance that there might be more to the stream than announced via track_frames. */

	INT123_frame_set_frameseek(mh, pos);
	pos = do_the_seek(mh);
	if(pos < 0) return pos;

	return mpg123_tellframe64(mh);
}

int attribute_align_arg mpg123_set_filesize64(mpg123_handle *mh, int64_t size)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	mh->rdat.filelen = size;
	return MPG123_OK;
}

int64_t attribute_align_arg mpg123_framelength64(mpg123_handle *mh)
{
	int b;
	if(mh == NULL)
		return MPG123_ERR;
	b = init_track(mh);
	if(b<0)
		return b;
	if(mh->track_frames > 0)
		return mh->track_frames;
	if(mh->rdat.filelen > 0)
	{ /* A bad estimate. Ignoring tags 'n stuff. */
		double bpf = mh->mean_framesize > 0.
			? mh->mean_framesize
			: INT123_compute_bpf(mh);
		return (int64_t)((double)(mh->rdat.filelen)/bpf+0.5);
	}
	/* Last resort: No view of the future, can at least count the frames that
	   were already parsed. */
	if(mh->num > -1)
		return mh->num+1;
	/* Giving up. */
	return MPG123_ERR;
}

int64_t attribute_align_arg mpg123_length64(mpg123_handle *mh)
{
	int b;
	int64_t length;

	if(mh == NULL) return MPG123_ERR;
	b = init_track(mh);
	if(b<0) return b;
	if(mh->track_samples > -1) length = mh->track_samples;
	else if(mh->track_frames > 0) length = mh->track_frames*mh->spf;
	else if(mh->rdat.filelen > 0) /* Let the case of 0 length just fall through. */
	{
		/* A bad estimate. Ignoring tags 'n stuff. */
		double bpf = mh->mean_framesize ? mh->mean_framesize : INT123_compute_bpf(mh);
		length = (int64_t)((double)(mh->rdat.filelen)/bpf*mh->spf);
	}
	else if(mh->rdat.filelen == 0) return mpg123_tell64(mh); /* we could be in feeder mode */
	else return MPG123_ERR; /* No length info there! */

	debug1("mpg123_length: internal sample length: %"PRIi64, length);

	length = INT123_frame_ins2outs(mh, length);
	debug1("mpg123_length: external sample length: %"PRIi64, length);
	length = SAMPLE_ADJUST(mh,length);
	return length;
}

int attribute_align_arg mpg123_scan(mpg123_handle *mh)
{
	int b;
	int64_t oldpos;
	int64_t track_frames = 0;
	int64_t track_samples = 0;

	if(mh == NULL) return MPG123_BAD_HANDLE;
	if(!(mh->rdat.flags & READER_SEEKABLE)){ mh->err = MPG123_NO_SEEK; return MPG123_ERR; }
	/* Scan through the _whole_ file, since the current position is no count but computed assuming constant samples per frame. */
	/* Also, we can just keep the current buffer and seek settings. Just operate on input frames here. */
	debug("issuing scan");
	b = init_track(mh); /* mh->num >= 0 !! */
	if(b<0)
	{
		if(b == MPG123_DONE) return MPG123_OK;
		else return MPG123_ERR; /* Must be error here, NEED_MORE is not for seekable streams. */
	}
	oldpos = mpg123_tell64(mh);
	b = mh->rd->seek_frame(mh, 0);
	if(b<0 || mh->num != 0) return MPG123_ERR;
	/* One frame must be there now. */
	track_frames = 1;
	track_samples = mh->spf; /* Internal samples. */
	debug("TODO: We should disable gapless code when encountering inconsistent mh->spf!");
	debug("      ... at least unset MPG123_ACCURATE.");
	/* Do not increment mh->track_frames in the loop as tha would confuse Frankenstein detection. */
	while(INT123_read_frame(mh) == 1)
	{
		++track_frames;
		track_samples += mh->spf;
	}
	mh->track_frames = track_frames;
	mh->track_samples = track_samples;
	debug2("Scanning yielded %"PRIi64" track samples, %"PRIi64" frames."
	,	mh->track_samples, mh->track_frames);
#ifdef GAPLESS
	/* Also, think about usefulness of that extra value track_samples ... it could be used for consistency checking. */
	if(mh->p.flags & MPG123_GAPLESS) INT123_frame_gapless_update(mh, mh->track_samples);
#endif
	return mpg123_seek64(mh, oldpos, SEEK_SET) >= 0 ? MPG123_OK : MPG123_ERR;
}

int attribute_align_arg mpg123_meta_check(mpg123_handle *mh)
{
	if(mh != NULL) return mh->metaflags;
	else return 0;
}

void attribute_align_arg mpg123_meta_free(mpg123_handle *mh)
{
	if(mh == NULL) return;

	INT123_reset_id3(mh);
	INT123_reset_icy(&mh->icy);
}

int attribute_align_arg mpg123_id3(mpg123_handle *mh, mpg123_id3v1 **v1, mpg123_id3v2 **v2)
{
	if(v1 != NULL) *v1 = NULL;
	if(v2 != NULL) *v2 = NULL;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	if(mh->metaflags & MPG123_ID3)
	{
		INT123_id3_link(mh);
		if(v1 != NULL && mh->rdat.flags & READER_ID3TAG) *v1 = (mpg123_id3v1*) mh->id3buf;
		if(v2 != NULL)
#ifdef NO_ID3V2
		*v2 = NULL;
#else
		*v2 = &mh->id3v2;
#endif

		mh->metaflags |= MPG123_ID3;
		mh->metaflags &= ~MPG123_NEW_ID3;
	}
	return MPG123_OK;
}

int attribute_align_arg mpg123_id3_raw( mpg123_handle *mh
,	unsigned char **v1, size_t *v1_size
,	unsigned char **v2, size_t *v2_size )
{
	if(!mh)
		return MPG123_ERR;
	if(v1 != NULL)
		*v1 = mh->id3buf[0] ? mh->id3buf : NULL;
	if(v1_size != NULL)
		*v1_size = mh->id3buf[0] ? 128 : 0;
	if(v2 != NULL)
		*v2 = mh->id3v2_raw;
	if(v2_size != NULL)
		*v2_size = mh->id3v2_size;
	return MPG123_OK;
}

int attribute_align_arg mpg123_icy(mpg123_handle *mh, char **icy_meta)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
#ifndef NO_ICY
	if(icy_meta == NULL)
	{
		mh->err = MPG123_NULL_POINTER;
		return MPG123_ERR;
	}
	*icy_meta = NULL;

	if(mh->metaflags & MPG123_ICY)
	{
		*icy_meta = mh->icy.data;
		mh->metaflags |= MPG123_ICY;
		mh->metaflags &= ~MPG123_NEW_ICY;
	}
	return MPG123_OK;
#else
	mh->err = MPG123_MISSING_FEATURE;
	return MPG123_ERR;
#endif
}

char* attribute_align_arg mpg123_icy2utf8(const char* icy_text)
{
#ifndef NO_ICY
	return INT123_icy2utf8(icy_text, 0);
#else
	return NULL;
#endif
}

/* That one is always defined... it's not worth it to remove it for NO_ID3V2. */
enum mpg123_text_encoding attribute_align_arg mpg123_enc_from_id3(unsigned char id3_enc_byte)
{
	switch(id3_enc_byte)
	{
		case mpg123_id3_latin1:   return mpg123_text_latin1;
		case mpg123_id3_utf16bom: return mpg123_text_utf16bom; /* ID3v2.3 has UCS-2 with BOM here. */
		case mpg123_id3_utf16be:  return mpg123_text_utf16be;
		case mpg123_id3_utf8:     return mpg123_text_utf8;
		default: return mpg123_text_unknown;
	}
}

int attribute_align_arg mpg123_enc_from_id3_2(unsigned char id3_enc_byte)
{
	return mpg123_enc_from_id3(id3_enc_byte);
}

#ifndef NO_STRING
int attribute_align_arg mpg123_store_utf8(mpg123_string *sb, enum mpg123_text_encoding enc, const unsigned char *source, size_t source_size)
{
	switch(enc)
	{
#ifndef NO_ID3V2
		/* The encodings we get from ID3v2 tags. */
		case mpg123_text_utf8:
			INT123_id3_to_utf8(sb, mpg123_id3_utf8, source, source_size, 0);
		break;
		case mpg123_text_latin1:
			INT123_id3_to_utf8(sb, mpg123_id3_latin1, source, source_size, 0);
		break;
		case mpg123_text_utf16bom:
		case mpg123_text_utf16:
			INT123_id3_to_utf8(sb, mpg123_id3_utf16bom, source, source_size, 0);
		break;
		/* Special because one cannot skip zero bytes here. */
		case mpg123_text_utf16be:
			INT123_id3_to_utf8(sb, mpg123_id3_utf16be, source, source_size, 0);
		break;
#endif
#ifndef NO_ICY
		/* ICY encoding... */
		case mpg123_text_icy:
		case mpg123_text_cp1252:
		{
			mpg123_free_string(sb);
			/* Paranoia: Make sure that the string ends inside the buffer... */
			if(source[source_size-1] == 0)
			{
				/* Convert from ICY encoding... with force applied or not. */
				char *tmpstring = INT123_icy2utf8((const char*)source, enc == mpg123_text_cp1252 ? 1 : 0);
				if(tmpstring != NULL)
				{
					mpg123_set_string(sb, tmpstring);
					free(tmpstring);
				}
			}
		}
		break;
#endif
		default:
			mpg123_free_string(sb);
	}
	/* At least a trailing null of some form should be there... */
	return (sb->fill > 0) ? 1 : 0;
}

int attribute_align_arg mpg123_store_utf8_2(mpg123_string *sb, int enc, const unsigned char *source, size_t source_size)
{
	return mpg123_store_utf8(sb, enc, source, source_size);
}
#endif

int attribute_align_arg mpg123_index64(mpg123_handle *mh, int64_t **offsets, int64_t *step, size_t *fill)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
	if(offsets == NULL || step == NULL || fill == NULL)
	{
		mh->err = MPG123_BAD_INDEX_PAR;
		return MPG123_ERR;
	}
#ifdef FRAME_INDEX
	*offsets = mh->index.data;
	*step    = mh->index.step;
	*fill    = mh->index.fill;
#else
	*offsets = NULL;
	*step    = 0;
	*fill    = 0;
#endif
	return MPG123_OK;
}

int attribute_align_arg mpg123_set_index64(mpg123_handle *mh, int64_t *offsets, int64_t step, size_t fill)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;
#ifdef FRAME_INDEX
	if(step == 0)
	{
		mh->err = MPG123_BAD_INDEX_PAR;
		return MPG123_ERR;
	}
	if(INT123_fi_set(&mh->index, offsets, step, fill) == -1)
	{
		mh->err = MPG123_OUT_OF_MEM;
		return MPG123_ERR;
	}
	return MPG123_OK;
#else
	mh->err = MPG123_MISSING_FEATURE;
	return MPG123_ERR;
#endif
}


int attribute_align_arg mpg123_close(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_BAD_HANDLE;

	/* mh->rd is never NULL! */
	if(mh->rd->close != NULL) mh->rd->close(mh);

	if(mh->new_format)
	{
		debug("Hey, we are closing a track before the new format has been queried...");
		INT123_invalidate_format(&mh->af);
		mh->new_format = 0;
	}
	/* Always reset the frame buffers on close, so we cannot forget it in funky opening routines (wrappers, even). */
	INT123_frame_reset(mh);
	return MPG123_OK;
}

void attribute_align_arg mpg123_delete(mpg123_handle *mh)
{
	if(mh != NULL)
	{
		mpg123_close(mh);
#ifndef PORTABLE_API
		INT123_wrap_destroy(mh->wrapperdata);
#endif
		INT123_frame_exit(mh); /* free buffers in frame */
		free(mh); /* free struct; cast? */
	}
}

void attribute_align_arg mpg123_free(void *ptr)
{
	free(ptr);
}

static const char *mpg123_error[] =
{
	"No error... (code 0)",
	"Unable to set up output format! (code 1)",
	"Invalid channel number specified. (code 2)",
	"Invalid sample rate specified. (code 3)",
	"Unable to allocate memory for 16 to 8 converter table! (code 4)",
	"Bad parameter id! (code 5)",
	"Bad buffer given -- invalid pointer or too small size. (code 6)",
	"Out of memory -- some malloc() failed. (code 7)",
	"You didn't initialize the library! (code 8)",
	"Invalid decoder choice. (code 9)",
	"Invalid mpg123 handle. (code 10)",
	"Unable to initialize frame buffers (out of memory?)! (code 11)",
	"Invalid RVA mode. (code 12)",
	"This build doesn't support gapless decoding. (code 13)",
	"Not enough buffer space. (code 14)",
	"Incompatible numeric data types. (code 15)",
	"Bad equalizer band. (code 16)",
	"Null pointer given where valid storage address needed. (code 17)",
	"Error reading the stream. (code 18)",
	"Cannot seek from end (end is not known). (code 19)",
	"Invalid 'whence' for seek function. (code 20)",
	"Build does not support stream timeouts. (code 21)",
	"File access error. (code 22)",
	"Seek not supported by stream. (code 23)",
	"No stream opened or missing reader setup while opening. (code 24)",
	"Bad parameter handle. (code 25)",
	"Invalid parameter addresses for index retrieval. (code 26)",
	"Lost track in the bytestream and did not attempt resync. (code 27)",
	"Failed to find valid MPEG data within limit on resync. (code 28)",
	"No 8bit encoding possible. (code 29)",
	"Stack alignment is not good. (code 30)",
	"You gave me a NULL buffer? (code 31)",
	"File position is screwed up, please do an absolute seek (code 32)",
	"Inappropriate NULL-pointer provided.",
	"Bad key value given.",
	"There is no frame index (disabled in this build).",
	"Frame index operation failed.",
	"Decoder setup failed (invalid combination of settings?)",
	"Feature not in this build."
	,"Some bad value has been provided."
	,"Low-level seeking has failed (call to lseek(), usually)."
	,"Custom I/O obviously not prepared."
	,"Overflow in LFS (large file support) conversion."
	,"Overflow in integer conversion."
	,"Bad IEEE 754 rounding. Re-build libmpg123 properly."
};

const char* attribute_align_arg mpg123_plain_strerror(int errcode)
{
	if(errcode >= 0 && errcode < sizeof(mpg123_error)/sizeof(char*))
	return mpg123_error[errcode];
	else switch(errcode)
	{
		case MPG123_ERR:
			return "A generic mpg123 error.";
		case MPG123_DONE:
			return "Message: I am done with this track.";
		case MPG123_NEED_MORE:
			return "Message: Feed me more input data!";
		case MPG123_NEW_FORMAT:
			return "Message: Prepare for a changed audio format (query the new one)!";
		default:
			return "I have no idea - an unknown error code!";
	}
}

int attribute_align_arg mpg123_errcode(mpg123_handle *mh)
{
	if(mh != NULL) return mh->err;
	return MPG123_BAD_HANDLE;
}

const char* attribute_align_arg mpg123_strerror(mpg123_handle *mh)
{
	return mpg123_plain_strerror(mpg123_errcode(mh));
}

#ifndef PORTABLE_API
// Isolation of lfs_wrap.c code, limited hook to get at its data and
// for storing error codes.

void ** INT123_wrap_handle(mpg123_handle *mh)
{
	if(mh == NULL)
		return NULL;
	return &(mh->wrapperdata);
}

int INT123_set_err(mpg123_handle *mh, int err)
{
	if(mh)
		mh->err = err;
	return MPG123_ERR;
}
#endif
