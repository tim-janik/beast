/* BSE - Better Sound Engine
 * Copyright (C) 1997-2004 Tim Janik
 * Copyright (C) 2001 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsemathsignal.h"


/* --- frequency modulation --- */
void
bse_frequency_modulator (const BseFrequencyModulator *fm,
			 uint                         n_values,
			 const float                 *ifreq,
			 const float                 *ifmod,
			 float                       *fm_buffer)
{
  float *bound, fine_tune, fm_strength;
  gboolean with_fine_tune;

  fine_tune = bse_cent_tune_fast (fm->fine_tune);
  with_fine_tune = fm->fine_tune != 0;
  fm_strength = fm->fm_strength;
  
  bound = fm_buffer + n_values;
  if (ifreq && ifmod)
    {
      if (fm->exponential_fm)
	{
	  if (with_fine_tune)
	    do {
	      *fm_buffer++ = *ifreq++ * bse_approx5_exp2 (fm_strength * *ifmod++) * fine_tune;
	    } while (fm_buffer < bound);
	  else
	    do {
	      *fm_buffer++ = *ifreq++ * bse_approx5_exp2 (fm_strength * *ifmod++);
	    } while (fm_buffer < bound);
	}
      else
	{
	  if (with_fine_tune)
	    do {
	      *fm_buffer++ = *ifreq++ * (1 + fm_strength * *ifmod++) * fine_tune;
	    } while (fm_buffer < bound);
	  else
	    do {
	      *fm_buffer++ = *ifreq++ * (1 + fm_strength * *ifmod++);
	    } while (fm_buffer < bound);
	}
    }
  else if (ifmod)
    {
      float signal_freq = fm->signal_freq * fine_tune;

      if (fm->exponential_fm)
	do {
	  *fm_buffer++ = signal_freq * bse_approx5_exp2 (fm_strength * *ifmod++);
	} while (fm_buffer < bound);
      else
	do {
	  *fm_buffer++ = signal_freq * (1 + fm_strength * *ifmod++);
	} while (fm_buffer < bound);
    }
  else if (ifreq)
    {
      if (with_fine_tune)
	do {
	  *fm_buffer++ = *ifreq++ * fine_tune;
	} while (fm_buffer < bound);
      else
	do {
	  *fm_buffer++ = *ifreq++;
	} while (fm_buffer < bound);
    }
  else
    {
      float signal_freq = fm->signal_freq * fine_tune;

      do {
	*fm_buffer++ = signal_freq;
      } while (fm_buffer < bound);
    }
}


/* --- windows --- */
double
bse_window_bartlett (double x)	/* triangle */
{
  if (fabs (x) > 1)
    return 0;

  return 1.0 - fabs (x);
}

double
bse_window_blackman (double x)
{
  if (fabs (x) > 1)
    return 0;

  return 0.42 + 0.5 * cos (PI * x) + 0.08 * cos (2.0 * PI * x);
}

double
bse_window_cos (double x)	/* von Hann window */
{
  if (fabs (x) > 1)
    return 0;

  return 0.5 * cos (x * PI) + 0.5;
}

double
bse_window_hamming (double x)	/* sharp (rectangle) cutoffs at boundaries */
{
  if (fabs (x) > 1)
    return 0;

  return 0.54 + 0.46 * cos (PI * x);
}

double
bse_window_sinc (double x)	/* noramlied C. Lanczos window */
{
  if (fabs (x) > 1)
    return 0;
  x = x * PI;
  if (fabs (x) < 1e-12)
    return 1.0;
  else
    return sin (x) / x;
}

double
bse_window_rect (double x)	/* a square */
{
  if (fabs (x) > 1)
    return 0;
  return 1.0;
}

/*
cos_roll_off(x)= x>fh?0:x<fl?1:cos(pi/2.*((fl-x)/(fh-fl))) 
*/


