/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstpreferences.h"



/* --- prototypes --- */
static void	bst_preferences_class_init	(BstPreferencesClass	*klass);
static void	bst_preferences_init		(BstPreferences		*prefs);
static void	bst_preferences_destroy		(GtkObject		*object);


/* --- static variables --- */
static gpointer             parent_class = NULL;
static BstPreferencesClass *bst_preferences_class = NULL;


/* --- functions --- */
GtkType
bst_preferences_get_type (void)
{
  static GtkType preferences_type = 0;
  
  if (!preferences_type)
    {
      GtkTypeInfo preferences_info =
      {
	"BstPreferences",
	sizeof (BstPreferences),
	sizeof (BstPreferencesClass),
	(GtkClassInitFunc) bst_preferences_class_init,
	(GtkObjectInitFunc) bst_preferences_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      preferences_type = gtk_type_unique (GTK_TYPE_VBOX, &preferences_info);
    }
  
  return preferences_type;
}

static void
bst_preferences_class_init (BstPreferencesClass *class)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (class);

  bst_preferences_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = bst_preferences_destroy;
}

static void
bst_preferences_init (BstPreferences *prefs)
{
  GtkWidget *any, *hbox, *button;

  prefs->gconf = NULL;
  prefs->param_box = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  BST_PARAM_VIEW (prefs->param_box)->base_type = BSE_TYPE_GCONFIG;
  gtk_widget_show (prefs->param_box);
  gtk_container_add (GTK_CONTAINER (prefs), prefs->param_box);

  /* dialog bits
   */
  any = gtk_widget_new (gtk_hseparator_get_type (),
			"visible", TRUE,
			NULL);
  gtk_box_pack_start (GTK_BOX (prefs), any, FALSE, TRUE, 0);
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "homogeneous", TRUE,
			 "spacing", 5,
			 "border_width", 5,
			 "visible", TRUE,
			 NULL);
  gtk_box_pack_end (GTK_BOX (prefs), hbox, FALSE, TRUE, 0);
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "label", "Apply",
			   "parent", hbox,
			   "visible", TRUE,
			   "can_default", TRUE,
			   "object_signal::clicked", bst_preferences_apply, prefs,
			   "object_signal::destroy", bse_nullify_pointer, &prefs->apply,
			   NULL);
  gtk_tooltips_set_tip (BST_PARAM_VIEW (prefs->param_box)->tooltips, button,
			"Apply the preference values. Some values may only take effect after"
			"a restart. The preference values are locked against modifcation during"
			"playback.",
			NULL);
  prefs->apply = button;
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "label", "Save",
			   "parent", hbox,
			   "visible", TRUE,
			   "can_default", TRUE,
			   "sensitive", FALSE,
			   //"object_signal::clicked", bst_preferences_apply, prefs,
			   "object_signal::destroy", bse_nullify_pointer, &prefs->save,
			   NULL);
  gtk_tooltips_set_tip (BST_PARAM_VIEW (prefs->param_box)->tooltips, button,
			"Save the preference values (does not apply them to the running program).",
			NULL);
  prefs->save = button;
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "label", "Revert",
			   "parent", hbox,
			   "visible", TRUE,
			   "can_default", TRUE,
			   "object_signal::clicked", bst_preferences_revert, prefs,
			   NULL);
  gtk_tooltips_set_tip (BST_PARAM_VIEW (prefs->param_box)->tooltips, button,
			"Revert the preference values to the current internal values.",
			NULL);
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "label", "Defaults",
			   "parent", hbox,
			   "visible", TRUE,
			   "can_default", TRUE,
			   "object_signal::clicked", bst_preferences_default_revert, prefs,
			   NULL);
  gtk_tooltips_set_tip (BST_PARAM_VIEW (prefs->param_box)->tooltips, button,
			"Revert to hardcoded default values (factory settings ;).",
			NULL);
}

static void
bst_preferences_destroy (GtkObject *object)
{
  BstPreferences *prefs = BST_PREFERENCES (object);

  bst_preferences_set_gconfig (prefs, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_preferences_new (BseGConfig *gconf)
{
  GtkWidget *prefs;

  g_return_val_if_fail (BSE_IS_GCONFIG (gconf), NULL);

  prefs = gtk_widget_new (BST_TYPE_PREFERENCES, NULL);
  bst_preferences_set_gconfig (BST_PREFERENCES (prefs), gconf);

  return prefs;
}

static void
preferences_lock_changed (BstPreferences *prefs)
{
  gtk_widget_set_sensitive (prefs->apply, prefs->gconf ? bse_gconfig_can_apply (prefs->gconf) : FALSE);
}

void
bst_preferences_set_gconfig (BstPreferences *prefs,
			     BseGConfig     *gconf)
{
  g_return_if_fail (BST_IS_PREFERENCES (prefs));
  if (gconf)
    g_return_if_fail (BSE_IS_GCONFIG (gconf));

  if (prefs->gconf)
    {
      bst_param_view_set_object (BST_PARAM_VIEW (prefs->param_box), NULL);
      bse_object_remove_notifiers_by_func (prefs->gconf,
					   preferences_lock_changed,
					   prefs);
      bse_object_unref (BSE_OBJECT (prefs->gconf));
      prefs->gconf = NULL;
    }
  prefs->gconf = gconf;
  if (prefs->gconf)
    {
      bse_object_ref (BSE_OBJECT (prefs->gconf));
      bse_object_add_data_notifier (prefs->gconf,
				    "lock_changed",
				    preferences_lock_changed,
				    prefs);
      bst_param_view_set_object (BST_PARAM_VIEW (prefs->param_box), BSE_OBJECT (prefs->gconf));
    }
  preferences_lock_changed (prefs);
}

void
bst_preferences_rebuild (BstPreferences *prefs)
{
  BseObject *object;
  BseObjectClass *class;
  
  g_return_if_fail (BST_IS_PREFERENCES (prefs));
  
  if (!prefs->gconf)
    return;

  object = BSE_OBJECT (prefs->gconf);
  class = BSE_OBJECT_GET_CLASS (object);
  
  bst_preferences_revert (prefs);
}

void
bst_preferences_apply (BstPreferences *prefs)
{
  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  bse_gconfig_apply (prefs->gconf);
}

void
bst_preferences_revert (BstPreferences *prefs)
{
  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  bse_gconfig_revert (prefs->gconf);
}

void
bst_preferences_default_revert (BstPreferences *prefs)
{
  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  bse_gconfig_default_revert (prefs->gconf);
}
