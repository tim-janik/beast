/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PATTERN_EDITOR_H__
#define __BST_PATTERN_EDITOR_H__

#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PATTERN_EDITOR		   (bst_pattern_editor_get_type ())
#define	BST_PATTERN_EDITOR(object)	   (GTK_CHECK_CAST ((object), BST_TYPE_PATTERN_EDITOR, BstPatternEditor))
#define	BST_PATTERN_EDITOR_CLASS(klass)	   (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PATTERN_EDITOR, BstPatternEditorClass))
#define	BST_IS_PATTERN_EDITOR(object)	   (GTK_CHECK_TYPE ((object), BST_TYPE_PATTERN_EDITOR))
#define	BST_IS_PATTERN_EDITOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PATTERN_EDITOR))
#define BST_PATTERN_EDITOR_GET_CLASS(obj)  ((BstPatternEditorClass*) (((GtkObject*) (obj))->klass))


typedef enum {
  BST_CELL_INVALID,
  BST_CELL_NOTE,
  BST_CELL_INSTRUMENT,
  BST_CELL_EFFECT,
} BstCellType;

typedef enum {
  BST_PEA_NOTHING,
  /* set a note
   */
  BST_PEA_NOTE_0		=  0 <<	 0	/* leave untouched */,
  BST_PEA_NOTE_C		=  1 <<	 0,
  BST_PEA_NOTE_Cis		=  2 <<	 0,
  BST_PEA_NOTE_Des		= BST_PEA_NOTE_Cis,
  BST_PEA_NOTE_D		=  3 <<	 0,
  BST_PEA_NOTE_Dis		=  4 <<	 0,
  BST_PEA_NOTE_Es		= BST_PEA_NOTE_Dis,
  BST_PEA_NOTE_E		=  5 <<	 0,
  BST_PEA_NOTE_F		=  6 <<	 0,
  BST_PEA_NOTE_Fis		=  7 <<	 0,
  BST_PEA_NOTE_Ges		= BST_PEA_NOTE_Fis,
  BST_PEA_NOTE_G		=  8 <<	 0,
  BST_PEA_NOTE_Gis		=  9 <<	 0,
  BST_PEA_NOTE_As		= BST_PEA_NOTE_Gis,
  BST_PEA_NOTE_A		= 10 <<	 0,
  BST_PEA_NOTE_Ais		= 11 <<	 0,
  BST_PEA_NOTE_Bes		= BST_PEA_NOTE_Ais,
  BST_PEA_NOTE_B		= 12 <<	 0,
  BST_PEA_NOTE_MASK		= 0x0000000f,
  /* set instrument
   */
  BST_PEA_INSTRUMENT_00		=  0 <<	 8	/* leave untouched */,
  BST_PEA_INSTRUMENT_01		=  1 <<	 8,
  BST_PEA_INSTRUMENT_02		=  2 <<	 8,
  BST_PEA_INSTRUMENT_03		=  3 <<	 8,
  BST_PEA_INSTRUMENT_04		=  4 <<	 8,
  BST_PEA_INSTRUMENT_05		=  5 <<	 8,
  BST_PEA_INSTRUMENT_06		=  6 <<	 8,
  BST_PEA_INSTRUMENT_07		=  7 <<	 8,
  BST_PEA_INSTRUMENT_08		=  8 <<	 8,
  BST_PEA_INSTRUMENT_09		=  9 <<	 8,
  BST_PEA_INSTRUMENT_0A		= 10 <<	 8,
  BST_PEA_INSTRUMENT_0B		= 11 <<	 8,
  BST_PEA_INSTRUMENT_0C		= 12 <<	 8,
  BST_PEA_INSTRUMENT_0D		= 13 <<	 8,
  BST_PEA_INSTRUMENT_0E		= 14 <<	 8,
  BST_PEA_INSTRUMENT_0F		= 15 <<	 8	/* default instrument */,
  BST_PEA_INSTRUMENT_MASK	= 0x00000f00,
  /* movements, _MOVE_ and _MOVE_PAGE_ support wrap flags
   */
  BST_PEA_MOVE_0		=  0 << 12	/* do nothing */,
  BST_PEA_MOVE_NEXT		=  1 << 12	/* step to next note */,
  BST_PEA_MOVE_LEFT		=  2 << 12	/* step to left-next note */,
  BST_PEA_MOVE_RIGHT		=  3 << 12,
  BST_PEA_MOVE_UP		=  4 << 12,
  BST_PEA_MOVE_DOWN		=  5 << 12,
  BST_PEA_MOVE_PAGE_LEFT	=  6 << 12	/* step left page-wise */,
  BST_PEA_MOVE_PAGE_RIGHT	=  7 << 12,
  BST_PEA_MOVE_PAGE_UP		=  8 << 12,
  BST_PEA_MOVE_PAGE_DOWN	=  9 << 12,
  BST_PEA_MOVE_JUMP_LEFT	= 10 << 12	/* jump to left border */,
  BST_PEA_MOVE_JUMP_RIGHT	= 11 << 12,
  BST_PEA_MOVE_JUMP_TOP		= 12 << 12,
  BST_PEA_MOVE_JUMP_BOTTOM	= 13 << 12,
  BST_PEA_MOVE_PREV_PATTERN	= 14 << 12,
  BST_PEA_MOVE_NEXT_PATTERN	= 15 << 12,
  BST_PEA_MOVE_MASK		= 0x0000f000,
  /* octave shifting
   */
  BST_PEA_OCTAVE_SHIFT_0	=  0 << 16,
  BST_PEA_OCTAVE_SHIFT_UP	=  1 << 16,
  BST_PEA_OCTAVE_SHIFT_DOWN	=  2 << 16,
  BST_PEA_OCTAVE_SHIFT_UP2	=  3 << 16,
  BST_PEA_OCTAVE_SHIFT_DOWN2	=  4 << 16,
  BST_PEA_OCTAVE_SHIFT_MASK	= 0x000f0000,
  /* flags
   */
  BST_PEA_WRAP_TO_NOTE		=  1 << 24	/* wrap on bounds */,
  BST_PEA_WRAP_TO_PATTERN	=  1 << 25	/* wrap to next pattern */,
  BST_PEA_WRAP_AS_CONFIG	=  1 << 26,
  BST_PEA_NOTE_RESET		=  1 << 27,
  BST_PEA_INSTRUMENT_RESET	=  1 << 28,
  BST_PEA_SET_INSTRUMENT_0F	=  1 << 29,
  BST_PEA_AFFECT_BASE_OCTAVE	=  1 << 30,
  /* internal tag
   */
  BST_PEA_TAG			=  1 << 31
} BstPEActionType;

