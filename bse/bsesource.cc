// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesource.hh"
#include "bsecontainer.hh"
#include "bsestorage.hh"
#include "gslcommon.hh"
#include "bseserver.hh"
#include "bseengine.hh"
#include "bse/internal.hh"
#include <string.h>

/* --- macros --- */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


/* --- typedefs & enums --- */
enum {
  PROP_0,
};
typedef struct
{
  guint	      id;
  union {
    struct {
      BseModule  *imodule;
      BseModule  *omodule;
    } mods;
    struct {
      gpointer d1;
      gpointer d2;
    } data;
  } u;
} BseSourceContext;

#define	BSE_SOURCE_N_CONTEXTS(source)	(g_bsearch_array_get_n_nodes ((source)->contexts))


/* --- prototypes --- */
static gint     contexts_compare        (gconstpointer	 bsearch_node1, /* key */
                                         gconstpointer	 bsearch_node2);


/* --- variables --- */
static GTypeClass          *parent_class = NULL;
static const GBSearchConfig context_config = {
  sizeof (BseSourceContext),
  contexts_compare,
  G_BSEARCH_ARRAY_ALIGN_POWER2,
};


/* --- functions --- */
static void
bse_source_init (BseSource      *source,
		 BseSourceClass *klass)
{
  source->channel_defs = &BSE_SOURCE_CLASS (klass)->channel_defs;
  source->inputs = g_new0 (BseSourceInput, BSE_SOURCE_N_ICHANNELS (source));
  source->outputs = NULL;
  source->contexts = NULL;
  source->probes = NULL;
}

static gboolean
source_check_pspec_editable (BseObject      *object,
                             GParamSpec     *pspec)
{
  if (!BSE_OBJECT_CLASS (parent_class)->check_pspec_editable (object, pspec))
    return FALSE;
  else if (BSE_SOURCE_PREPARED (object))
    return !sfi_pspec_check_option (pspec, "unprepared");
  else
    return TRUE;
}

static void
bse_source_set_property (GObject      *object,
			 guint         param_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_source_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_source_dispose (GObject *object)
{
  BseSource *source = BSE_SOURCE (object);

  bse_source_clear_ochannels (source);
  if (BSE_SOURCE_PREPARED (source))
    {
      Bse::warning (G_STRLOC ": source still prepared during destruction");
      bse_source_reset (source);
    }

  bse_source_clear_ichannels (source);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_source_finalize (GObject *object)
{
  BseSource *source = BSE_SOURCE (object);
  for (uint i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
      g_free (BSE_SOURCE_INPUT (source, i)->jdata.joints);
  g_free (source->inputs);
  source->inputs = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
bse_source_class_has_channel (BseSourceClass *klass,
                              const gchar    *channel_ident)
{
  guint i;
  for (i = 0; i < klass->channel_defs.n_ichannels; i++)
    if (strcmp (channel_ident, klass->channel_defs.ichannel_idents[i]) == 0)
      return TRUE;
  for (i = 0; i < klass->channel_defs.n_ochannels; i++)
    if (strcmp (channel_ident, klass->channel_defs.ochannel_idents[i]) == 0)
      return TRUE;
  return FALSE;
}

static gchar*
channel_dup_canonify (const gchar *name)
{
  gchar *cname = g_new (gchar, strlen (name) + 1);
  const gchar *s;
  gchar *c = cname;

  for (s = name; *s; s++)
    if ((*s >= '0' && *s <= '9') ||
	(*s >= 'a' && *s <= 'z'))
      *c++ = *s;
    else if (*s >= 'A' && *s <= 'Z')
      *c++ = *s - 'A' + 'a';
    else
      *c++ = '-';
  *c++ = 0;
  return cname;
}

static guint
bse_source_class_add_ijchannel (BseSourceClass *source_class,
				const gchar    *ident,
				const gchar    *label,
				const gchar    *blurb,
				gboolean        is_joint_channel)
{
  BseSourceChannelDefs *defs;
  guint i;
  gchar *cname;

  assert_return (BSE_IS_SOURCE_CLASS (source_class), 0);
  assert_return (ident != NULL, 0);
  if (!label)
    label = ident;

  cname = channel_dup_canonify (ident);
  if (bse_source_class_has_channel (source_class, cname))
    {
      Bse::warning ("%s: attempt to reregister channel \"%s\" with source class `%s'", __func__,
                    cname, G_OBJECT_CLASS_NAME (source_class));
      g_free (cname);
      return ~0;
    }
  defs = &source_class->channel_defs;
  i = defs->n_ichannels++;
  defs->ichannel_idents = g_renew (gchar*, defs->ichannel_idents, defs->n_ichannels);
  defs->ichannel_labels = g_renew (gchar*, defs->ichannel_labels, defs->n_ichannels);
  defs->ichannel_blurbs = g_renew (gchar*, defs->ichannel_blurbs, defs->n_ichannels);
  defs->ijstreams = g_renew (guint, defs->ijstreams, defs->n_ichannels);
  defs->ichannel_idents[i] = cname;
  defs->ichannel_labels[i] = g_strdup (label);
  defs->ichannel_blurbs[i] = g_strdup (blurb);
  if (is_joint_channel)
    {
      defs->ijstreams[i] = defs->n_jstreams++;
      defs->ijstreams[i] |= BSE_SOURCE_JSTREAM_FLAG;
    }
  else
    defs->ijstreams[i] = i - defs->n_jstreams;

  return i;
}

guint
bse_source_class_add_ichannel (BseSourceClass *source_class,
                               const gchar    *ident,
                               const gchar    *label,
                               const gchar    *blurb)
{
  return bse_source_class_add_ijchannel (source_class, ident, label, blurb, FALSE);
}

guint
bse_source_class_add_jchannel (BseSourceClass *source_class,
                               const gchar    *ident,
                               const gchar    *label,
                               const gchar    *blurb)
{
  return bse_source_class_add_ijchannel (source_class, ident, label, blurb, TRUE);
}

guint
bse_source_find_ichannel (BseSource   *source,
			  const gchar *ichannel_cname)
{
  guint i;

  assert_return (BSE_IS_SOURCE (source), ~0);
  assert_return (ichannel_cname != NULL, ~0);

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    if (strcmp (BSE_SOURCE_ICHANNEL_IDENT (source, i), ichannel_cname) == 0)
      return i;
  return ~0;
}

guint
bse_source_class_add_ochannel (BseSourceClass *source_class,
                               const gchar    *ident,
                               const gchar    *label,
                               const gchar    *blurb)
{
  BseSourceChannelDefs *defs;
  guint i;
  gchar *cname;

  assert_return (BSE_IS_SOURCE_CLASS (source_class), 0);
  assert_return (ident != NULL, 0);
  if (!label)
    label = ident;

  cname = channel_dup_canonify (ident);
  if (bse_source_class_has_channel (source_class, cname))
    {
      Bse::warning ("%s: attempt to reregister channel \"%s\" with source class `%s'", __func__,
                    cname, G_OBJECT_CLASS_NAME (source_class));
      g_free (cname);
      return ~0;
    }
  defs = &source_class->channel_defs;
  i = defs->n_ochannels++;
  defs->ochannel_idents = g_renew (gchar*, defs->ochannel_idents, defs->n_ochannels);
  defs->ochannel_labels = g_renew (gchar*, defs->ochannel_labels, defs->n_ochannels);
  defs->ochannel_blurbs = g_renew (gchar*, defs->ochannel_blurbs, defs->n_ochannels);
  defs->ochannel_idents[i] = cname;
  defs->ochannel_labels[i] = g_strdup (label);
  defs->ochannel_blurbs[i] = g_strdup (blurb);

  return i;
}

void
bse_source_class_cache_engine_class (BseSourceClass *source_class,
                                     const BseModuleClass *engine_class)
{
  assert_return (source_class->engine_class == NULL);
  assert_return (engine_class != NULL);

  source_class->engine_class = (BseModuleClass*) g_memdup (engine_class, sizeof (*engine_class));
}

guint
bse_source_find_ochannel (BseSource   *source,
			  const gchar *ochannel_cname)
{
  guint i;

  assert_return (BSE_IS_SOURCE (source), ~0);
  assert_return (ochannel_cname != NULL, ~0);

  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    if (strcmp (BSE_SOURCE_OCHANNEL_IDENT (source, i), ochannel_cname) == 0)
      return i;
  return ~0;
}

static void
source_class_collect_properties (BseSourceClass *klass)
{
  if (!klass->filtered_properties)
    {
      guint n, i;
      GParamSpec **pspecs = g_object_class_list_properties (G_OBJECT_CLASS (klass), &n);
      for (i = 0; i < n; i++)
        {
          GParamSpec *pspec = pspecs[i];
          gboolean automate = sfi_pspec_check_option (pspec, "automate");
          gboolean preparation = automate || (sfi_pspec_check_option (pspec, "prepared") ||
                                              sfi_pspec_check_option (pspec, "unprepared"));
          if (preparation)
            klass->unprepared_properties = sfi_ring_append (klass->unprepared_properties, pspec);
          if (automate && (pspec->flags & G_PARAM_WRITABLE) &&
              g_type_is_a (pspec->owner_type, BSE_TYPE_SOURCE) &&
              (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), SFI_TYPE_REAL) ||
               g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), SFI_TYPE_BOOL) ||
               g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), SFI_TYPE_INT) ||
               g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), SFI_TYPE_NUM)))
            {
              BseSourceClass *source_class = (BseSourceClass*) g_type_class_ref (pspec->owner_type);
              if (!source_class || !source_class->property_updated)
                Bse::warning ("%s: ignoring automation property \"%s\" without property_updated() implementation",
                              g_type_name (pspec->owner_type), pspec->name);
              else
                klass->automation_properties = sfi_ring_append (klass->automation_properties, pspec);
              g_type_class_unref (source_class);
            }
        }
      g_free (pspecs);
      klass->filtered_properties = TRUE;
    }
}

static void
source_notify_properties (BseSource *self)
{
  BseSourceClass *klass = BSE_SOURCE_GET_CLASS (self);
  source_class_collect_properties (BSE_SOURCE_GET_CLASS (self));
  SfiRing *ring;
  for (ring = klass->unprepared_properties; ring; ring = sfi_ring_walk (ring, klass->unprepared_properties))
    g_object_notify (self, G_PARAM_SPEC (ring->data)->name);
}

