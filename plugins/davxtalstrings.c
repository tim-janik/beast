/* DavXtalStrings - DAV Physical Modelling String Synthesizer
 * Copyright (c) 2000 David A. Bartold, 2001 Tim Janik
 *
 * Generate a string pluck sound using a modified Karplus-Strong algorithm
 * and then use Brensenham's algorithm to correct the frequency.
 *
 * This software uses technology under patent US 4,649,783, which is
 * set to expire in May of 2004.  In the meantime, a non-patented
 * alternative to this module is in the works.
 *
 ***********************************************************************
 *
 * Any commercial use of this module requires a license from
 * Stanford University until 2004.
 *
 ***********************************************************************
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "davxtalstrings.h"

#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>

#include <string.h>

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
  PARAM_TRANSPOSE,
  PARAM_FINE_TUNE,
  PARAM_TRIGGER_VEL,
  PARAM_TRIGGER_HIT,
  PARAM_NOTE_DECAY,
  PARAM_TENSION_DECAY,
  PARAM_METALLIC_FACTOR,
  PARAM_SNAP_FACTOR
};


/* --- prototypes --- */
static void	   dav_xtal_strings_init	     (DavXtalStrings	   *self);
static void	   dav_xtal_strings_class_init	     (DavXtalStringsClass  *class);
static void	   dav_xtal_strings_set_property     (GObject              *object,
						      guint                 param_id,
						      const GValue         *value,
						      GParamSpec           *pspec);
static void	   dav_xtal_strings_get_property     (GObject              *object,
						      guint                 param_id,
						      GValue               *value,
						      GParamSpec           *pspec);
static void        dav_xtal_prepare                  (BseSource            *source);
static void	   dav_xtal_strings_context_create   (BseSource		   *source,
						      guint		    context_handle,
						      BseTrans		   *trans);
static void	   dav_xtal_strings_update_modules   (DavXtalStrings	   *self,
						      gboolean		    trigger_now);


/* --- Export to BSE --- */
#include "./icons/strings.c"
BSE_REGISTER_OBJECT (DavXtalStrings, BseSource, "/Modules/Audio Sources/XtalStrings", "",
                     "DavXtalStrings is a plucked string synthesizer, using the "
                     "Karplus-Strong Algorithm. Commercial use of this module "
                     "until 2004 requires a license from Stanford University.",
                     strings_icon,
                     dav_xtal_strings_class_init, NULL, dav_xtal_strings_init);
BSE_DEFINE_EXPORTS ();


/* --- variables --- */
static gpointer	       parent_class = NULL;


