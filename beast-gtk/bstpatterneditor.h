/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __BST_PATTERN_EDITOR_H__
#define __BST_PATTERN_EDITOR_H__

#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- type macros --- */
#define BST_TYPE_PATTERN_EDITOR              (bst_pattern_editor_get_type ())
#define BST_PATTERN_EDITOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PATTERN_EDITOR, BstPatternEditor))
#define BST_PATTERN_EDITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PATTERN_EDITOR, BstPatternEditorClass))
#define BST_IS_PATTERN_EDITOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PATTERN_EDITOR))
#define BST_IS_PATTERN_EDITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PATTERN_EDITOR))
#define BST_PATTERN_EDITOR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PATTERN_EDITOR, BstPatternEditorClass))


/* --- enums --- */
typedef enum
{
  BST_CELL_NONE,
  BST_CELL_NOTE,
  BST_CELL_INSTRUMENT,
  BST_CELL_EFFECT
} BstCellType;
typedef enum
{
  BST_PEA_TYPE_CHANGE_DEFAULTS  = 1, /* change octave & instrument */
  BST_PEA_TYPE_MODIFY_NOTE      = 2, /* modify note & move */
  BST_PEA_TYPE_ACTIVATE_CELL    = 3, /* acivate cells */
  BST_PEA_TYPE_START_FOCUS_SEL  = 4,
  BST_PEA_TYPE_STOP_FOCUS_SEL   = 5,
  BST_PEA_TYPE_MASK		= 0x0f,

  /* cell type */
  BST_PEA_CELL_SHIFT            = 4,
  BST_PEA_CELL_MASK             = 0x0f << BST_PEA_CELL_SHIFT,
  BST_PEA_CELL_NOTE             =    1 << BST_PEA_CELL_SHIFT,
  BST_PEA_CELL_INSTRUMENT       =    2 << BST_PEA_CELL_SHIFT,
  BST_PEA_CELL_EFFECT           =    3 << BST_PEA_CELL_SHIFT,
  BST_PEA_CELL_ANY              = 0,

  /* note setting */
  BST_PEA_NOTE_SHIFT            = 8,
  BST_PEA_NOTE_MASK             = 0x0f << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_C                =    1 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Cis              =    2 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Des              = BST_PEA_NOTE_Cis,
  BST_PEA_NOTE_D                =    3 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Dis              =    4 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Es               = BST_PEA_NOTE_Dis,
  BST_PEA_NOTE_E                =    5 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_F                =    6 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Fis              =    7 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Ges              = BST_PEA_NOTE_Fis,
  BST_PEA_NOTE_G                =    8 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Gis              =    9 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_As               = BST_PEA_NOTE_Gis,
  BST_PEA_NOTE_A                =   10 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Ais              =   11 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_Bes              = BST_PEA_NOTE_Ais,
  BST_PEA_NOTE_B                =   12 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_VOID             =   13 << BST_PEA_NOTE_SHIFT,
  BST_PEA_NOTE_same             = 0, /* leave untouched */

  /* octave shifting */
  BST_PEA_OCTAVE_SHIFT          = 12,
  BST_PEA_OCTAVE_MASK		= 0x0f << BST_PEA_OCTAVE_SHIFT,
  BST_PEA_OCTAVE_UP             =    1 << BST_PEA_OCTAVE_SHIFT,
  BST_PEA_OCTAVE_DOWN           =    2 << BST_PEA_OCTAVE_SHIFT,
  BST_PEA_OCTAVE_UP2            =    3 << BST_PEA_OCTAVE_SHIFT,
  BST_PEA_OCTAVE_DOWN2          =    4 << BST_PEA_OCTAVE_SHIFT,
  BST_PEA_OCTAVE_DFLT           =    5 << BST_PEA_OCTAVE_SHIFT,
  BST_PEA_OCTAVE_same           = 0, /* leave untouched */

  /* instrument setting */
  BST_PEA_INSTRUMENT_SHIFT      = 16,
  BST_PEA_INSTRUMENT_MASK       = 0xff << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_01         =    1 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_02         =    2 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_03         =    3 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_04         =    4 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_05         =    5 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_06         =    6 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_07         =    7 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_08         =    8 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_09         =    9 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_10         =   10 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_11         =   11 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_12         =   12 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_13         =   13 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_14         =   14 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_15         =   15 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_16         =   16 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_17         =   17 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_18         =   18 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_19         =   19 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_20         =   20 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_21         =   21 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_22         =   22 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_23         =   23 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_24         =   24 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_25         =   25 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_26         =   26 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_27         =   27 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_28         =   28 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_29         =   29 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_30         =   30 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_31         =   31 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_32         =   32 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_DFLT       =   33 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_VOID       =   34 << BST_PEA_INSTRUMENT_SHIFT,
  BST_PEA_INSTRUMENT_same       = 0, /* leave untouched */

  /* movements, _MOVE_ and _MOVE_PAGE_ support wrapping */
  BST_PEA_MOVE_SHIFT            = 24,
  BST_PEA_MOVE_MASK             = 0x0f << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_NEXT             =    1 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_LEFT             =    2 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_RIGHT            =    3 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_UP               =    4 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_DOWN             =    5 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_PAGE_LEFT        =    6 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_PAGE_RIGHT       =    7 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_PAGE_UP          =    8 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_PAGE_DOWN        =    9 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_JUMP_LEFT        =   10 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_JUMP_RIGHT       =   11 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_JUMP_TOP         =   12 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_JUMP_BOTTOM      =   13 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_PREV_PATTERN     =   14 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_NEXT_PATTERN     =   15 << BST_PEA_MOVE_SHIFT,
  BST_PEA_MOVE_none             = 0, /* leave untouched */

  /* wrapping */
  BST_PEA_WRAP_MASK		= 0x70000000,
  BST_PEA_WRAP_TO_NOTE          = 1 << 28, /* wrap around borders */
  BST_PEA_WRAP_TO_PATTERN       = 1 << 29, /* wrap to next pattern */
  BST_PEA_WRAP_AS_CONFIG        = 1 << 30,
  BST_PEA_RECTANGLE_SELECT      = 1 << 31
} BstPEActionType;

