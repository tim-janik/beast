/* BseResampler - FPU and SSE optimized FIR Resampling code
 * Copyright (C) 2006 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bseresampler.hh"
#include "bseblockutils.hh"

namespace Bse
{

namespace Resampler
{

/*---- Resampler2 methods ----*/

Resampler2*
Resampler2::create (BseResampler2Mode      mode,
                    BseResampler2Precision precision)
{
  return Block::create_resampler2 (mode, precision);
}

Resampler2::~Resampler2()
{
}

/*---- coefficient sets for Resampler2 ----*/
/* 
 * halfband FIR filter for factor 2 resampling, created with octave
 *
 * design method: windowed sinc,  using ultraspherical window
 *
 *   coefficients = 32
 *             x0 = 1.01065
 *          alpha = 0.75
 *
 * design criteria (44100 Hz => 88200 Hz):
 * 
 *       passband = [     0, 18000 ]  1 - 2^-16 <= H(z) <= 1+2^-16
 *     transition = [ 18000, 26100 ]
 *       stopband = [ 26100, 44100 ]  | H(z) | <= -96 dB
 *
 * and for 48 kHz => 96 kHz:
 *
 *       passband = [     0, 19589 ]  1 - 2^-16 <= H(z) <= 1+2^-16
 *     transition = [ 19588, 29386 ]
 *       stopband = [ 29386, 48000 ]  | H(z) | <= -96 dB
 *
 * in order to keep the coefficient number down to 32, the filter
 * does only "almost" fulfill the spec, but its really really close
 * (no stopband ripple > -95 dB)
 */

const double Resampler2::halfband_fir_96db_coeffs[32] =
{
  -3.48616530828033e-05,
   0.000112877490936198,
  -0.000278961878372482,
   0.000590495306376081,
  -0.00112566995029848,
   0.00198635062559427,
  -0.00330178798332932,
   0.00523534239035401,
  -0.00799905465189065,
   0.0118867161189188,
  -0.0173508611368417,
   0.0251928452706978,
  -0.0370909694665106,
   0.057408291607388,
  -0.102239638342325,
   0.317002929635456,
   /* here, a 0.5 coefficient will be used */
   0.317002929635456,
  -0.102239638342325,
   0.0574082916073878,
  -0.0370909694665105,
   0.0251928452706976,
  -0.0173508611368415,
   0.0118867161189186,
  -0.00799905465189052,
   0.0052353423903539,
  -0.00330178798332923,
   0.00198635062559419,
  -0.00112566995029842,
   0.000590495306376034,
  -0.00027896187837245,
   0.000112877490936177,
  -3.48616530827983e-05
};

/*
 *   coefficients = 16
 *             x0 = 1.013
 *          alpha = 0.2
 */
const double Resampler2::halfband_fir_48db_coeffs[16] =
{
  -0.00270578824181636,
   0.00566964586625895,
  -0.0106460585587187,
   0.0185209590435965,
  -0.0310433957594089,
   0.0525722488176905,
  -0.0991138314110143,
   0.315921760444802,
   /* here, a 0.5 coefficient will be used */
   0.315921760444802,
  -0.0991138314110145,
   0.0525722488176907,
  -0.031043395759409,
   0.0185209590435966,
  -0.0106460585587187,
   0.00566964586625899,
  -0.00270578824181638
};

/*
 *   coefficients = 24
 *             x0 = 1.0105
 *          alpha = 0.93
 */
const double Resampler2::halfband_fir_72db_coeffs[24] =
{
  -0.0002622341634289771,
   0.0007380549701258316,
  -0.001634275943268986,
   0.00315564206632209,
  -0.005564668530702518,
   0.009207977968023688,
  -0.0145854155294611,
   0.02253220964143239,
  -0.03474055058489597,
   0.05556350980411048,
  -0.1010616834297558,
   0.316597934725021,
   /* here, a 0.5 coefficient will be used */
   0.3165979347250216,
  -0.1010616834297563,
   0.0555635098041109,
  -0.03474055058489638,
   0.02253220964143274,
  -0.01458541552946141,
   0.00920797796802395,
  -0.005564668530702722,
   0.003155642066322248,
  -0.001634275943269096,
   0.000738054970125897,
  -0.0002622341634290046,
};

