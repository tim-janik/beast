/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
#include	"bsesuper.h"

#include	"bseproject.h"
#include	<string.h>


enum
{
  PARAM_0,
  PARAM_AUTHOR,
  PARAM_COPYRIGHT,
  PARAM_CREATION_TIME,
  PARAM_MOD_TIME
};


/* --- prototypes --- */
static void	bse_super_class_init	(BseSuperClass		*class);
static void	bse_super_init		(BseSuper		*super,
					 gpointer		 rclass);
static void	bse_super_finalize	(GObject		*object);
static void	bse_super_set_property	(GObject		*object,
					 guint                   param_id,
					 const GValue           *value,
					 GParamSpec             *pspec);
static void	bse_super_get_property	(GObject		*object,
					 guint                   param_id,
					 GValue                 *value,
					 GParamSpec             *pspec);
static void	bse_super_do_modified	(BseSuper		*super,
					 SfiTime		 stamp);


/* --- variables --- */
static GTypeClass	*parent_class = NULL;
static GQuark		 quark_author = 0;
static GQuark		 quark_copyright = 0;
static GSList		*bse_super_objects = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSuper)
{
  static const GTypeInfo super_info = {
    sizeof (BseSuperClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_super_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSuper),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_super_init,
  };
  
  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseSuper",
				   "Base type for item managers and undo facility",
				   &super_info);
}

static void
bse_super_class_init (BseSuperClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  quark_author = g_quark_from_static_string ("author");
  quark_copyright = g_quark_from_static_string ("copyright");
  
  gobject_class->set_property = bse_super_set_property;
  gobject_class->get_property = bse_super_get_property;
  gobject_class->finalize = bse_super_finalize;
  
  class->modified = bse_super_do_modified;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_AUTHOR,
			      sfi_pspec_string ("author", "Author", NULL,
						NULL,
						SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_COPYRIGHT,
			      sfi_pspec_string ("copyright", "Copyright", NULL,
						NULL,
						SFI_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Time Stamps",
			      PARAM_CREATION_TIME,
			      sfi_pspec_time ("creation_time", "Creation Time", NULL,
					      SFI_PARAM_DEFAULT_RDONLY));
  bse_object_class_add_param (object_class, "Time Stamps",
			      PARAM_MOD_TIME,
			      sfi_pspec_time ("modification_time", "Last modification time", NULL,
					      SFI_PARAM_DEFAULT_RDONLY));
}

static void
bse_super_init (BseSuper *super,
		gpointer  rclass)
{
  BseObject *object;
  
  object = BSE_OBJECT (super);
  
  super->creation_time = sfi_time_from_utc (sfi_time_system ());
  super->mod_time = super->creation_time;
  super->context_handle = ~0;
  
  bse_super_objects = g_slist_prepend (bse_super_objects, super);
  
  /* we want Unnamed-xxx default unames */
  g_object_set (object,
		"uname", "Unnamed",
		NULL);
}

static void
bse_super_finalize (GObject *object)
{
  BseSuper *super = BSE_SUPER (object);
  
  bse_super_objects = g_slist_remove (bse_super_objects, super);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_super_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BseSuper *super = BSE_SUPER (object);
  switch (param_id)
    {
    case PARAM_AUTHOR:
      g_object_set_qdata_full (super,
			       quark_author,
			       g_strdup (g_value_get_string (value)),
			       g_free);
      break;
    case PARAM_COPYRIGHT:
      g_object_set_qdata_full (super,
			       quark_copyright,
			       g_strdup (g_value_get_string (value)),
			       g_free);
      break;
    case PARAM_MOD_TIME:
      super->mod_time = MAX (super->creation_time, sfi_value_get_time (value));
      break;
    case PARAM_CREATION_TIME:
      super->creation_time = sfi_value_get_time (value);
      /* we have to ensure that mod_time is always >= creation_time */
      if (super->creation_time > super->mod_time)
	{
	  super->mod_time = super->creation_time;
	  g_object_notify (super, "modification-time");
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (super, param_id, pspec);
      break;
    }
}

static void
bse_super_get_property (GObject     *object,
			guint        param_id,
			GValue      *value,
			GParamSpec  *pspec)
{
  BseSuper *super = BSE_SUPER (object);
  switch (param_id)
    {
    case PARAM_AUTHOR:
      g_value_set_string (value, g_object_get_qdata (super, quark_author));
      break;
    case PARAM_COPYRIGHT:
      g_value_set_string (value, g_object_get_qdata (super, quark_copyright));
      break;
    case PARAM_MOD_TIME:
      sfi_value_set_time (value, super->mod_time);
      break;
    case PARAM_CREATION_TIME:
      sfi_value_set_time (value, super->creation_time);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (super, param_id, pspec);
      break;
    }
}

static void
bse_super_do_modified (BseSuper *super,
		       SfiTime	 stamp)
{
  super->mod_time = MAX (super->mod_time, stamp);
}

BseProject*
bse_super_get_project (BseSuper *super)
{
  BseItem *item;
  
  g_return_val_if_fail (BSE_IS_SUPER (super), NULL);
  
  item = BSE_ITEM (super);
  
  return BSE_IS_PROJECT (item->parent) ? BSE_PROJECT (item->parent) : NULL;
}