#define BST_PEA_CONCAT(cell, note, octave, instrument, movement) ( \
    BST_PEA_CELL_ ## cell | \
    BST_PEA_NOTE_ ## note | \
    BST_PEA_OCTAVE_ ## octave | \
    BST_PEA_INSTRUMENT_ ## instrument | \
    BST_PEA_MOVE_ ## movement \
)

typedef enum
{
  BST_MOD_000   = 0,
  BST_MOD_00A   = GDK_MOD1_MASK,
  BST_MOD_0C0   = GDK_CONTROL_MASK,
  BST_MOD_0CA   = GDK_CONTROL_MASK | GDK_MOD1_MASK,
  BST_MOD_S00   = GDK_SHIFT_MASK,
  BST_MOD_S0A   = GDK_SHIFT_MASK | GDK_MOD1_MASK,
  BST_MOD_SC0   = GDK_SHIFT_MASK | GDK_CONTROL_MASK,
  BST_MOD_SCA   = GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK
} BstModifierType;


/* --- structures & typedefs --- */
typedef struct _BstPatternEditor        BstPatternEditor;
typedef struct _BstPatternEditorClass   BstPatternEditorClass;
typedef guint (*BstPatternEffectAreaWidth)      (BstPatternEditor       *pe,
                                                 gpointer                user_data);
typedef void  (*BstPatternEffectAreaDraw)       (BstPatternEditor       *pe,
                                                 guint                   channel,
                                                 guint                   row,
                                                 GdkWindow              *window,
                                                 guint                   x,
                                                 guint                   y,
                                                 guint                   width,
                                                 guint                   height,
                                                 GdkGC                  *fg_gc,
                                                 GdkGC                  *light_gc,
                                                 GdkGC                  *bg_gc,
                                                 gpointer                user_data);
struct _BstPatternEditor
{
  GtkContainer    parent_instance;
  
  guint           channel_grid;
  guint           row_grid;
  
  guint           char_width;
  guint           char_height;
  guint           char_descent;
  
  GdkWindow      *index_sa;
  GdkWindow      *index;
  GdkWindow      *headline_sa;
  GdkWindow      *headline;
  GdkWindow      *panel_sa;
  GdkWindow      *panel;
  
  guint           popup_tag;
  GtkWidget      *channel_popup;
  
  GtkAdjustment  *vadjustment;
  GtkAdjustment  *hadjustment;
  
  BsePattern     *pattern;
  BseInstrument **instruments;
  
  /* key handling */
  guint16         channel_page;
  guint16         row_page;
  BstPEActionType wrap_type;    /* either BST_PEA_WRAP_TO_PATTERN or BST_PEA_WRAP_TO_NOTE */
  /* where to move after a note got set */
  guint           next_moves_left : 1;
  guint           next_moves_right : 1;
  guint           next_moves_up : 1;
  guint           next_moves_down : 1;
  