static gint
contexts_compare (gconstpointer bsearch_node1, /* key */
		  gconstpointer bsearch_node2)
{
  const BseSourceContext *c1 = (const BseSourceContext*) bsearch_node1;
  const BseSourceContext *c2 = (const BseSourceContext*) bsearch_node2;

  return G_BSEARCH_ARRAY_CMP (c1->id, c2->id);
}

static void
bse_source_real_prepare (BseSource *source)
{
}

void
bse_source_prepare (BseSource *source)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (!BSE_SOURCE_PREPARED (source));
  assert_return (source->contexts == NULL);

  g_object_ref (source);
  g_object_freeze_notify (source);
  source_class_collect_properties (BSE_SOURCE_GET_CLASS (source));
  source->contexts = g_bsearch_array_create (&context_config);
  source->set_flag (BSE_SOURCE_FLAG_PREPARED);      /* guard properties from _before_ prepare() */
  BSE_SOURCE_GET_CLASS (source)->prepare (source);
  Bse::SourceImpl *self = source->as<Bse::SourceImpl*>();
  self->cmon_activate();
  source_notify_properties (source);
  g_object_thaw_notify (source);
  g_object_unref (source);
}

static void
bse_source_real_reset (BseSource *source)
{
}

void
bse_source_reset (BseSource *source)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (source->contexts != NULL);

  g_object_ref (source);
  g_object_freeze_notify (source);
  Bse::SourceImpl *self = source->as<Bse::SourceImpl*>();
  self->cmon_deactivate();
  uint n_contexts = BSE_SOURCE_N_CONTEXTS (source);
  if (n_contexts)
    {
      BseTrans *trans = bse_trans_open ();
      while (n_contexts)
        {
          BseSourceContext *context = (BseSourceContext*) g_bsearch_array_get_nth (source->contexts, &context_config, n_contexts - 1);
          bse_source_dismiss_context (source, context->id, trans);
          n_contexts = BSE_SOURCE_N_CONTEXTS (source);
        }
      bse_trans_commit (trans);
    }
  bse_engine_wait_on_trans ();
  BSE_SOURCE_GET_CLASS (source)->reset (source);
  source->unset_flag (BSE_SOURCE_FLAG_PREPARED);    /* guard properties until _after_ reset() */
  g_bsearch_array_free (source->contexts, &context_config);
  source->contexts = NULL;
  source_notify_properties (source);
  g_object_thaw_notify (source);
  g_object_unref (source);
}

static gint
automation_properties_cmp (gconstpointer bsearch_node1, /* key */
                           gconstpointer bsearch_node2)
{
  const BseAutomationProperty *ap1 = (const BseAutomationProperty*) bsearch_node1;
  const BseAutomationProperty *ap2 = (const BseAutomationProperty*) bsearch_node2;
  return G_BSEARCH_ARRAY_CMP (ap1->pspec, ap2->pspec);
}

static const GBSearchConfig aprop_bconfig = { sizeof (BseAutomationProperty), automation_properties_cmp, 0 };

static void
aprop_array_free (gpointer data)
{
  GBSearchArray *aparray = (GBSearchArray*) data;
  g_bsearch_array_free (aparray, &aprop_bconfig);
}

Bse::Error
bse_source_set_automation_property (BseSource        *source,
                                    const gchar      *prop_name,
                                    guint             midi_channel,
                                    Bse::MidiSignal signal_type)
{
  static_assert (Bse::MidiControl::NONE          == 0 &&
                 Bse::MidiControl::CONTINUOUS_0  == int64 (Bse::MidiSignal::CONTINUOUS_0) &&
                 Bse::MidiControl::CONTINUOUS_31 == int64 (Bse::MidiSignal::CONTINUOUS_31) &&
                 Bse::MidiControl::CONTROL_0     == int64 (Bse::MidiSignal::CONTROL_0) &&
                 Bse::MidiControl::CONTROL_127   == int64 (Bse::MidiSignal::CONTROL_127), "");
  assert_return (BSE_IS_SOURCE (source), Bse::Error::INTERNAL);
  assert_return (prop_name != NULL, Bse::Error::INTERNAL);
  if (BSE_SOURCE_PREPARED (source))
    return Bse::Error::SOURCE_BUSY;
  const Bse::MidiControl control_type = Bse::MidiControl (signal_type);
  if (control_type != Bse::MidiControl::NONE &&
      (control_type < Bse::MidiControl::CONTINUOUS_0 || control_type > Bse::MidiControl::CONTINUOUS_31) &&
      (control_type < Bse::MidiControl::CONTROL_0 || control_type > Bse::MidiControl::CONTROL_127))
    return Bse::Error::INVALID_MIDI_CONTROL;
  source_class_collect_properties (BSE_SOURCE_GET_CLASS (source));
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (source), prop_name);
  SfiRing *ring = sfi_ring_find (BSE_SOURCE_GET_CLASS (source)->automation_properties, pspec);
  if (!ring)    /* !pspec or pspec not found */
    return Bse::Error::INVALID_PROPERTY;
  GBSearchArray *aparray = (GBSearchArray*) g_object_get_data (source, "BseSource-AutomationProperties"), *oarray = aparray;
  if (!aparray)
    aparray = g_bsearch_array_create (&aprop_bconfig);
  BseAutomationProperty key = { pspec, }, *ap = (BseAutomationProperty*) g_bsearch_array_lookup (aparray, &aprop_bconfig, &key);
  if (!ap)
    {
      key.midi_channel = 0;
      key.signal_type = Bse::MidiSignal (0);
      aparray = g_bsearch_array_insert (aparray, &aprop_bconfig, &key);
      ap = (BseAutomationProperty*) g_bsearch_array_lookup (aparray, &aprop_bconfig, &key);
    }
  if (oarray != aparray)
    {
      g_object_steal_data (source, "BseSource-AutomationProperties");
      g_object_set_data_full (source, "BseSource-AutomationProperties", aparray, aprop_array_free);
    }
  if (ap->midi_channel != midi_channel ||
      ap->signal_type != signal_type)
    {
      ap->midi_channel = midi_channel;
      ap->signal_type = signal_type;
      g_object_notify (source, pspec->name);
    }
  return Bse::Error::NONE;
}

void
bse_source_get_automation_property (BseSource         *source,
                                    const gchar       *prop_name,
                                    guint             *pmidi_channel,
                                    Bse::MidiSignal *psignal_type)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (prop_name != NULL);
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (source), prop_name);
  if (pspec)
    {
      GBSearchArray *aparray = (GBSearchArray*) g_object_get_data (source, "BseSource-AutomationProperties");
      if (aparray)
        {
          BseAutomationProperty key = { pspec, }, *ap = (BseAutomationProperty*) g_bsearch_array_lookup (aparray, &aprop_bconfig, &key);
          if (ap)
            {
              if (psignal_type)
                *psignal_type = ap->signal_type;
              if (pmidi_channel)
                *pmidi_channel = ap->midi_channel;
              return;
            }
        }
    }
  return;
}

BseAutomationProperty* /* g_free() result */
bse_source_get_automation_properties (BseSource        *source,
                                      guint            *n_props)
{
  assert_return (BSE_IS_SOURCE (source), NULL);
  if (n_props)
    {
      GBSearchArray *aparray = (GBSearchArray*) g_object_get_data (source, "BseSource-AutomationProperties");
      if (aparray)
        {
          *n_props = g_bsearch_array_get_n_nodes (aparray);
          void *ap = g_memdup (g_bsearch_array_get_nth (aparray, &aprop_bconfig, 0), sizeof (BseAutomationProperty) * *n_props);
          return (BseAutomationProperty*) ap;
        }
      *n_props = 0;
    }
  return NULL;
}

static void
bse_source_real_context_create	(BseSource *source,
				 guint      context_handle,
				 BseTrans  *trans)
{
}

static inline BseSourceContext*
context_nth (BseSource *source,
	     guint      index)
{
  assert_return (index < BSE_SOURCE_N_CONTEXTS (source), NULL);

  return (BseSourceContext*) g_bsearch_array_get_nth (source->contexts, &context_config, index);
}

static inline BseSourceContext*
context_lookup (BseSource *source,
		guint      context_handle)
{
  BseSourceContext key;

  key.id = context_handle;
  return (BseSourceContext*) g_bsearch_array_lookup (source->contexts, &context_config, &key);
}

gboolean
bse_source_has_context (BseSource *source,
			guint      context_handle)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source), FALSE);

  context = context_lookup (source, context_handle);

  return context != NULL;
}

guint*
bse_source_context_ids (BseSource *source,
			guint     *n_ids)
{
  guint *cids, i;

  assert_return (BSE_IS_SOURCE (source), NULL);
  assert_return (n_ids != NULL, NULL);

  cids = g_new (guint, BSE_SOURCE_N_CONTEXTS (source));
  for (i = 0; i < BSE_SOURCE_N_CONTEXTS (source); i++)
    {
      BseSourceContext *context = context_nth (source, i);

      cids[i] = context->id;
    }
  *n_ids = BSE_SOURCE_N_CONTEXTS (source);

  return cids;
}

gpointer
bse_source_get_context_data (BseSource *source,
			     guint      context_handle)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source), NULL);
  assert_return (BSE_SOURCE_PREPARED (source), NULL);
  assert_return (!BSE_SOURCE_N_ICHANNELS (source) && !BSE_SOURCE_N_OCHANNELS (source), NULL);
  assert_return (context_handle > 0, NULL);

  context = context_lookup (source, context_handle);
  return context ? context->u.data.d2 : NULL;
}

static void
source_create_context (BseSource               *source,
		       guint                    context_handle,
		       gpointer                 data,
		       BseSourceFreeContextData free_data,
		       const gchar             *str_loc,
		       BseTrans                *trans)
{
  BseSourceContext *context, key = { 0, };

  context = context_lookup (source, context_handle);
  if (context)
    {
      Bse::warning ("%s: context %u on %p exists already", str_loc, context->id, source);
      return;
    }

  g_object_ref (source);
  key.id = context_handle;
  key.u.data.d1 = (void*) free_data;
  key.u.data.d2 = data;
  source->contexts = g_bsearch_array_insert (source->contexts, &context_config, &key);
  BSE_SOURCE_GET_CLASS (source)->context_create (source, key.id, trans);
  context = context_lookup (source, context_handle);
  assert_return (context != NULL);
  if (BSE_SOURCE_N_ICHANNELS (source) && !context->u.mods.imodule)
    Bse::warning ("%s: source `%s' failed to create %s module", str_loc, G_OBJECT_TYPE_NAME (source), "input");
  if (BSE_SOURCE_N_OCHANNELS (source) && !context->u.mods.omodule)
    Bse::warning ("%s: source `%s' failed to create %s module", str_loc, G_OBJECT_TYPE_NAME (source), "output");
  g_object_unref (source);
}