/* --- fine tune factors for -100..+100 cent --- */
static const double cent_table201[100 + 1 + 100] = {
  /* 2^(1/1200*-100) .. 2^(1/1200*0) .. 2^(1/1200*+100) */
  0.94387431268169349664, 0.94441967335506765930, 0.94496534913211618524, 0.94551134019490267099, /* 2^(1/1200*-97) */
  0.94605764672559590751, 0.94660426890646994096, 0.94715120691990413357, 0.94769846094838322441, /* 2^(1/1200*-93) */
  0.94824603117449739035, 0.94879391778094230692, 0.94934212095051920932, 0.94989064086613495337, /* 2^(1/1200*-89) */
  0.95043947771080207655, 0.95098863166763885907, 0.95153810291986938497, 0.95208789165082360322, /* 2^(1/1200*-85) */
  0.95263799804393738893, 0.95318842228275260453, 0.95373916455091716100, 0.95429022503218507919, /* 2^(1/1200*-81) */
  0.95484160391041655104, 0.95539330136957800103, 0.95594531759374214748, 0.95649765276708806401, /* 2^(1/1200*-77) */
  0.95705030707390124097, 0.95760328069857364694, 0.95815657382560379022, 0.95871018663959678045, /* 2^(1/1200*-73) */
  0.95926411932526439013, 0.95981837206742511631, 0.96037294505100424222, 0.96092783846103389896, /* 2^(1/1200*-69) */
  0.96148305248265312728, 0.96203858730110793932, 0.96259444310175138040, 0.96315062007004359091, /* 2^(1/1200*-65) */
  0.96370711839155186816, 0.96426393825195072828, 0.96482107983702196821, 0.96537854333265472764, /* 2^(1/1200*-61) */
  0.96593632892484555106, 0.96649443679969844984, 0.96705286714342496425, 0.96761162014234422567, /* 2^(1/1200*-57) */
  0.96817069598288301869, 0.96873009485157584337, 0.96928981693506497742, 0.96984986242010053851, /* 2^(1/1200*-53) */
  0.97041023149354054658, 0.97097092434235098615, 0.97153194115360586874, 0.97209328211448729528, /* 2^(1/1200*-49) */
  0.97265494741228551852, 0.97321693723439900559, 0.97377925176833450047, 0.97434189120170708655, /* 2^(1/1200*-45) */
  0.97490485572224024929, 0.97546814551776593878, 0.97603176077622463245, 0.97659570168566539775, /* 2^(1/1200*-41) */
  0.97715996843424595493, 0.97772456121023273979, 0.97828948020200096649, 0.97885472559803469042, /* 2^(1/1200*-37) */
  0.97942029758692687108, 0.97998619635737943501, 0.98055242209820333873, 0.98111897499831863174, /* 2^(1/1200*-33) */
  0.98168585524675451960, 0.98225306303264942693, 0.98282059854525106055, 0.98338846197391647262, /* 2^(1/1200*-29) */
  0.98395665350811212383, 0.98452517333741394660, 0.98509402165150740832, 0.98566319864018757467, /* 2^(1/1200*-25) */
  0.98623270449335917291, 0.98680253940103665527, 0.98737270355334426234, 0.98794319714051608649, /* 2^(1/1200*-21) */
  0.98851402035289613536, 0.98908517338093839536, 0.98965665641520689521, 0.99022846964637576952, /* 2^(1/1200*-17) */
  0.99080061326522932245, 0.99137308746266209128, 0.99194589242967891017, 0.99251902835739497389, /* 2^(1/1200*-13) */
  0.99309249543703590153, 0.99366629385993780037, 0.99424042381754732964, 0.99481488550142176449, /* 2^(1/1200*-9) */
  0.99538967910322905982, 0.99596480481474791428, 0.99654026282786783423, 0.99711605333458919778, /* 2^(1/1200*-5) */
  0.99769217652702331884, 0.99826863259739251122, 0.99884542173803015276, 0.99942254414138074953, /* 2^(1/1200*-1) */
  1.00000000000000000000, /* 2^(1/1200*0) */
  1.00057778950655485930, 1.00115591285382360350, 1.00173437023469589396, 1.00231316184217284163, /* 2^(1/1200*4) */
  1.00289228786936707150, 1.00347174850950278700, 1.00405154395591583449, 1.00463167440205376771, /* 2^(1/1200*8) */
  1.00521214004147591243, 1.00579294106785343092, 1.00637407767496938663, 1.00695555005671880883, /* 2^(1/1200*12) */
  1.00753735840710875731, 1.00811950292025838709, 1.00870198379039901323, 1.00928480121187417556, /* 2^(1/1200*16) */
  1.00986795537913970359, 1.01045144648676378139, 1.01103527472942701245, 1.01161944030192248469, /* 2^(1/1200*20) */
  1.01220394339915583542, 1.01278878421614531640, 1.01337396294802185887, 1.01395947979002913869, /* 2^(1/1200*24) */
  1.01454533493752364145, 1.01513152858597472769, 1.01571806093096469807, 1.01630493216818885868, /* 2^(1/1200*28) */
  1.01689214249345558626, 1.01747969210268639364, 1.01806758119191599497, 1.01865580995729237127, /* 2^(1/1200*32) */
  1.01924437859507683576, 1.01983328730164409940, 1.02042253627348233639, 1.02101212570719324976, /* 2^(1/1200*36) */
  1.02160205579949213692, 1.02219232674720795532, 1.02278293874728338810, 1.02337389199677490985, /* 2^(1/1200*40) */
  1.02396518669285285230, 1.02455682303280147013, 1.02514880121401900679, 1.02574112143401776038, /* 2^(1/1200*44) */
  1.02633378389042414951, 1.02692678878097877927, 1.02752013630353650722, 1.02811382665606650935, /* 2^(1/1200*48) */
  1.02870786003665234616, 1.02930223664349202878, 1.02989695667489808505, 1.03049202032929762572, /* 2^(1/1200*52) */
  1.03108742780523241063, 1.03168317930135891498, 1.03227927501644839557, 1.03287571514938695719, /* 2^(1/1200*56) */
  1.03347249989917561889, 1.03406962946493038044, 1.03466710404588228876, 1.03526492384137750435, /* 2^(1/1200*60) */
  1.03586308905087736785, 1.03646159987395846655, 1.03706045651031270103, 1.03765965915974735173, /* 2^(1/1200*64) */
  1.03825920802218514564, 1.03885910329766432300, 1.03945934518633870407, 1.04005993388847775587, /* 2^(1/1200*68) */
  1.04066086960446665901, 1.04126215253480637458, 1.04186378288011371099, 1.04246576084112139095, /* 2^(1/1200*72) */
  1.04306808661867811845, 1.04367076041374864571, 1.04427378242741384032, 1.04487715286087075226, /* 2^(1/1200*76) */
  1.04548087191543268106, 1.04608493979252924297, 1.04668935669370643814, 1.04729412282062671789, /* 2^(1/1200*80) */
  1.04789923837506905201, 1.04850470355892899603, 1.04911051857421875864, 1.04971668362306726905, /* 2^(1/1200*84) */
  1.05032319890772024444, 1.05093006463054025745, 1.05153728099400680369, 1.05214484820071636931, /* 2^(1/1200*88) */
  1.05275276645338249856, 1.05336103595483586147, 1.05396965690802432148, 1.05457862951601300320, /* 2^(1/1200*92) */
  1.05518795398198436013, 1.05579763050923824245, 1.05640765930119196488, 1.05701804056138037450, /* 2^(1/1200*96) */
  1.05762877449345591872, 1.05823986130118871317, 1.05885130118846660974, 1.05946309435929526456, /* 2^(1/1200*100) */
};
const double * const bse_cent_table = cent_table201 + 100;

/**
 * @param fine_tune	fine tuning in cent
 * @return		a factor corresponding to this
 *
 * This function computes a factor which corresponds to a given fine tuning in
 * cent. The result can be used as factor for the frequency or the play speed.
 * It is similar to the bse_cent_tune_fast(), but also works for non-integer
 * floating point values. It is however computationally more expensive.
 */
double
bse_cent_tune (double fine_tune)
{
  return exp (fine_tune * BSE_LN_2_POW_1_DIV_1200_d);
}

/* --- musical tuning systems --- */
#define SCALED_INTERVAL(scale, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12)       \
  scale * (F1), scale * (F2), scale * (F3), scale * (F4), scale * (F5), scale * (F6),   \
  scale * (F7), scale * (F8), scale * (F9), scale * (F10), scale * (F11), scale * (F12)

// http://en.wikipedia.org/wiki/Equal_temperament
static const double semitone_table265_equal_temperament_12_tet[132 + 1 + 132] = {
#define EQTEMP_12_TET(scale)                                                    \
  SCALED_INTERVAL (scale, 1.0,                                                  \
    1.0594630943592952646, 1.1224620483093729814, 1.1892071150027210667,        \
    1.2599210498948731648, 1.3348398541700343648, 1.4142135623730950488,        \
    1.4983070768766814988, 1.5874010519681994748, 1.6817928305074290861,        \
    1.7817974362806786095, 1.8877486253633869933) /* 2^(1/12*[0..11]) */
  EQTEMP_12_TET (1.0 / 2048.0),         EQTEMP_12_TET (1.0 / 1024.0),           EQTEMP_12_TET (1.0 / 512.0),
  EQTEMP_12_TET (1.0 / 256.0),          EQTEMP_12_TET (1.0 / 128.0),            EQTEMP_12_TET (1.0 / 64.0),
  EQTEMP_12_TET (1.0 / 32.0),           EQTEMP_12_TET (1.0 / 16.0),             EQTEMP_12_TET (1.0 / 8.0),
  EQTEMP_12_TET (1.0 / 4.0),            EQTEMP_12_TET (1.0 / 2.0),
  EQTEMP_12_TET (1.0),
  EQTEMP_12_TET (2.0),                  EQTEMP_12_TET (4.0),                    EQTEMP_12_TET (8.0),
  EQTEMP_12_TET (16.0),                 EQTEMP_12_TET (32.0),                   EQTEMP_12_TET (64.0),
  EQTEMP_12_TET (128.0),                EQTEMP_12_TET (256.0),                  EQTEMP_12_TET (512.0),
  EQTEMP_12_TET (1024.0),               2048.0, /* 2^11 */
#undef EQTEMP_12_TET
};

