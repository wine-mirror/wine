/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifndef _FLUID_IIR_FILTER_H
#define _FLUID_IIR_FILTER_H

#include "fluidsynth_priv.h"

// Uncomment to get debug logging for filter parameters
// #define DBG_FILTER
#ifdef DBG_FILTER
#define LOG_FILTER(...) FLUID_LOG(FLUID_DBG, __VA_ARGS__)
#else
#define LOG_FILTER(...)
#endif

typedef struct _fluid_iir_filter_t fluid_iir_filter_t;

DECLARE_FLUID_RVOICE_FUNCTION(fluid_iir_filter_init);
DECLARE_FLUID_RVOICE_FUNCTION(fluid_iir_filter_set_fres);
DECLARE_FLUID_RVOICE_FUNCTION(fluid_iir_filter_set_q);

void fluid_iir_filter_apply(fluid_iir_filter_t *iir_filter,
                            fluid_real_t *dsp_buf, int dsp_buf_count, fluid_real_t output_rate);

void fluid_iir_filter_reset(fluid_iir_filter_t *iir_filter);

void fluid_iir_filter_calc(fluid_iir_filter_t *iir_filter,
                           fluid_real_t output_rate,
                           fluid_real_t fres_mod);

/* We can't do information hiding here, as fluid_voice_t includes the struct
   without a pointer. */
struct _fluid_iir_filter_t
{
    enum fluid_iir_filter_type type; /* specifies the type of this filter */
    enum fluid_iir_filter_flags flags; /* additional flags to customize this filter */

    /* filter coefficients */
    /* The coefficients are normalized to a0. */
    /* b0 and b2 are identical => b02 */
    fluid_real_t b02;              /* b0 / a0 */
    fluid_real_t b1;              /* b1 / a0 */
    fluid_real_t a1;              /* a0 / a0 */
    fluid_real_t a2;              /* a1 / a0 */

    fluid_real_t hist1, hist2;      /* Sample history for the IIR filter */
    int filter_startup;             /* Flag: If set, the filter parameters will be set directly. Else it changes smoothly. */

    fluid_real_t fres;              /* The desired resonance frequency, in absolute cents, this filter is currently set to */
    fluid_real_t last_fres;         /* The filter's current (smoothed out) resonance frequency in Hz, which will converge towards its target fres once fres_incr_count has become zero */
    fluid_real_t fres_incr;         /* The linear increment of fres each sample */
    int fres_incr_count;            /* The number of samples left for the smoothed last_fres adjustment to complete */

    fluid_real_t last_q;            /* The filter's current (smoothed) Q-factor (or "bandwidth", or "resonance-friendlyness") on a linear scale. Just like fres, this will converge towards its target Q once q_incr_count has become zero. */
    fluid_real_t q_incr;            /* The linear increment of q each sample */
    int q_incr_count;               /* The number of samples left for the smoothed Q adjustment to complete */
#ifdef DBG_FILTER
    fluid_real_t target_fres;       /* The filter's target fres, that last_fres should converge towards - for debugging only */
    fluid_real_t target_q;          /* The filter's target Q - for debugging only */
#endif
};

#endif