void
bse_source_create_context_with_data (BseSource      *source,
				     guint           context_handle,
				     gpointer        data,
				     BseSourceFreeContextData free_data,
				     BseTrans       *trans)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (!BSE_SOURCE_N_ICHANNELS (source) && !BSE_SOURCE_N_OCHANNELS (source));
  assert_return (context_handle > 0);
  assert_return (trans != NULL);

  source_create_context (source, context_handle, data, free_data, G_STRLOC, trans);
}

void
bse_source_create_context (BseSource *source,
			   guint      context_handle,
			   BseTrans  *trans)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (context_handle > 0);
  assert_return (trans != NULL);

  source_create_context (source, context_handle, NULL, NULL, G_STRLOC, trans);
}

static void
bse_source_context_connect_ichannel (BseSource        *source,
				     BseSourceContext *context,
				     guint             ichannel,
				     BseTrans         *trans,
				     guint	       first_joint)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      guint i;

      for (i = first_joint; i < input->jdata.n_joints; i++)
	{
	  BseSourceOutput *output = input->jdata.joints + i;

	  if (output->osource)
	    {
	      BseModule *omodule = bse_source_get_context_omodule (output->osource,
								   context->id);
	      bse_trans_add (trans,
			     bse_job_jconnect (omodule,
					       BSE_SOURCE_OCHANNEL_OSTREAM (output->osource,
									   output->ochannel),
					       context->u.mods.imodule,
					       BSE_SOURCE_ICHANNEL_JSTREAM (source, ichannel)));
	    }
	}
    }
  else
    {
      if (input->idata.osource)
	{
	  BseModule *omodule = bse_source_get_context_omodule (input->idata.osource,
							       context->id);
	  bse_trans_add (trans,
			 bse_job_connect (omodule,
					  BSE_SOURCE_OCHANNEL_OSTREAM (input->idata.osource,
								       input->idata.ochannel),
					  context->u.mods.imodule,
					  BSE_SOURCE_ICHANNEL_ISTREAM (source, ichannel)));
	}
    }
}

static void
bse_source_real_context_connect	(BseSource *source,
				 guint      context_handle,
				 BseTrans  *trans)
{
  guint i;

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceContext *context = context_lookup (source, context_handle);

      bse_source_context_connect_ichannel (source, context, i, trans, 0);
    }
}

void
bse_source_connect_context (BseSource *source,
			    guint      context_handle,
			    BseTrans  *trans)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (context_handle > 0);
  assert_return (trans != NULL);

  context = context_lookup (source, context_handle);
  if (context)
    {
      g_object_ref (source);
      BSE_SOURCE_GET_CLASS (source)->context_connect (source, context_handle, trans);
      g_object_unref (source);
    }
  else
    Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
}

static void
bse_source_real_context_dismiss	(BseSource *source,
				 guint      context_handle,
				 BseTrans  *trans)
{
  BseSourceContext *context = context_lookup (source, context_handle);

  if (BSE_SOURCE_N_ICHANNELS (source) || BSE_SOURCE_N_OCHANNELS (source))
    {
      if (context->u.mods.imodule)
	bse_trans_add (trans, bse_job_discard (context->u.mods.imodule));
      if (context->u.mods.omodule && context->u.mods.omodule != context->u.mods.imodule)
	bse_trans_add (trans, bse_job_discard (context->u.mods.omodule));
      context->u.mods.imodule = NULL;
      context->u.mods.omodule = NULL;
    }
}

void
bse_source_dismiss_context (BseSource *source,
			    guint      context_handle,
			    BseTrans  *trans)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (context_handle > 0);
  assert_return (trans != NULL);

  context = context_lookup (source, context_handle);
  if (context)
    {
      BseSourceFreeContextData free_cdata = NULL;
      gpointer cdata = NULL;
      g_object_ref (source);
      BSE_SOURCE_GET_CLASS (source)->context_dismiss (source, context_handle, trans);
      context = context_lookup (source, context_handle);
      assert_return (context != NULL);
      if (BSE_SOURCE_N_ICHANNELS (source) && context->u.mods.imodule)
	Bse::warning ("%s: source `%s' failed to dismiss %s module", G_STRLOC,
                      G_OBJECT_TYPE_NAME (source), "input");
      if (BSE_SOURCE_N_OCHANNELS (source) && context->u.mods.omodule)
	Bse::warning ("%s: source `%s' failed to dismiss %s module", G_STRLOC,
                      G_OBJECT_TYPE_NAME (source), "output");
      if (BSE_SOURCE_N_OCHANNELS (source) == 0 &&
	  BSE_SOURCE_N_ICHANNELS (source) == 0)
	{
	  free_cdata = (BseSourceFreeContextData) context->u.data.d1;
	  cdata = context->u.data.d2;
	}
      source->contexts = g_bsearch_array_remove (source->contexts, &context_config,
						 g_bsearch_array_get_index (source->contexts,
									    &context_config,
									    context));
      if (free_cdata)
	free_cdata (source, cdata, trans);
      g_object_unref (source);
    }
  else
    Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
}

void
bse_source_set_context_imodule (BseSource *source, uint context_handle, BseModule *imodule, BseTrans *trans)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (context_handle > 0);
  assert_return (BSE_SOURCE_N_ICHANNELS (source) > 0);
  if (imodule)
    {
      assert_return (BSE_MODULE_N_JSTREAMS (imodule) >= BSE_SOURCE_N_JOINT_ICHANNELS (source));
      if (BSE_SOURCE_N_JOINT_ICHANNELS (source))
	{
	  guint n_non_joint_ichannels = BSE_SOURCE_N_ICHANNELS (source) - BSE_SOURCE_N_JOINT_ICHANNELS (source);
	  assert_return (BSE_MODULE_N_ISTREAMS (imodule) >= n_non_joint_ichannels);
	}
      else
	assert_return (BSE_MODULE_N_ISTREAMS (imodule) >= BSE_SOURCE_N_ICHANNELS (source));
    }

  context = context_lookup (source, context_handle);
  if (!context)
    {
      Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
      return;
    }
  if (imodule)
    assert_return (context->u.mods.imodule == NULL);
  else
    assert_return (context->u.mods.imodule != NULL);

  context->u.mods.imodule = imodule;
}

BseModule*
bse_source_get_context_imodule (BseSource *source,
				guint      context_handle)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source), NULL);
  assert_return (BSE_SOURCE_PREPARED (source), NULL);
  assert_return (BSE_SOURCE_N_ICHANNELS (source) > 0, NULL);

  context = context_lookup (source, context_handle);
  if (!context)
    {
      Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
      return NULL;
    }
  return context->u.mods.imodule;
}

static bool
source_find_omodule (BseSource *source, BseModule *omodule)
{
  const uint n_contexts = BSE_SOURCE_PREPARED (source) ? BSE_SOURCE_N_CONTEXTS (source) : 0;
  if (BSE_SOURCE_N_OCHANNELS (source))
    for (uint i = 0; i < n_contexts; i++)
      {
        BseSourceContext *context = (BseSourceContext*) g_bsearch_array_get_nth (source->contexts, &context_config, i);
        if (context->u.mods.omodule == omodule)
          return true;
      }
  return false;
}

void
bse_source_set_context_omodule (BseSource *source, uint context_handle, BseModule *omodule, BseTrans *trans)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (context_handle > 0);
  assert_return (BSE_SOURCE_N_OCHANNELS (source) > 0);
  if (omodule)
    assert_return (BSE_MODULE_N_OSTREAMS (omodule) >= BSE_SOURCE_N_OCHANNELS (source));

  BseSourceContext *context = context_lookup (source, context_handle);
  if (!context)
    {
      Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
      return;
    }

  Bse::SourceImpl *self = source->as<Bse::SourceImpl*>();

  // add module
  if (omodule)
    {
      assert_return (context->u.mods.omodule == NULL);
      const bool seen_module = source_find_omodule (source, omodule);
      context->u.mods.omodule = omodule;
      if (!seen_module)         // notify on first module reference
        self->cmon_omodule_changed (omodule, true, trans);
    }

  // remove module
  if (!omodule)
    {
      assert_return (context->u.mods.omodule != NULL);
      omodule = context->u.mods.omodule; // save for notification
      context->u.mods.omodule = NULL;
      const bool seen_module = source_find_omodule (source, omodule);
      if (!seen_module)         // notify on last module reference
        {
          context->u.mods.omodule = omodule; // keep reference around during notification
          self->cmon_omodule_changed (context->u.mods.omodule, false, trans);
          context->u.mods.omodule = NULL;
        }
    }
}

void
bse_source_list_omodules (BseSource *source, std::vector<BseModule*> &modules)
{
  const uint n_contexts = BSE_SOURCE_PREPARED (source) ? BSE_SOURCE_N_CONTEXTS (source) : 0;
  const size_t offset = modules.size();
  if (BSE_SOURCE_N_OCHANNELS (source))
    for (uint i = 0; i < n_contexts; i++)
      {
        BseSourceContext *context = (BseSourceContext*) g_bsearch_array_get_nth (source->contexts, &context_config, i);
        if (context->u.mods.omodule)
          modules.push_back (context->u.mods.omodule);
      }
  std::sort (modules.begin() + offset, modules.end());
  auto end = std::unique (modules.begin() + offset, modules.end()); // collapses dups
  modules.erase (end, modules.end()); // eliminate std::move leftovers
}

BseModule*
bse_source_get_context_omodule (BseSource *source,
				guint      context_handle)
{
  BseSourceContext *context;

  assert_return (BSE_IS_SOURCE (source), NULL);
  assert_return (BSE_SOURCE_PREPARED (source), NULL);
  assert_return (BSE_SOURCE_N_OCHANNELS (source) > 0, NULL);

  context = context_lookup (source, context_handle);
  if (!context)
    {
      Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
      return NULL;
    }
  return context->u.mods.omodule;
}

void
bse_source_set_context_module (BseSource *source, uint context_handle, BseModule *module, BseTrans *trans)
{
  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (module != NULL);
  assert_return (BSE_MODULE_N_OSTREAMS (module) >= BSE_SOURCE_N_OCHANNELS (source));
  assert_return (BSE_MODULE_N_ISTREAMS (module) + BSE_MODULE_N_JSTREAMS (module) >= BSE_SOURCE_N_ICHANNELS (source));

  if (BSE_SOURCE_N_ICHANNELS (source))
    bse_source_set_context_imodule (source, context_handle, module, trans);
  if (BSE_SOURCE_N_OCHANNELS (source))
    bse_source_set_context_omodule (source, context_handle, module, trans);
}

