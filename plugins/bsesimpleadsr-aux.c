/* BseSimpleADSR - BSE Simpl ADSR Envelope Generator
 * Copyright (C) 2001-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library Simpleeral Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Simpleeral Public License for more details.
 *
 * You should have received a copy of the GNU Library Simpleeral Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef	__BSE_MIX_RAMP_AUX__
#define	__BSE_MIX_RAMP_AUX__
typedef enum
{
  BSE_MIX_RAMP_REACHED_BOUND,
  BSE_MIX_RAMP_REACHED_BORDER,
  BSE_MIX_RAMP_GATE_CHANGE,
  BSE_MIX_RAMP_RETRIGGER
} BseMixRampState;
typedef struct
{
  gfloat       *wave_out;
  gfloat       *bound;
  const gfloat *gate_in;
  const gfloat *trig_in;
  gfloat	last_trigger;
  gfloat	level;
  gfloat	level_step;
  gfloat	level_border;
} BseMixRampLinear;
#define	BSE_MIX_RAMP_WITH_GATE	(1)
#define	BSE_MIX_RAMP_WITH_IGATE	(2)
#define	BSE_MIX_RAMP_WITH_TRIG	(4)
#define	BSE_MIX_RAMP_WITH_INC	(8)
#define	BSE_MIX_RAMP_WITH_DEC	(16)
#define BSE_MIX_EPSILON         (1e-8 /* threshold, coined for 24 bit */)
#endif	/* __BSE_MIX_RAMP_AUX__ */


#define	CHECK_GATE		(BSE_MIX_VARIANT & (BSE_MIX_RAMP_WITH_GATE | BSE_MIX_RAMP_WITH_IGATE))
#define	CHECK_TRIG		(BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_TRIG)
#define	STEP_UP			(BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_INC)
#define	STEP_DOWN		(BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_DEC)
#define GATE_CHECK(v)		((BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_IGATE) ? (v) >= 0.5 : (v) < 0.5)
#define RAISING_EDGE(v1,v2)	((v1) < (v2))

static inline BseMixRampState
BSE_MIX_VARIANT_NAME (BseMixRampLinear *ramp)
{
#define	RETURN(cause)	{ \
    ramp->wave_out = wave_out; \
    if (STEP_UP || STEP_DOWN) ramp->level = value; \
    return (cause); \
  }
  register gfloat *wave_out = ramp->wave_out;
  const gfloat *bound = ramp->bound;
  register gfloat value = ramp->level;
  gfloat level_step = ramp->level_step;
  gfloat eps = STEP_DOWN ? ramp->level_border + BSE_MIX_EPSILON : ramp->level_border - BSE_MIX_EPSILON;
  
  if (wave_out >= bound)
    return BSE_MIX_RAMP_REACHED_BOUND;
  do
    {
      if (CHECK_GATE && GATE_CHECK (*ramp->gate_in))
	RETURN (BSE_MIX_RAMP_GATE_CHANGE);
      if (CHECK_TRIG)
	{
	  gfloat trig_val = *ramp->trig_in;

	  if (RAISING_EDGE (ramp->last_trigger, trig_val))
	    {
	      ramp->last_trigger = trig_val;
	      RETURN (BSE_MIX_RAMP_RETRIGGER);
	    }
	  ramp->last_trigger = trig_val;
	}
      if (CHECK_GATE)
	ramp->gate_in++;
      if (CHECK_TRIG)
	ramp->trig_in++;
      if (STEP_UP)
	value += level_step;
      if (STEP_DOWN)
	value -= level_step;
      if ((STEP_UP && value >= eps) || (STEP_DOWN && value <= eps))
	{
	  value = ramp->level_border; /* prevent overshoot */
	  *wave_out++ = value;
	  RETURN (BSE_MIX_RAMP_REACHED_BORDER);
	}
      *wave_out++ = value;
    }
  while (wave_out < bound);
  RETURN (BSE_MIX_RAMP_REACHED_BOUND);
#undef	RETURN
}

#undef	CHECK_GATE
#undef	CHECK_TRIG
#undef	STEP_UP
#undef	STEP_DOWN
#undef	GATE_CHANGE
#undef	RAISING_EDGE
#undef  BSE_MIX_VARIANT
#undef  BSE_MIX_VARIANT_NAME
