/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
  PARAM_MOD_TIME,
};


/* --- prototypes --- */
static void	bse_super_class_init	(BseSuperClass		*class);
static void	bse_super_init		(BseSuper		*super,
					 gpointer		 rclass);
static void	bse_super_destroy	(BseObject		*object);
static void	bse_super_set_param	(BseSuper		*super,
					 BseParam		*param);
static void	bse_super_get_param	(BseSuper		*super,
					 BseParam		*param);
static gboolean	bse_super_do_is_dirty	(BseSuper		*super);
static void	bse_super_do_modified	(BseSuper		*super,
					 BseTime		 stamp);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;
static GQuark		 quark_author = 0;
static GQuark		 quark_copyright = 0;
static GSList		*bse_super_objects = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSuper)
{
  static const BseTypeInfo super_info = {
    sizeof (BseSuperClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_super_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSuper),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_super_init,
  };
  
  return bse_type_register_static (BSE_TYPE_CONTAINER,
				   "BseSuper",
				   "Base type for item managers and undo facility",
				   &super_info);
}

static void
bse_super_class_init (BseSuperClass *class)
{
  BseObjectClass *object_class;
  BseItemClass *item_class;
  BseContainerClass *container_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_CONTAINER);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  container_class = BSE_CONTAINER_CLASS (class);
  
  quark_author = g_quark_from_static_string ("author");
  quark_copyright = g_quark_from_static_string ("copyright");
  
  object_class->set_param = (BseObjectSetParamFunc) bse_super_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_super_get_param;
  object_class->destroy = bse_super_destroy;

  class->is_dirty = bse_super_do_is_dirty;
  class->modified = bse_super_do_modified;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_AUTHOR,
			      bse_param_spec_fstring ("author", "Author",
						      NULL,
						      BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_COPYRIGHT,
			      bse_param_spec_fstring ("copyright", "Copyright",
						      NULL,
						      BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Time Stamps",
			      PARAM_CREATION_TIME,
			      bse_param_spec_time ("creation_time", "Creation Time",
						   0,
						   BSE_PARAM_DEFAULT |
						   BSE_PARAM_HINT_RDONLY));
  bse_object_class_add_param (object_class, "Time Stamps",
			      PARAM_MOD_TIME,
			      bse_param_spec_time ("modification_time", "Last modification time",
						   0,
						   BSE_PARAM_READWRITE |
						   BSE_PARAM_HINT_RDONLY |
						   BSE_PARAM_SERVE_STORAGE));
}

static void
bse_super_init (BseSuper *super,
		gpointer  rclass)
{
  BseObject *object;
  
  object = BSE_OBJECT (super);

  super->mod_time = bse_time_current ();
  super->creation_time = super->mod_time;
  super->saved_mod_time = super->mod_time;
  
  bse_super_objects = g_slist_prepend (bse_super_objects, super);

  /* we want Unnamed-xxx default names */
  bse_object_set_name (object, "Unnamed");
}

static void
bse_super_destroy (BseObject *object)
{
  BseSuper *super;
  
  super = BSE_SUPER (object);
  
  bse_super_objects = g_slist_remove (bse_super_objects, super);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_super_set_param (BseSuper *super,
		     BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_AUTHOR:
      bse_object_set_qdata_full (BSE_OBJECT (super),
				 quark_author,
				 bse_strdup_stripped (param->value.v_string),
				 g_free);
      break;
    case PARAM_COPYRIGHT:
      bse_object_set_qdata_full (BSE_OBJECT (super),
				 quark_copyright,
				 bse_strdup_stripped (param->value.v_string),
				 g_free);
      break;
    case PARAM_MOD_TIME:
      super->mod_time = MAX (super->creation_time, param->value.v_time);
      break;
    case PARAM_CREATION_TIME:
      super->creation_time = param->value.v_time;
      /* we have to ensure that mod_time is always >= creation_time */
      if (super->creation_time > super->mod_time)
	{
	  super->mod_time = super->creation_time;
	  bse_object_param_changed (BSE_OBJECT (super), "mod-time");
	}
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (super),
		 BSE_OBJECT_NAME (super),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_super_get_param (BseSuper *super,
		     BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_AUTHOR:
      g_free (param->value.v_string);
      param->value.v_string = g_strdup (bse_object_get_qdata (BSE_OBJECT (super), quark_author));
      break;
    case PARAM_COPYRIGHT:
      g_free (param->value.v_string);
      param->value.v_string = g_strdup (bse_object_get_qdata (BSE_OBJECT (super), quark_copyright));
      break;
    case PARAM_MOD_TIME:
      param->value.v_time = super->mod_time;
      break;
    case PARAM_CREATION_TIME:
      param->value.v_time = super->creation_time;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (super),
		 BSE_OBJECT_NAME (super),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static gboolean
bse_super_do_is_dirty (BseSuper *super)
{
  return super->mod_time > super->saved_mod_time;
}

static void
bse_super_do_modified (BseSuper *super,
		       BseTime	 stamp)
{
  super->mod_time = MAX (super->mod_time, stamp);
}

gboolean
bse_super_is_dirty (BseSuper *super)
{
  BseSuperClass *class;
  
  g_return_val_if_fail (BSE_IS_SUPER (super), FALSE);
  g_return_val_if_fail (BSE_SUPER_GET_CLASS (super)->is_dirty != NULL, 0); /* paranoid */
  
  class = BSE_SUPER_GET_CLASS (super);
  
  return class->is_dirty (super);
}

void
bse_super_set_creation_time (BseSuper       *super,
			     BseTime         creation_time)
{
  g_return_if_fail (BSE_IS_SUPER (super));
  
  FIXME ("route set_creation_time() through set_param?");
  
  if (super->mod_time < creation_time)
    {
      super->mod_time = creation_time;
      super->saved_mod_time = super->mod_time;
    }
  super->creation_time = creation_time;
}

void
bse_super_reset_mod_time (BseSuper *super,
			  BseTime   mod_time)
{
  g_return_if_fail (BSE_IS_SUPER (super));
  
  FIXME ("route reset_mod_time() through set_param?");
  
  if (super->creation_time > mod_time)
    super->creation_time = mod_time;
  super->mod_time = mod_time;
  super->saved_mod_time = super->mod_time;
}

BseProject*
bse_super_get_project (BseSuper *super)
{
  BseItem *item;

  g_return_val_if_fail (BSE_IS_SUPER (super), NULL);

  item = BSE_ITEM (super);

  return BSE_IS_PROJECT (item->container) ? BSE_PROJECT (item->container) : NULL;
}

void
bse_super_set_author (BseSuper	  *super,
		      const gchar *author)
{
  g_return_if_fail (BSE_IS_SUPER (super));
  g_return_if_fail (author != NULL);
  
  bse_object_set (BSE_OBJECT (super),
		  "author", author,
		  NULL);
}

void
bse_super_set_copyright (BseSuper    *super,
			 const gchar *copyright)
{
  g_return_if_fail (BSE_IS_SUPER (super));
  g_return_if_fail (copyright != NULL);
  
  bse_object_set (BSE_OBJECT (super),
		  "copyright", copyright,
		  NULL);
}

gchar*
bse_super_get_author (BseSuper *super)
{
  g_return_val_if_fail (BSE_IS_SUPER (super), NULL);
  
  return bse_object_get_qdata (BSE_OBJECT (super), quark_author);
}

gchar*
bse_super_get_copyright (BseSuper *super)
{
  g_return_val_if_fail (BSE_IS_SUPER (super), NULL);
  
  return bse_object_get_qdata (BSE_OBJECT (super), quark_copyright);
}
