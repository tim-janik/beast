#ifndef	__BSE_MIX_RAMP_AUX__
#define	__BSE_MIX_RAMP_AUX__
typedef enum
{
  BSE_MIX_RAMP_REACHED_BOUND,
  BSE_MIX_RAMP_REACHED_BORDER,
  BSE_MIX_RAMP_GATE_LOW,
  BSE_MIX_RAMP_RETRIGGER
} BseMixRampState;
typedef struct
{
  BseSampleValue       *wave_out;
  BseSampleValue       *bound;
  const BseSampleValue *gate_in;
  const BseSampleValue *trig_in;
  BseSampleValue	last_trigger;
  BseSampleValue	level;
  BseSampleValue	level_step;
  BseSampleValue	level_border;
} BseMixRampLinear;
#define	BSE_MIX_RAMP_WITH_GATE	(1)
#define	BSE_MIX_RAMP_WITH_IGATE	(2)
#define	BSE_MIX_RAMP_WITH_TRIG	(4)
#define	BSE_MIX_RAMP_WITH_INC	(8)
#define	BSE_MIX_RAMP_WITH_DEC	(16)
#endif	/* __BSE_MIX_RAMP_AUX__ */

#define	CHECK_GATE		(BSE_MIX_VARIANT & (BSE_MIX_RAMP_WITH_GATE | BSE_MIX_RAMP_WITH_IGATE))
#define	CHECK_TRIG		(BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_TRIG)
#define	STEP_UP			(BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_INC)
#define	STEP_DOWN		(BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_DEC)
#define GATE_LOW(v)		((BSE_MIX_VARIANT & BSE_MIX_RAMP_WITH_IGATE) ? (v) >= 0.5 : (v) < 0.5)
#define RAISING_EDGE(v1,v2)	((v1) < (v2))

static inline BseMixRampState
BSE_MIX_VARIANT_NAME (BseMixRampLinear *ramp)
{
#define	RETURN(cause)	{ \
    ramp->wave_out = wave_out; \
    if (CHECK_GATE) ramp->gate_in = gate_in; \
    if (CHECK_TRIG) { ramp->trig_in = trig_in; ramp->last_trigger = last_trig; } \
    if (STEP_UP || STEP_DOWN) ramp->level = value; \
    return (cause); \
  }
  register BseSampleValue *wave_out = ramp->wave_out;
  const BseSampleValue *bound = ramp->bound;
  const BseSampleValue *gate_in = ramp->gate_in;
  const BseSampleValue *trig_in = ramp->trig_in;
  register BseSampleValue value = ramp->level;
  BseSampleValue level_step = ramp->level_step;
  BseSampleValue last_trig = ramp->last_trigger;
  BseSampleValue eps = STEP_DOWN ? ramp->level_border + BSE_EPSILON : ramp->level_border - BSE_EPSILON;
  
  if (wave_out >= bound)
    return BSE_MIX_RAMP_REACHED_BOUND;
  do
    {
      if (CHECK_GATE && GATE_LOW (*gate_in))
	RETURN (BSE_MIX_RAMP_GATE_LOW);
      if (CHECK_TRIG)
	{
	  BseSampleValue trig = *trig_in;

	  if (RAISING_EDGE (last_trig, trig))
	    {
	      last_trig = trig;
	      RETURN (BSE_MIX_RAMP_RETRIGGER);
	    }
	  last_trig = trig;
	}
      gate_in++;
      trig_in++;
      if (STEP_UP)
	value += level_step;
      if (STEP_DOWN)
	value -= level_step;
      if ((STEP_UP && value >= eps) || (STEP_DOWN && value <= eps))
	{
	  value = ramp->level_border;
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
#undef	GATE_LOW
#undef	RAISING_EDGE