/* --- functions --- */
static void
dav_xtal_strings_class_init (DavXtalStringsClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = dav_xtal_strings_set_property;
  gobject_class->get_property = dav_xtal_strings_get_property;
  
  source_class->prepare = dav_xtal_prepare;
  source_class->context_create = dav_xtal_strings_context_create;
  
  bse_object_class_add_param (object_class, _("Frequency"),
			      PARAM_BASE_FREQ,
			      bse_param_spec_freq ("base_freq", _("Frequency"),
                                                   _("String oscillation frequency in Herz"),
						   BSE_KAMMER_FREQUENCY, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY,
						   SFI_PARAM_STANDARD ":dial"));
  bse_object_class_add_param (object_class, _("Frequency"),
			      PARAM_BASE_NOTE,
			      bse_pspec_note_simple ("base_note", _("Note"),
                                                     _("String oscillation frequency as note, converted to Herz according to the current musical tuning"),
                                                     SFI_PARAM_GUI));
  bse_object_class_add_param (object_class, _("Frequency"),
			      PARAM_TRANSPOSE,
			      sfi_pspec_int ("transpose", _("Transpose"), _("Transposition of the frequency in semitones"),
					     0, BSE_MIN_TRANSPOSE, BSE_MAX_TRANSPOSE, 12,
					     SFI_PARAM_STANDARD ":f:dial:skip-default"));
  bse_object_class_add_param (object_class, _("Frequency"),
			      PARAM_FINE_TUNE,
			      sfi_pspec_int ("fine_tune", _("Fine Tune"), _("Amount of detuning in cent (hundredth part of a semitone)"),
					     0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10,
					     SFI_PARAM_STANDARD ":f:dial:skip-default"));
  bse_object_class_add_param (object_class, _("Trigger"), PARAM_TRIGGER_VEL,
			      sfi_pspec_real ("trigger_vel", _("Trigger Velocity [%]"),
                                              _("Velocity of the string pluck"),
                                              100.0,  0.0, 100.0, 1,
                                              SFI_PARAM_GUI ":scale"));
  bse_object_class_add_param (object_class, _("Trigger"), PARAM_TRIGGER_HIT,
			      sfi_pspec_bool ("trigger_pulse", _("Trigger Hit"), _("Pluck the string"),
                                              FALSE, SFI_PARAM_GUI ":trigger:skip-undo"));
  bse_object_class_add_param (object_class, _("Decay"), PARAM_NOTE_DECAY,
			      sfi_pspec_real ("note_decay", _("Note Decay"),
                                              _("Note decay is the 'half-life' of the note's decay in seconds"),
                                              0.4, 0.001, 4.0, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Decay"), PARAM_TENSION_DECAY,
			      sfi_pspec_real ("tension_decay", _("Tension Decay"),
                                              _("Tension of the string"),
                                              0.04, 0.001, 1.0, 0.01,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Flavour"), PARAM_METALLIC_FACTOR,
			      sfi_pspec_real ("metallic_factor", _("Metallic Factor [%]"),
                                              _("Metallicness of the string"),
                                              16.0, 0.0, 100.0, 1,
                                              SFI_PARAM_STANDARD ":scale"));
  bse_object_class_add_param (object_class, _("Flavour"), PARAM_SNAP_FACTOR,
			      sfi_pspec_real ("snap_factor", _("Snap Factor [%]"),
                                              _("Snappiness of the string"),
                                              34.0, 0.0, 100.0, 1,
                                              SFI_PARAM_STANDARD ":scale"));
  
  channel_id = bse_source_class_add_ichannel (source_class, "freq-in", _("Freq In"), _("Pluck frequency input"));
  g_assert (channel_id == DAV_XTAL_STRINGS_ICHANNEL_FREQ);
  channel_id = bse_source_class_add_ichannel (source_class, "trigger-in", _("Trigger In"), _("Pluck strings on raising edge"));
  g_assert (channel_id == DAV_XTAL_STRINGS_ICHANNEL_TRIGGER);
  channel_id = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("XtalStrings Output"));
  g_assert (channel_id == DAV_XTAL_STRINGS_OCHANNEL_MONO);
}

static void
dav_xtal_strings_init (DavXtalStrings *self)
{
  self->params.freq = BSE_KAMMER_FREQUENCY;
  self->params.transpose_factor = 0.0; // updated when prepared
  self->params.trigger_vel = 1.0;
  self->params.note_decay = 0.4;
  self->params.tension_decay = 0.04;
  self->params.metallic_factor = 0.16;
  self->params.snap_factor = 0.34;
#if 0
  self->string = NULL;
  self->size = 1;
  self->pos = 0;
  self->count = 0;
  
  self->a = 0.0;
  self->damping_factor = 0.0;
#endif
}

static void
dav_xtal_strings_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  DavXtalStrings *self = DAV_XTAL_STRINGS (object);
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      self->params.freq = sfi_value_get_real (value);
      dav_xtal_strings_update_modules (self, FALSE);
      g_object_notify (G_OBJECT (self), "base_note");
      break;
    case PARAM_BASE_NOTE:
      self->params.freq = bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), sfi_value_get_note (value));
      dav_xtal_strings_update_modules (self, FALSE);
      g_object_notify (G_OBJECT (self), "base_freq");
      break;
    case PARAM_TRANSPOSE:
      self->transpose = sfi_value_get_int (value);
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    case PARAM_FINE_TUNE:
      self->params.fine_tune = sfi_value_get_int (value);
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    case PARAM_TRIGGER_HIT:	/* GUI hack */
      dav_xtal_strings_update_modules (self, TRUE);
      break;
    case PARAM_TRIGGER_VEL:
      self->params.trigger_vel = sfi_value_get_real (value) / 100.0;
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    case PARAM_NOTE_DECAY:
      self->params.note_decay = sfi_value_get_real (value);
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    case PARAM_TENSION_DECAY:
      self->params.tension_decay = sfi_value_get_real (value);
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    case PARAM_METALLIC_FACTOR:
      self->params.metallic_factor = sfi_value_get_real (value) / 100.0;
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    case PARAM_SNAP_FACTOR:
      self->params.snap_factor = sfi_value_get_real (value) / 100.0;
      dav_xtal_strings_update_modules (self, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_xtal_strings_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  DavXtalStrings *self = DAV_XTAL_STRINGS (object);
  switch (param_id)
    {
    case PARAM_BASE_FREQ:
      sfi_value_set_real (value, self->params.freq);
      break;
    case PARAM_BASE_NOTE:
      sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), self->params.freq));
      break;
    case PARAM_TRANSPOSE:
      sfi_value_set_int (value, self->transpose);
      break;
    case PARAM_FINE_TUNE:
      sfi_value_set_int (value, self->params.fine_tune);
      break;
    case PARAM_TRIGGER_HIT:
      sfi_value_set_bool (value, FALSE);
      break;
    case PARAM_TRIGGER_VEL:
      sfi_value_set_real (value, self->params.trigger_vel * 100.0);
      break;
    case PARAM_NOTE_DECAY:
      sfi_value_set_real (value, self->params.note_decay);
      break;
    case PARAM_TENSION_DECAY:
      sfi_value_set_real (value, self->params.tension_decay);
      break;
    case PARAM_METALLIC_FACTOR:
      sfi_value_set_real (value, self->params.metallic_factor * 100.0);
      break;
    case PARAM_SNAP_FACTOR:
      sfi_value_set_real (value, self->params.snap_factor * 100.0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
dav_xtal_prepare (BseSource *source)
{
  DavXtalStrings *self = DAV_XTAL_STRINGS (source);
  self->params.transpose_factor = bse_transpose_factor (bse_source_prepared_musical_tuning (source), self->transpose);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static gfloat inline
calc_factor (gfloat freq,
	     gfloat t)
{
  return pow (0.5, 1.0 / (freq * t));
}

/* the GSL engine module that generates the signal, there may be many
 * modules per DavXtalStrings object
 */
typedef struct {
  gfloat               a, d;
  gfloat               damping_factor;
  gint                 pos, size;
  guint	               count;
  gfloat              *string;
  gfloat	       last_trigger_level;
  gdouble	       last_transposed_trigger_freq;
  DavXtalStringsParams tparams;
} XtalStringsModule;

/* trigger a XtalStringsModule by altering its state.
 */
static inline void
xmod_trigger (XtalStringsModule *xmod,
	      gdouble            untransposed_trigger_freq)
{
  guint i;
  guint pivot;

  /* calculate transposed trigger frequency; the "real frequency" that will
   * be played, including detuning and transpose operations
   */
  gdouble trigger_freq = untransposed_trigger_freq;
  trigger_freq *= xmod->tparams.transpose_factor;
  trigger_freq *= bse_cent_factor (xmod->tparams.fine_tune);
  trigger_freq = CLAMP (trigger_freq, 27.5, 4000.0);
  xmod->last_transposed_trigger_freq = trigger_freq;
  
  xmod->pos = 0;
  xmod->count = 0;
  xmod->size = (int) ((bse_engine_sample_freq() + trigger_freq - 1) / trigger_freq);
  
  xmod->a = calc_factor (trigger_freq, xmod->tparams.tension_decay);
  xmod->damping_factor = calc_factor (trigger_freq, xmod->tparams.note_decay);
  
  /* Create envelope. */
  pivot = xmod->size / 5;
  for (i = 0; i <= pivot; i++)
    xmod->string[i] = ((float) i) / pivot;
  for (; i < xmod->size; i++)
    xmod->string[i] = ((float) (xmod->size - i - 1)) / (xmod->size - pivot - 1);
  
  /* Add some snap. */
  for (i = 0; i < xmod->size; i++)
    xmod->string[i] = pow (xmod->string[i], xmod->tparams.snap_factor * 10.0 + 1.0);
  
  /* Add static to displacements. */
  for (i = 0; i < xmod->size; i++)
    xmod->string[i] = (xmod->string[i] * (1.0F - xmod->tparams.metallic_factor) +
		       (bse_rand_bool () ? -1.0F : 1.0F) * xmod->tparams.metallic_factor);
  /* Set velocity. */
  for (i = 0; i < xmod->size; i++)
    xmod->string[i] *= xmod->tparams.trigger_vel;
}

static void
xmod_process (BseModule *module,
	      guint      n_values)
{
  XtalStringsModule *xmod = module->user_data;
  const gfloat *freq_in, *trigger_in = BSE_MODULE_IBUFFER (module, DAV_XTAL_STRINGS_ICHANNEL_TRIGGER);
  gfloat *wave_out = BSE_MODULE_OBUFFER (module, DAV_XTAL_STRINGS_OCHANNEL_MONO);
  gint32 pos2;
  gfloat sample, last_trigger_level;
  guint real_freq_256, actual_freq_256;
  guint i;
  
  real_freq_256 = (int) (xmod->last_transposed_trigger_freq * 256);
  actual_freq_256 = (int) (bse_engine_sample_freq() * 256. / xmod->size);
  
  if (BSE_MODULE_ISTREAM (module, DAV_XTAL_STRINGS_ICHANNEL_FREQ).connected)
    freq_in = BSE_MODULE_IBUFFER (module, DAV_XTAL_STRINGS_ICHANNEL_FREQ);
  else
    freq_in = NULL;
  last_trigger_level = xmod->last_trigger_level;
  for (i = 0; i < n_values; i++)
    {
      /* check input triggers */
      if (G_UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_trigger_level, trigger_in[i])))
      	{
	  xmod_trigger (xmod, freq_in ? BSE_SIGNAL_TO_FREQ (freq_in[i]) : xmod->tparams.freq);
	  real_freq_256 = (int) (xmod->last_transposed_trigger_freq * 256);
	  actual_freq_256 = (int) (bse_engine_sample_freq() * 256. / xmod->size);
	}
      last_trigger_level = trigger_in[i];
      
      /* Get next position. */
      pos2 = xmod->pos + 1;
      if (pos2 >= xmod->size)
	pos2 = 0;
      
      /* Linearly interpolate sample. */
      sample = xmod->string[xmod->pos] * (1.0 - (((float) xmod->count) / actual_freq_256));
      sample += xmod->string[pos2] * (((float) xmod->count) / actual_freq_256);
      
      /* store sample, clipping is required as the algorithm generates
       * overshoot on purpose
       */
      wave_out[i] = CLAMP (sample, -1.0, +1.0);
      
      /* Use Bresenham's algorithm to advance to the next position. */
      xmod->count += real_freq_256;
      while (xmod->count >= actual_freq_256)
	{
	  xmod->d = ((xmod->d * (1.0 - xmod->a)) + (xmod->string[xmod->pos] * xmod->a)) * xmod->damping_factor;
	  xmod->string[xmod->pos] = xmod->d;
	  
	  xmod->pos++;
	  if (xmod->pos >= xmod->size)
	    xmod->pos = 0;
	  
	  xmod->count -= actual_freq_256;
	}
    }
  xmod->last_trigger_level = last_trigger_level;
}