void
bse_source_flow_access_module (BseSource    *source,
			       guint         context_handle,
			       guint64       tick_stamp,
			       BseEngineAccessFunc access_func,
			       gpointer      data,
			       BseFreeFunc   data_free_func,
			       BseTrans     *trans)
{
  BseSourceContext *context;
  BseModule *m1, *m2;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (access_func != NULL);
  assert_return (context_handle > 0);
  assert_return (BSE_SOURCE_N_ICHANNELS (source) || BSE_SOURCE_N_OCHANNELS (source));

  context = context_lookup (source, context_handle);
  if (!context)
    {
      Bse::warning ("%s: no such context %u", G_STRLOC, context_handle);
      return;
    }
  m1 = context->u.mods.imodule;
  m2 = context->u.mods.omodule;
  if (m1 == m2)
    m1 = NULL;

  if (m1 || m2)
    {
      BseTrans *my_trans = trans ? trans : bse_trans_open ();

      if (m1)
	bse_trans_add (my_trans, bse_job_flow_access (m1, tick_stamp, access_func, data,
						      m2 ? NULL : data_free_func));
      if (m2)
	bse_trans_add (my_trans, bse_job_flow_access (m2, tick_stamp, access_func, data,
						      data_free_func));
      if (!trans)
	bse_trans_commit (my_trans);
    }
  else if (data_free_func)
    data_free_func (data);
}

void
bse_source_flow_access_modules (BseSource    *source,
				guint64       tick_stamp,
				BseEngineAccessFunc access_func,
				gpointer      data,
				BseFreeFunc   data_free_func,
				BseTrans     *trans)
{
  GSList *modules = NULL;
  guint i;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (access_func != NULL);
  assert_return (BSE_SOURCE_N_ICHANNELS (source) || BSE_SOURCE_N_OCHANNELS (source));

  for (i = 0; i < BSE_SOURCE_N_CONTEXTS (source); i++)
    {
      BseSourceContext *context = context_nth (source, i);

      if (context->u.mods.imodule)
	modules = g_slist_prepend (modules, context->u.mods.imodule);
      else if (context->u.mods.omodule && context->u.mods.omodule != context->u.mods.imodule)
	modules = g_slist_prepend (modules, context->u.mods.omodule);
    }

  if (modules)
    {
      BseTrans *my_trans = trans ? trans : bse_trans_open ();

      for (GSList *slist = modules; slist; slist = slist->next)
	bse_trans_add (my_trans, bse_job_flow_access ((BseModule*) slist->data, tick_stamp, access_func, data,
						      slist->next ? NULL : data_free_func));
      if (!trans)
	bse_trans_commit (my_trans);
      g_slist_free (modules);
    }
  else if (data_free_func)
    data_free_func (data);
}

void
bse_source_access_modules (BseSource    *source,
			   BseEngineAccessFunc access_func,
			   gpointer      data,
			   BseFreeFunc   data_free_func,
			   BseTrans     *trans)
{
  GSList *modules = NULL;
  guint i;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (access_func != NULL);
  assert_return (BSE_SOURCE_N_ICHANNELS (source) || BSE_SOURCE_N_OCHANNELS (source));

  for (i = 0; i < BSE_SOURCE_N_CONTEXTS (source); i++)
    {
      BseSourceContext *context = context_nth (source, i);

      if (context->u.mods.imodule)
	modules = g_slist_prepend (modules, context->u.mods.imodule);
      else if (context->u.mods.omodule && context->u.mods.omodule != context->u.mods.imodule)
	modules = g_slist_prepend (modules, context->u.mods.omodule);
    }

  if (modules)
    {
      BseTrans *my_trans = trans ? trans : bse_trans_open ();
      GSList *slist;

      for (slist = modules; slist; slist = slist->next)
	bse_trans_add (my_trans, bse_job_access ((BseModule*) slist->data, access_func, data,
						 slist->next ? NULL : data_free_func));
      if (!trans)
	bse_trans_commit (my_trans);
      g_slist_free (modules);
    }
  else if (data_free_func)
    data_free_func (data);
}

typedef struct {
  guint  member_offset;
  guint  member_size;
} AccessData;

static void
op_access_update (BseModule *module,
		  gpointer   data)
{
  AccessData *adata = (AccessData*) data;
  guint8 *m = (guint8*) module->user_data;

  memcpy (m + adata->member_offset, adata + 1, adata->member_size);
}

void
bse_source_update_modules (BseSource *source,
			   guint      member_offset,
			   gpointer   member_data,
			   guint      member_size,
			   BseTrans  *trans)
{
  AccessData *adata;

  assert_return (BSE_IS_SOURCE (source));
  assert_return (BSE_SOURCE_PREPARED (source));
  assert_return (member_data != NULL);
  assert_return (member_size > 0);

  adata = (AccessData*) g_malloc (sizeof (AccessData) + member_size);
  adata->member_offset = member_offset;
  adata->member_size = member_size;
  memcpy (adata + 1, member_data, member_size);
  bse_source_access_modules (source, op_access_update, adata, g_free, trans);
}

static void
bse_source_real_add_input (BseSource *source,
			   guint      ichannel,
			   BseSource *osource,
			   guint      ochannel)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);
  guint j = 0;

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      j = input->jdata.n_joints++;
      input->jdata.joints = g_renew (BseSourceOutput, input->jdata.joints, input->jdata.n_joints);
      input->jdata.joints[j].osource = osource;
      input->jdata.joints[j].ochannel = ochannel;
    }
  else
    {
      assert_return (input->idata.osource == NULL);

      input->idata.osource = osource;
      input->idata.ochannel = ochannel;
    }
  osource->outputs = g_slist_prepend (osource->outputs, source);

  if (BSE_SOURCE_PREPARED (source) && BSE_SOURCE_N_CONTEXTS (source))
    {
      BseTrans *trans = bse_trans_open ();
      guint c;

      for (c = 0; c < BSE_SOURCE_N_CONTEXTS (source); c++)
	{
	  BseSourceContext *context = context_nth (source, c);

	  bse_source_context_connect_ichannel (source, context, ichannel, trans, j);
	}
      bse_trans_commit (trans);
    }
}

static gint
check_jchannel_connection (BseSource *source,
			   guint      ichannel,
			   BseSource *osource,
			   guint      ochannel)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      guint j;

      for (j = 0; j < input->jdata.n_joints; j++)
	if (input->jdata.joints[j].osource == osource &&
	    input->jdata.joints[j].ochannel == ochannel)
	  break;
      return j < input->jdata.n_joints ? j : -1;
    }
  else
    return ochannel == input->idata.ochannel && osource == input->idata.osource ? 0 : -1;
}

Bse::Error
bse_source_set_input (BseSource *source,
		      guint      ichannel,
		      BseSource *osource,
		      guint      ochannel)
{
  assert_return (BSE_IS_SOURCE (source), Bse::Error::INTERNAL);
  assert_return (BSE_IS_SOURCE (osource), Bse::Error::INTERNAL);
  assert_return (BSE_ITEM (source)->parent == BSE_ITEM (osource)->parent, Bse::Error::INTERNAL);
  if (BSE_SOURCE_PREPARED (source))	/* FIXME: check context sets */
    {
      assert_return (BSE_SOURCE_PREPARED (osource), Bse::Error::INTERNAL); /* paranoid, checked parent already */
      assert_return (BSE_SOURCE_N_CONTEXTS (source) == BSE_SOURCE_N_CONTEXTS (osource), Bse::Error::INTERNAL);
    }
  else
    assert_return (!BSE_SOURCE_PREPARED (osource), Bse::Error::INTERNAL);

  if (ichannel >= BSE_SOURCE_N_ICHANNELS (source))
    return Bse::Error::SOURCE_NO_SUCH_ICHANNEL;
  if (ochannel >= BSE_SOURCE_N_OCHANNELS (osource))
    return Bse::Error::SOURCE_NO_SUCH_OCHANNEL;
  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      if (check_jchannel_connection (source, ichannel, osource, ochannel) >= 0)
	return Bse::Error::SOURCE_CHANNELS_CONNECTED;
    }
  else if (BSE_SOURCE_INPUT (source, ichannel)->idata.osource)
    return Bse::Error::SOURCE_ICHANNEL_IN_USE;
  if (bse_source_test_input_recursive (osource, source))
    return Bse::Error::SOURCE_BAD_LOOPBACK;

  g_object_ref (source);
  g_object_ref (osource);
  BSE_SOURCE_GET_CLASS (source)->add_input (source, ichannel, osource, ochannel);
  g_object_unref (source);
  g_object_unref (osource);

  return Bse::Error::NONE;
}

static void
bse_source_real_remove_input (BseSource *source,
			      guint      ichannel,
			      BseSource *osource,
			      guint      ochannel)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);
  BseTrans *trans = NULL;
  gint j = -1;

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      j = check_jchannel_connection (source, ichannel, osource, ochannel);
      assert_return (j >= 0);
    }
  else
    assert_return (osource == BSE_SOURCE_INPUT (source, ichannel)->idata.osource);

  if (BSE_SOURCE_PREPARED (source) && BSE_SOURCE_N_CONTEXTS (source))
    {
      guint c;
      trans = bse_trans_open ();
      for (c = 0; c < BSE_SOURCE_N_CONTEXTS (source); c++)
	{
	  BseSourceContext *context = context_nth (source, c);
	  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
	    {
	      BseSourceContext *src_context = context_nth (osource, c);
	      bse_trans_add (trans, bse_job_jdisconnect (context->u.mods.imodule,
							 BSE_SOURCE_ICHANNEL_JSTREAM (source, ichannel),
							 src_context->u.mods.omodule,
							 BSE_SOURCE_OCHANNEL_OSTREAM (osource, ochannel)));
	    }
	  else
	    {
	      for (c = 0; c < BSE_SOURCE_N_CONTEXTS (source); c++)
		{
		  BseSourceContext *context = context_nth (source, c);

		  bse_trans_add (trans, bse_job_disconnect (context->u.mods.imodule,
							    BSE_SOURCE_ICHANNEL_ISTREAM (source, ichannel)));
		}
	    }
	}
    }

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      guint k = --input->jdata.n_joints;

      input->jdata.joints[j].osource = input->jdata.joints[k].osource;
      input->jdata.joints[j].ochannel = input->jdata.joints[k].ochannel;
    }
  else
    {
      input->idata.osource = NULL;
      input->idata.ochannel = 0;
    }
  osource->outputs = g_slist_remove (osource->outputs, source);

  if (trans)
    bse_trans_commit (trans);
}

