/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
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
#include        "bsesnet.h"

#include        "bseproject.h"
#include        "bsecategories.h"
#include        "bsehunkmixer.h"
#include        "bsestorage.h"
#include        <string.h>
#include        <time.h>
#include        <fcntl.h>
#include        <unistd.h>

#include        "./icons/snet.c"

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_AUTO_ACTIVATE
};


/* --- prototypes --- */
static void      bse_snet_class_init             (BseSNetClass   *class);
static void      bse_snet_init                   (BseSNet        *snet);
static void      bse_snet_do_destroy             (BseObject      *object);
static void      bse_snet_set_property           (BseSNet        *snet,
						  guint           param_id,
						  GValue         *value,
						  GParamSpec     *pspec);
static void      bse_snet_get_property           (BseSNet        *snet,
						  guint           param_id,
						  GValue         *value,
						  GParamSpec     *pspec);
static void      bse_snet_add_item               (BseContainer   *container,
						  BseItem        *item);
static void      bse_snet_forall_items           (BseContainer   *container,
						  BseForallItemsFunc func,
						  gpointer        data);
static void      bse_snet_remove_item            (BseContainer   *container,
						  BseItem        *item);
static void      bse_snet_prepare                (BseSource      *source);
static void      bse_snet_reset                  (BseSource      *source);


/* --- variables --- */
static GTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSNet)
{
  GType   snet_type;

  static const GTypeInfo snet_info = {
    sizeof (BseSNetClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_snet_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSNet),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_snet_init,
  };
  
  snet_type = bse_type_register_static (BSE_TYPE_SUPER,
                                        "BseSNet",
                                        "BSE Synthesis (Filter) Network",
                                        &snet_info);
  g_assert (BSE_SNET_FLAGS_USHIFT <= 32);

  return snet_type;
}

static void
bse_snet_class_init (BseSNetClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_snet_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_snet_get_property;

  object_class->destroy = bse_snet_do_destroy;
  
  source_class->prepare = bse_snet_prepare;
  source_class->reset = bse_snet_reset;

  container_class->add_item = bse_snet_add_item;
  container_class->remove_item = bse_snet_remove_item;
  container_class->forall_items = bse_snet_forall_items;
  
  bse_object_class_add_param (object_class, "Playback Settings",
			      PARAM_AUTO_ACTIVATE,
			      g_param_spec_boolean ("auto_activate", "Auto Activate",
						    "Automatic activation only needs to be enabled for synthesis networks "
						    "that don't use virtual ports for their input or output",
						    FALSE, BSE_PARAM_DEFAULT));
}

static void
bse_snet_init (BseSNet *snet)
{
  BSE_OBJECT_SET_FLAGS (snet, BSE_SNET_FLAG_FINAL);
  BSE_SUPER (snet)->auto_activate = FALSE;
  snet->sources = NULL;
}

static void
bse_snet_do_destroy (BseObject *object)
{
  BseSNet *snet = BSE_SNET (object);
  
  while (snet->sources)
    bse_container_remove_item (BSE_CONTAINER (snet), snet->sources->data);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_snet_set_property (BseSNet     *snet,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_AUTO_ACTIVATE:
      BSE_SUPER (snet)->auto_activate = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (snet, param_id, pspec);
      break;
    }
}

static void
bse_snet_get_property (BseSNet     *snet,
		       guint        param_id,
		       GValue      *value,
		       GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_AUTO_ACTIVATE:
      g_value_set_boolean (value, BSE_SUPER (snet)->auto_activate);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (snet, param_id, pspec);
      break;
    }
}

