// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidicontroller.hh"

#include "bsecategories.hh"
#include "bsemidireceiver.hh"
#include "bsesnet.hh"
#include "bseengine.hh"
#include "bsecxxplugin.hh"



/* --- properties --- */
enum
{
  PROP_0,
  PROP_MIDI_CHANNEL,
  PROP_CONTROL_1,
  PROP_CONTROL_2,
  PROP_CONTROL_3,
  PROP_CONTROL_4
};


/* --- prototypes --- */
static void bse_midi_controller_init            (BseMidiController      *self);
static void bse_midi_controller_class_init      (BseMidiControllerClass *klass);
static void bse_midi_controller_set_property    (GObject                *object,
                                                 uint                    param_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static void bse_midi_controller_get_property    (GObject                *object,
                                                 uint                    param_id,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);
static void bse_midi_controller_context_create  (BseSource              *source,
                                                 uint                    instance_id,
                                                 BseTrans               *trans);
static void bse_midi_controller_context_connect (BseSource              *source,
                                                 uint                    instance_id,
                                                 BseTrans               *trans);
static void bse_midi_controller_update_modules  (BseMidiController      *self);


/* --- variables --- */
static void *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiController)
{
  static const GTypeInfo midi_controller_info = {
    sizeof (BseMidiControllerClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_controller_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseMidiController),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_controller_init,
  };
#include "./icons/midi-ctrl-input.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BseMidiController",
                                         "MIDI controller input module. With this module, "
                                         "MIDI control signals can be used in synthesis networks.",
                                         __FILE__, __LINE__,
                                         &midi_controller_info);
  bse_categories_register_stock_module (N_("/Input & Output/MIDI Control Input"), type, midi_ctrl_input_pixstream);
  return type;
}