Bse::Error
bse_source_check_input (BseSource *source,
			guint      ichannel,
			BseSource *osource,
			guint      ochannel)
{
  assert_return (BSE_IS_SOURCE (source), Bse::Error::INTERNAL);
  assert_return (BSE_IS_SOURCE (osource), Bse::Error::INTERNAL);

  if (BSE_ITEM (source)->parent != BSE_ITEM (osource)->parent)
    return Bse::Error::SOURCE_PARENT_MISMATCH;
  if (BSE_SOURCE_PREPARED (source))     /* FIXME: check context sets */
    {
      assert_return (BSE_SOURCE_PREPARED (osource), Bse::Error::INTERNAL);	/* paranoid, checked parent already */
      /* prolly wrong: */ assert_return (BSE_SOURCE_N_CONTEXTS (source) == BSE_SOURCE_N_CONTEXTS (osource), Bse::Error::INTERNAL);
    }
  else
    assert_return (!BSE_SOURCE_PREPARED (osource), Bse::Error::INTERNAL);

  if (ichannel >= BSE_SOURCE_N_ICHANNELS (source))
    return Bse::Error::SOURCE_NO_SUCH_ICHANNEL;
  if (ochannel >= BSE_SOURCE_N_OCHANNELS (osource))
    return Bse::Error::SOURCE_NO_SUCH_OCHANNEL;
  if (check_jchannel_connection (source, ichannel, osource, ochannel) < 0)
    return Bse::Error::SOURCE_NO_SUCH_CONNECTION;

  return Bse::Error::NONE;
}

Bse::Error
bse_source_unset_input (BseSource *source,
			guint      ichannel,
			BseSource *osource,
			guint      ochannel)
{
  Bse::Error error = bse_source_check_input (source, ichannel, osource, ochannel);
  if (error != Bse::Error::NONE)
    return error;

  g_object_ref (source);
  g_object_ref (osource);
  BSE_SOURCE_GET_CLASS (source)->remove_input (source, ichannel, osource, ochannel);
  g_object_unref (osource);
  g_object_unref (source);

  return Bse::Error::NONE;
}

gboolean
bse_source_get_input (BseSource      *source,
                      guint           ichannel,
                      BseSource     **osourcep,
                      guint          *ochannelp)
{
  assert_return (BSE_IS_SOURCE (source), false);
  if (ichannel < BSE_SOURCE_N_ICHANNELS (source) &&
      !BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);
      if (osourcep)
        *osourcep = input->idata.osource;
      if (ochannelp)
        *ochannelp = input->idata.ochannel;
      return TRUE;
    }
  return FALSE;
}

gboolean
bse_source_has_output (BseSource *source,
                       guint      ochannel)
{
  assert_return (BSE_IS_SOURCE (source), FALSE);
  if (ochannel < BSE_SOURCE_N_OCHANNELS (source))
    {
      GSList *slist;
      for (slist = source->outputs; slist; slist = slist->next)
        {
          BseSource *isource = (BseSource*) slist->data;
          guint i, j;
          for (i = 0; i < BSE_SOURCE_N_ICHANNELS (isource); i++)
            {
              BseSourceInput *input = BSE_SOURCE_INPUT (isource, i);
              if (BSE_SOURCE_IS_JOINT_ICHANNEL (isource, i))
                {
                  for (j = 0; j < input->jdata.n_joints; j++)
                    if (input->jdata.joints[j].osource == source && input->jdata.joints[j].ochannel == ochannel)
                      return TRUE;
                }
              else if (input->idata.osource == source && input->idata.ochannel == ochannel)
                return TRUE;
	    }
	}
    }
  return FALSE;
}

void
bse_source_must_set_input_loc (BseSource      *source,
                               guint           ichannel,
                               BseSource      *osource,
                               guint           ochannel,
                               const gchar    *strloc)
{
  Bse::Error error = bse_source_set_input (source, ichannel, osource, ochannel);
  if (error != 0)
    Bse::warning ("%s: failed to connect module %s channel %u to module %s channel %u: %s", strloc,
                  bse_object_debug_name (source), ichannel,
                  bse_object_debug_name (osource), ochannel,
                  bse_error_blurb (error));
}

static SfiRing*
collect_inputs_flat (SfiRing *ring, BseSource *source)
{
  guint i, j;

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (source, i);

      if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
	for (j = 0; j < input->jdata.n_joints; j++)
	  {
	    BseSource *isource = input->jdata.joints[j].osource;
	    if (!BSE_SOURCE_COLLECTED (isource))
	      {
		isource->set_flag (BSE_SOURCE_FLAG_COLLECTED);
		ring = sfi_ring_append (ring, isource);
	      }
	  }
      else if (input->idata.osource)
	{
	  BseSource *isource = input->idata.osource;
	  if (!BSE_SOURCE_COLLECTED (isource))
	    {
	      isource->set_flag (BSE_SOURCE_FLAG_COLLECTED);
	      ring = sfi_ring_append (ring, isource);
	    }
	}
    }
  return ring;
}

SfiRing*
bse_source_collect_inputs_flat (BseSource *source)
{
  assert_return (BSE_IS_SOURCE (source), NULL);
  return collect_inputs_flat (NULL, source);
}

SfiRing*
bse_source_collect_inputs_recursive (BseSource *source)
{
  SfiRing *node, *ring = NULL;

  assert_return (BSE_IS_SOURCE (source), NULL);

  ring = collect_inputs_flat (ring, source);
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    ring = collect_inputs_flat (ring, (BseSource*) node->data);
  return ring;
}

void
bse_source_free_collection (SfiRing *ring)
{
  SfiRing *node;

  for (node = ring; node; node = sfi_ring_walk (node, ring))
    {
      BseSource *source = BSE_SOURCE (node->data);
      source->unset_flag (BSE_SOURCE_FLAG_COLLECTED);
    }
  sfi_ring_free (ring);
}

static gboolean
ring_search_backwards (SfiRing *ring,
                       SfiRing *bound,
                       void    *data)
{
  while (ring != bound)
    if (ring->data == data)
      return TRUE;
    else
      ring = ring->prev;
  return FALSE;
}

gboolean
bse_source_test_input_recursive (BseSource      *source,
                                 BseSource      *test)
{
  SfiRing *node, *last, *ring = NULL;

  assert_return (BSE_IS_SOURCE (source) && BSE_IS_SOURCE (test), FALSE);

  source->set_flag (BSE_SOURCE_FLAG_COLLECTED);
  last = ring = sfi_ring_append (NULL, source);
  gboolean match = last->data == test;
  for (node = ring; node && !match; node = sfi_ring_walk (node, ring))
    {
      ring = collect_inputs_flat (ring, (BseSource*) node->data);
      match = ring_search_backwards (ring->prev, last, test);
      last = ring->prev;
    }
  bse_source_free_collection (ring);
  return match;
}

void
bse_source_clear_ichannels (BseSource *source)
{
  gboolean io_changed = FALSE;
  guint i;

  assert_return (BSE_IS_SOURCE (source));

  g_object_ref (source);
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (source, i);
      BseSource *osource;

      if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
	{
	  guint ochannel;

	  while (input->jdata.n_joints)
	    {
	      osource = input->jdata.joints[0].osource;
	      ochannel = input->jdata.joints[0].ochannel;

	      io_changed = TRUE;
	      g_object_ref (osource);
	      BSE_SOURCE_GET_CLASS (source)->remove_input (source, i, osource, ochannel);
	      g_object_unref (osource);
	    }
	}
      else if (input->idata.osource)
	{
	  osource = input->idata.osource;

	  io_changed = TRUE;
	  g_object_ref (osource);
	  BSE_SOURCE_GET_CLASS (source)->remove_input (source, i, osource, input->idata.ochannel);
	  g_object_unref (osource);
	}
    }
  if (io_changed)
    ; // emit ("iochanged");
  g_object_unref (source);
}

void
bse_source_backup_ichannels_to_undo (BseSource *source)
{
  BseUndoStack *ustack;

  assert_return (BSE_IS_SOURCE (source));

  ustack = bse_item_undo_open (source, "unset-input %s", bse_object_debug_name (source));
  if (ustack)
    {
      guint i, j;
      for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
        {
          BseSourceInput *input = BSE_SOURCE_INPUT (source, i);

          if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
            for (j = 0; j < input->jdata.n_joints; j++)
              bse_source_input_backup_to_undo (source, i,
                                               input->jdata.joints[j].osource,
                                               input->jdata.joints[j].ochannel);
          else if (input->idata.osource)
            bse_source_input_backup_to_undo (source, i,
                                             input->idata.osource,
                                             input->idata.ochannel);
        }
    }
  bse_item_undo_close (ustack);
}

void
bse_source_clear_ochannels (BseSource *source)
{
  gboolean io_changed = FALSE;

  assert_return (BSE_IS_SOURCE (source));

  g_object_ref (source);
  while (source->outputs)
    {
      BseSource *isource = (BseSource*) source->outputs->data;
      guint i;

      g_object_ref (isource);
      for (i = 0; i < BSE_SOURCE_N_ICHANNELS (isource); i++)
	{
	  BseSourceInput *input = BSE_SOURCE_INPUT (isource, i);

	  if (BSE_SOURCE_IS_JOINT_ICHANNEL (isource, i))
	    {
	      guint j;

	      for (j = 0; j < input->jdata.n_joints; j++)
		if (input->jdata.joints[j].osource == source)
		  break;
	      if (j < input->jdata.n_joints)
		{
		  io_changed = TRUE;
		  BSE_SOURCE_GET_CLASS (isource)->remove_input (isource, i,
								source, input->jdata.joints[j].ochannel);
		  break;
		}
	    }
	  else if (input->idata.osource == source)
	    {
	      io_changed = TRUE;
	      BSE_SOURCE_GET_CLASS (isource)->remove_input (isource, i,
							    source, input->idata.ochannel);
	      break;
	    }
	}
      g_object_unref (isource);
    }
  if (io_changed)
    ; // emit ("iochanged");
  g_object_unref (source);
}

