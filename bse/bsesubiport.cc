// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesubiport.hh"

#include "bsecategories.hh"
#include "bsesnet.hh"

#include <string.h>

/* --- parameters --- */
enum
{
  PROP_0,
  /* don't add properties after here */
  PROP_IPORT_NAME
};


/* --- prototypes --- */
static void      bse_sub_iport_update_modules   (BseSubIPort            *self,
                                                 const gchar            *old_name,
                                                 const gchar            *new_name,
                                                 guint                   port);


/* --- variables --- */
static gpointer          parent_class = NULL;


/* --- functions --- */
static void
bse_sub_iport_init (BseSubIPort *self)
{
  guint i;
  self->input_ports = g_new (gchar*, BSE_SOURCE_N_OCHANNELS (self));
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (self); i++)
    self->input_ports[i] = g_strdup_printf ("synth_in_%u", i + 1);
}

static void
bse_sub_iport_finalize (GObject *object)
{
  BseSubIPort *self = BSE_SUB_IPORT (object);
  guint i;
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (self); i++)
    g_free (self->input_ports[i]);
  g_free (self->input_ports);
  self->input_ports = NULL;
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_sub_iport_set_property (GObject      *object,
                            guint         param_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BseSubIPort *self = BSE_SUB_IPORT (object);
  BseItem *item = BSE_ITEM (self);
  
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PROP_IPORT_NAME) % 2 + PROP_IPORT_NAME;
      n = (param_id - PROP_IPORT_NAME) / 2;
      switch (indx)
        {
        case PROP_IPORT_NAME:
          if (n < BSE_SOURCE_N_OCHANNELS (self))
            {
              const gchar *name = g_value_get_string (value);
              if (item->parent)
                {
                  bse_snet_iport_name_unregister (BSE_SNET (item->parent), self->input_ports[n]);
                  name = bse_snet_iport_name_register (BSE_SNET (item->parent), name);
                }
              if (BSE_SOURCE_PREPARED (self))
                bse_sub_iport_update_modules (self, self->input_ports[n], name, n);
              g_free (self->input_ports[n]);
              self->input_ports[n] = g_strdup (name);
            }
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
          break;
        }
    }
}

static void
bse_sub_iport_get_property (GObject     *object,
                            guint        param_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  BseSubIPort *self = BSE_SUB_IPORT (object);
  
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PROP_IPORT_NAME) % 2 + PROP_IPORT_NAME;
      n = (param_id - PROP_IPORT_NAME) / 2;
      switch (indx)
        {
        case PROP_IPORT_NAME:
          if (n < BSE_SOURCE_N_OCHANNELS (self))
            g_value_set_string (value, self->input_ports[n]);
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
          break;
        }
      break;
    }
}

static void
bse_sub_iport_set_parent (BseItem *item,
                          BseItem *parent)
{
  BseSubIPort *self = BSE_SUB_IPORT (item);
  guint i;
  
  /* remove port name from old parent */
  if (item->parent)
    for (i = 0; i < BSE_SOURCE_N_OCHANNELS (self); i++)
      bse_snet_iport_name_unregister (BSE_SNET (item->parent), self->input_ports[i]);
  
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  /* add port name to new parent */
  if (item->parent)
    for (i = 0; i < BSE_SOURCE_N_OCHANNELS (self); i++)
      {
        const gchar *name = bse_snet_iport_name_register (BSE_SNET (item->parent), self->input_ports[i]);
        if (strcmp (name, self->input_ports[i]) != 0)
          {
            g_free (self->input_ports[i]);
            self->input_ports[i] = g_strdup (name);
            gchar *string = g_strdup_printf ("in_port_%u", i + 1);
            g_object_notify (G_OBJECT (item), string);
            g_free (string);
          }
      }
}

static void
sub_iport_process (BseModule *module,
                   guint      n_values)
{
  guint i, n = BSE_MODULE_N_OSTREAMS (module);

  for (i = 0; i < n; i++)
    BSE_MODULE_OBUFFER (module, i) = (gfloat*) BSE_MODULE_IBUFFER (module, i);
}

