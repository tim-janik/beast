/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_PARAM_VIEW_H__
#define __BST_PARAM_VIEW_H__

#include	"bstparam.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PARAM_VIEW		(bst_param_view_get_type ())
#define	BST_PARAM_VIEW(object)		(GTK_CHECK_CAST ((object), BST_TYPE_PARAM_VIEW, BstParamView))
#define	BST_PARAM_VIEW_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PARAM_VIEW, BstParamViewClass))
#define	BST_IS_PARAM_VIEW(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_PARAM_VIEW))
#define	BST_IS_PARAM_VIEW_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PARAM_VIEW))
#define BST_PARAM_VIEW_GET_CLASS(obj)	((BstParamViewClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstParamView		BstParamView;
typedef	struct	_BstParamViewClass	BstParamViewClass;
struct _BstParamView
{
  GtkVWrapBox	 parent_object;

  BseObject	*object;

  GSList	*bparams;

  GType  	 base_type;
  GType  	 object_type;
  GPatternSpec  *reject_pattern;
  GPatternSpec  *match_pattern;

  GtkTooltips	*tooltips;
};
struct _BstParamViewClass
{
  GtkVWrapBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_param_view_get_type		(void);
GtkWidget*	bst_param_view_new		(BseObject	*object);
void		bst_param_view_update		(BstParamView	*param_view);
void		bst_param_view_rebuild		(BstParamView	*param_view);
void		bst_param_view_set_object	(BstParamView	*param_view,
						 BseObject	*object);
void		bst_param_view_set_mask		(BstParamView	*param_view,
						 GType  	 base_type,
						 GType  	 param_object_type,
						 const gchar	*reject_pattern,
						 const gchar	*match_pattern);





#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_PARAM_VIEW_H__ */