void
bse_source_backup_ochannels_to_undo (BseSource *source)
{
  BseUndoStack *ustack;

  assert_return (BSE_IS_SOURCE (source));

  ustack = bse_item_undo_open (source, "unset-input %s", bse_object_debug_name (source));
  if (ustack)
    {
      GSList *slist, *uniq_outputs = NULL;

      for (slist = source->outputs; slist; slist = slist->next)
        if (!g_slist_find (uniq_outputs, slist->data))
          uniq_outputs = g_slist_prepend (uniq_outputs, slist->data);

      for (slist = uniq_outputs; slist; slist = slist->next)
        {
          BseSource *isource = (BseSource*) slist->data;
          guint i;
          for (i = 0; i < BSE_SOURCE_N_ICHANNELS (isource); i++)
            {
              BseSourceInput *input = BSE_SOURCE_INPUT (isource, i);
              if (BSE_SOURCE_IS_JOINT_ICHANNEL (isource, i))
                {
                  guint j;
                  for (j = 0; j < input->jdata.n_joints; j++)
                    if (input->jdata.joints[j].osource == source)
                      bse_source_input_backup_to_undo (isource, i,
                                                       input->jdata.joints[j].osource,
                                                       input->jdata.joints[j].ochannel);
                }
              else if (input->idata.osource == source)
                bse_source_input_backup_to_undo (isource, i, source, input->idata.ochannel);
            }
        }
      g_slist_free (uniq_outputs);
    }
  bse_item_undo_close (ustack);
}

static void
bse_source_real_store_private (BseObject  *object,
			       BseStorage *storage)
{
  BseSource *source = BSE_SOURCE (object);
  guint i, j;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  if (BSE_SOURCE_PRIVATE_INPUTS (source))
    return;

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (source, i);
      GSList *slist, *outputs = NULL;

      if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
        for (j = 0; j < input->jdata.n_joints; j++)
          outputs = g_slist_append (outputs, input->jdata.joints + j);
      else if (input->idata.osource)
        outputs = g_slist_append (outputs, &input->idata);

      for (slist = outputs; slist; slist = slist->next)
        {
          BseSourceOutput *output = (BseSourceOutput*) slist->data;

          bse_storage_break (storage);
          bse_storage_printf (storage,
                              "(source-input \"%s\" ",
                              BSE_SOURCE_ICHANNEL_IDENT (source, i));
          bse_storage_put_item_link (storage, BSE_ITEM (source), BSE_ITEM (output->osource));
          bse_storage_printf (storage, " \"%s\")", BSE_SOURCE_OCHANNEL_IDENT (output->osource, output->ochannel));
        }
      g_slist_free (outputs);
    }
}

void
bse_source_input_backup_to_undo (BseSource      *source,
                                 guint           ichannel,
                                 BseSource      *osource,
                                 guint           ochannel)
{
  Bse::Error error = bse_source_check_input (source, ichannel, osource, ochannel);
  BseUndoStack *ustack;
  BseStorage *storage;

  assert_return (error == Bse::Error::NONE);

  if (BSE_SOURCE_PRIVATE_INPUTS (source))
    return;

  ustack = bse_item_undo_open (source, "unset-input %s", bse_object_debug_name (source));

  storage = (BseStorage*) bse_object_new (BSE_TYPE_STORAGE, NULL);
  bse_storage_prepare_write (storage, BSE_STORAGE_DBLOCK_CONTAINED);

  bse_storage_break (storage);
  bse_storage_printf (storage,
                      "(source-input \"%s\" ",
                      BSE_SOURCE_ICHANNEL_IDENT (source, ichannel));
  bse_storage_put_item_link (storage, BSE_ITEM (source), BSE_ITEM (osource));
  bse_storage_printf (storage, " \"%s\")", BSE_SOURCE_OCHANNEL_IDENT (osource, ochannel));

  bse_item_push_undo_storage (BSE_ITEM (source), ustack, storage);
  g_object_unref (storage);

  bse_item_undo_close (ustack);
}

typedef struct _DeferredInput DeferredInput;
struct _DeferredInput
{
  DeferredInput *next;
  gchar		*ichannel_ident;
  gchar         *osource_path;
  gchar         *ochannel_ident;
};


static void
resolve_osource_input (gpointer     data,
		       BseStorage  *storage,
		       BseItem     *from_item,
		       BseItem     *to_item,
		       const gchar *error)
{
  DeferredInput *dinput = (DeferredInput*) data;
  BseSource *source = BSE_SOURCE (from_item);
  BseSource *osource = to_item ? BSE_SOURCE (to_item) : NULL;

  if (error)
    bse_storage_warn (storage,
		      "failed to connect input \"%s\" of `%s' to output \"%s\" of unresolved object: %s",
		      dinput->ichannel_ident ? dinput->ichannel_ident : "???",
		      BSE_OBJECT_UNAME (source),
		      dinput->ochannel_ident ? dinput->ochannel_ident : "???",
		      error);
  else
    {
      Bse::Error cerror;

      if (!osource)
	cerror = Bse::Error::SOURCE_NO_SUCH_MODULE;
      else if (!dinput->ichannel_ident)
	cerror = Bse::Error::SOURCE_NO_SUCH_ICHANNEL;
      else if (!dinput->ochannel_ident)
	cerror = Bse::Error::SOURCE_NO_SUCH_OCHANNEL;
      else if (BSE_SOURCE_PRIVATE_INPUTS (source))
        cerror = Bse::Error::SOURCE_PRIVATE_ICHANNEL;
      else
        {
          const gchar *itype = bse_storage_item_get_compat_type (BSE_ITEM (source));
          const gchar *otype = bse_storage_item_get_compat_type (BSE_ITEM (osource));
          gchar *compat_ichannel_ident = bse_compat_rewrite_ichannel_ident (storage, itype, dinput->ichannel_ident);
          gchar *compat_ochannel_ident = bse_compat_rewrite_ochannel_ident (storage, otype, dinput->ochannel_ident);
          guint ichannel = bse_source_find_ichannel (source, compat_ichannel_ident ? compat_ichannel_ident : dinput->ichannel_ident);
          guint ochannel = bse_source_find_ochannel (osource, compat_ochannel_ident ? compat_ochannel_ident : dinput->ochannel_ident);
          g_free (compat_ichannel_ident);
          g_free (compat_ochannel_ident);
          cerror = bse_source_set_input (source, ichannel, osource, ochannel);
        }
      if (cerror != 0)
	bse_storage_warn (storage,
			  "failed to connect input \"%s\" of `%s' to output \"%s\" of `%s': %s",
			  dinput->ichannel_ident ? dinput->ichannel_ident : "???",
			  BSE_OBJECT_UNAME (source),
			  dinput->ochannel_ident ? dinput->ochannel_ident : "???",
			  osource ? BSE_OBJECT_UNAME (osource) : ":<NULL>:",
			  bse_error_blurb (cerror));
    }

  g_free (dinput->ichannel_ident);
  g_free (dinput->osource_path);
  g_free (dinput->ochannel_ident);
  g_free (dinput);
}

static GTokenType
bse_source_restore_private (BseObject  *object,
			    BseStorage *storage,
                            GScanner   *scanner)
{
  BseSource *source = BSE_SOURCE (object);

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
      bse_string_equals ("source-input", scanner->next_value.v_identifier))
    {
      DeferredInput *dinput;

      parse_or_return (scanner, G_TOKEN_IDENTIFIER);	/* eat identifier */

      /* parse ichannel name */
      parse_or_return (scanner, G_TOKEN_STRING);
      dinput = g_new0 (DeferredInput, 1);
      dinput->ichannel_ident = g_strdup (scanner->value.v_string);

      /* parse osource upath and queue handler */
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)	/* bad, bad, hack */
	{
	  dinput->osource_path = g_strdup (scanner->next_value.v_identifier);
	  /* error = */ bse_storage_parse_item_link (storage, BSE_ITEM (source), resolve_osource_input, dinput);
	  bse_storage_warn (storage, "deprecated syntax: non-string uname path: %s", dinput->osource_path); // FIXME
	}
      else
	{
	  GTokenType token = bse_storage_parse_item_link (storage, BSE_ITEM (source), resolve_osource_input, dinput);
	  if (token != G_TOKEN_NONE)
	    return token;
	}

      /* parse ochannel name */
      parse_or_return (scanner, G_TOKEN_STRING);
      peek_or_return (scanner, ')');
      dinput->ochannel_ident = g_strdup (scanner->value.v_string);

      /* close statement */
      parse_or_return (scanner, ')');
      return G_TOKEN_NONE;
    }
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);
}

static void
bse_source_class_base_init (BseSourceClass *klass)
{
  /* reset ichannel, jchannel and ochannel defs */
  klass->channel_defs.n_ichannels = 0;
  klass->channel_defs.ichannel_idents = NULL;
  klass->channel_defs.ichannel_labels = NULL;
  klass->channel_defs.ichannel_blurbs = NULL;
  klass->channel_defs.ijstreams = NULL;
  klass->channel_defs.n_jstreams = 0;
  klass->channel_defs.n_ochannels = 0;
  klass->channel_defs.ochannel_idents = NULL;
  klass->channel_defs.ochannel_labels = NULL;
  klass->channel_defs.ochannel_blurbs = NULL;
  /* reset other class members */
  klass->property_updated = NULL;
  klass->engine_class = NULL;
  klass->filtered_properties = FALSE;
  klass->unprepared_properties = NULL;
  klass->automation_properties = NULL;
}

Bse::MusicalTuning
bse_source_prepared_musical_tuning (BseSource *source)
{
  assert_return (BSE_IS_SOURCE (source), Bse::MusicalTuning::OD_12_TET);
  assert_return (BSE_SOURCE_PREPARED (source), Bse::MusicalTuning::OD_12_TET);
  return bse_item_current_musical_tuning (BSE_ITEM (source)); // FIXME: cache this, once we have source->SynthesisContext
}

