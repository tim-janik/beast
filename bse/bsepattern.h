/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
  guint		 n_effects : 8;
  guint		 selected : 1;
  BseEffect    **effects;
};
struct _BsePattern
{
  BseItem parent_object;
  
  guint   n_channels		/* mirrored from BseSong */;
  guint   n_rows		/* mirrored from BseSong.pattern_length */;

  BsePatternNote *notes		/* ->notes [ row * n_channels + channel] */;

  /* only used during parsing phase */
  guint	  current_channel;
  guint   current_row;
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
gboolean	bse_pattern_has_selection   (BsePattern		*pattern);


/* --- procedure short hands --- */
void		bse_pattern_set_note	    (BsePattern		*pattern,
					     guint               channel,
					     guint               row,
					     gint		 note);
void		bse_pattern_set_instrument  (BsePattern		*pattern,
					     guint               channel,
					     guint               row,
					     BseInstrument	*instrument);


/* --- internal --- */
void		bse_pattern_set_n_channels  (BsePattern		*pattern,
					     guint		 n_channels);
void		bse_pattern_set_n_rows	    (BsePattern		*pattern,
					     guint		 n_rows);
void		bse_pattern_modify_note	    (BsePattern		*pattern,
					     guint		 channel,
					     guint		 row,
					     gint		 note,
					     BseInstrument	*instrument);
void		bse_pattern_select_note     (BsePattern		*pattern,
					     guint      	 channel,
					     guint      	 row);
void		bse_pattern_unselect_note   (BsePattern		*pattern,
					     guint      	 channel,
					     guint      	 row);
     

/* --- selections --- */
/* selections within a BsePattern are supplied for procedure invocation
 * from a pattern editor only, they don't actually affect core BSE
 * behaviour. thus we provide functions to keep an external selection
 * mask updated and functions to sync that with a pattern's inetrnal
 * selection.
 */
void     bse_pattern_save_selection       (BsePattern  *pattern,
					   guint32     *selection);
void     bse_pattern_restore_selection    (BsePattern  *pattern,
					   guint32     *selection);
guint32* bse_pattern_selection_new	  (guint        n_channels,
					   guint        n_rows);
guint32* bse_pattern_selection_copy       (guint32     *src_selection);
void     bse_pattern_selection_free       (guint32     *selection);
void     bse_pattern_selection_fill       (guint32     *selection,
					   gboolean     selected);
#define  BSE_PATTERN_SELECTION_N_CHANNELS(selection)  (selection[0])
#define  BSE_PATTERN_SELECTION_N_ROWS(selection)      (selection[1])
#define  BSE_PATTERN_SELECTION_MARK(selection, channel, row)	\
  _bse_pattern_selection_mark ((selection), (channel), (row))
#define  BSE_PATTERN_SELECTION_UNMARK(selection, channel, row)	\
  _bse_pattern_selection_unmark ((selection), (channel), (row))
#define  BSE_PATTERN_SELECTION_TEST(selection, channel, row)	\
  _bse_pattern_selection_test ((selection), (channel), (row))


/* --- implementation details --- */
static inline gboolean
_bse_pattern_selection_test (guint32 *selection,
			     guint    channel,
			     guint    row)
{
  guint n = BSE_PATTERN_SELECTION_N_CHANNELS (selection) * row + channel;

  /* return (selection[n / 32 + 2] & (1 << n % 32)) != 0; */
  return (selection[(n >> 5) + 2] & (1 << (n & 0x1f))) != 0;
}
static inline void
_bse_pattern_selection_mark (guint32 *selection,
			     guint    channel,
			     guint    row)
{
  guint n = BSE_PATTERN_SELECTION_N_CHANNELS (selection) * row + channel;

  selection[(n >> 5) + 2] |= 1 << (n & 0x1f);
}
static inline void
_bse_pattern_selection_unmark (guint32 *selection,
			       guint    channel,
			       guint    row)
{
  guint n = BSE_PATTERN_SELECTION_N_CHANNELS (selection) * row + channel;

  selection[(n >> 5) + 2] &= ~(1 << (n & 0x1f));
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PATTERN_H__ */
