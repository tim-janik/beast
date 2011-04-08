/* BSE - Better Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
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
					BseItem  *link,
					gpointer  data);
struct _BseContainer
{
  BseSource	parent_instance;
  
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
  gboolean	(*check_restore)	(BseContainer		*container,
					 const gchar		*child_type);
  BseItem*	(*retrieve_child)	(BseContainer		*container,
					 GType			 child_type,
					 const gchar		*uname);
  GSList*	(*context_children)	(BseContainer		*container);
  void		(*release_children)	(BseContainer		*container);
};


/* --- prototypes --- */
void		bse_container_forall_items	(BseContainer	*container,
						 BseForallItemsFunc func,
						 gpointer	 data);
BseItemSeq*	bse_container_list_children	(BseContainer	*container);
guint		bse_container_get_item_seqid	(BseContainer	*container,
						 BseItem	*item);
BseItem*	bse_container_get_item		(BseContainer	*container,
						 GType  	 item_type,
						 guint		 seq_id);
void		bse_container_store_children	(BseContainer	*container,
						 BseStorage	*storage);
BseItem*	bse_container_lookup_item	(BseContainer	*container,
						 const gchar	*uname);
BseItem*	bse_container_retrieve_child	(BseContainer	*container,
						 const gchar	*type_uname);
BseItem*	bse_container_resolve_upath	(BseContainer	*container,
						 const gchar	*upath);
gchar* /*fr*/	bse_container_make_upath	(BseContainer	*container,
						 BseItem	*item);
gboolean	bse_container_check_restore	(BseContainer	*container,
						 const gchar	*child_type);
/* non-undo functions */
gpointer        bse_container_new_child_bname   (BseContainer   *container,
						 GType           child_type,
						 const gchar    *base_name,
						 const gchar    *first_param_name,
						 ...);
#define         bse_container_new_child(         container, child_type, ...) \
                bse_container_new_child_bname(   container, child_type, NULL, __VA_ARGS__)
void		bse_container_add_item          (BseContainer	*container,
						 BseItem	*item);
void		bse_container_remove_item	(BseContainer	*container,
						 BseItem	*item);
/* undo+redo functions or undo-only (backup) functions */
void        bse_container_uncross_undoable      (BseContainer   *container,
                                                 BseItem        *child);
void        bse_container_remove_backedup       (BseContainer   *container,
                                                 BseItem        *child,
                                                 BseUndoStack   *ustack);


/* --- internal functions --- */
void          _bse_container_cross_link		(BseContainer    *container,
						 BseItem         *owner,
						 BseItem         *link,
						 BseItemUncross   uncross_func);
void          _bse_container_cross_unlink	(BseContainer    *container,
						 BseItem         *owner,
						 BseItem         *link,
						 BseItemUncross   uncross);
void          _bse_container_uncross		(BseContainer    *container,
						 BseItem         *owner,
						 BseItem         *link);
void          bse_container_debug_tree          (BseContainer    *container);


G_END_DECLS

#endif /* __BSE_CONTAINER_H__ */