void
bse_source_class_inherit_channels (BseSourceClass *source_class)
{
  assert_return (BSE_IS_SOURCE_CLASS (source_class));
  BseSourceClass *parent_class = (BseSourceClass*) g_type_class_peek_parent (source_class);
  assert_return (BSE_IS_SOURCE_CLASS (parent_class));
  assert_return (source_class->channel_defs.n_ichannels == 0);
  assert_return (source_class->channel_defs.n_ochannels == 0);
  assert_return (source_class->channel_defs.n_jstreams == 0);
  assert_return (source_class->engine_class == 0);
  BseSourceChannelDefs *defs = &parent_class->channel_defs;
  guint i;
  /* dup ichannel arrays */
  source_class->channel_defs.n_ichannels = defs->n_ichannels;
  source_class->channel_defs.ichannel_idents = (char**) g_memdup (defs->ichannel_idents, sizeof (gchar*) * defs->n_ichannels);
  source_class->channel_defs.ichannel_labels = (char**) g_memdup (defs->ichannel_labels, sizeof (gchar*) * defs->n_ichannels);
  source_class->channel_defs.ichannel_blurbs = (char**) g_memdup (defs->ichannel_blurbs, sizeof (gchar*) * defs->n_ichannels);
  /* dup ichannel array contents */
  for (i = 0; i < defs->n_ichannels; i++)
    {
      source_class->channel_defs.ichannel_idents[i] = g_strdup (defs->ichannel_idents[i]);
      source_class->channel_defs.ichannel_labels[i] = g_strdup (defs->ichannel_labels[i]);
      source_class->channel_defs.ichannel_blurbs[i] = g_strdup (defs->ichannel_blurbs[i]);
    }
  /* dup jstream ids + jchannel flags */
  source_class->channel_defs.ijstreams = (guint*) g_memdup (defs->ijstreams, sizeof (guint) * defs->n_ichannels);
  source_class->channel_defs.n_jstreams = defs->n_jstreams;
  /* dup ochannel arrays */
  source_class->channel_defs.n_ochannels = defs->n_ochannels;
  source_class->channel_defs.ochannel_idents = (char**) g_memdup (defs->ochannel_idents, sizeof (gchar*) * defs->n_ochannels);
  source_class->channel_defs.ochannel_labels = (char**) g_memdup (defs->ochannel_labels, sizeof (gchar*) * defs->n_ochannels);
  source_class->channel_defs.ochannel_blurbs = (char**) g_memdup (defs->ochannel_blurbs, sizeof (gchar*) * defs->n_ochannels);
  /* dup ochannel array contents */
  for (i = 0; i < defs->n_ochannels; i++)
    {
      source_class->channel_defs.ochannel_idents[i] = g_strdup (defs->ochannel_idents[i]);
      source_class->channel_defs.ochannel_labels[i] = g_strdup (defs->ochannel_labels[i]);
      source_class->channel_defs.ochannel_blurbs[i] = g_strdup (defs->ochannel_blurbs[i]);
    }
}

static void
bse_source_class_base_finalize (BseSourceClass *klass)
{
  guint i;

  for (i = 0; i < klass->channel_defs.n_ichannels; i++)
    {
      g_free (klass->channel_defs.ichannel_idents[i]);
      g_free (klass->channel_defs.ichannel_labels[i]);
      g_free (klass->channel_defs.ichannel_blurbs[i]);
    }
  g_free (klass->channel_defs.ichannel_idents);
  g_free (klass->channel_defs.ichannel_labels);
  g_free (klass->channel_defs.ichannel_blurbs);
  g_free (klass->channel_defs.ijstreams);
  klass->channel_defs.n_jstreams = 0;
  klass->channel_defs.n_ichannels = 0;
  klass->channel_defs.ichannel_idents = NULL;
  klass->channel_defs.ichannel_labels = NULL;
  klass->channel_defs.ichannel_blurbs = NULL;
  klass->channel_defs.ijstreams = NULL;
  for (i = 0; i < klass->channel_defs.n_ochannels; i++)
    {
      g_free (klass->channel_defs.ochannel_idents[i]);
      g_free (klass->channel_defs.ochannel_labels[i]);
      g_free (klass->channel_defs.ochannel_blurbs[i]);
    }
  g_free (klass->channel_defs.ochannel_idents);
  g_free (klass->channel_defs.ochannel_labels);
  g_free (klass->channel_defs.ochannel_blurbs);
  klass->channel_defs.n_ochannels = 0;
  klass->channel_defs.ochannel_idents = NULL;
  klass->channel_defs.ochannel_labels = NULL;
  klass->channel_defs.ochannel_blurbs = NULL;
  g_free (klass->engine_class);
  klass->engine_class = NULL;
  sfi_ring_free (klass->unprepared_properties);
  sfi_ring_free (klass->automation_properties);
}

static void
bse_source_class_init (BseSourceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);
  gobject_class->set_property = bse_source_set_property;
  gobject_class->get_property = bse_source_get_property;
  gobject_class->dispose = bse_source_dispose;
  gobject_class->finalize = bse_source_finalize;

  object_class->check_pspec_editable = source_check_pspec_editable;
  object_class->store_private = bse_source_real_store_private;
  object_class->restore_private = bse_source_restore_private;

  klass->prepare = bse_source_real_prepare;
  klass->context_create = bse_source_real_context_create;
  klass->context_connect = bse_source_real_context_connect;
  klass->context_dismiss = bse_source_real_context_dismiss;
  klass->reset = bse_source_real_reset;
  klass->add_input = bse_source_real_add_input;
  klass->remove_input = bse_source_real_remove_input;
}

BSE_BUILTIN_TYPE (BseSource)
{
  static const GTypeInfo source_info = {
    sizeof (BseSourceClass),

    (GBaseInitFunc) bse_source_class_base_init,
    (GBaseFinalizeFunc) bse_source_class_base_finalize,
    (GClassInitFunc) bse_source_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseSource),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_source_init,
  };

  assert_return (BSE_SOURCE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT, 0);

  return bse_type_register_abstract (BSE_TYPE_ITEM,
                                     "BseSource",
                                     "Base type for sound sources",
                                     __FILE__, __LINE__,
                                     &source_info);
}

namespace Bse {

SourceImpl::SourceImpl (BseObject *bobj) :
  ItemImpl (bobj)
{}

SourceImpl::~SourceImpl ()
{
  cmon_delete();        // delete cmons_ and cmon_block_
}

double
SourceImpl::pos_x() const
{
  return pos_x_;
}

void
SourceImpl::pos_x (double x)
{
  APPLY_IDL_PROPERTY (pos_x_, x);
}

double
SourceImpl::pos_y() const
{
  return pos_y_;
}

void
SourceImpl::pos_y (double y)
{
  APPLY_IDL_PROPERTY (pos_y_, y);
}

bool
SourceImpl::prepared() const
{
  BseObject *bseobject = const_cast<SourceImpl*> (this)->as_bse_object();
  return BSE_SOURCE_PREPARED (bseobject);
}

SourceIfaceP
SourceImpl::ichannel_get_osource (int input_channel, int input_joint)
{
  BseSource *self = as<BseSource*>();
  BseSourceInput *input = BSE_SOURCE_INPUT (self, input_channel);
  BseSource *osource;
  if (BSE_SOURCE_IS_JOINT_ICHANNEL (self, input_channel) && uint (input_joint) < input->jdata.n_joints)
    osource = input->jdata.joints[input_joint].osource;
  else if (input_joint < 1)
    osource = input->idata.osource;
  else
    osource = NULL;
  return osource ? osource->as<SourceIfaceP>() : NULL;
}

bool
SourceImpl::has_output (int ochannel)
{
  BseSource *self = as<BseSource*>();
  return bse_source_has_output (self, ochannel);
}

bool
SourceImpl::has_outputs ()
{
  BseSource *self = as<BseSource*>();
  return self->outputs != NULL;
}

String
SourceImpl::ichannel_blurb (int input_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), "");
  return BSE_SOURCE_ICHANNEL_BLURB (self, input_channel);
}

int
SourceImpl::ichannel_get_n_joints (int input_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), 0);
  BseSourceInput *input = BSE_SOURCE_INPUT (self, input_channel);
  if (BSE_SOURCE_IS_JOINT_ICHANNEL (self, input_channel))
    return input->jdata.n_joints;
  else
    return input->idata.osource ? 1 : 0;
}


int
SourceImpl::ichannel_get_ochannel (int input_channel, int input_joint)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), INT_MAX);
  assert_return (input_joint >= 0, INT_MAX);
  BseSourceInput *input = BSE_SOURCE_INPUT (self, input_channel);
  if (BSE_SOURCE_IS_JOINT_ICHANNEL (self, input_channel) && size_t (input_joint) < input->jdata.n_joints)
    return input->jdata.joints[input_joint].ochannel;
  else if (input_joint < 1)
    return input->idata.ochannel;
  else
    return INT_MAX;
}

String
SourceImpl::ichannel_ident (int input_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), "");
  return BSE_SOURCE_ICHANNEL_IDENT (self, input_channel);
}


String
SourceImpl::ichannel_label (int input_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), "");
  return BSE_SOURCE_ICHANNEL_LABEL (self, input_channel);
}

bool
SourceImpl::is_joint_ichannel (const String &input_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (input_channel.empty() == false, false);
  const uint id = bse_source_find_ichannel (self, input_channel.c_str());
  return id < BSE_SOURCE_N_ICHANNELS (self) ? BSE_SOURCE_IS_JOINT_ICHANNEL (self, id) : false;
}

bool
SourceImpl::is_joint_ichannel_by_id (int input_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), false);
  return BSE_SOURCE_IS_JOINT_ICHANNEL (self, input_channel);
}

bool
SourceImpl::is_prepared ()
{
  BseSource *self = as<BseSource*>();
  return BSE_SOURCE_PREPARED (self);
}

int
SourceImpl::n_ichannels()
{
  BseSource *self = as<BseSource*>();
  return BSE_SOURCE_N_ICHANNELS (self);
}

int
SourceImpl::n_ochannels()
{
  BseSource *self = as<BseSource*>();
  return BSE_SOURCE_N_OCHANNELS (self);
}

String
SourceImpl::ochannel_blurb (int output_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (output_channel) < BSE_SOURCE_N_OCHANNELS (self), "");
  return BSE_SOURCE_OCHANNEL_BLURB (self, output_channel);
}

String
SourceImpl::ochannel_ident (int output_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (output_channel) < BSE_SOURCE_N_OCHANNELS (self), "");
  return BSE_SOURCE_OCHANNEL_IDENT (self, output_channel);
}

String
SourceImpl::ochannel_label (int output_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (output_channel) < BSE_SOURCE_N_OCHANNELS (self), "");
  return BSE_SOURCE_OCHANNEL_LABEL (self, output_channel);
}