#define	STRING_LENGTH()	((bse_engine_sample_freq() + 19) / 20)

static void
xmod_reset (BseModule *module)
{
  XtalStringsModule *xmod = module->user_data;
  
  /* this function is called whenever we need to start from scratch */
  memset (xmod->string, 0, STRING_LENGTH () * sizeof (xmod->string[0]));
  xmod->size = 1;
  xmod->pos = 0;
  xmod->count = 0;
  xmod->a = 0.0;
  xmod->damping_factor = 0.0;
  xmod->last_transposed_trigger_freq = 440.0;	/* just _some_ valid freq for the stepping */
  xmod->last_trigger_level = 0;
}

static void
xmod_free (gpointer        data,
	   const BseModuleClass *klass)
{
  XtalStringsModule *xmod = data;
  
  g_free (xmod->string);
  g_free (xmod);
}

static void
dav_xtal_strings_context_create (BseSource *source,
				 guint      context_handle,
				 BseTrans  *trans)
{
  static const BseModuleClass xmod_class = {
    DAV_XTAL_STRINGS_N_ICHANNELS,	/* n_istreams */
    0,					/* n_jstreams */
    DAV_XTAL_STRINGS_N_OCHANNELS,	/* n_ostreams */
    xmod_process,			/* process */
    NULL,				/* process_defer */
    xmod_reset,				/* reset */
    xmod_free,				/* free */
    BSE_COST_NORMAL,			/* cost */
  };
  DavXtalStrings *self = DAV_XTAL_STRINGS (source);
  XtalStringsModule *xmod = g_new0 (XtalStringsModule, 1);
  BseModule *module;
  
  xmod->string = g_new0 (gfloat, STRING_LENGTH ());
  xmod->tparams = self->params;
  module = bse_module_new (&xmod_class, xmod);
  xmod_reset (module); /* value initialization */
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

/* update module configuration from a new set of DavXtalStringsParams
 */
static void
xmod_access (BseModule *module,
	     gpointer   data)
{
  XtalStringsModule *xmod = module->user_data;
  DavXtalStringsParams *params = data;
  
  xmod->tparams = *params;
  if (params->trigger_now)
    xmod_trigger (xmod, params->freq);
}

static void
dav_xtal_strings_update_modules (DavXtalStrings *self,
				 gboolean	 trigger_now)
{
  if (BSE_SOURCE_PREPARED (self))
    {
      /* update all XtalStringsModules that this object created
       * with the new parameter set.
       * we have to copy the parameters to this function since
       * it's called in different thread than the one the
       * DavXtalStrings object lives in. also, this function
       * will be called multiple times for all XtalStringsModules,
       * the data will be freed after the last execution occoured.
       * the trigger_now assignment is merely a hack to allow easy
       * test triggers from the GUI.
       */
      self->params.trigger_now = trigger_now;
      self->params.transpose_factor = bse_transpose_factor (bse_source_prepared_musical_tuning (BSE_SOURCE (self)), self->transpose);
      bse_source_access_modules (BSE_SOURCE (self),
				 xmod_access,
				 g_memdup (&self->params, sizeof (self->params)),
				 g_free,
				 NULL);
    }
}