static void
bse_midi_controller_class_init (BseMidiControllerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  uint ochannel_id;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bse_midi_controller_set_property;
  gobject_class->get_property = bse_midi_controller_get_property;

  source_class->context_create = bse_midi_controller_context_create;
  source_class->context_connect = bse_midi_controller_context_connect;

  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_MIDI_CHANNEL,
                              sfi_pspec_int ("midi_channel", "MIDI Channel",
                                             "Input MIDI channel, 0 uses network's default channel",
                                             0, 0, BSE_MIDI_MAX_CHANNELS, 1,
					     SFI_PARAM_GUI SFI_PARAM_STORAGE ":scale:skip-default"));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_1,
			      bse_param_spec_genum ("control_1", "Signal 1", NULL,
						    BSE_TYPE_MIDI_SIGNAL_TYPE,
						    BSE_MIDI_SIGNAL_PITCH_BEND,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_2,
			      bse_param_spec_genum ("control_2", "Signal 2", NULL,
						    BSE_TYPE_MIDI_SIGNAL_TYPE,
						    BSE_MIDI_SIGNAL_CONTINUOUS_1,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_3,
			      bse_param_spec_genum ("control_3", "Signal 3", NULL,
						    BSE_TYPE_MIDI_SIGNAL_TYPE,
						    BSE_MIDI_SIGNAL_CONTINUOUS_7,
						    SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_4,
			      bse_param_spec_genum ("control_4", "Signal 4", NULL,
						    BSE_TYPE_MIDI_SIGNAL_TYPE,
						    BSE_MIDI_SIGNAL_PRESSURE,
						    SFI_PARAM_STANDARD));

  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl-out1", _("Ctrl Out1"), _("MIDI Signal 1"));
  g_assert (ochannel_id == BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL1);
  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl-out2", _("Ctrl Out2"), _("MIDI Signal 2"));
  g_assert (ochannel_id == BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL2);
  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl-out3", _("Ctrl Out3"), _("MIDI Signal 3"));
  g_assert (ochannel_id == BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL3);
  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl-out4", _("Ctrl Out4"), _("MIDI Signal 4"));
  g_assert (ochannel_id == BSE_MIDI_CONTROLLER_OCHANNEL_CONTROL4);
}

static void
bse_midi_controller_init (BseMidiController *self)
{
  self->midi_channel = 0;
  self->controls[0] = BSE_MIDI_SIGNAL_PITCH_BEND;
  self->controls[1] = BSE_MIDI_SIGNAL_CONTINUOUS_1;
  self->controls[2] = BSE_MIDI_SIGNAL_CONTINUOUS_7;
  self->controls[3] = BSE_MIDI_SIGNAL_PRESSURE;
}

static void
bse_midi_controller_set_property (GObject      *object,
                                  uint          param_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BseMidiController *self = BSE_MIDI_CONTROLLER (object);

  switch (param_id)
    {
    case PROP_MIDI_CHANNEL:
      self->midi_channel = sfi_value_get_int (value);
      bse_midi_controller_update_modules (self);
      break;
    case PROP_CONTROL_1:
      self->controls[0] = BseMidiSignalType (g_value_get_enum (value));
      bse_midi_controller_update_modules (self);
      break;
    case PROP_CONTROL_2:
      self->controls[1] = BseMidiSignalType (g_value_get_enum (value));
      bse_midi_controller_update_modules (self);
      break;
    case PROP_CONTROL_3:
      self->controls[2] = BseMidiSignalType (g_value_get_enum (value));
      bse_midi_controller_update_modules (self);
      break;
    case PROP_CONTROL_4:
      self->controls[3] = BseMidiSignalType (g_value_get_enum (value));
      bse_midi_controller_update_modules (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_controller_get_property (GObject    *object,
                                  uint        param_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BseMidiController *self = BSE_MIDI_CONTROLLER (object);

  switch (param_id)
    {
    case PROP_MIDI_CHANNEL:
      sfi_value_set_int (value, self->midi_channel);
      break;
    case PROP_CONTROL_1:
      g_value_set_enum (value, self->controls[0]);
      break;
    case PROP_CONTROL_2:
      g_value_set_enum (value, self->controls[1]);
      break;
    case PROP_CONTROL_3:
      g_value_set_enum (value, self->controls[2]);
      break;
    case PROP_CONTROL_4:
      g_value_set_enum (value, self->controls[3]);
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
  BseModule       *control_module;
} ModuleData;

static void
module_data_free (void *data)
{
  ModuleData *mdata = (ModuleData*) data;
  BseTrans *trans = bse_trans_open ();

  bse_midi_receiver_discard_control_module (mdata->midi_receiver, mdata->control_module, trans);
  bse_trans_commit (trans);
  g_free (mdata);
}

static void
bse_midi_controller_context_create (BseSource *source,
                                    uint       context_handle,
                                    BseTrans  *trans)
{
  BseMidiController *self = BSE_MIDI_CONTROLLER (source);
  ModuleData *mdata = g_new (ModuleData, 1);
  BseModule *module = bse_module_new_virtual (BSE_MIDI_CONTROLLER_N_OCHANNELS, mdata, module_data_free);
  BseItem *parent = BSE_ITEM (self)->parent;
  BseMidiContext mcontext = bse_snet_get_midi_context (BSE_SNET (parent), context_handle);

  /* setup module data */
  mdata->midi_receiver = mcontext.midi_receiver;
  mdata->default_channel = mcontext.midi_channel;
  mdata->midi_channel = self->midi_channel > 0 ? self->midi_channel : mdata->default_channel;
  mdata->control_module = bse_midi_receiver_retrieve_control_module (mdata->midi_receiver,
								     mdata->midi_channel,
								     self->controls,
								     trans);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_omodule (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_controller_context_connect (BseSource *source,
                                     uint       context_handle,
                                     BseTrans  *trans)
{
  BseModule *module = bse_source_get_context_omodule (source, context_handle);
  ModuleData *mdata = (ModuleData*) module->user_data;

  /* connect module to midi control uplink */
  bse_trans_add (trans, bse_job_connect (mdata->control_module, 0, module, 0));
  bse_trans_add (trans, bse_job_connect (mdata->control_module, 1, module, 1));
  bse_trans_add (trans, bse_job_connect (mdata->control_module, 2, module, 2));
  bse_trans_add (trans, bse_job_connect (mdata->control_module, 3, module, 3));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_midi_controller_update_modules (BseMidiController *self)
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
	  bse_midi_receiver_discard_control_module (mdata->midi_receiver, mdata->control_module, trans);

          /* update midi channel */
          mdata->midi_channel = self->midi_channel > 0 ? self->midi_channel : mdata->default_channel;

          /* fetch new module */
	  mdata->control_module = bse_midi_receiver_retrieve_control_module (mdata->midi_receiver,
									     mdata->midi_channel,
									     self->controls,
									     trans);
	  /* connect to new module */
	  bse_trans_add (trans, bse_job_connect (mdata->control_module, 0, module, 0));
	  bse_trans_add (trans, bse_job_connect (mdata->control_module, 1, module, 1));
	  bse_trans_add (trans, bse_job_connect (mdata->control_module, 2, module, 2));
	  bse_trans_add (trans, bse_job_connect (mdata->control_module, 3, module, 3));
	}

      /* commit and cleanup */
      g_free (cids);
      bse_trans_commit (trans);
    }
}
