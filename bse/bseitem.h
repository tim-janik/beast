/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __BSE_ITEM_H__
#define __BSE_ITEM_H__

#include        <bse/bseobject.h>

G_BEGIN_DECLS


/* --- object type macros --- */
#define BSE_TYPE_ITEM               (BSE_TYPE_ID (BseItem))
#define BSE_ITEM(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_ITEM, BseItem))
#define BSE_ITEM_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_ITEM, BseItemClass))
#define BSE_IS_ITEM(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_ITEM))
#define BSE_IS_ITEM_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ITEM))
#define BSE_ITEM_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_ITEM, BseItemClass))


/* --- BseItem member macros --- */
#define BSE_ITEM_SINGLETON(object)      ((BSE_OBJECT_FLAGS (object) & BSE_ITEM_FLAG_SINGLETON) != 0)
#define BSE_ITEM_STORAGE_IGNORE(object) ((BSE_OBJECT_FLAGS (object) & BSE_ITEM_FLAG_STORAGE_IGNORE) != 0)


/* --- bse item flags --- */
typedef enum                            /*< skip >*/
{
  BSE_ITEM_FLAG_SINGLETON	= 1 << (BSE_OBJECT_FLAGS_USHIFT + 0),
  BSE_ITEM_FLAG_STORAGE_IGNORE	= 1 << (BSE_OBJECT_FLAGS_USHIFT + 1)
} BseItemFlags;
#define BSE_ITEM_FLAGS_USHIFT          (BSE_OBJECT_FLAGS_USHIFT + 2)


/* --- BseItem object --- */
struct _BseItem
{
  BseObject     parent_object;

  guint         use_count;
  BseItem      *parent;
};
struct _BseItemClass
{
  BseObjectClass parent_class;

  BswIterProxy*	(*list_proxies)	(BseItem	*item,
				 guint		 param_id,
				 GParamSpec	*pspec);

  void		(*set_parent)	(BseItem	*item,
				 BseItem	*parent);
  guint		(*get_seqid)	(BseItem	*item);
};

#if 0
typedef void     (*BseItemCrossFunc)          (BseItem        *owner,
					      BseItem        *ref_item,
					      gpointer        data);
#endif
typedef void     (*BseItemUncross)	     (BseItem        *owner,
					      BseItem        *ref_item);
typedef gboolean (*BseItemCheckContainer)    (BseContainer   *container,
					      BseItem	     *item,
					      gpointer	      data);
typedef gboolean (*BseItemCheckProxy)	     (BseItem	     *proxy,
					      BseItem	     *item,
					      gpointer	      data);


/* --- prototypes --- */
BswIterProxy*	bse_item_gather_proxies	     (BseItem	           *item,
					      BswIterProxy         *iter,
					      GType		    base_type,
					      BseItemCheckContainer ccheck,
					      BseItemCheckProxy     pcheck,
					      gpointer	            data);
BswIterProxy*	bse_item_list_proxies	     (BseItem	      *item,
					      const gchar     *property);
guint           bse_item_get_seqid           (BseItem         *item);
void            bse_item_queue_seqid_changed (BseItem         *item);
BseSuper*       bse_item_get_super           (BseItem         *item);
BseProject*     bse_item_get_project         (BseItem         *item);
gboolean        bse_item_has_ancestor        (BseItem         *item,
					      BseItem         *ancestor);
BseItem*        bse_item_common_ancestor     (BseItem         *item1,
					      BseItem         *item2);
void            bse_item_cross_ref           (BseItem         *owner,
					      BseItem         *ref_item,
					      BseItemUncross   uncross_func);
void            bse_item_cross_unref         (BseItem         *owner,
					      BseItem         *ref_item);
void            bse_item_uncross	     (BseItem         *owner,
					      BseItem         *ref_item);
gboolean        bse_item_has_cross_owners    (BseItem         *ref_item);
GList* /*fr*/   bse_item_list_cross_owners   (BseItem         *ref_item);
BseErrorType    bse_item_exec_proc           (gpointer	       item,
					      const gchar     *procedure,
					      ...);
#define	bse_item_exec /* (item, in_params..., &out_params...) */	bse_item_exec_proc
BseErrorType    bse_item_exec_void_proc      (gpointer	       item,
					      const gchar     *procedure,
					      ...);
BseStorage*     bse_item_open_undo           (BseItem         *item,
					      const gchar     *undo_group);
void            bse_item_close_undo          (BseItem         *item,
					      BseStorage      *storage);
void		bse_item_use		     (BseItem	      *item);


/* --- internal --- */
void            bse_item_set_parent          (BseItem        *item,
					      BseItem        *parent);



G_END_DECLS

#endif /* __BSE_ITEM_H__ */