// http://en.wikipedia.org/wiki/Equal_temperament
static const double semitone_table265_equal_temperament_7_tet[132 + 1 + 132] = {
#define EQTEMP_7_TET(scale)                                                     \
  SCALED_INTERVAL (scale,                                                       \
                   1.0,                               /* 2^(0*171/1200) */      \
                   1.0506265879517071439385515999019, /* 2^(85.5/1200) (NA) */  \
                   1.1038162273110462268251028980804, /* 2^(1*171/1200) */      \
                   1.1596986766255304738157085667472, /* 2^(256.5/1200) (NA) */ \
                   1.2184102636751912741300922949139, /* 2^(2*171/1200) */      \
                   1.3449010205670067009547481593893, /* 2^(3*171/1200) */      \
                   1.4129887703710829641940858921341, /* 2^(598.5/1200) (NA) */ \
                   1.4845235706290491252321849551821, /* 2^(4*171/1200) */      \
                   1.5596799337438830130108590901031, /* 2^(769.5/1200) (NA) */ \
                   1.6386412070860804772885357729621, /* 2^(5*171/1200) */      \
                   1.7216000202779154901968996860588, /* 2^(940.5/1200) (NA) */ \
                   1.8087587551221761812432481457391) /* 2^(6*171/1200) */
  EQTEMP_7_TET (1.0 / 2048.0),          EQTEMP_7_TET (1.0 / 1024.0),            EQTEMP_7_TET (1.0 / 512.0),
  EQTEMP_7_TET (1.0 / 256.0),           EQTEMP_7_TET (1.0 / 128.0),             EQTEMP_7_TET (1.0 / 64.0),
  EQTEMP_7_TET (1.0 / 32.0),            EQTEMP_7_TET (1.0 / 16.0),              EQTEMP_7_TET (1.0 / 8.0),
  EQTEMP_7_TET (1.0 / 4.0),             EQTEMP_7_TET (1.0 / 2.0),
  EQTEMP_7_TET (1.0),
  EQTEMP_7_TET (2.0),                   EQTEMP_7_TET (4.0),                     EQTEMP_7_TET (8.0),
  EQTEMP_7_TET (16.0),                  EQTEMP_7_TET (32.0),                    EQTEMP_7_TET (64.0),
  EQTEMP_7_TET (128.0),                 EQTEMP_7_TET (256.0),                   EQTEMP_7_TET (512.0),
  EQTEMP_7_TET (1024.0),                2048.0, /* 2^11 */
#undef EQTEMP_7_TET
};

// http://en.wikipedia.org/wiki/Equal_temperament
static const double semitone_table265_equal_temperament_5_tet[132 + 1 + 132] = {
#define EQTEMP_5_TET(scale)                                                     \
  SCALED_INTERVAL (scale,                                                       \
                   1.0,                               /* 2^(0*240/1200) */      \
                   1.0717734625362931642130063250233, /* 2^(120.0/1200) (NA) */ \
                   1.1486983549970350067986269467779, /* 2^(1*240/1200) */      \
                   1.2311444133449162844993930691677, /* 2^(360.0/1200) (NA) */ \
                   1.3195079107728942593740019712296, /* 2^(2*240/1200) */      \
                   1.3819128799677760808194941234190, /* 2^(560.0/1200) (NA) */ \
                   1.4472692374403780699545336842135, /* 2^(640.0/1200) (NA) */ \
                   1.5157165665103980823472598013064, /* 2^(3*240/1200) */      \
                   1.6245047927124710452194187655506, /* 2^(840.0/1200) (NA) */ \
                   1.7411011265922482782725400349595, /* 2^(4*240/1200) */      \
                   1.8234449771164336156322101570883, /* 2^(1040./1200) (NA) */ \
                   1.9096832078208331020817981494413) /* 2^(1120./1200) (NA) */
  EQTEMP_5_TET (1.0 / 2048.0),          EQTEMP_5_TET (1.0 / 1024.0),            EQTEMP_5_TET (1.0 / 512.0),
  EQTEMP_5_TET (1.0 / 256.0),           EQTEMP_5_TET (1.0 / 128.0),             EQTEMP_5_TET (1.0 / 64.0),
  EQTEMP_5_TET (1.0 / 32.0),            EQTEMP_5_TET (1.0 / 16.0),              EQTEMP_5_TET (1.0 / 8.0),
  EQTEMP_5_TET (1.0 / 4.0),             EQTEMP_5_TET (1.0 / 2.0),
  EQTEMP_5_TET (1.0),
  EQTEMP_5_TET (2.0),                   EQTEMP_5_TET (4.0),                     EQTEMP_5_TET (8.0),
  EQTEMP_5_TET (16.0),                  EQTEMP_5_TET (32.0),                    EQTEMP_5_TET (64.0),
  EQTEMP_5_TET (128.0),                 EQTEMP_5_TET (256.0),                   EQTEMP_5_TET (512.0),
  EQTEMP_5_TET (1024.0),                2048.0, /* 2^11 */
#undef EQTEMP_5_TET
};

// http://en.wikipedia.org/wiki/Diatonic_scale
static const double semitone_table265_diatonic_scale[132 + 1 + 132] = {
#define DIATONIC_SCALE(scale)                                                   \
  SCALED_INTERVAL (scale, 1.0,                                                  \
    16 / 15.0, 9 / 8.0, 6 / 5.0, 5 / 4.0, 4 / 3.0, 45 / 32.0,                   \
    3 / 2.0, 8 / 5.0, 5 / 3.0, 16 / 9.0, 15 / 8.0)
  DIATONIC_SCALE (1.0 / 2048.0),        DIATONIC_SCALE (1.0 / 1024.0),          DIATONIC_SCALE (1.0 / 512.0),
  DIATONIC_SCALE (1.0 / 256.0),         DIATONIC_SCALE (1.0 / 128.0),           DIATONIC_SCALE (1.0 / 64.0),
  DIATONIC_SCALE (1.0 / 32.0),          DIATONIC_SCALE (1.0 / 16.0),            DIATONIC_SCALE (1.0 / 8.0),
  DIATONIC_SCALE (1.0 / 4.0),           DIATONIC_SCALE (1.0 / 2.0),
  DIATONIC_SCALE (1.0),
  DIATONIC_SCALE (2.0),                 DIATONIC_SCALE (4.0),                   DIATONIC_SCALE (8.0),
  DIATONIC_SCALE (16.0),                DIATONIC_SCALE (32.0),                  DIATONIC_SCALE (64.0),
  DIATONIC_SCALE (128.0),               DIATONIC_SCALE (256.0),                 DIATONIC_SCALE (512.0),
  DIATONIC_SCALE (1024.0),              2048.0, /* 2^11 */
#undef DIATONIC_SCALE
};

