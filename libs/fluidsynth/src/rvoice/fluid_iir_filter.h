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
#include "fluid_sys.h"

// Uncomment to get debug logging for filter parameters
// #define DBG_FILTER
#ifdef DBG_FILTER
#define LOG_FILTER(...) FLUID_LOG(FLUID_DBG, __VA_ARGS__)
#else
#define LOG_FILTER(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct _fluid_iir_filter_t fluid_iir_filter_t;

DECLARE_FLUID_RVOICE_FUNCTION(fluid_iir_filter_init);
DECLARE_FLUID_RVOICE_FUNCTION(fluid_iir_filter_set_fres);
DECLARE_FLUID_RVOICE_FUNCTION(fluid_iir_filter_set_q);

#define Q_MIN ((fluid_real_t)0.001)

static FLUID_INLINE void
fluid_iir_filter_calculate_coefficients(fluid_iir_filter_t *iir_filter, fluid_real_t output_rate,
                                        fluid_real_t *a1_out, fluid_real_t *a2_out,
                                        fluid_real_t *b02_out, fluid_real_t *b1_out)
{
    int flags = iir_filter->flags;
    fluid_real_t filter_gain = 1.0f;

    /*
     * Those equations from Robert Bristow-Johnson's `Cookbook
     * formulae for audio EQ biquad filter coefficients', obtained
     * from Harmony-central.com / Computer / Programming. They are
     * the result of the bilinear transform on an analogue filter
     * prototype. To quote, `BLT frequency warping has been taken
     * into account for both significant frequency relocation and for
     * bandwidth readjustment'. */

    fluid_real_t omega = (fluid_real_t)(2.0 * M_PI) *
                                       (iir_filter->last_fres / output_rate);
    fluid_real_t sin_coeff = FLUID_SIN(omega);
    fluid_real_t cos_coeff = FLUID_COS(omega);
    fluid_real_t alpha_coeff = sin_coeff / (2.0f * iir_filter->last_q);
    fluid_real_t a0_inv = 1.0f / (1.0f + alpha_coeff);

    /* Calculate the filter coefficients. All coefficients are
     * normalized by a0. Think of `a1' as `a1/a0'.
     *
     * Here a couple of multiplications are saved by reusing common expressions.
     * The original equations should be:
     *  iir_filter->b0=(1.-cos_coeff)*a0_inv*0.5*filter_gain;
     *  iir_filter->b1=(1.-cos_coeff)*a0_inv*filter_gain;
     *  iir_filter->b2=(1.-cos_coeff)*a0_inv*0.5*filter_gain; */

    /* "a" coeffs are same for all 3 available filter types */
    fluid_real_t a1_temp = -2.0f * cos_coeff * a0_inv;
    fluid_real_t a2_temp = (1.0f - alpha_coeff) * a0_inv;
    fluid_real_t b02_temp, b1_temp;

    if(!(flags & FLUID_IIR_NO_GAIN_AMP))
    {
        /* SF 2.01 page 59:
         *
         *  The SoundFont specs ask for a gain reduction equal to half the
         *  height of the resonance peak (Q).  For example, for a 10 dB
         *  resonance peak, the gain is reduced by 5 dB.  This is done by
         *  multiplying the total gain with sqrt(1/Q).  `Sqrt' divides dB
         *  by 2 (100 lin = 40 dB, 10 lin = 20 dB, 3.16 lin = 10 dB etc)
         *  The gain is later factored into the 'b' coefficients
         *  (numerator of the filter equation).  This gain factor depends
         *  only on Q, so this is the right place to calculate it.
         */
        filter_gain /= FLUID_SQRT(iir_filter->last_q);
    }

    switch(iir_filter->type)
    {
    case FLUID_IIR_HIGHPASS:
        b1_temp = (1.0f + cos_coeff) * a0_inv * filter_gain;

        /* both b0 -and- b2 */
        b02_temp = b1_temp * 0.5f;

        b1_temp *= -1.0f;
        break;

    case FLUID_IIR_LOWPASS:
        b1_temp = (1.0f - cos_coeff) * a0_inv * filter_gain;

        /* both b0 -and- b2 */
        b02_temp = b1_temp * 0.5f;
        break;

    default:
        /* filter disabled, should never get here */
        return;
    }

    *a1_out = a1_temp;
    *a2_out = a2_temp;
    *b02_out = b02_temp;
    *b1_out = b1_temp;

    fluid_check_fpe("voice_write filter calculation");
}

/**
 * Applies a low- or high-pass filter with variable cutoff frequency and quality factor
 * for a given biquad transfer function:
 *          b0 + b1*z^-1 + b2*z^-2
 *  H(z) = ------------------------
 *          a0 + a1*z^-1 + a2*z^-2
 *
 * Also modifies filter state accordingly.
 * @param iir_filter Filter parameter
 * @param dsp_buf Pointer to the synthesized audio data
 * @param count Count of samples in dsp_buf
 */