static void
bse_sub_iport_context_create (BseSource *source,
                              guint      context_handle,
                              BseTrans  *trans)
{
  BseSubIPort *self = BSE_SUB_IPORT (source);

  if (!BSE_SOURCE_GET_CLASS (self)->engine_class)
    {
      BseModuleClass module_class = {
        BSE_SOURCE_N_OCHANNELS (self),  // n_istreams
        0,                              // n_jstreams
        BSE_SOURCE_N_OCHANNELS (self),  // n_ostreams
        sub_iport_process,              // process
        NULL,                           // process_defer
        NULL,                           // reset
        NULL,                           // free
        BSE_COST_CHEAP,                 // mflags
      };
      bse_source_class_cache_engine_class (BSE_SOURCE_GET_CLASS (self), &module_class);
    }
  BseModule *module = bse_module_new (BSE_SOURCE_GET_CLASS (self)->engine_class, NULL);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_omodule (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_sub_iport_context_connect (BseSource *source,
                               guint      context_handle,
                               BseTrans  *trans)
{
  BseSubIPort *self = BSE_SUB_IPORT (source);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  BseModule *module = bse_source_get_context_omodule (source, context_handle);
  guint i;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (self); i++)
    bse_snet_set_iport_dest (snet, self->input_ports[i], context_handle, module, i, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_sub_iport_context_dismiss (BseSource *source,
                               guint      context_handle,
                               BseTrans  *trans)
{
  BseSubIPort *self = BSE_SUB_IPORT (source);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  guint i;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (self); i++)
    bse_snet_set_iport_dest (snet, self->input_ports[i], context_handle, NULL, i, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_sub_iport_update_modules (BseSubIPort   *self,
                              const gchar *old_name,
                              const gchar *new_name,
                              guint        port)
{
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  BseSource *source = BSE_SOURCE (self);
  BseTrans *trans = bse_trans_open ();
  guint *cids, n, i;
  
  g_return_if_fail (BSE_SOURCE_PREPARED (self));
  
  cids = bse_source_context_ids (source, &n);
  for (i = 0; i < n; i++)
    {
      BseModule *module = bse_source_get_context_omodule (source, cids[i]);
      bse_snet_set_iport_dest (snet, old_name, cids[i], NULL, port, trans);
      bse_snet_set_iport_dest (snet, new_name, cids[i], module, port, trans);
    }
  g_free (cids);
  bse_trans_commit (trans);
}

static void
bse_sub_iport_class_init (BseSubIPortClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint i, channel_id;
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->set_property = bse_sub_iport_set_property;
  gobject_class->get_property = bse_sub_iport_get_property;
  gobject_class->finalize = bse_sub_iport_finalize;
  
  item_class->set_parent = bse_sub_iport_set_parent;
  
  source_class->context_create = bse_sub_iport_context_create;
  source_class->context_connect = bse_sub_iport_context_connect;
  source_class->context_dismiss = bse_sub_iport_context_dismiss;

  for (i = 0; i < BSE_SUB_IPORT_N_PORTS; i++)
    {
      gchar *ident, *label, *value;
      
      ident = g_strdup_printf ("output-%u", i + 1);
      label = g_strdup_printf (_("Virtual input %u"), i + 1);
      channel_id = bse_source_class_add_ochannel (source_class, ident, label, NULL);
      g_assert (channel_id == i);
      g_free (ident);
      g_free (label);
      
      ident = g_strdup_printf ("in_port_%u", i + 1);
      label = g_strdup_printf (_("Input Port %u"), i + 1);
      value = g_strdup_printf ("synth_in_%u", i + 1);
      bse_object_class_add_param (object_class, _("Assignments"), PROP_IPORT_NAME + i * 2,
                                  sfi_pspec_string (ident, label,
                                                    _("The port name is a unique name to establish input<->output "
                                                      "port relationships"),
                                                    value, SFI_PARAM_STANDARD ":skip-default"));
      g_free (ident);
      g_free (label);
      g_free (value);
    }
}

BSE_BUILTIN_TYPE (BseSubIPort)
{
  static const GTypeInfo type_info = {
    sizeof (BseSubIPortClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sub_iport_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSubIPort),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_sub_iport_init,
  };
#include "./icons/virtual-input.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BseSubIPort",
                                         "Virtual input port connector, used to provide a synthesis network "
                                         "with input signals from other synthesis networks",
                                         __FILE__, __LINE__,
                                         &type_info);
  bse_categories_register_stock_module (N_("/Virtualization/Virtual Input"), type, virtual_input_pixstream);
  return type;
}
