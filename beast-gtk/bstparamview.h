/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PARAM_VIEW_H__
#define __BST_PARAM_VIEW_H__

#include	"bstutils.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- type macros --- */
#define BST_TYPE_PARAM_VIEW              (bst_param_view_get_type ())
#define BST_PARAM_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PARAM_VIEW, BstParamView))
#define BST_PARAM_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PARAM_VIEW, BstParamViewClass))
#define BST_IS_PARAM_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PARAM_VIEW))
#define BST_IS_PARAM_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PARAM_VIEW))
#define BST_PARAM_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PARAM_VIEW, BstParamViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstParamView		BstParamView;
typedef	struct	_BstParamViewClass	BstParamViewClass;
struct _BstParamView
{
  GtkVBox	 parent_object;

  SfiProxy	 item;

  GSList	*bparams;

  gchar         *first_base_type;
  gchar         *last_base_type;
  GPatternSpec  *reject_pattern;
  GPatternSpec  *match_pattern;
  GtkWidget	*nil_container;	/* null group */
  GtkWidget	*container;
};
struct _BstParamViewClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GType		bst_param_view_get_type		(void);
GtkWidget*	bst_param_view_new		(SfiProxy	 item);
void		bst_param_view_rebuild		(BstParamView	*param_view);
void		bst_param_view_set_item		(BstParamView	*param_view,
						 SfiProxy	 item);
void		bst_param_view_set_mask		(BstParamView	*param_view,
						 const gchar    *first_base_type,
						 const gchar    *last_base_type,
						 const gchar	*reject_pattern,
						 const gchar	*match_pattern);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PARAM_VIEW_H__ */