/*
 * Variable description:
 * - dsp_a1, dsp_a2: Filter coefficients for the the previously filtered output signal
 * - dsp_b0, dsp_b1, dsp_b2: Filter coefficients for input signal
 * - coefficients normalized to a0
 *
 * A couple of variables are used internally, their results are discarded:
 * - dsp_i: Index through the output buffer
 * - dsp_centernode: delay line for the IIR filter
 * - dsp_hist1: same
 * - dsp_hist2: same
 */
static FLUID_INLINE void
fluid_iir_filter_apply(fluid_iir_filter_t *iir_filter,
                       fluid_real_t *dsp_buf, int count, fluid_real_t output_rate)
{
    // FLUID_IIR_Q_LINEAR may switch the filter off by setting Q==0
    // Due to the linear smoothing, last_q may not exactly become zero.
    if (iir_filter->type == FLUID_IIR_DISABLED || FLUID_FABS(iir_filter->last_q) < Q_MIN)
    {
        return;
    }
    else
    {
        /* IIR filter sample history */
        fluid_real_t dsp_hist1 = iir_filter->hist1;
        fluid_real_t dsp_hist2 = iir_filter->hist2;

        /* IIR filter coefficients */
        fluid_real_t dsp_a1 = iir_filter->a1;
        fluid_real_t dsp_a2 = iir_filter->a2;
        fluid_real_t dsp_b02 = iir_filter->b02;
        fluid_real_t dsp_b1 = iir_filter->b1;

        int fres_incr_count = iir_filter->fres_incr_count;
        int q_incr_count = iir_filter->q_incr_count;

        fluid_real_t dsp_centernode;
        int dsp_i;

        /* filter (implement the voice filter according to SoundFont standard) */

        /* Check for denormal number (too close to zero). */
        if(FLUID_FABS(dsp_hist1) < 1e-20f)
        {
            dsp_hist1 = 0.0f;    /* FIXME JMG - Is this even needed? */
        }

        /* Two versions of the filter loop. One, while the filter is
        * changing towards its new setting. The other, if the filter
        * doesn't change.
        */

        for(dsp_i = 0; dsp_i < count; dsp_i++)
        {
            /* The filter is implemented in Direct-II form. */
            dsp_centernode = dsp_buf[dsp_i] - dsp_a1 * dsp_hist1 - dsp_a2 * dsp_hist2;
            dsp_buf[dsp_i] = dsp_b02 * (dsp_centernode + dsp_hist2) + dsp_b1 * dsp_hist1;
            dsp_hist2 = dsp_hist1;
            dsp_hist1 = dsp_centernode;
            /* Alternatively, it could be implemented in Transposed Direct Form II */
            // fluid_real_t dsp_input = dsp_buf[dsp_i];
            // dsp_buf[dsp_i] = dsp_b02 * dsp_input + dsp_hist1;
            // dsp_hist1 = dsp_b1 * dsp_input - dsp_a1 * dsp_buf[dsp_i] + dsp_hist2;
            // dsp_hist2 = dsp_b02 * dsp_input - dsp_a2 * dsp_buf[dsp_i];

            if(fres_incr_count > 0 || q_incr_count > 0)
            {
                if(fres_incr_count > 0)
                {
                    --fres_incr_count;
                    iir_filter->last_fres += iir_filter->fres_incr;
                }
                if(q_incr_count > 0)
                {
                    --q_incr_count;
                    iir_filter->last_q += iir_filter->q_incr;
                }

                LOG_FILTER("last_fres: %.2f Hz  |  target_fres: %.2f Hz  |---|  last_q: %.4f  |  target_q: %.4f", iir_filter->last_fres, iir_filter->target_fres, iir_filter->last_q, iir_filter->target_q);

                fluid_iir_filter_calculate_coefficients(iir_filter, output_rate, &dsp_a1, &dsp_a2, &dsp_b02, &dsp_b1);
            }
        }

        iir_filter->hist1 = dsp_hist1;
        iir_filter->hist2 = dsp_hist2;
        iir_filter->a1 = dsp_a1;
        iir_filter->a2 = dsp_a2;
        iir_filter->b02 = dsp_b02;
        iir_filter->b1 = dsp_b1;

        iir_filter->fres_incr_count = fres_incr_count;
        iir_filter->q_incr_count = q_incr_count;

        fluid_check_fpe("voice_filter");
    }
}

void fluid_iir_filter_reset(fluid_iir_filter_t *iir_filter);

void fluid_iir_filter_calc(fluid_iir_filter_t *iir_filter,
                           fluid_real_t output_rate,
                           fluid_real_t fres_mod);

#ifdef __cplusplus
}
#endif
#endif