#define	BST_PEA(note, shift, instrument, zero, movement, flags)	( \
    BST_PEA_NOTE_ ## note | \
    BST_PEA_OCTAVE_SHIFT_ ## shift | \
    BST_PEA_INSTRUMENT_ ## instrument | \
    BST_PEA_MOVE_ ## movement | \
    (flags))

typedef enum
{
  BST_MOD_000	= 0,
  BST_MOD_00A	= GDK_MOD1_MASK,
  BST_MOD_0C0	= GDK_CONTROL_MASK,
  BST_MOD_0CA	= GDK_CONTROL_MASK | GDK_MOD1_MASK,
  BST_MOD_S00	= GDK_SHIFT_MASK,
  BST_MOD_S0A	= GDK_SHIFT_MASK | GDK_MOD1_MASK,
  BST_MOD_SC0	= GDK_SHIFT_MASK | GDK_CONTROL_MASK,
  BST_MOD_SCA	= GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK
} BstModifierType;


/* --- structures & typedefs --- */
typedef	struct _BstPatternEditor	BstPatternEditor;
typedef	struct _BstPatternEditorClass	BstPatternEditorClass;
typedef guint (*BstPatternEffectAreaWidth)	(BstPatternEditor	*pe,
						 gpointer		 user_data);
typedef void  (*BstPatternEffectAreaDraw)	(BstPatternEditor	*pe,
						 guint			 channel,
						 guint			 row,
						 GdkWindow		*window,
						 guint			 x,
						 guint			 y,
						 guint			 width,
						 guint			 height,
						 GdkGC			*fg_gc,
						 GdkGC			*bg_gc,
						 gpointer		 user_data);
struct _BstPatternEditor
{
  GtkContainer	container;
  
  guint16	channel_grid;
  guint16	row_grid;
  
  guint16	char_width;
  guint16	char_height;
  guint16	char_descent;
  
  GdkWindow	*index_sa;
  GdkWindow	*index;
  GdkWindow	*headline_sa;
  GdkWindow	*headline;
  GdkWindow	*panel_sa;
  GdkWindow	*panel;
  
  guint		popup_tag;
  GtkWidget	*channel_popup;
  
  GtkAdjustment	*vadjustment;
  GtkAdjustment	*hadjustment;
  
  BsePattern	*pattern;
  BseInstrument	**instruments;
  
