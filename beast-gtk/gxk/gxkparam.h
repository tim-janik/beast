/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PARAM_H__
#define __BST_PARAM_H__

#include	"bstutils.h"

G_BEGIN_DECLS


/* --- macros --- */
#define BST_PARAM_IS_GMASK(bparam)	((bparam)->impl->create_gmask != NULL)


/* --- structures & enums --- */
typedef enum /*< skip >*/
{
  BST_PARAM_EDITABLE		= 1 << 0,
  BST_PARAM_PROXY_LIST		= 1 << 1,
} BstParamFlags;
typedef struct _BstParamBinding BstParamBinding;
typedef struct _BstParamImpl	BstParamImpl;
typedef struct {
  GValue	   value;
  GParamSpec	  *pspec;
  BstParamImpl	  *impl;
  guint		   column : 8;
  guint		   readonly : 1; /* precond, GUI impl && pspec */
  guint		   writable : 1; /* dynamic, binding owned */
  guint		   editable : 1; /* dynamic, API owned */
  guint		   updating : 1;
  union {
    BstGMask	  *gmask;
    GtkWidget	  *widget;
  }		   gdata;
  /* binding data */
  BstParamBinding *binding;
  union {
    gulong	   v_long;
    gpointer	   v_pointer;
  }		   mdata[4];
} BstParam;
struct _BstParamImpl
{
  gchar		*name;
  gint8		 rating;
  guint8	 variant;
  guint8	 flags;		// BstParamFlags
  guint		 scat;		// SfiSCategory
  gchar		*hints;		// must match if present
  BstGMask*	(*create_gmask)		(BstParam	*bparam,
					 const gchar	*tooltip,
					 GtkWidget	*gmask_parent);
  GtkWidget*	(*create_widget)	(BstParam	*bparam,
					 const gchar	*tooltip);
  void		(*update)		(BstParam	*bparam,
					 GtkWidget	*action);
};
struct _BstParamBinding
{
  // FIXME: post_create() for gmask xframe settings
  void		(*set_value)		(BstParam	*bparam,
					 const GValue	*value);
  void		(*get_value)		(BstParam	*bparam,
					 GValue		*value);
  void		(*destroy)		(BstParam	*bparam);
  /* optional: */
  gboolean	(*check_writable)	(BstParam	*bparam);
  SfiProxy	(*rack_item)		(BstParam	*bparam);
  BseProxySeq*	(*list_proxies)		(BstParam	*bparam);
};


/* --- functions --- */
void		 bst_param_pack_property  (BstParam	   *bparam,
					   GtkWidget	   *parent);
GtkWidget*	 bst_param_rack_widget	  (BstParam	   *bparam);
void		 bst_param_update	  (BstParam	   *bparam);
void		 bst_param_apply_value	  (BstParam	   *bparam);
void		 bst_param_set_editable	  (BstParam	   *bparam,
					   gboolean	    editable);
const gchar*	 bst_param_get_name	  (BstParam	   *bparam);
const gchar*	 bst_param_get_view_name  (BstParam	   *bparam);
void		 bst_param_destroy	  (BstParam	   *bparam);
guint		 bst_param_rate_check	  (GParamSpec	   *pspec,
					   gboolean	    rack_widget,
					   const gchar	   *view_name,
					   BstParamBinding *binding);
const gchar**	 bst_param_list_names	  (gboolean	    rack_widget,
					   guint	   *n_p);
const gchar*	 bst_param_lookup_view	  (GParamSpec	   *pspec,
					   gboolean	    rack_widget,
					   const gchar	   *view_name,
					   BstParamBinding *binding);


/* --- bindings --- */
BstParamBinding* bst_param_binding_proxy  (void);
BstParam*	 bst_param_proxy_create	  (GParamSpec	   *pspec,
					   gboolean	    rack_widget,
					   const gchar	   *view_name,
					   SfiProxy	    proxy);
void		 bst_param_set_proxy	  (BstParam	   *bparam,
					   SfiProxy	    proxy);
BstParamBinding* bst_param_binding_rec	  (void);
BstParam*	 bst_param_rec_create	  (GParamSpec	   *pspec,
					   gboolean	    rack_widget,
					   const gchar	   *view_name,
					   SfiRec	   *rec);


/* --- param implementation utils --- */
void	      _bst_init_params		(void);
BstParam*     bst_param_alloc		(BstParamImpl	*impl,
					 GParamSpec	*pspec);
gboolean  bst_param_xframe_check_button (BstParam	*bparam,
					 guint		 button);
gboolean  bst_param_entry_key_press	(GtkEntry	*entry,
					 GdkEventKey	*event);
gboolean  bst_param_ensure_focus	(GtkWidget	*widget);


G_END_DECLS

#endif /* __BST_PARAM_H__ */

/* vim:set ts=8 sts=2 sw=2: */