// http://en.wikipedia.org/wiki/Just_intonation#Indian_scales
static const double semitone_table265_indian_scale[132 + 1 + 132] = {
#define INDIAN_SCALE(scale)                                             \
  SCALED_INTERVAL (scale, 1.0,                                          \
    16 / 15.0, 9 / 8.0, 6 / 5.0, 5 / 4.0, 4 / 3.0, 45 / 32.0,           \
    3 / 2.0, 8 / 5.0, 5 / 3.0, 27 / 16.0, 15 / 8.0)
  INDIAN_SCALE (1.0 / 2048.0),          INDIAN_SCALE (1.0 / 1024.0),            INDIAN_SCALE (1.0 / 512.0),
  INDIAN_SCALE (1.0 / 256.0),           INDIAN_SCALE (1.0 / 128.0),             INDIAN_SCALE (1.0 / 64.0),
  INDIAN_SCALE (1.0 / 32.0),            INDIAN_SCALE (1.0 / 16.0),              INDIAN_SCALE (1.0 / 8.0),
  INDIAN_SCALE (1.0 / 4.0),             INDIAN_SCALE (1.0 / 2.0),
  INDIAN_SCALE (1.0),
  INDIAN_SCALE (2.0),                   INDIAN_SCALE (4.0),                     INDIAN_SCALE (8.0),
  INDIAN_SCALE (16.0),                  INDIAN_SCALE (32.0),                    INDIAN_SCALE (64.0),
  INDIAN_SCALE (128.0),                 INDIAN_SCALE (256.0),                   INDIAN_SCALE (512.0),
  INDIAN_SCALE (1024.0),                2048.0, /* 2^11 */
#undef INDIAN_SCALE
};

// http://en.wikipedia.org/wiki/Pythagorean_tuning
static const double semitone_table265_pythagorean_tuning[132 + 1 + 132] = {
#define PYTHAGOREAN_TUNING(scale)                                       \
  SCALED_INTERVAL (scale, 1.0,                                          \
    256 / 243.0, 9 / 8.0, 32 / 27.0, 81 / 64.0, 4 / 3.0, 729 / 512.0,   \
    3 / 2.0, 128 / 81.0, 27 / 16.0, 16 / 9.0, 243 / 128.0)
  PYTHAGOREAN_TUNING (1.0 / 2048.0),    PYTHAGOREAN_TUNING (1.0 / 1024.0),      PYTHAGOREAN_TUNING (1.0 / 512.0),
  PYTHAGOREAN_TUNING (1.0 / 256.0),     PYTHAGOREAN_TUNING (1.0 / 128.0),       PYTHAGOREAN_TUNING (1.0 / 64.0),
  PYTHAGOREAN_TUNING (1.0 / 32.0),      PYTHAGOREAN_TUNING (1.0 / 16.0),        PYTHAGOREAN_TUNING (1.0 / 8.0),
  PYTHAGOREAN_TUNING (1.0 / 4.0),       PYTHAGOREAN_TUNING (1.0 / 2.0),
  PYTHAGOREAN_TUNING (1.0),
  PYTHAGOREAN_TUNING (2.0),             PYTHAGOREAN_TUNING (4.0),               PYTHAGOREAN_TUNING (8.0),
  PYTHAGOREAN_TUNING (16.0),            PYTHAGOREAN_TUNING (32.0),              PYTHAGOREAN_TUNING (64.0),
  PYTHAGOREAN_TUNING (128.0),           PYTHAGOREAN_TUNING (256.0),             PYTHAGOREAN_TUNING (512.0),
  PYTHAGOREAN_TUNING (1024.0),          2048.0, /* 2^11 */
#undef PYTHAGOREAN_TUNING
};

// http://en.wikipedia.org/wiki/Pentatonic_scale
static const double semitone_table265_pentatonic_5_limit[132 + 1 + 132] = {
#define PENTATONIC_5_LIMIT(scale)                                       \
  SCALED_INTERVAL (scale, 1.0,                                          \
    256 / 243.0, 9 / 8.0, 32 / 27.0, 5 / 4.0, 4 / 3.0, 729 / 512.0,     \
    3 / 2.0, 128 / 81.0, 5 / 3.0, 16 / 9.0, 243 / 128.0)
  PENTATONIC_5_LIMIT (1.0 / 2048.0),    PENTATONIC_5_LIMIT (1.0 / 1024.0),      PENTATONIC_5_LIMIT (1.0 / 512.0),
  PENTATONIC_5_LIMIT (1.0 / 256.0),     PENTATONIC_5_LIMIT (1.0 / 128.0),       PENTATONIC_5_LIMIT (1.0 / 64.0),
  PENTATONIC_5_LIMIT (1.0 / 32.0),      PENTATONIC_5_LIMIT (1.0 / 16.0),        PENTATONIC_5_LIMIT (1.0 / 8.0),
  PENTATONIC_5_LIMIT (1.0 / 4.0),       PENTATONIC_5_LIMIT (1.0 / 2.0),
  PENTATONIC_5_LIMIT (1.0),
  PENTATONIC_5_LIMIT (2.0),             PENTATONIC_5_LIMIT (4.0),               PENTATONIC_5_LIMIT (8.0),
  PENTATONIC_5_LIMIT (16.0),            PENTATONIC_5_LIMIT (32.0),              PENTATONIC_5_LIMIT (64.0),
  PENTATONIC_5_LIMIT (128.0),           PENTATONIC_5_LIMIT (256.0),             PENTATONIC_5_LIMIT (512.0),
  PENTATONIC_5_LIMIT (1024.0),          2048.0, /* 2^11 */
#undef PENTATONIC_5_LIMIT
};

