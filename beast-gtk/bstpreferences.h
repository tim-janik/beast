/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999, 2000 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PREFERENCES_H__
#define __BST_PREFERENCES_H__

#include	"bstparamview.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PREFERENCES		(bst_preferences_get_type ())
#define	BST_PREFERENCES(object)	        (GTK_CHECK_CAST ((object), BST_TYPE_PREFERENCES, BstPreferences))
#define	BST_PREFERENCES_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PREFERENCES, BstPreferencesClass))
#define	BST_IS_PREFERENCES(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_PREFERENCES))
#define	BST_IS_PREFERENCES_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PREFERENCES))
#define BST_PREFERENCES_GET_CLASS(obj)  ((BstPreferencesClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstPreferences      BstPreferences;
typedef	struct	_BstPreferencesClass BstPreferencesClass;
struct _BstPreferences
{
  GtkVBox	 parent_object;

  BseGConfig	*gconf;

  GtkWidget	*bse_param_view;
  GtkWidget	*bst_param_view;
  GtkWidget	*notebook;
  GtkWidget	*apply;
  GtkWidget	*close;
};
struct _BstPreferencesClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		 bst_preferences_get_type	(void);
GtkWidget*	 bst_preferences_new		(BseGConfig	*gconf);
void		 bst_preferences_rebuild	(BstPreferences *prefs);
void		 bst_preferences_set_gconfig	(BstPreferences *prefs,
						 BseGConfig     *gconf);
void		 bst_preferences_apply		(BstPreferences *prefs);
void		 bst_preferences_save		(BstPreferences *prefs);
void		 bst_preferences_revert		(BstPreferences *prefs);
void		 bst_preferences_default_revert	(BstPreferences *prefs);
#define	bst_preferences_update	bst_preferences_revert


/* --- rc file --- */
BseErrorType	 bst_rc_dump			(const gchar	*file_name,
						 BseGConfig     *gconf);
BseErrorType	 bst_rc_parse			(const gchar	*file_name,
						 BseGConfig     *gconf);



#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_PREFERENCES_H__ */