  /* selection */
  guint           in_selection : 1;
  guint           selection_subtract : 1;
  guint32        *saved_selection;
  guint16         selection_channel;
  guint16         selection_row;
  guint           selection_timer;
  guint16         selection_timer_channel;
  guint16         selection_timer_row;
  
  /* draw frame */
  guint16         channel_mod;
  guint16         row_mod;
  
  guint16         last_focus_channel;
  guint16         last_focus_row;
  guint16         focus_channel;
  guint16         focus_row;
  guint		  focus_changed_handler;
  gint            marked_row;

  gint            base_octave;
  
  /* effect area */
  guint                        ea_width;
  BstPatternEffectAreaWidth    ea_get_width;
  BstPatternEffectAreaDraw     ea_draw;
  gpointer                     ea_data;
  GtkDestroyNotify             ea_destroy;
};
struct _BstPatternEditorClass
{
  GtkContainerClass     parent_class;
  
  GHashTable            *pea_ktab;
  
  void  (*set_scroll_adjustments) (BstPatternEditor *pe,
                                   GtkAdjustment    *hadjustment,
                                   GtkAdjustment    *vadjustment);
  void  (*pattern_step)           (BstPatternEditor *pe,
                                   guint             current_guid,
                                   gint              difference);
  void  (*cell_activate)          (BstPatternEditor *pe,
                                   guint             channel,
                                   guint             row,
                                   BstCellType       cell_type,
                                   guint             root_x,
                                   guint             root_y,
                                   guint             button,
                                   guint             time);
  void  (*focus_changed)          (BstPatternEditor *pe,
                                   guint             channel,
                                   guint             row);
};


/* --- prototypes --- */
GType      bst_pattern_editor_get_type        (void);
GtkWidget* bst_pattern_editor_new             (BsePattern               *pattern);
void       bst_pattern_editor_set_hadjustment (BstPatternEditor         *pe,
                                               GtkAdjustment            *adjustment);
void       bst_pattern_editor_set_vadjustment (BstPatternEditor         *pe,
                                               GtkAdjustment            *adjustment);
void       bst_pattern_editor_set_pattern     (BstPatternEditor         *pe,
                                               BsePattern               *pattern);
void       bst_pattern_editor_allocate_tone   (BstPatternEditor         *pe,
                                               guint                     channel,
                                               guint                     row,
                                               guint                    *x_p,
                                               guint                    *y_p,
                                               guint                    *width_p,
                                               guint                    *height_p);
void       bst_pattern_editor_offset_cell     (BstPatternEditor         *pe,
                                               BstCellType               cell_type,
                                               guint                    *x_p,
                                               guint                    *y_p,
                                               guint                    *width_p,
                                               guint                    *height_p);
gint       bst_pattern_editor_get_cell        (BstPatternEditor         *pe,
                                               gint                      loc_x,
                                               gint                      loc_y,
                                               BstCellType              *cell_type_p,
                                               gint                     *channel_p,
                                               gint                     *row_p);
void       bst_pattern_editor_set_focus       (BstPatternEditor         *pe,
                                               guint                     channel,
                                               guint                     row,
                                               gboolean                  reset_selection);
void       bst_pattern_editor_adjust_sas      (BstPatternEditor         *pe,
                                               gboolean                  check_bounds);
void       bst_pattern_editor_set_octave      (BstPatternEditor         *pe,
                                               gint                      octave);
void       bst_pattern_editor_mark_row        (BstPatternEditor         *pe,
                                               gint                      row);
void       bst_pattern_editor_dfl_stepper     (BstPatternEditor         *pe,
                                               guint                     current_guid,
                                               gint                      difference);
void bst_pattern_editor_set_effect_hooks   (BstPatternEditor            *pe,
                                            BstPatternEffectAreaWidth    ea_width,
                                            BstPatternEffectAreaDraw     ea_draw,
                                            gpointer                     user_data,
                                            GtkDestroyNotify             ea_destroy);
void       bst_pattern_editor_class_clear_keys(BstPatternEditorClass    *pe_class);
void       bst_pattern_editor_class_set_key   (BstPatternEditorClass    *pe_class,
                                               guint16                   keyval,
                                               guint16                   modifier,
                                               BstPEActionType           pe_action);
GString*   bst_pattern_editor_class_keydump   (BstPatternEditorClass    *pe_class);


/* --- selections --- */
void bst_pattern_editor_reset_selection  (BstPatternEditor *pe);
void bst_pattern_editor_select_rectangle (BstPatternEditor *pe,
					  guint		    start_channel,
					  guint		    start_row,
					  guint		    end_channel,
					  guint		    end_row);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PATTERN_EDITOR_H__ */
