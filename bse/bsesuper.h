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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsesuper.h: undo aware base class for songs, samples and MS networks
 */
#ifndef __BSE_SUPER_H__
#define __BSE_SUPER_H__

#include        <bse/bsecontainer.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define	BSE_TYPE_SUPER		    (BSE_TYPE_ID (BseSuper))
#define BSE_SUPER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUPER, BseSuper))
#define BSE_SUPER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUPER, BseSuperClass))
#define BSE_IS_SUPER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUPER))
#define BSE_IS_SUPER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUPER))
#define BSE_SUPER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUPER, BseSuperClass))


/* --- BseSuper member macros --- */
#define BSE_SUPER_NEEDS_CONTEXT(object)		  ((BSE_OBJECT_FLAGS (object) & BSE_SUPER_FLAG_NEEDS_CONTEXT) != 0)
#define BSE_SUPER_NEEDS_SEQUENCER_CONTEXT(object) ((BSE_OBJECT_FLAGS (object) & BSE_SUPER_FLAG_NEEDS_SEQUENCER_CONTEXT) != 0)
#define BSE_SUPER_NEEDS_SEQUENCER(object)	  ((BSE_OBJECT_FLAGS (object) & BSE_SUPER_FLAG_NEEDS_SEQUENCER) != 0)


/* --- bse item flags --- */
typedef enum                            /*< skip >*/
{
  BSE_SUPER_FLAG_NEEDS_CONTEXT		 = 1 << (BSE_CONTAINER_FLAGS_USHIFT + 0),
  BSE_SUPER_FLAG_NEEDS_SEQUENCER_CONTEXT = 1 << (BSE_CONTAINER_FLAGS_USHIFT + 1),
  BSE_SUPER_FLAG_NEEDS_SEQUENCER	 = 1 << (BSE_CONTAINER_FLAGS_USHIFT + 2)
} BseSuperFlags;
#define BSE_SUPER_FLAGS_USHIFT	       (BSE_CONTAINER_FLAGS_USHIFT + 3)


/* --- BseSuper object --- */
struct _BseSuper
{
  BseContainer	 parent_object;

  BseTime	 creation_time;
  BseTime	 mod_time;
  BseTime	 saved_mod_time;

  /* for BseProject */
  guint          context_handle;
};
struct _BseSuperClass
{
  BseContainerClass parent_class;
  
  gboolean	(*is_dirty)		(BseSuper	*super);
  void		(*modified)		(BseSuper	*super,
					 BseTime	 stamp);
};


/* --- prototypes --- */
BseProject*     bse_super_get_project		(BseSuper	*super);
gboolean	bse_super_is_dirty		(BseSuper	*super);
void		bse_super_set_creation_time	(BseSuper	*super,
						 BseTime	 creation_time);
void		bse_super_reset_mod_time	(BseSuper	*super,
						 BseTime	 mod_time);

/* convenience functions */
void		bse_super_set_author		(BseSuper	*super,
						 const gchar	*author);
void		bse_super_set_copyright		(BseSuper	*super,
						 const gchar	*copyright);
gchar*		bse_super_get_author		(BseSuper	*super);
gchar*		bse_super_get_copyright		(BseSuper	*super);



G_END_DECLS

#endif /* __BSE_SUPER_H__ */