// http://en.wikipedia.org/wiki/Pentatonic_scale
static const double semitone_table265_pentatonic_blues[132 + 1 + 132] = {
#define PENTATONIC_BLUES(scale)                                         \
  SCALED_INTERVAL (scale, 1.0,                                          \
    256 / 243.0, 9 / 8.0, 7 / 6.0, 21 / 16.0, 4 / 3.0, 7 / 5.0,         \
    3 / 2.0, 128 / 81.0, 7 / 4.0, 16 / 9.0, 243 / 128.0)
  PENTATONIC_BLUES (1.0 / 2048.0),      PENTATONIC_BLUES (1.0 / 1024.0),        PENTATONIC_BLUES (1.0 / 512.0),
  PENTATONIC_BLUES (1.0 / 256.0),       PENTATONIC_BLUES (1.0 / 128.0),         PENTATONIC_BLUES (1.0 / 64.0),
  PENTATONIC_BLUES (1.0 / 32.0),        PENTATONIC_BLUES (1.0 / 16.0),          PENTATONIC_BLUES (1.0 / 8.0),
  PENTATONIC_BLUES (1.0 / 4.0),         PENTATONIC_BLUES (1.0 / 2.0),
  PENTATONIC_BLUES (1.0),
  PENTATONIC_BLUES (2.0),               PENTATONIC_BLUES (4.0),                 PENTATONIC_BLUES (8.0),
  PENTATONIC_BLUES (16.0),              PENTATONIC_BLUES (32.0),                PENTATONIC_BLUES (64.0),
  PENTATONIC_BLUES (128.0),             PENTATONIC_BLUES (256.0),               PENTATONIC_BLUES (512.0),
  PENTATONIC_BLUES (1024.0),            2048.0, /* 2^11 */
#undef PENTATONIC_BLUES
};

// http://en.wikipedia.org/wiki/Pentatonic_scale
static const double semitone_table265_pentatonic_gogo[132 + 1 + 132] = {
#define PENTATONIC_GOGO(scale)                                          \
  SCALED_INTERVAL (scale, 1.0,                                          \
    256 / 243.0, 9 / 8.0, 32 / 27.0, 5 / 4.0, 4 / 3.0, 729 / 512.0,     \
    3 / 2.0, 128 / 81.0, 7 / 4.0, 16 / 9.0, 243 / 128.0)
  PENTATONIC_GOGO (1.0 / 2048.0),       PENTATONIC_GOGO (1.0 / 1024.0),         PENTATONIC_GOGO (1.0 / 512.0),
  PENTATONIC_GOGO (1.0 / 256.0),        PENTATONIC_GOGO (1.0 / 128.0),          PENTATONIC_GOGO (1.0 / 64.0),
  PENTATONIC_GOGO (1.0 / 32.0),         PENTATONIC_GOGO (1.0 / 16.0),           PENTATONIC_GOGO (1.0 / 8.0),
  PENTATONIC_GOGO (1.0 / 4.0),          PENTATONIC_GOGO (1.0 / 2.0),
  PENTATONIC_GOGO (1.0),
  PENTATONIC_GOGO (2.0),                PENTATONIC_GOGO (4.0),                  PENTATONIC_GOGO (8.0),
  PENTATONIC_GOGO (16.0),               PENTATONIC_GOGO (32.0),                 PENTATONIC_GOGO (64.0),
  PENTATONIC_GOGO (128.0),              PENTATONIC_GOGO (256.0),                PENTATONIC_GOGO (512.0),
  PENTATONIC_GOGO (1024.0),             2048.0, /* 2^11 */
#undef PENTATONIC_GOGO
};

// http://en.wikipedia.org/wiki/Quarter-comma_meantone
static const double semitone_table265_quarter_comma_meantone[132 + 1 + 132] = {
#define QCOMMA_MEANTONE(scale)                                                          \
  SCALED_INTERVAL (scale, 1.0,                                                          \
                   1.0449067265256594125050516769666, /* (2187/128)/(81/80)^1.75/16 */  \
                   1.1180339887498948482045868343656, /* (9/4)/(81/80)^.5/2 */          \
                   1.1962790249769764335295191953127, /* (8/27)*(81/80)^.75*4 */        \
                   5 / 4.0,                           /* (81/16)/(81/80)/4 */           \
                   1.3374806099528440480064661465173, /* (2/3)*(81/80)^.25*2 */         \
                   1.3975424859373685602557335429570, /* (729/64)/(81/80)^1.5/8 */      \
                   1.4953487812212205419118989941409, /* (3/2)/(81/80)^.25 */           \
                   25 / 16.0,                                                           \
                   1.6718507624410550600080826831466, /* (27/8)/(81/80)^.75/2 */        \
                   1.7888543819998317571273389349850, /* (4/9)*(81/80)^.5*4 */          \
                   1.8691859765265256773898737426761) /* (243/32)/(81/80)^1.25/4 */
  QCOMMA_MEANTONE (1.0 / 2048.0),       QCOMMA_MEANTONE (1.0 / 1024.0),         QCOMMA_MEANTONE (1.0 / 512.0),
  QCOMMA_MEANTONE (1.0 / 256.0),        QCOMMA_MEANTONE (1.0 / 128.0),          QCOMMA_MEANTONE (1.0 / 64.0),
  QCOMMA_MEANTONE (1.0 / 32.0),         QCOMMA_MEANTONE (1.0 / 16.0),           QCOMMA_MEANTONE (1.0 / 8.0),
  QCOMMA_MEANTONE (1.0 / 4.0),          QCOMMA_MEANTONE (1.0 / 2.0),
  QCOMMA_MEANTONE (1.0),
  QCOMMA_MEANTONE (2.0),                QCOMMA_MEANTONE (4.0),                  QCOMMA_MEANTONE (8.0),
  QCOMMA_MEANTONE (16.0),               QCOMMA_MEANTONE (32.0),                 QCOMMA_MEANTONE (64.0),
  QCOMMA_MEANTONE (128.0),              QCOMMA_MEANTONE (256.0),                QCOMMA_MEANTONE (512.0),
  QCOMMA_MEANTONE (1024.0),             2048.0, /* 2^11 */
#undef QCOMMA_MEANTONE
};