Error
SourceImpl::set_input (const String &ichannel_ident, SourceIface &omodule, const String &ochannel_ident)
{
  BseSource *self = as<BseSource*>();
  assert_return (ichannel_ident.empty() == false, Bse::Error::PROC_PARAM_INVAL);
  BseSource *osource = omodule.as<BseSource*>();
  assert_return (osource != NULL, Bse::Error::PROC_PARAM_INVAL);
  assert_return (ochannel_ident.empty() == false, Bse::Error::PROC_PARAM_INVAL);
  const size_t input_channel = bse_source_find_ichannel (self, ichannel_ident.c_str());
  assert_return (input_channel < BSE_SOURCE_N_ICHANNELS (self), Bse::Error::PROC_PARAM_INVAL);
  const size_t output_channel = bse_source_find_ochannel (osource, ochannel_ident.c_str());
  assert_return (output_channel < BSE_SOURCE_N_OCHANNELS (osource), Bse::Error::PROC_PARAM_INVAL);
  if (self->parent != osource->parent)
    return Bse::Error::PROC_PARAM_INVAL;
  Bse::Error error = bse_source_set_input (self, input_channel, osource, output_channel);
  if (error == Bse::Error::NONE)
    {
      UndoDescriptor<SourceImpl> omodule_descriptor = undo_descriptor (*osource->as<SourceImpl*>());
      auto lambda = [input_channel, omodule_descriptor, output_channel] (SourceImpl &self, BseUndoStack *ustack) -> Error {
        SourceImpl &omodule = self.undo_resolve (omodule_descriptor);
        Bse::Error error = self.unset_input_by_id (input_channel, omodule, output_channel);
        return error;
      };
      push_undo (__func__, *this, lambda);
    }
  return error;
}

Error
SourceImpl::set_input_by_id (int input_channel, SourceIface &omodule, int output_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), Bse::Error::PROC_PARAM_INVAL);
  BseSource *osource = omodule.as<BseSource*>();
  assert_return (osource != NULL, Bse::Error::PROC_PARAM_INVAL);
  assert_return (size_t (output_channel) < BSE_SOURCE_N_OCHANNELS (osource), Bse::Error::PROC_PARAM_INVAL);
  if (self->parent != osource->parent)
    return Bse::Error::PROC_PARAM_INVAL;
  Bse::Error error = bse_source_set_input (self, input_channel, osource, output_channel);
  if (error == Bse::Error::NONE)
    {
      UndoDescriptor<SourceImpl> omodule_descriptor = undo_descriptor (*osource->as<SourceImpl*>());
      auto lambda = [input_channel, omodule_descriptor, output_channel] (SourceImpl &self, BseUndoStack *ustack) -> Error {
        SourceImpl &omodule = self.undo_resolve (omodule_descriptor);
        Bse::Error error = self.unset_input_by_id (input_channel, omodule, output_channel);
        return error;
      };
      push_undo (__func__, *this, lambda);
    }
  return error;
}

Error
SourceImpl::unset_input (const String &ichannel_ident, SourceIface &omodule, const String &ochannel_ident)
{
  BseSource *self = as<BseSource*>();
  assert_return (ichannel_ident.empty() == false, Bse::Error::PROC_PARAM_INVAL);
  BseSource *osource = omodule.as<BseSource*>();
  assert_return (osource != NULL, Bse::Error::PROC_PARAM_INVAL);
  assert_return (ochannel_ident.empty() == false, Bse::Error::PROC_PARAM_INVAL);
  const size_t input_channel = bse_source_find_ichannel (self, ichannel_ident.c_str());
  assert_return (input_channel < BSE_SOURCE_N_ICHANNELS (self), Bse::Error::PROC_PARAM_INVAL);
  const size_t output_channel = bse_source_find_ochannel (osource, ochannel_ident.c_str());
  assert_return (output_channel < BSE_SOURCE_N_OCHANNELS (osource), Bse::Error::PROC_PARAM_INVAL);
  Bse::Error error = bse_source_check_input (self, input_channel, osource, output_channel);
  if (error == Bse::Error::NONE)
    {
      BseUndoStack *ustack = bse_item_undo_open (self, "unset-input-by-id");
      bse_source_input_backup_to_undo (self, input_channel, osource, output_channel);
      UndoDescriptor<SourceImpl> omodule_descriptor = undo_descriptor (*osource->as<SourceImpl*>());
      auto lambda = [input_channel, omodule_descriptor, output_channel] (SourceImpl &self, BseUndoStack *ustack) -> Error {
        SourceImpl &omodule = self.undo_resolve (omodule_descriptor);
        Bse::Error error = self.unset_input_by_id (input_channel, omodule, output_channel);
        return error;
      };
      error = bse_source_unset_input (self, input_channel, osource, output_channel);
      push_undo_to_redo (__func__, *this, lambda);
      bse_item_undo_close (ustack);
    }
  return error;
}

Error
SourceImpl::unset_input_by_id (int input_channel, SourceIface &omodule, int output_channel)
{
  BseSource *self = as<BseSource*>();
  assert_return (size_t (input_channel) < BSE_SOURCE_N_ICHANNELS (self), Bse::Error::PROC_PARAM_INVAL);
  BseSource *osource = omodule.as<BseSource*>();
  assert_return (osource != NULL, Bse::Error::PROC_PARAM_INVAL);
  assert_return (size_t (output_channel) < BSE_SOURCE_N_OCHANNELS (osource), Bse::Error::PROC_PARAM_INVAL);
  Bse::Error error = bse_source_check_input (self, input_channel, osource, output_channel);
  if (error == Bse::Error::NONE)
    {
      BseUndoStack *ustack = bse_item_undo_open (self, "unset-input-by-id");
      bse_source_input_backup_to_undo (self, input_channel, osource, output_channel);
      UndoDescriptor<SourceImpl> omodule_descriptor = undo_descriptor (*osource->as<SourceImpl*>());
      auto lambda = [input_channel, omodule_descriptor, output_channel] (SourceImpl &self, BseUndoStack *ustack) -> Error {
        SourceImpl &omodule = self.undo_resolve (omodule_descriptor);
        Bse::Error error = self.unset_input_by_id (input_channel, omodule, output_channel);
        return error;
      };
      error = bse_source_unset_input (self, input_channel, osource, output_channel);
      push_undo_to_redo (__func__, *this, lambda);
      bse_item_undo_close (ustack);
    }
  return error;
}

void
SourceImpl::set_pos (double x_pos, double y_pos)
{
  BseSource *self = as<BseSource*>();
  auto impl = self->as<Bse::SourceImpl*>();
  const double epsilon = 1e-5;

  if (fabs (x_pos - impl->pos_x()) > epsilon ||
      fabs (y_pos - impl->pos_y()) > epsilon)
    {
      BseUndoStack *ustack = bse_item_undo_open (self, "set-xy-pos");
      impl->pos_x (x_pos);
      impl->pos_y (y_pos);
      bse_item_undo_close (ustack);
    }
}

int32
SourceImpl::get_automation_channel (const String &property_name)
{
  BseSource *self = as<BseSource*>();
  uint midi_channel = 0;
  bse_source_get_automation_property (self, property_name.c_str(), &midi_channel, NULL);
  return midi_channel;
}

MidiControl
SourceImpl::get_automation_control (const String &property_name)
{
  BseSource *self = as<BseSource*>();
  MidiSignal control_type = MidiSignal (0);
  bse_source_get_automation_property (self, property_name.c_str(), NULL, &control_type);
  return MidiControl (control_type);                            // MidiControl values are identical to MidiSignal valus
}

Error
SourceImpl::set_automation (const String &property_name, int midi_channel, MidiControl control_type)
{
  BseSource *self = as<BseSource*>();
  MidiSignal control_signal = MidiSignal (control_type);        // MidiControl values are identical to MidiSignal valuas
  if (midi_channel < 0 || property_name.empty())
    return Error::PROC_PARAM_INVAL;
  uint old_midi_channel = 0;
  MidiSignal old_control_type = MidiSignal (0);
  bse_source_get_automation_property (self, property_name.c_str(), &old_midi_channel, &old_control_type);
  Error error = Error::NONE;
  if (old_midi_channel != uint (midi_channel) || old_control_type != uint (control_type))
    {
      error = bse_source_set_automation_property (self, property_name.c_str(), midi_channel, control_signal);
      if (error == Error::NONE)
        {
          auto lambda = [property_name, old_midi_channel, old_control_type] (SourceImpl &self, BseUndoStack *ustack) -> Error {
            return self.set_automation (property_name, old_midi_channel, MidiControl (old_control_type));
          };
          push_undo (__func__, *this, lambda);
        }
    }
  return error;
}

void
SourceImpl::clear_inputs ()
{
  BseSource *self = as<BseSource*>();
  g_object_ref (self);
  BseUndoStack *ustack = bse_item_undo_open (self, "%s", __func__);
  for (size_t i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (self, i);
      if (BSE_SOURCE_IS_JOINT_ICHANNEL (self, i))
        while (input->jdata.n_joints)
          {
            BseSource *osource = input->jdata.joints[0].osource;
            uint ochannel = input->jdata.joints[0].ochannel;
            unset_input_by_id (i, *osource->as<SourceIfaceP>(), ochannel);
          }
      else if (input->idata.osource)
	{
	  BseSource *osource = input->idata.osource;
          unset_input_by_id (i, *osource->as<SourceIfaceP>(), input->idata.ochannel);
	}
    }
  bse_item_undo_close (ustack);
  g_object_unref (self);
}

void
SourceImpl::clear_outputs ()
{
  BseSource *self = as<BseSource*>();
  g_object_ref (self);
  BseUndoStack *ustack = bse_item_undo_open (self, "%s", __func__);
  while (self->outputs)
    {
      BseSource *isource = static_cast<BseSource*> (self->outputs->data);
      g_object_ref (isource);
      for (uint i = 0; i < BSE_SOURCE_N_ICHANNELS (isource); i++)
	{
	  BseSourceInput *input = BSE_SOURCE_INPUT (isource, i);
	  if (BSE_SOURCE_IS_JOINT_ICHANNEL (isource, i))
	    {
	      uint j;
	      for (j = 0; j < input->jdata.n_joints; j++)
		if (input->jdata.joints[j].osource == self)
		  break;
	      if (j < input->jdata.n_joints)
                isource->as<SourceIfaceP>()->unset_input_by_id (i, *this, input->jdata.joints[j].ochannel);
	    }
	  else if (input->idata.osource == self)
            isource->as<SourceIfaceP>()->unset_input_by_id (i, *this, input->idata.ochannel);
	}
      g_object_unref (isource);
    }
  bse_item_undo_close (ustack);
  g_object_unref (self);
}

int
SourceImpl::get_mix_freq ()
{
  BseSource *self = as<BseSource*>();
  return BSE_SOURCE_PREPARED (self) ? bse_engine_sample_freq() : 0;
}

} // Bse
