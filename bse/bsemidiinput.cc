// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidiinput.hh"
#include "bsecategories.hh"
#include "bsemidireceiver.hh"
#include "bsesnet.hh"
#include "bseengine.hh"
/* --- properties --- */
enum
{
  PROP_0,
  PROP_MIDI_CHANNEL,
};
/* --- prototypes --- */
static void bse_midi_input_init            (BseMidiInput      *self);
static void bse_midi_input_class_init      (BseMidiInputClass *klass);
static void bse_midi_input_set_property    (GObject           *object,
                                            uint               param_id,
                                            const GValue      *value,
                                            GParamSpec        *pspec);
static void bse_midi_input_get_property    (GObject           *object,
                                            uint               param_id,
                                            GValue            *value,
                                            GParamSpec        *pspec);
static void bse_midi_input_context_create  (BseSource         *source,
                                            uint               instance_id,
                                            BseTrans          *trans);
static void bse_midi_input_context_connect (BseSource         *source,
                                            uint               instance_id,
                                            BseTrans          *trans);
static void bse_midi_input_update_modules  (BseMidiInput      *self);
/* --- variables --- */
static void *parent_class = NULL;
/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiInput)
{
  static const GTypeInfo midi_input_info = {
    sizeof (BseMidiInputClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_input_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseMidiInput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_input_init,
  };
#include "./icons/mono-synth.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BseMidiInput",
                                         "Monophonic MIDI input module. With this module, monophonic "
                                         "keyboard control signals can be used in synthesis networks.",
                                         __FILE__, __LINE__,
                                         &midi_input_info);
  bse_categories_register_stock_module (N_("/Input & Output/MIDI Voice Input"), type, mono_synth_pixstream);
  return type;
}
static void
bse_midi_input_class_init (BseMidiInputClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  uint ochannel_id;
  parent_class = g_type_class_peek_parent (klass);
  gobject_class->set_property = bse_midi_input_set_property;
  gobject_class->get_property = bse_midi_input_get_property;
  source_class->context_create = bse_midi_input_context_create;
  source_class->context_connect = bse_midi_input_context_connect;
  bse_object_class_add_param (object_class, "MIDI",
			      PROP_MIDI_CHANNEL,
			      sfi_pspec_int ("midi_channel", "MIDI Channel",
                                             "Input MIDI channel, 0 uses network's default channel",
					     0, 0, BSE_MIDI_MAX_CHANNELS, 1,
					     SFI_PARAM_GUI SFI_PARAM_STORAGE ":scale:skip-default"));
  ochannel_id = bse_source_class_add_ochannel (source_class, "frequency", _("Frequency"), _("Note Frequency"));
  g_assert (ochannel_id == BSE_MIDI_INPUT_OCHANNEL_FREQUENCY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "gate", _("Gate"), _("High if the note is currently being pressed"));
  g_assert (ochannel_id == BSE_MIDI_INPUT_OCHANNEL_GATE);
  ochannel_id = bse_source_class_add_ochannel (source_class, "velocity", _("Velocity"), _("Velocity of the note press"));
  g_assert (ochannel_id == BSE_MIDI_INPUT_OCHANNEL_VELOCITY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "aftertouch", _("Aftertouch"), _("Velocity while the note is pressed"));
  g_assert (ochannel_id == BSE_MIDI_INPUT_OCHANNEL_AFTERTOUCH);
}
static void
bse_midi_input_init (BseMidiInput *self)
{
  self->midi_channel = 0;
}
static void
bse_midi_input_set_property (GObject      *object,
                             uint          param_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BseMidiInput *self = BSE_MIDI_INPUT (object);
  switch (param_id)
    {
    case PROP_MIDI_CHANNEL:
      self->midi_channel = sfi_value_get_int (value);
      bse_midi_input_update_modules (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
static void
bse_midi_input_get_property (GObject    *object,
                             uint        param_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BseMidiInput *self = BSE_MIDI_INPUT (object);
  switch (param_id)
    {
    case PROP_MIDI_CHANNEL:
      sfi_value_set_int (value, self->midi_channel);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}
typedef struct {
  BseMidiReceiver *midi_receiver;
  uint             midi_channel;
  uint             default_channel;
  BseModule       *mvoice_module;
} ModuleData;
static void
module_data_free (void *data)
{
  ModuleData *mdata = (ModuleData*) data;
  BseTrans *trans = bse_trans_open ();
  bse_midi_receiver_discard_mono_voice (mdata->midi_receiver, mdata->midi_channel, mdata->mvoice_module, trans);
  bse_trans_commit (trans);
  g_free (mdata);
}
static void
bse_midi_input_context_create (BseSource *source,
                               uint       context_handle,
                               BseTrans  *trans)
{
  BseMidiInput *self = BSE_MIDI_INPUT (source);
  ModuleData *mdata = g_new (ModuleData, 1);
  BseModule *module = bse_module_new_virtual (BSE_MIDI_INPUT_N_OCHANNELS, mdata, module_data_free);
  BseItem *parent = BSE_ITEM (self)->parent;
  BseMidiContext mcontext = bse_snet_get_midi_context (BSE_SNET (parent), context_handle);
  /* setup module data */
  mdata->midi_receiver = mcontext.midi_receiver;
  mdata->default_channel = mcontext.midi_channel;
  mdata->midi_channel = self->midi_channel > 0 ? self->midi_channel : mdata->default_channel;
  mdata->mvoice_module = bse_midi_receiver_retrieve_mono_voice (mdata->midi_receiver,
                                                                mdata->midi_channel,
                                                                trans);
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_omodule (source, context_handle, module);
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}
static void
bse_midi_input_context_connect (BseSource *source,
                                uint       context_handle,
                                BseTrans  *trans)
{
  BseModule *module = bse_source_get_context_omodule (source, context_handle);
  ModuleData *mdata = (ModuleData*) module->user_data;
  /* connect module to mono control uplink */
  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 0, module, 0));
  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 1, module, 1));
  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 2, module, 2));
  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 3, module, 3));
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}
static void
bse_midi_input_update_modules (BseMidiInput *self)
{
  if (BSE_SOURCE_PREPARED (self))
    {
      BseSource *source = BSE_SOURCE (self);
      BseTrans *trans = bse_trans_open ();
      uint *cids, n, i;
      /* forall contexts */
      cids = bse_source_context_ids (source, &n);
      /* reconnect modules */
      for (i = 0; i < n; i++)
	{
	  BseModule *module = bse_source_get_context_omodule (source, cids[i]);
	  ModuleData *mdata = (ModuleData*) module->user_data;
	  /* disconnect from old module */
	  bse_trans_add (trans, bse_job_disconnect (module, 0));
	  bse_trans_add (trans, bse_job_disconnect (module, 1));
	  bse_trans_add (trans, bse_job_disconnect (module, 2));
	  bse_trans_add (trans, bse_job_disconnect (module, 3));
	  /* discard old module */
	  bse_midi_receiver_discard_mono_voice (mdata->midi_receiver, mdata->midi_channel, mdata->mvoice_module, trans);
          /* update midi channel */
          mdata->midi_channel = self->midi_channel > 0 ? self->midi_channel : mdata->default_channel;
	  /* fetch new module */
	  mdata->mvoice_module = bse_midi_receiver_retrieve_mono_voice (mdata->midi_receiver,
                                                                        mdata->midi_channel,
                                                                        trans);
	  /* connect to new module */
	  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 0, module, 0));
	  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 1, module, 1));
	  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 2, module, 2));
	  bse_trans_add (trans, bse_job_connect (mdata->mvoice_module, 3, module, 3));
	}
      /* commit and cleanup */
      g_free (cids);
      bse_trans_commit (trans);
    }
}
