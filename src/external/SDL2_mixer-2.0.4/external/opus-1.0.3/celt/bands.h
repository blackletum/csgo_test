﻿/* Copyright (c) 2007-2008 CSIRO
   Copyright (c) 2007-2009 Xiph.Org Foundation
   Copyright (c) 2008-2009 Gregory Maxwell
   Written by Jean-Marc Valin and Gregory Maxwell */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef BANDS_H
#define BANDS_H

#include "arch.h"
#include "modes.h"
#include "entenc.h"
#include "entdec.h"
#include "rate.h"

/** Compute the amplitude (sqrt energy) in each of the bands
 * @param m Mode data
 * @param X Spectrum
 * @param bands Square root of the energy for each band (returned)
 */
void compute_band_energies(const CELTMode *m, const celt_sig *X, celt_ener *bandE, int end, int C, int M);

/*void compute_noise_energies(const CELTMode *m, const celt_sig *X, const opus_val16 *tonality, celt_ener *bandE);*/

/** Normalise each band of X such that the energy in each band is
    equal to 1
 * @param m Mode data
 * @param X Spectrum (returned normalised)
 * @param bands Square root of the energy for each band
 */
void normalise_bands(const CELTMode *m, const celt_sig * OPUS_RESTRICT freq, celt_norm * OPUS_RESTRICT X, const celt_ener *bandE, int end, int C, int M);

/** Denormalise each band of X to restore full amplitude
 * @param m Mode data
 * @param X Spectrum (returned de-normalised)
 * @param bands Square root of the energy for each band
 */
void denormalise_bands(const CELTMode *m, const celt_norm * OPUS_RESTRICT X, celt_sig * OPUS_RESTRICT freq, const celt_ener *bandE, int end, int C, int M);

#define SPREAD_NONE       (0)
#define SPREAD_LIGHT      (1)
#define SPREAD_NORMAL     (2)
#define SPREAD_AGGRESSIVE (3)

int spreading_decision(const CELTMode *m, celt_norm *X, int *average,
      int last_decision, int *hf_average, int *tapset_decision, int update_hf,
      int end, int C, int M);

#ifdef MEASURE_NORM_MSE
void measure_norm_mse(const CELTMode *m, float *X, float *X0, float *bandE, float *bandE0, int M, int N, int C);
#endif

void haar1(celt_norm *X, int N0, int stride);

/** Quantisation/encoding of the residual spectrum
 * @param m Mode data
 * @param X Residual (normalised)
 * @param total_bits Total number of bits that can be used for the frame (including the ones already spent)
 * @param enc Entropy encoder
 */
void quant_all_bands(int encode, const CELTMode *m, int start, int end,
      celt_norm * X, celt_norm * Y, unsigned char *collapse_masks, const celt_ener *bandE, int *pulses,
      int time_domain, int fold, int dual_stereo, int intensity, int *tf_res,
      opus_int32 total_bits, opus_int32 balance, ec_ctx *ec, int M, int codedBands, opus_uint32 *seed);

void anti_collapse(const CELTMode *m, celt_norm *X_, unsigned char *collapse_masks, int LM, int C, int size,
      int start, int end, opus_val16 *logE, opus_val16 *prev1logE,
      opus_val16 *prev2logE, int *pulses, opus_uint32 seed);

opus_uint32 celt_lcg_rand(opus_uint32 seed);

#endif /* BANDS_H */
