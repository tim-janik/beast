/* BseAmplifier - BSE Amplifier
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#define AMP_FLAGS	(GSL_INCLUDER_CASE | AMP_INCLUDER_FLAGS)
#define	WITH_MASTER	(AMP_FLAGS & AMP_FLAG_MASTER)
#define	AU1b_AU2b	((AMP_FLAGS & AMP_FLAGS_AUDIO) == AMP_FLAGS_A1b_A2b)
#define	AU1y_AU2n	((AMP_FLAGS & AMP_FLAGS_AUDIO) == AMP_FLAGS_A1y_A2n)
#define	AU1n_AU2y	((AMP_FLAGS & AMP_FLAGS_AUDIO) == AMP_FLAGS_A1n_A2y)
#define	SIMPLE_CTRL	(AMP_FLAGS & AMP_FLAG_SIMPLE_CONTROL)
#define	CV1b_CV2b	((AMP_FLAGS & AMP_FLAGS_CONTROL) == AMP_FLAGS_C1b_C2b)
#define	CV1y_CV2n	((AMP_FLAGS & AMP_FLAGS_CONTROL) == AMP_FLAGS_C1y_C2n)
#define	CV1n_CV2y	((AMP_FLAGS & AMP_FLAGS_CONTROL) == AMP_FLAGS_C1n_C2y)
#define	CV1m_CV2m	((AMP_FLAGS & AMP_FLAGS_CONTROL) == AMP_FLAGS_C1m_C2m)
#define	WITH_EXP_CTRL	(AMP_FLAGS & AMP_FLAG_EXP_CONTROLS)

static void
GSL_INCLUDER_FUNC (Amplifier	*amplifier,
		   guint         n_values,
		   const gfloat *cv1in,
		   const gfloat *cv2in,
		   const gfloat *au1in,
		   const gfloat *au2in,
		   gfloat	*audio_out)
{
  gfloat *audio_bound = audio_out + n_values;
  gfloat cv1balance, cv2balance, cstrength, pregain, au1balance, au2balance, mgain;

  cv1balance = 1.0 - amplifier->config.ctrl_balance;
  cv2balance = amplifier->config.ctrl_balance;
  cstrength = amplifier->config.ctrl_strength;
  pregain = amplifier->config.audio_gain;
  au1balance = 1.0 - amplifier->config.audio_balance;
  au2balance = amplifier->config.audio_balance;
  mgain = amplifier->config.master_gain;

  do
    {
      gfloat cv_sum, au_out;

#if   SIMPLE_CTRL		/* no control inputs */
      cv_sum = pregain;
#else
#if   CV1b_CV2b			/* control input, cv1 and/or cv2 */
      cv_sum = cv1balance * *cv1in++ + cv2balance * *cv2in++;
#elif CV1m_CV2m
      {
	gfloat c1 = *cv1in++, c2 = *cv2in++;
	cv_sum = c1 > 0 && c2 > 0 ? c1 * c2 : 0;
      }
#elif CV1y_CV2n
      cv_sum = *cv1in++;
#elif CV1n_CV2y
      cv_sum = *cv2in++;
#endif
      if_reject (cv_sum < 0)
	cv_sum = 0;
      else
	cv_sum *= cstrength;
#if   WITH_EXP_CTRL		/* exponential controls */
      cv_sum = gsl_approx_qcircle2 (cv_sum);
#endif
      cv_sum += pregain;
      if_reject (cv_sum > 1.0)
	cv_sum = 1.0;
#endif

#if   AU1b_AU2b			/* audio input, au1 and/or au2 */
      au_out = au1balance * *au1in++ + au2balance * *au2in++;
#elif AU1y_AU2n
      au_out = *au1in++;
#elif AU1n_AU2y
      au_out = *au2in++;
#endif

      au_out = cv_sum * au_out;

#if   WITH_MASTER
      au_out *= mgain;
#endif
      *audio_out++ = au_out;
    }
  while (audio_out < audio_bound);
}

#undef	WITH_EXP_CTRL
#undef	CV1n_CV2y
#undef	CV1y_CV2n
#undef	CV1b_CV2b
#undef	SIMPLE_CTRL
#undef	AU1n_AU2y
#undef	AU1y_AU2n
#undef	AU1b_AU2b
#undef	WITH_MASTER
#undef	AMP_FLAGS
