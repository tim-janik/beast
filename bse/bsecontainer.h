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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsecontainer.h: base type to manage BSE items
 */
#ifndef __BSE_CONTAINER_H__
#define __BSE_CONTAINER_H__

#include	<bse/bsesource.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define	BSE_TYPE_CONTAINER		(BSE_TYPE_ID (BseContainer))
#define BSE_CONTAINER(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CONTAINER, BseContainer))
#define BSE_CONTAINER_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CONTAINER, BseContainerClass))
#define BSE_IS_CONTAINER(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CONTAINER))
#define BSE_IS_CONTAINER_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CONTAINER))
#define BSE_CONTAINER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CONTAINER, BseContainerClass))
#define BSE_CONTAINER_FLAGS_USHIFT	(BSE_SOURCE_FLAGS_USHIFT + 0)


/* --- BseContainer object --- */
typedef gboolean (*BseForallItemsFunc) (BseItem	 *item,
					gpointer  data);
typedef gboolean (*BseForallCrossFunc) (BseItem	 *owner,
					BseItem  *ref_item,
					gpointer  data);
struct _BseContainer
{
  BseSource	parent_object;
  
  guint		n_items;	/* paranoid checks */
};
struct _BseContainerClass
{
  BseSourceClass  parent_class;
  
  void		(*add_item)		(BseContainer		*container,
					 BseItem		*item);
  void		(*remove_item)		(BseContainer		*container,
					 BseItem		*item);
  void		(*forall_items)		(BseContainer		*container,
					 BseForallItemsFunc	 func,
					 gpointer		 data);
  BseItem*	(*retrieve_child)	(BseContainer		*container,
					 GType			 child_type,
					 const gchar		*uname);
  GSList*	(*context_children)	(BseContainer		*container);
  void		(*release_children)	(BseContainer		*container);
};


/* --- prototypes --- */
void		bse_container_add_item		(BseContainer	*container,
						 BseItem	*item);
gpointer        bse_container_new_item          (BseContainer   *container,
						 GType           item_type,
						 const gchar    *first_param_name,
						 ...);
void		bse_container_remove_item	(BseContainer	*container,
						 BseItem	*item);
void		bse_container_forall_items	(BseContainer	*container,
						 BseForallItemsFunc func,
						 gpointer	 data);
BseProxySeq*	bse_container_list_items	(BseContainer	*container);
guint		bse_container_get_item_seqid	(BseContainer	*container,
						 BseItem	*item);
BseItem*	bse_container_get_item		(BseContainer	*container,
						 GType  	 item_type,
						 guint		 seq_id);
void		bse_container_store_items	(BseContainer	*container,
						 BseStorage	*storage,
						 const gchar	*restore_func);
BseItem*	bse_container_lookup_item	(BseContainer	*container,
						 const gchar	*uname);
BseItem*	bse_container_retrieve_child	(BseContainer	*container,
						 const gchar	*type_uname);
BseItem*	bse_container_resolve_upath	(BseContainer	*container,
						 const gchar	*upath);
gchar* /*fr*/	bse_container_make_upath	(BseContainer	*container,
						 BseItem	*item);


/* --- internal functions --- */
void            bse_container_cross_ref         (BseContainer    *container,
						 BseItem         *owner,
						 BseItem         *ref_item,
						 BseItemUncross   uncross_func);
void            bse_container_cross_unref       (BseContainer    *container,
						 BseItem         *owner,
						 BseItem         *ref_item,
						 gboolean	 notify);
void		bse_container_uncross_item	(BseContainer    *container,
						 BseItem         *item);
void		bse_container_cross_forall	(BseContainer	*container,
						 BseForallCrossFunc func,
						 gpointer	 data);



G_END_DECLS

#endif /* __BSE_CONTAINER_H__ */
