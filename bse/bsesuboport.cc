// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsesuboport.hh"
#include "bsecategories.hh"
#include "bsesnet.hh"
#include <string.h>
/* --- parameters --- */
enum
{
  PROP_0,
  /* don't add properties after here */
  PROP_OPORT_NAME
};
/* --- prototypes --- */
static void      bse_sub_oport_update_modules   (BseSubOPort            *self,
                                                 const gchar            *old_name,
                                                 const gchar            *new_name,
                                                 guint                   port);
/* --- variables --- */
static gpointer          parent_class = NULL;
/* --- functions --- */
static void
bse_sub_oport_init (BseSubOPort *self)
{
  guint i;
  self->output_ports = g_new (gchar*, BSE_SOURCE_N_ICHANNELS (self));
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
    self->output_ports[i] = g_strdup_printf ("synth_out_%u", i + 1);
}
static void
bse_sub_oport_finalize (GObject *object)
{
  BseSubOPort *self = BSE_SUB_OPORT (object);
  guint i;
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
    g_free (self->output_ports[i]);
  g_free (self->output_ports);
  self->output_ports = NULL;
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}
static void
bse_sub_oport_set_property (GObject      *object,
                            guint         param_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BseSubOPort *self = BSE_SUB_OPORT (object);
  BseItem *item = BSE_ITEM (self);
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PROP_OPORT_NAME) % 2 + PROP_OPORT_NAME;
      n = (param_id - PROP_OPORT_NAME) / 2;
      switch (indx)
        {
        case PROP_OPORT_NAME:
          if (n < BSE_SOURCE_N_ICHANNELS (self))
            {
              const gchar *name = g_value_get_string (value);
              if (item->parent)
                {
                  bse_snet_oport_name_unregister (BSE_SNET (item->parent), self->output_ports[n]);
                  name = bse_snet_oport_name_register (BSE_SNET (item->parent), name);
                }
              if (BSE_SOURCE_PREPARED (self))
                bse_sub_oport_update_modules (self, self->output_ports[n], name, n);
              g_free (self->output_ports[n]);
              self->output_ports[n] = g_strdup (name);
            }
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
          break;
        }
    }
}
static void
bse_sub_oport_get_property (GObject     *object,
                            guint        param_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  BseSubOPort *self = BSE_SUB_OPORT (object);
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PROP_OPORT_NAME) % 2 + PROP_OPORT_NAME;
      n = (param_id - PROP_OPORT_NAME) / 2;
      switch (indx)
        {
        case PROP_OPORT_NAME:
          if (n < BSE_SOURCE_N_ICHANNELS (self))
            g_value_set_string (value, self->output_ports[n]);
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
          break;
        }
      break;
    }
}
static void
bse_sub_oport_set_parent (BseItem *item,
                          BseItem *parent)
{
  BseSubOPort *self = BSE_SUB_OPORT (item);
  guint i;
  /* remove port name from old parent */
  if (item->parent)
    for (i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
      bse_snet_oport_name_unregister (BSE_SNET (item->parent), self->output_ports[i]);
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  /* add port name to new parent */
  if (item->parent)
    for (i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
      {
        const gchar *name = bse_snet_oport_name_register (BSE_SNET (item->parent), self->output_ports[i]);
        if (strcmp (name, self->output_ports[i]) != 0)
          {
            gchar *string;
            g_free (self->output_ports[i]);
            self->output_ports[i] = g_strdup (name);
            string = g_strdup_printf ("out_port_%u", i + 1);
            g_object_notify (G_OBJECT (item), string);
            g_free (string);
          }
      }
}
static void
sub_oport_process (BseModule *module,
                   guint      n_values)
{
  guint i, n = BSE_MODULE_N_ISTREAMS (module);
  for (i = 0; i < n; i++)
    BSE_MODULE_OBUFFER (module, i) = (gfloat*) BSE_MODULE_IBUFFER (module, i);
}
static void
bse_sub_oport_context_create (BseSource *source,
                              guint      context_handle,
                              BseTrans  *trans)
{
  BseSubOPort *self = BSE_SUB_OPORT (source);
  if (!BSE_SOURCE_GET_CLASS (self)->engine_class)
    {
      BseModuleClass module_class = {
        BSE_SOURCE_N_ICHANNELS (self),  // n_istreams
        0,                              // n_jstreams
        BSE_SOURCE_N_ICHANNELS (self),  // n_ostreams
        sub_oport_process,              // process
        NULL,                           // process_defer
        NULL,                           // reset
        NULL,                           // free
        BSE_COST_CHEAP,                 // mflags
      };
      bse_source_class_cache_engine_class (BSE_SOURCE_GET_CLASS (self), &module_class);
    }
  BseModule *module = bse_module_new (BSE_SOURCE_GET_CLASS (self)->engine_class, NULL);
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, module);
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}
static void
bse_sub_oport_context_connect (BseSource *source,
                               guint      context_handle,
                               BseTrans  *trans)
{
  BseSubOPort *self = BSE_SUB_OPORT (source);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  BseModule *module = bse_source_get_context_imodule (source, context_handle);
  guint i;
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
    bse_snet_set_oport_src (snet, self->output_ports[i], context_handle, module, i, trans);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}
static void
bse_sub_oport_context_dismiss (BseSource *source,
                               guint      context_handle,
                               BseTrans  *trans)
{
  BseSubOPort *self = BSE_SUB_OPORT (source);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  guint i;
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (self); i++)
    bse_snet_set_oport_src (snet, self->output_ports[i], context_handle, NULL, i, trans);
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}
static void
bse_sub_oport_update_modules (BseSubOPort *self,
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
      BseModule *module = bse_source_get_context_imodule (source, cids[i]);
      bse_snet_set_oport_src (snet, old_name, cids[i], NULL, port, trans);
      bse_snet_set_oport_src (snet, new_name, cids[i], module, port, trans);
    }
  g_free (cids);
  bse_trans_commit (trans);
}
static void
bse_sub_oport_class_init (BseSubOPortClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint i, channel_id;
  parent_class = g_type_class_peek_parent (klass);
  gobject_class->set_property = bse_sub_oport_set_property;
  gobject_class->get_property = bse_sub_oport_get_property;
  gobject_class->finalize = bse_sub_oport_finalize;
  item_class->set_parent = bse_sub_oport_set_parent;
  source_class->context_create = bse_sub_oport_context_create;
  source_class->context_connect = bse_sub_oport_context_connect;
  source_class->context_dismiss = bse_sub_oport_context_dismiss;
  for (i = 0; i < BSE_SUB_OPORT_N_PORTS; i++)
    {
      gchar *ident, *label, *value;
      ident = g_strdup_printf ("input-%u", i + 1);
      label = g_strdup_printf (_("Virtual output %u"), i + 1);
      channel_id = bse_source_class_add_ichannel (source_class, ident, label, NULL);
      g_assert (channel_id == i);
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("out_port_%u", i + 1);
      label = g_strdup_printf (_("Output Port %u"), i + 1);
      value = g_strdup_printf ("synth_out_%u", i + 1);
      bse_object_class_add_param (object_class, _("Assignments"), PROP_OPORT_NAME + i * 2,
                                  sfi_pspec_string (ident, label,
						    _("The port name is a unique name to establish input<->output "
                                                      "port relationships"),
						    value, SFI_PARAM_STANDARD ":skip-default"));
      g_free (ident);
      g_free (label);
      g_free (value);
    }
}
BSE_BUILTIN_TYPE (BseSubOPort)
{
  static const GTypeInfo type_info = {
    sizeof (BseSubOPortClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sub_oport_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseSubOPort),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_sub_oport_init,
  };
#include "./icons/virtual-output.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BseSubOPort",
                                         "Virtual output port connector, used to provide a synthesis network "
                                         "with output signals from other synthesis networks",
                                         __FILE__, __LINE__,
                                         &type_info);
  bse_categories_register_stock_module (N_("/Virtualization/Virtual Output"), type, virtual_output_pixstream);
  return type;
}