// http://de.wikipedia.org/wiki/Silbermann-Sorge-Temperatur
static const double semitone_table265_silbermann_sorge_temperament[132 + 1 + 132] = {
#define SILBERMANN_SORGE(scale)                                                 \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   1.0509300646305402574490134498012, /* 2^(86/1200) */         \
                   1.1198716040467591250082361837530, /* 2^(196/1200) */        \
                   1.1933357430317218729952278581798, /* 2^(306/1200) */        \
                   1.2541124095502612486514063523921, /* 2^(392/1200) */        \
                   1.3363828127152655319782668003685, /* 2^(502/1200) */        \
                   1.4044448757379971820788342284695, /* 2^(588/1200) */        \
                   1.4965771640959640975810263196965, /* 2^(698/1200) */        \
                   1.5727979357879622015539138854402, /* 2^(784/1200) */        \
                   1.6759742693358971625849112463630, /* 2^(894/1200) */        \
                   1.7859190221207644704151890003616, /* 2^(1004/1200) */       \
                   1.8768759933422862606134778581222) /* 2^(1090/1200) */
  SILBERMANN_SORGE (1.0 / 2048.0),      SILBERMANN_SORGE (1.0 / 1024.0),        SILBERMANN_SORGE (1.0 / 512.0),
  SILBERMANN_SORGE (1.0 / 256.0),       SILBERMANN_SORGE (1.0 / 128.0),         SILBERMANN_SORGE (1.0 / 64.0),
  SILBERMANN_SORGE (1.0 / 32.0),        SILBERMANN_SORGE (1.0 / 16.0),          SILBERMANN_SORGE (1.0 / 8.0),
  SILBERMANN_SORGE (1.0 / 4.0),         SILBERMANN_SORGE (1.0 / 2.0),
  SILBERMANN_SORGE (1.0),
  SILBERMANN_SORGE (2.0),               SILBERMANN_SORGE (4.0),                 SILBERMANN_SORGE (8.0),
  SILBERMANN_SORGE (16.0),              SILBERMANN_SORGE (32.0),                SILBERMANN_SORGE (64.0),
  SILBERMANN_SORGE (128.0),             SILBERMANN_SORGE (256.0),               SILBERMANN_SORGE (512.0),
  SILBERMANN_SORGE (1024.0),            2048.0, /* 2^11 */
#undef SILBERMANN_SORGE
};

#define SQRT2_2 1.4142135623730950488016887242097       /* 2^0.5 */
#define SQRT4_2 1.1892071150027210667174999705605       /* 2^0.25 */
#define SQRT4_8 1.6817928305074290860622509524664       /* 8^0.25 */

// http://en.wikipedia.org/wiki/Werckmeister_temperament
static const double semitone_table265_werckmeister3_temperament[132 + 1 + 132] = {
#define WMEISTER3_TEMPERAMENT(scale)                                            \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   256 / 243.0, 64 * SQRT2_2 / 81.0, 32 / 27.0,                 \
                   256 * SQRT4_2 / 243.0, 4 / 3.0, 1024 / 729.0,                \
                   8 * SQRT4_8 / 9.0, 128 / 81.0, 1024 * SQRT4_2 / 729.0,       \
                   16 / 9.0, 128 * SQRT4_2 / 81.0)
  WMEISTER3_TEMPERAMENT (1.0 / 2048.0), WMEISTER3_TEMPERAMENT (1.0 / 1024.0),   WMEISTER3_TEMPERAMENT (1.0 / 512.0),
  WMEISTER3_TEMPERAMENT (1.0 / 256.0),  WMEISTER3_TEMPERAMENT (1.0 / 128.0),    WMEISTER3_TEMPERAMENT (1.0 / 64.0),
  WMEISTER3_TEMPERAMENT (1.0 / 32.0),   WMEISTER3_TEMPERAMENT (1.0 / 16.0),     WMEISTER3_TEMPERAMENT (1.0 / 8.0),
  WMEISTER3_TEMPERAMENT (1.0 / 4.0),    WMEISTER3_TEMPERAMENT (1.0 / 2.0),
  WMEISTER3_TEMPERAMENT (1.0),
  WMEISTER3_TEMPERAMENT (2.0),          WMEISTER3_TEMPERAMENT (4.0),            WMEISTER3_TEMPERAMENT (8.0),
  WMEISTER3_TEMPERAMENT (16.0),         WMEISTER3_TEMPERAMENT (32.0),           WMEISTER3_TEMPERAMENT (64.0),
  WMEISTER3_TEMPERAMENT (128.0),        WMEISTER3_TEMPERAMENT (256.0),          WMEISTER3_TEMPERAMENT (512.0),
  WMEISTER3_TEMPERAMENT (1024.0),       2048.0, /* 2^11 */
#undef WMEISTER3_TEMPERAMENT
};

#define SQRT3_2 1.2599210498948731647672106072782       /* 2^(1/3) */
#define SQRT3_4 1.5874010519681994747517056392723       /* 4^(1/3) */

// http://en.wikipedia.org/wiki/Werckmeister_temperament
static const double semitone_table265_werckmeister4_temperament[132 + 1 + 132] = {
#define WMEISTER4_TEMPERAMENT(scale)                                            \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   16384 * SQRT3_2 / 19683.0, 8 * SQRT3_2 / 9.0, 32 / 27.0,     \
                   64 * SQRT3_4 / 81.0, 4 / 3.0, 1024 / 729.0,                  \
                   32 * SQRT3_2 / 27.0, 8192 * SQRT3_2 / 6561.0,                \
                   256 * SQRT3_4 / 243.0, 9 / 8.0 * SQRT3_4, 4096 / 2187.0)
  WMEISTER4_TEMPERAMENT (1.0 / 2048.0), WMEISTER4_TEMPERAMENT (1.0 / 1024.0),   WMEISTER4_TEMPERAMENT (1.0 / 512.0),
  WMEISTER4_TEMPERAMENT (1.0 / 256.0),  WMEISTER4_TEMPERAMENT (1.0 / 128.0),    WMEISTER4_TEMPERAMENT (1.0 / 64.0),
  WMEISTER4_TEMPERAMENT (1.0 / 32.0),   WMEISTER4_TEMPERAMENT (1.0 / 16.0),     WMEISTER4_TEMPERAMENT (1.0 / 8.0),
  WMEISTER4_TEMPERAMENT (1.0 / 4.0),    WMEISTER4_TEMPERAMENT (1.0 / 2.0),
  WMEISTER4_TEMPERAMENT (1.0),
  WMEISTER4_TEMPERAMENT (2.0),          WMEISTER4_TEMPERAMENT (4.0),            WMEISTER4_TEMPERAMENT (8.0),
  WMEISTER4_TEMPERAMENT (16.0),         WMEISTER4_TEMPERAMENT (32.0),           WMEISTER4_TEMPERAMENT (64.0),
  WMEISTER4_TEMPERAMENT (128.0),        WMEISTER4_TEMPERAMENT (256.0),          WMEISTER4_TEMPERAMENT (512.0),
  WMEISTER4_TEMPERAMENT (1024.0),       2048.0, /* 2^11 */
#undef WMEISTER4_TEMPERAMENT
};