  /* key handling */
  guint16	channel_page;
  guint16	row_page;
  BstPEActionType wrap_type;	/* either BST_PEA_WRAP_TO_PATTERN or BST_PEA_WRAP_TO_NOTE */
  /* where to move after a note got set */
  guint		next_moves_left : 1;
  guint		next_moves_right : 1;
  guint		next_moves_up : 1;
  guint		next_moves_down : 1;
  
  /* selection */
  guint		in_selection : 1;
  guint		selection_subtract : 1;
  guint32	*saved_selection;
  guint16	selection_channel;
  guint16	selection_row;
  guint		selection_timer;
  guint16	selection_timer_channel;
  guint16	selection_timer_row;
  
  /* draw frame */
  guint16	channel_mod;
  guint16	row_mod;
  
  guint16	focus_channel;
  guint16	focus_row;
  gint		last_row;
  
  gint		base_octave;
  
  /* effect area */
  guint			       ea_width;
  BstPatternEffectAreaWidth    ea_get_width;
  BstPatternEffectAreaDraw     ea_draw;
  gpointer		       ea_data;
  GtkDestroyNotify	       ea_destroy;
};
struct _BstPatternEditorClass
{
  GtkContainerClass	parent_class;
  
  GHashTable		*pea_ktab;
  
  void	(*set_scroll_adjustments) (BstPatternEditor *pe,
				   GtkAdjustment    *hadjustment,
				   GtkAdjustment    *vadjustment);
  void	(*pattern_step)		  (BstPatternEditor *pe,
				   guint	     current_guid,
				   gint		     difference);
  void	(*cell_clicked)		  (BstPatternEditor *pe,
				   guint	     channel,
				   guint	     row,
				   BstCellType	     cell_type,
				   guint	     root_x,
				   guint	     root_y,
				   guint	     button,
				   guint	     time);
};


/* --- prototypes --- */
GtkType	   bst_pattern_editor_get_type	      (void);
GtkWidget* bst_pattern_editor_new	      (BsePattern		*pattern);
void	   bst_pattern_editor_set_hadjustment (BstPatternEditor		*pe,
					       GtkAdjustment		*adjustment);
void	   bst_pattern_editor_set_vadjustment (BstPatternEditor		*pe,
					       GtkAdjustment		*adjustment);
void	   bst_pattern_editor_set_pattern     (BstPatternEditor		*pe,
					       BsePattern		*pattern);
void	   bst_pattern_editor_allocate_tone   (BstPatternEditor		*pe,
					       guint			 channel,
					       guint			 row,
					       guint			*x_p,
					       guint			*y_p,
					       guint			*width_p,
					       guint			*height_p);
void	   bst_pattern_editor_offset_cell     (BstPatternEditor		*pe,
					       BstCellType		 cell_type,
					       guint			*x_p,
					       guint			*y_p,
					       guint			*width_p,
					       guint			*height_p);
gint	   bst_pattern_editor_get_cell	      (BstPatternEditor		*pe,
					       gint			 loc_x,
					       gint			 loc_y,
					       BstCellType		*cell_type_p,
					       gint			*channel_p,
					       gint			*row_p);
void	   bst_pattern_editor_set_focus	      (BstPatternEditor		*pe,
					       guint			 channel,
					       guint			 row);
void	   bst_pattern_editor_adjust_sas      (BstPatternEditor		*pe,
					       gboolean			 check_bounds);
void	   bst_pattern_editor_set_octave      (BstPatternEditor		*pe,
					       gint			 octave);
void	   bst_pattern_editor_mark_row	      (BstPatternEditor		*pe,
					       gint			 row);
void	   bst_pattern_editor_dfl_stepper     (BstPatternEditor		*pe,
					       guint			 current_guid,
					       gint			 difference);
void bst_pattern_editor_set_effect_hooks   (BstPatternEditor		*pe,
					    BstPatternEffectAreaWidth	 ea_width,
					    BstPatternEffectAreaDraw	 ea_draw,
					    gpointer			 user_data,
					    GtkDestroyNotify		 ea_destroy);
void	   bst_pattern_editor_class_clear_keys(BstPatternEditorClass	*pe_class);
void	   bst_pattern_editor_class_set_key   (BstPatternEditorClass	*pe_class,
					       guint16			 keyval,
					       guint16			 modifier,
					       BstPEActionType		 pe_action);
GString*   bst_pattern_editor_class_keydump   (BstPatternEditorClass	*pe_class);





#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_PATTERN_EDITOR_H__ */
