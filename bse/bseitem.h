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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bseitem.h: base type for auxillary objects for BseSuper
 */
#ifndef __BSE_ITEM_H__
#define __BSE_ITEM_H__

#include	<bse/bseobject.h>


/* --- object type macros --- */
#define	BSE_TYPE_ITEM		    (BSE_TYPE_ID (BseItem))
#define BSE_ITEM(object)	    (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_ITEM, BseItem))
#define BSE_ITEM_CLASS(class)	    (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_ITEM, BseItemClass))
#define BSE_IS_ITEM(object)	    (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_ITEM))
#define BSE_IS_ITEM_CLASS(class)    (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ITEM))
#define BSE_ITEM_GET_CLASS(object)  ((BseItemClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseItem member macros --- */
#define BSE_ITEM_PARENT_REF(object) ((BSE_OBJECT_FLAGS (object) & BSE_ITEM_FLAG_PARENT_REF) != 0)


/* --- bse item flags --- */
typedef enum                            /* <skip> */
{
  BSE_ITEM_FLAG_PARENT_REF	= 1 << BSE_OBJECT_FLAGS_USER_SHIFT
} BseItemFlags;
#define BSE_ITEM_FLAGS_USER_SHIFT	(BSE_OBJECT_FLAGS_USER_SHIFT + 1)


/* --- BseItem object --- */
struct _BseItem
{
  BseObject	parent_object;
  
  BseItem	*container;
};
struct _BseItemClass
{
  BseObjectClass parent_class;

  void	(*set_container)  (BseItem	*item,
			   BseItem	*container);
  guint	 (*get_seqid)	  (BseItem	*item);
  void	 (*seqid_changed) (BseItem	*item);
};


/* --- prototypes --- */
guint		bse_item_get_seqid	(BseItem	*item);
void		bse_item_seqid_changed	(BseItem	*item);
BseSuper*	bse_item_get_super	(BseItem	*item);
BseProject*	bse_item_get_project	(BseItem	*item);
gboolean	bse_item_has_ancestor	(BseItem	*item,
					 BseItem	*ancestor);
void		bse_item_set_container	(BseItem	*item,
					 BseItem	*container);
gchar* /*fr*/	bse_item_make_handle	(BseItem	*item,
					 gboolean	 named);
BseErrorType	bse_item_exec_proc	(BseItem	*item,
					 const gchar	*procedure,
					 ...);
BseErrorType	bse_item_exec_void_proc	(BseItem	*item,
					 const gchar	*procedure,
					 ...);
BseStorage*	bse_item_open_undo	(BseItem	*item,
					 const gchar	*undo_group);
void		bse_item_close_undo	(BseItem	*item,
					 BseStorage	*storage);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_ITEM_H__ */