// http://en.wikipedia.org/wiki/Werckmeister_temperament
static const double semitone_table265_werckmeister5_temperament[132 + 1 + 132] = {
#define WMEISTER5_TEMPERAMENT(scale)                                            \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   8 * SQRT4_2 / 9.0, 9 / 8.0, SQRT4_2, 8 * SQRT2_2 / 9.0,      \
                   9 / 8.0 * SQRT4_2, SQRT2_2, 3 / 2.0, 128 / 81.0,             \
                   SQRT4_8, 3 / SQRT4_8, 4 * SQRT2_2 / 3.0)
  WMEISTER5_TEMPERAMENT (1.0 / 2048.0), WMEISTER5_TEMPERAMENT (1.0 / 1024.0),   WMEISTER5_TEMPERAMENT (1.0 / 512.0),
  WMEISTER5_TEMPERAMENT (1.0 / 256.0),  WMEISTER5_TEMPERAMENT (1.0 / 128.0),    WMEISTER5_TEMPERAMENT (1.0 / 64.0),
  WMEISTER5_TEMPERAMENT (1.0 / 32.0),   WMEISTER5_TEMPERAMENT (1.0 / 16.0),     WMEISTER5_TEMPERAMENT (1.0 / 8.0),
  WMEISTER5_TEMPERAMENT (1.0 / 4.0),    WMEISTER5_TEMPERAMENT (1.0 / 2.0),
  WMEISTER5_TEMPERAMENT (1.0),
  WMEISTER5_TEMPERAMENT (2.0),          WMEISTER5_TEMPERAMENT (4.0),            WMEISTER5_TEMPERAMENT (8.0),
  WMEISTER5_TEMPERAMENT (16.0),         WMEISTER5_TEMPERAMENT (32.0),           WMEISTER5_TEMPERAMENT (64.0),
  WMEISTER5_TEMPERAMENT (128.0),        WMEISTER5_TEMPERAMENT (256.0),          WMEISTER5_TEMPERAMENT (512.0),
  WMEISTER5_TEMPERAMENT (1024.0),       2048.0, /* 2^11 */
#undef WMEISTER5_TEMPERAMENT
};

// http://en.wikipedia.org/wiki/Werckmeister_temperament
static const double semitone_table265_werckmeister6_temperament[132 + 1 + 132] = {
#define WMEISTER6_TEMPERAMENT(scale)                                            \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   98 / 93.0, 28 / 25.0, 196 / 165.0, 49 / 39.0, 4 / 3.0,       \
                   196 / 139.0, 196 / 131.0, 49 / 31.0, 196 / 117.0,            \
                   98 / 55.0, 49 / 26.0)
  WMEISTER6_TEMPERAMENT (1.0 / 2048.0), WMEISTER6_TEMPERAMENT (1.0 / 1024.0),   WMEISTER6_TEMPERAMENT (1.0 / 512.0),
  WMEISTER6_TEMPERAMENT (1.0 / 256.0),  WMEISTER6_TEMPERAMENT (1.0 / 128.0),    WMEISTER6_TEMPERAMENT (1.0 / 64.0),
  WMEISTER6_TEMPERAMENT (1.0 / 32.0),   WMEISTER6_TEMPERAMENT (1.0 / 16.0),     WMEISTER6_TEMPERAMENT (1.0 / 8.0),
  WMEISTER6_TEMPERAMENT (1.0 / 4.0),    WMEISTER6_TEMPERAMENT (1.0 / 2.0),
  WMEISTER6_TEMPERAMENT (1.0),
  WMEISTER6_TEMPERAMENT (2.0),          WMEISTER6_TEMPERAMENT (4.0),            WMEISTER6_TEMPERAMENT (8.0),
  WMEISTER6_TEMPERAMENT (16.0),         WMEISTER6_TEMPERAMENT (32.0),           WMEISTER6_TEMPERAMENT (64.0),
  WMEISTER6_TEMPERAMENT (128.0),        WMEISTER6_TEMPERAMENT (256.0),          WMEISTER6_TEMPERAMENT (512.0),
  WMEISTER6_TEMPERAMENT (1024.0),       2048.0, /* 2^11 */
#undef WMEISTER6_TEMPERAMENT
};

// http://en.wikipedia.org/wiki/Johann_Philipp_Kirnberger_temperament
static const double semitone_table265_kirnberger_temperament[132 + 1 + 132] = {
#define KBERGER3_TEMPERAMENT(scale)                                             \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   25 / 24.0, 9 / 8.0, 6 / 5.0, 5 / 4.0,                        \
                   4 / 3.0, 45 / 32.0, 3 / 2.0, 25 / 16.0,                      \
                   5 / 3.0, 16 / 9.0, 15 / 8.0)
  KBERGER3_TEMPERAMENT (1.0 / 2048.0),  KBERGER3_TEMPERAMENT (1.0 / 1024.0),    KBERGER3_TEMPERAMENT (1.0 / 512.0),
  KBERGER3_TEMPERAMENT (1.0 / 256.0),   KBERGER3_TEMPERAMENT (1.0 / 128.0),     KBERGER3_TEMPERAMENT (1.0 / 64.0),
  KBERGER3_TEMPERAMENT (1.0 / 32.0),    KBERGER3_TEMPERAMENT (1.0 / 16.0),      KBERGER3_TEMPERAMENT (1.0 / 8.0),
  KBERGER3_TEMPERAMENT (1.0 / 4.0),     KBERGER3_TEMPERAMENT (1.0 / 2.0),
  KBERGER3_TEMPERAMENT (1.0),
  KBERGER3_TEMPERAMENT (2.0),           KBERGER3_TEMPERAMENT (4.0),             KBERGER3_TEMPERAMENT (8.0),
  KBERGER3_TEMPERAMENT (16.0),          KBERGER3_TEMPERAMENT (32.0),            KBERGER3_TEMPERAMENT (64.0),
  KBERGER3_TEMPERAMENT (128.0),         KBERGER3_TEMPERAMENT (256.0),           KBERGER3_TEMPERAMENT (512.0),
  KBERGER3_TEMPERAMENT (1024.0),        2048.0, /* 2^11 */
#undef KBERGER3_TEMPERAMENT
};

// http://en.wikipedia.org/wiki/Young_temperament
static const double semitone_table265_young_temperament[132 + 1 + 132] = {
#define YOUNG_TEMPERAMENT(scale)                                                \
  SCALED_INTERVAL (scale, 1.0, /* 2^(0/1200) */                                 \
                   1.0631412837780103584827404056079, /* 2^(106/1200) */        \
                   1.1211660780285088308680165651114, /* 2^(198/1200) */        \
                   1.1933357430317218729952278581798, /* 2^(306/1200) */        \
                   1.2599210498948731647672106072782, /* 2^(400/1200) */        \
                   1.3363828127152655319782668003685, /* 2^(502/1200) */        \
                   1.4174848672222598142134666752417, /* 2^(604/1200) */        \
                   1.4965771640959640975810263196965, /* 2^(698/1200) */        \
                   1.5929121092043532274138841376768, /* 2^(806/1200) */        \
                   1.6798510690641884687069808937168, /* 2^(898/1200) */        \
                   1.7859190221207644704151890003616, /* 2^(1004/1200) */       \
                   1.8899306982642323704825496345032) /* 2^(1102/1200) */
  YOUNG_TEMPERAMENT (1.0 / 2048.0),     YOUNG_TEMPERAMENT (1.0 / 1024.0),       YOUNG_TEMPERAMENT (1.0 / 512.0),
  YOUNG_TEMPERAMENT (1.0 / 256.0),      YOUNG_TEMPERAMENT (1.0 / 128.0),        YOUNG_TEMPERAMENT (1.0 / 64.0),
  YOUNG_TEMPERAMENT (1.0 / 32.0),       YOUNG_TEMPERAMENT (1.0 / 16.0),         YOUNG_TEMPERAMENT (1.0 / 8.0),
  YOUNG_TEMPERAMENT (1.0 / 4.0),        YOUNG_TEMPERAMENT (1.0 / 2.0),
  YOUNG_TEMPERAMENT (1.0),
  YOUNG_TEMPERAMENT (2.0),              YOUNG_TEMPERAMENT (4.0),                YOUNG_TEMPERAMENT (8.0),
  YOUNG_TEMPERAMENT (16.0),             YOUNG_TEMPERAMENT (32.0),               YOUNG_TEMPERAMENT (64.0),
  YOUNG_TEMPERAMENT (128.0),            YOUNG_TEMPERAMENT (256.0),              YOUNG_TEMPERAMENT (512.0),
  YOUNG_TEMPERAMENT (1024.0),           2048.0, /* 2^11 */
#undef YOUNG_TEMPERAMENT
};