static void
bse_snet_add_item (BseContainer *container,
                   BseItem      *item)
{
  BseSNet *snet = BSE_SNET (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = g_list_append (snet->sources, item);
  else if (BSE_SNET_FINAL (snet))
    g_warning ("BseSNet: cannot hold non-source item type `%s'",
               BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_snet_forall_items (BseContainer      *container,
                       BseForallItemsFunc func,
                       gpointer           data)
{
  BseSNet *snet = BSE_SNET (container);
  GList *list;

  list = snet->sources;
  while (list)
    {
      BseItem *item;

      item = list->data;
      list = list->next;
      if (!func (item, data))
        return;
    }
}

static void
bse_snet_remove_item (BseContainer *container,
                      BseItem      *item)
{
  BseSNet *snet;

  snet = BSE_SNET (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = g_list_remove (snet->sources, item);
  else if (BSE_SNET_FINAL (snet))
    g_warning ("BseSNet: cannot hold non-source item type `%s'",
               BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static BseSNetVPort*
snet_find_port (BseSNet     *snet,
		const gchar *name,
		gboolean     in_port)
{
  guint i;

  if (!in_port)
    for (i = 0; i < snet->n_out_ports; i++)
      if (strcmp (name, snet->out_ports[i].name) == 0)
	return snet->out_ports + i;
  if (in_port)
    for (i = 0; i < snet->n_in_ports; i++)
      if (strcmp (name, snet->in_ports[i].name) == 0)
	return snet->in_ports + i;
  return NULL;
}

const gchar*
bse_snet_add_in_port (BseSNet     *snet,
		      const gchar *tmpl_name,
		      BseSource   *source,
		      guint        ochannel,
		      guint	   module_istream)
{
  BseSNetVPort *vport;
  gchar *name;
  guint i;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (tmpl_name != NULL, NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (ochannel < BSE_SOURCE_N_OCHANNELS (source), NULL);
  g_return_val_if_fail (BSE_ITEM (source)->parent == BSE_ITEM (snet), NULL);
  
  vport = snet_find_port (snet, tmpl_name, TRUE);
  name = NULL;
  i = 1;
  while (vport)
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", tmpl_name, i++);
      vport = snet_find_port (snet, name, TRUE);
    }
  if (!name)
    name = g_strdup (tmpl_name);
  i = snet->n_in_ports++;
  snet->in_ports = g_renew (BseSNetVPort, snet->in_ports, snet->n_in_ports);
  snet->in_ports[i].name = name;
  snet->in_ports[i].source = source;
  snet->in_ports[i].channel = ochannel;
  snet->in_ports[i].module_stream = module_istream;
  
  return name;
}

void
bse_snet_remove_in_port (BseSNet     *snet,
			 const gchar *name)
{
  BseSNetVPort *vport;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  
  vport = snet_find_port (snet, name, TRUE);
  if (vport)
    {
      guint i = vport - snet->in_ports;
      
      g_free (vport->name);
      snet->n_in_ports -= 1;
      g_memmove (snet->in_ports + i, snet->in_ports + i + 1, sizeof (snet->in_ports[0]) * (snet->n_in_ports - i));
    }
  else
    g_return_if_fail (snet_find_port (snet, name, TRUE)  != NULL);
}

const gchar*
bse_snet_add_out_port (BseSNet     *snet,
		       const gchar *tmpl_name,
		       BseSource   *source,
		       guint        ichannel,
		       guint	    module_ostream)
{
  BseSNetVPort *vport;
  gchar *name;
  guint i;

  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (tmpl_name != NULL, NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (ichannel < BSE_SOURCE_N_ICHANNELS (source), NULL);
  g_return_val_if_fail (BSE_ITEM (source)->parent == BSE_ITEM (snet), NULL);

  vport = snet_find_port (snet, tmpl_name, FALSE);
  name = NULL;
  i = 1;
  while (vport)
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", tmpl_name, i++);
      vport = snet_find_port (snet, name, FALSE);
    }
  if (!name)
    name = g_strdup (tmpl_name);
  i = snet->n_out_ports++;
  snet->out_ports = g_renew (BseSNetVPort, snet->out_ports, snet->n_out_ports);
  snet->out_ports[i].name = name;
  snet->out_ports[i].source = source;
  snet->out_ports[i].channel = ichannel;
  snet->out_ports[i].module_stream = module_ostream;

  return name;
}

void
bse_snet_remove_out_port (BseSNet     *snet,
			  const gchar *name)
{
  BseSNetVPort *vport;

  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);

  vport = snet_find_port (snet, name, FALSE);
  if (vport)
    {
      guint i = vport - snet->out_ports;

      g_free (vport->name);
      snet->n_out_ports -= 1;
      g_memmove (snet->out_ports + i, snet->out_ports + i + 1, sizeof (snet->out_ports[0]) * (snet->n_out_ports - i));
    }
  else
    g_return_if_fail (snet_find_port (snet, name, FALSE)  != NULL);
}

GslModule*
bse_snet_get_in_port_module (BseSNet     *snet,
			     const gchar *name,
			     guint        context_handle,
			     guint       *module_istream_p)
{
  BseSNetVPort *vport;
  GslModule *module;

  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (context_handle < BSE_SOURCE (snet)->n_contexts, NULL);

  vport = snet_find_port (snet, name, TRUE);
  if (vport)
    {
      module = bse_source_get_ochannel_module (vport->source, vport->channel, context_handle, NULL);
      *module_istream_p = vport->module_stream;
    }
  else
    {
      module = NULL;
      *module_istream_p = ~0;
    }
  return module;
}

GslModule*
bse_snet_get_out_port_module (BseSNet     *snet,
			      const gchar *name,
			      guint        context_handle,
			      guint       *module_ostream_p)
{
  BseSNetVPort *vport;
  GslModule *module;

  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (context_handle < BSE_SOURCE (snet)->n_contexts, NULL);

  vport = snet_find_port (snet, name, FALSE);
  if (vport)
    {
      module = bse_source_get_ichannel_module (vport->source, vport->channel, context_handle, NULL);
      *module_ostream_p = vport->module_stream;
    }
  else
    {
      module = NULL;
      *module_ostream_p = ~0;
    }
  return module;
}

static void
bse_snet_prepare (BseSource *source)
{
  BseSNet *snet = BSE_SNET (source);

  bse_object_lock (BSE_OBJECT (snet));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_snet_reset (BseSource *source)
{
  BseSNet *snet = BSE_SNET (source);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  bse_object_unlock (BSE_OBJECT (snet));
}