/*
 *   coefficients = 42
 *             x0 = 1.0106
 *          alpha = 0.8
 */
const double Resampler2::halfband_fir_120db_coeffs[42] = {
   2.359361930421347e-06,
  -9.506281154947505e-06,
   2.748456705299089e-05,
  -6.620621425709478e-05,
   0.0001411845354098405,
  -0.0002752082937581387,
   0.0005000548069542907,
  -0.0008581650926168509,
   0.001404290771748464,
  -0.002207303823772437,
   0.003352696749689989,
  -0.004946913550236211,
   0.007125821223639453,
  -0.01007206140806936,
   0.01405163477932994,
  -0.01949467352546547,
   0.02718899890919871,
  -0.038810852733035,
   0.05873397010869939,
  -0.1030762204838426,
   0.317288892550808,
   /* here, a 0.5 coefficient will be used */
   0.3172888925508079,
  -0.1030762204838425,
   0.0587339701086993,
  -0.03881085273303492,
   0.02718899890919862,
  -0.01949467352546535,
   0.01405163477932982,
  -0.01007206140806923,
   0.007125821223639309,
  -0.004946913550236062,
   0.003352696749689839,
  -0.00220730382377229,
   0.001404290771748321,
  -0.0008581650926167192,
   0.0005000548069541726,
  -0.0002752082937580344,
   0.0001411845354097548,
  -6.620621425702783e-05,
   2.748456705294319e-05,
  -9.506281154917077e-06,
   2.359361930409472e-06
};

/*
 *   coefficients = 52
 *             x0 = 1.0104
 *          alpha = 0.8
 */
const double Resampler2::halfband_fir_144db_coeffs[52] = {
  -1.841826652087099e-07,
   8.762360674826639e-07,
  -2.867933918842901e-06,
   7.670965310712155e-06,
  -1.795091436711159e-05,
   3.808294405088742e-05,
  -7.483688716947913e-05,
   0.0001381756990743866,
  -0.0002421379200249195,
   0.0004057667984715052,
  -0.0006540521320531017,
   0.001018873594538604,
  -0.001539987101083099,
   0.002266194978575507,
  -0.003257014968854008,
   0.004585469100383752,
  -0.006343174213238195,
   0.008650017657145861,
  -0.01167305853124126,
   0.01566484143899151,
  -0.02104586507283325,
   0.02859957136356252,
  -0.04000402932277326,
   0.05964131775019404,
  -0.1036437507243546,
   0.3174820359034792,
   /* here, a 0.5 coefficient will be used */
   0.3174820359034791,
  -0.1036437507243545,
   0.05964131775019401,
  -0.04000402932277325,
   0.0285995713635625,
  -0.02104586507283322,
   0.01566484143899148,
  -0.01167305853124122,
   0.008650017657145822,
  -0.006343174213238157,
   0.004585469100383712,
  -0.003257014968853964,
   0.002266194978575464,
  -0.00153998710108306,
   0.001018873594538566,
  -0.0006540521320530672,
   0.0004057667984714751,
  -0.0002421379200248905,
   0.0001381756990743623,
  -7.483688716946011e-05,
   3.808294405087123e-05,
  -1.795091436709889e-05,
   7.670965310702215e-06,
  -2.867933918835638e-06,
   8.762360674786308e-07,
  -1.841826652067372e-07,
};

} /* namespace Resampler */

} /* namespace Bse */

/*---- Resampler2 C API ----*/

BseResampler2*
bse_resampler2_create (BseResampler2Mode      mode,
                       BseResampler2Precision precision)
{
  return reinterpret_cast<BseResampler2 *> (Bse::Resampler::Resampler2::create (mode, precision));
}

void
bse_resampler2_destroy (BseResampler2 *resampler)
{
  delete reinterpret_cast<Bse::Resampler::Resampler2 *> (resampler);
}

void
bse_resampler2_process_block (BseResampler2 *resampler,
                              const float   *input,
                              unsigned int   n_input_samples,
			      float         *output)
{
  reinterpret_cast<Bse::Resampler::Resampler2 *> (resampler)->process_block (input, n_input_samples, output);
}

guint
bse_resampler2_order (BseResampler2 *resampler)
{
  return reinterpret_cast<Bse::Resampler::Resampler2 *> (resampler)->order();
}