const double*
bse_semitone_table_from_tuning (BseMusicalTuningType musical_tuning)
{
  switch (musical_tuning)
    {
      /* Equal Temperament: http://en.wikipedia.org/wiki/Equal_temperament */
    default:
    case BSE_MUSICAL_TUNING_12_TET:
      return 132 + semitone_table265_equal_temperament_12_tet;
    case BSE_MUSICAL_TUNING_7_TET:
      return 132 + semitone_table265_equal_temperament_7_tet;
    case BSE_MUSICAL_TUNING_5_TET:
      return 132 + semitone_table265_equal_temperament_5_tet;
      /* Rational Intonation: http://en.wikipedia.org/wiki/Just_intonation */
    case BSE_MUSICAL_TUNING_DIATONIC_SCALE:
      return 132 + semitone_table265_diatonic_scale;
    case BSE_MUSICAL_TUNING_INDIAN_SCALE:
      return 132 + semitone_table265_indian_scale;
    case BSE_MUSICAL_TUNING_PYTHAGOREAN_TUNING:
      return 132 + semitone_table265_pythagorean_tuning;
    case BSE_MUSICAL_TUNING_PENTATONIC_5_LIMIT:
      return 132 + semitone_table265_pentatonic_5_limit;
    case BSE_MUSICAL_TUNING_PENTATONIC_BLUES:
      return 132 + semitone_table265_pentatonic_blues;
    case BSE_MUSICAL_TUNING_PENTATONIC_GOGO:
      return 132 + semitone_table265_pentatonic_gogo;
      /* Meantone Temperament: http://en.wikipedia.org/wiki/Meantone_temperament */
    case BSE_MUSICAL_TUNING_QUARTER_COMMA_MEANTONE:
      return 132 + semitone_table265_quarter_comma_meantone;
    case BSE_MUSICAL_TUNING_SILBERMANN_SORGE:
      return 132 + semitone_table265_silbermann_sorge_temperament;
      /* Well Temperament: http://en.wikipedia.org/wiki/Well_temperament */
    case BSE_MUSICAL_TUNING_WERCKMEISTER_3:
      return 132 + semitone_table265_werckmeister3_temperament;
    case BSE_MUSICAL_TUNING_WERCKMEISTER_4:
      return 132 + semitone_table265_werckmeister4_temperament;
    case BSE_MUSICAL_TUNING_WERCKMEISTER_5:
      return 132 + semitone_table265_werckmeister5_temperament;
    case BSE_MUSICAL_TUNING_WERCKMEISTER_6:
      return 132 + semitone_table265_werckmeister6_temperament;
    case BSE_MUSICAL_TUNING_KIRNBERGER_3:
      return 132 + semitone_table265_kirnberger_temperament;
    case BSE_MUSICAL_TUNING_YOUNG:
      return 132 + semitone_table265_young_temperament;
    }
}

double
bse_transpose_factor (BseMusicalTuningType musical_tuning,
                      int                  index /* [-132..+132] */)
{
  const double *table = bse_semitone_table_from_tuning (musical_tuning);
  return table[CLAMP (index, -132, +132)];
}

/* --- cents & init --- */
void
_bse_init_signal (void) { /* FIXME: remove */ }

/* --- bse_approx_atan1() --- */
double
bse_approx_atan1_prescale (double boost_amount)
{
  double max_boost_factor = 100;	/* atan1(x*100) gets pretty close to 1 for x=1 */
  double recip_tan_1_div_0_75 = 0.24202942695518667705824990442766; /* 1/tan(1/0.75) */
  double scale;

  g_return_val_if_fail (boost_amount >= 0 && boost_amount <= 1.0, 1.0);

  /* scale boost_amount from [0..1] to -1..1 */
  boost_amount = boost_amount * 2 - 1.0;

  /* prescale factor for atan1(x*prescale), ranges from 1/max_boost_factor..max_boost_factor */
  scale = pow (max_boost_factor, tan (boost_amount / 0.75) * recip_tan_1_div_0_75);

  /* atan1_prescale(ba)=100 ** (tan ((ba*2-1) / 0.75) * 0.24202942695518667705824990442766) */

  return scale;
}


/* --- exp2f() approximation taylor coefficients finder --- */
#if 0
#include <stdio.h>
double
exp2coeff (int n)
{
  double r = 1;
  int i;

  for (i = 1; i <= n; i++)
    {
      r *= BSE_LN2;
      r /= i;
    }
  return r;
}
/* generate taylor coefficients */
int
main (int   argc,
      char *argv[])
{
  int i;

  for (i = 0; i < 20; i++)
    printf ("#define EXP2_TAYLOR_COEFF_%u\t(%.40f)\n", i, exp2coeff (i));

  return 0;
}
/* test/bench program */
#define _GNU_SOURCE
#include <math.h> /* for main() in testprogram */
int
main (int   argc,
      char *argv[])
{
  double x, dummy = 0, l = 4;

  if (1)	/* print errors */
    for (x = -3; x < 3.01; x += 0.1)
      {
	g_print ("%+f %+1.20f \t (%.20f - %.20f)\n",
		 x, exp (x * BSE_LN2) - bse_approx5_exp2 (x),
		 exp (x * BSE_LN2), bse_approx5_exp2 (x));
      }

  if (0)	/* bench test */
    for (x = -l; x < l; x += 0.000001)
      {
	dummy += bse_approx5_exp2 (x);
	// dummy += exp2f (x);
      }

  g_print ("%f\r                            \n", dummy);

  return 0;
}
#endif  /* coeff generation */

