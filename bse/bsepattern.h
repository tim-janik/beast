/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
 * bsepattern.h: notes container for songs
 */
#ifndef __BSE_PATTERN_H__
#define __BSE_PATTERN_H__

#include	<bse/bseitem.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PATTERN	      (BSE_TYPE_ID (BsePattern))
#define BSE_PATTERN(object)	      (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_PATTERN, BsePattern))
#define BSE_PATTERN_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_PATTERN, BsePatternClass))
#define BSE_IS_PATTERN(object)	      (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_PATTERN))
#define BSE_IS_PATTERN_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PATTERN))
#define BSE_PATTERN_GET_CLASS(object) ((BsePatternClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BsePattern object --- */
struct _BsePatternNote
{
  BseInstrument *instrument;
  guint		 note : 20;
  guint		 selected : 1;
  guint		 n_effects : 8;
  BseEffect    **effects;
};
struct _BsePattern
{
  BseItem	parent_object;
  
  guint n_channels		/* mirrored from BseSong */;
  guint n_rows			/* mirrored from BseSong.pattern_length */;
  
  guint	current_channel;
  guint current_row;
  
  BsePatternNote *notes		/* ->notes [ row * n_channels + channel] */;
};
struct _BsePatternClass
{
  BseItemClass parent_class;
};


/* --- prototypes --- */
/* returns a pointer to relocatable data, make sure to lock the
 * pattern to maintain validity.
 */
BsePatternNote*	bse_pattern_peek_note	    (BsePattern		*pattern,
					     guint               channel,
					     guint               row);
GList* /*fl*/	bse_pattern_list_selection  (BsePattern		*pattern);


/* --- procedure short hands --- */
void		bse_pattern_set_note	    (BsePattern		*pattern,
					     guint               channel,
					     guint               row,
					     gint		 note);
void		bse_pattern_set_instrument  (BsePattern		*pattern,
					     guint               channel,
					     guint               row,
					     BseInstrument	*instrument);
void		bse_pattern_select_note	    (BsePattern		*pattern,
					     guint               channel,
					     guint               row);
void		bse_pattern_unselect_note   (BsePattern		*pattern,
					     guint               channel,
					     guint               row);


/* --- internal --- */
void		bse_pattern_set_n_channels  (BsePattern		*pattern,
					     guint		 n_channels);
void		bse_pattern_set_n_rows	    (BsePattern		*pattern,
					     guint		 n_rows);
void		bse_pattern_modify_note	    (BsePattern		*pattern,
					     guint		 channel,
					     guint		 row,
					     gint		 note,
					     BseInstrument	*instrument,
					     gboolean		 selected);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PATTERN_H__ */
