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
#include	"bstpatterneditor.h"

#include	<gtk/gtksignal.h>
#include	<gdk/gdkkeysyms.h>
#include	<string.h>
#include	<ctype.h>


/* FIXME: the layout needs to be changed to something like:
   
   FFFFFFFFFFFFFFFFFF
   F::::::O::::::O::F
   F::::::O::::::O::F
   F::::::O::::::O::F
   F::::::O::::::O::F
   FOOOOOOOOOOOOOOOOF
   F::::::O::::::O::F
   F::::::O::::::O::F
   FFFFFFFFFFFFFFFFFF
   
   F = outer frame
   : = panel
   O = outlined note borders, within panel
   
   also, darkness (F) must be >= darkness (O)
*/

/* Panel allocation:
 * -----------------
 *
 * for a complete 4x4 pannel with channel_grid=2 and row_grid=2, the cell
 * allocation within the panel looks like this:
 *
 * sssssssSssssssOssssssSsssssss
 * s::::::S::::::O::::::S::::::s
 * SSSSSSSSSSSSSSOSSSSSSSSSSSSSS
 * s::::::S::::::O::::::S::::::s
 * OOOOOOOOOOOOOOOOOOOOOOOOOOOOO
 * s::::::S::::::O::::::S::::::s
 * SSSSSSSSSSSSSSOSSSSSSSSSSSSSS
 * s::::::S::::::O::::::S::::::s
 * sssssssSssssssOssssssSsssssss
 *
 * : = note contents, belonging to the cell
 * S = note border (spaced, could be outlined), !within (cell)
 * O = note border (outlined), !within (cell)
 * s = inner panel border, always spaced out (except focus)
 *
 * s, S and O are of size GRID_BORDER
 *
 * for partial exposes, we simply draw the complete lines of the grid
 * enclosing the affected notes (where s, S and O are considered grid lines).
 * to save space, the cell focus is drawn within s, S and O, unfocussing is
 * done like grid drawing for partial exposes.
 *
 * Cell allocation within Tone:
 * ----------------------------
 *
 * +-------------+
 * |*************|
 * |.nnnnn:iii:e.|
 * |*************|
 * +-------------+
 *
 * . = TONE_X_BORDER
 * * = TONE_Y_BORDER
 * n = CHAR_WIDTH (number of Ns depends on NOTE_DIGITS)
 * i = CHAR_WIDTH (number of Is depends on INSTRUMENT_DIGITS)
 * : = CELL_SPACE
 * e = EFFECT_WIDTH (effect area width)
 *
 */

/* TODO:
 * - right click on instrument should popup a per-note instrument menu
 * - left/right click on seqid needs to reset pattern to prev/next
 * - complete recalculation of requisition upon style settings and pattern
 *   changes, this includes *real* calculation of channel and row label
 *   lengths.
 */




/* --- defines --- */
#define	SELECTION_TIMEOUT		(33)
#define PE(pe)				((BstPatternEditor*) (pe))
#define	ALLOCATION(w)			(&((GtkWidget*) (w))->allocation)
#define	X_THICK(w)			(((GtkWidget*) (w))->style->klass->xthickness)
#define	Y_THICK(w)			(((GtkWidget*) (w))->style->klass->ythickness)
#define OUTER_X_BORDER(pe)		(((GtkContainer*) (pe))->border_width)
#define OUTER_Y_BORDER(pe)		(((GtkContainer*) (pe))->border_width)
#define INNER_X_BORDER(w)		(X_THICK(w) + 1)
#define INNER_Y_BORDER(w)		(Y_THICK(w) + 1)
#define X_BORDER(w)			(OUTER_X_BORDER (w) + INNER_X_BORDER (w))
#define Y_BORDER(w)			(OUTER_Y_BORDER (w) + INNER_Y_BORDER (w))
#define N_CHANNELS(pe)			(PE (pe)->pattern->n_channels)
#define N_ROWS(pe)			(PE (pe)->pattern->n_rows)
#define	CHAR_WIDTH(pe)			(PE (pe)->char_width)
#define	CHAR_HEIGHT(pe)			(PE (pe)->char_height)

/* scroll areas */
#define	PANEL_SA_X(pe)			(INDEX_SA_X (pe) + INDEX_SA_WIDTH (pe))
#define	PANEL_SA_Y(pe)			(HEADLINE_SA_Y (pe) + HEADLINE_HEIGHT (pe))
#define	PANEL_SA_WIDTH(pe)		(HEADLINE_SA_WIDTH (pe))
#define	PANEL_SA_HEIGHT(pe)		(INDEX_SA_HEIGHT (pe))
#define	INDEX_SA_X(pe)			(X_BORDER (pe))
#define	INDEX_SA_Y(pe)			(PANEL_SA_Y (pe))
#define	INDEX_SA_WIDTH(pe)		(INDEX_WIDTH (pe))
#define	INDEX_SA_HEIGHT(pe)		(ALLOCATION (pe)->height - INDEX_SA_Y (pe) - Y_BORDER (pe))
#define	HEADLINE_SA_X(pe)		(PANEL_SA_X (pe))
#define	HEADLINE_SA_Y(pe)		(Y_BORDER (pe))
#define	HEADLINE_SA_WIDTH(pe)		(ALLOCATION (pe)->width - HEADLINE_SA_X (pe) - X_BORDER (pe))
#define	HEADLINE_SA_HEIGHT(pe)		(HEADLINE_HEIGHT (pe))

/* panel allocation */
#define	GRID_BORDER			(1)
#define	PANEL_WIDTH(pe)			(N_CHANNELS (pe) * (GRID_BORDER + TONE_WIDTH (pe)) + \
					 GRID_BORDER)
#define	PANEL_HEIGHT(pe)		(N_ROWS (pe) * (GRID_BORDER + TONE_HEIGHT (pe)) + \
					 GRID_BORDER)

/* note cell */
#define	NOTE_DIGITS			(4)
#define	NOTE_FMT			"%c%c% d"
#define	NOTE_EMPTY			"----"
#define	NOTE_WIDTH(pe)			(NOTE_DIGITS * CHAR_WIDTH (pe))
#define	NOTE_X(pe)			(TONE_X_BORDER)
#define	NOTE_Y(pe)			(CHAR_HEIGHT (pe) - PE (pe)->char_descent)

/* instrument cell */
#define	INSTRUMENT_DIGITS		(3)
#define	INSTRUMENT_FMT			"%03u"
#define	INSTRUMENT_EMPTY		"---"
#define	INSTRUMENT_WIDTH(pe)		(INSTRUMENT_DIGITS * CHAR_WIDTH (pe))
#define	INSTRUMENT_X(pe)		(NOTE_X (pe) + NOTE_WIDTH (pe) + CELL_SPACE)
#define	INSTRUMENT_Y(pe)		(CHAR_HEIGHT (pe) - PE (pe)->char_descent)

/* effect cell */
#define	EFFECT_WIDTH(pe)		(PE (pe)->ea_width)
#define	EFFECT_HEIGHT(pe)		(CHAR_HEIGHT (pe))
#define	EFFECT_X(pe)			(INSTRUMENT_X (pe) + INSTRUMENT_WIDTH (pe) + CELL_SPACE)
#define	EFFECT_Y(pe)			(0)

/* tone allocation */
#define	TONE_X_BORDER			(5)
#define	TONE_Y_BORDER			(0)
#define	CELL_SPACE			(3)
#define	TONE_WIDTH(pe)			(2 * TONE_X_BORDER + 2 * CELL_SPACE + \
					 NOTE_WIDTH (pe) + INSTRUMENT_WIDTH (pe) + \
					 EFFECT_WIDTH (pe))
#define	TONE_HEIGHT(pe)			(2 * TONE_Y_BORDER + CHAR_HEIGHT (pe))
#define	TONE_X(pe, ch)			((ch) * (GRID_BORDER + TONE_WIDTH (pe)) + GRID_BORDER)
#define	TONE_Y(pe, rw)			((rw) * (GRID_BORDER + TONE_HEIGHT (pe)) + GRID_BORDER)

/* headline & index */
#define	HEADLINE_HEIGHT(pe)		(/* GRID_BORDER + */ CHAR_HEIGHT (pe) + 1)
#define	HEADLINE_X(pe, ch)		(TONE_X (pe, ch) + NOTE_X (pe))
#define	HEADLINE_Y(pe)			(/* GRID_BORDER + */ NOTE_Y (pe))
#define	INDEX_WIDTH(pe)			(/* GRID_BORDER + */ 3 * CHAR_WIDTH (pe) + 1)
#define	INDEX_X(pe)			(/* GRID_BORDER + */ 0)
#define	INDEX_Y(pe, rw)			(TONE_Y (pe, rw) + NOTE_Y (pe))


/* --- signals --- */
enum
{
  SIGNAL_PATTERN_STEP,
  SIGNAL_CELL_CLICKED,
  LAST_SIGNAL
};
typedef	void	(*SignalPatternStep)	(GtkObject	*object,
					 guint		 current_seqid,
					 gint		 difference,
					 gpointer	 user_data);
typedef	void	(*SignalCellClicked)	(GtkObject	*object,
					 guint		 channel,
					 guint		 row,
					 BstCellType	 cell,
					 guint		 root_x,
					 guint		 root_y,
					 guint		 button,
					 guint		 time,
					 gpointer	 user_data);


/* --- prototypes --- */
static void bst_pattern_editor_class_init	(BstPatternEditorClass	*klass);
static void bst_pattern_editor_init		(BstPatternEditor	*pe);
static void bst_pattern_editor_shutdown		(GtkObject		*object);
static void bst_pattern_editor_destroy		(GtkObject		*object);
static void bst_pattern_editor_finalize		(GtkObject		*object);
static void bst_pattern_editor_set_scroll_adjustments (BstPatternEditor *pe,
						       GtkAdjustment	*hadjustment,
						       GtkAdjustment	*vadjustment);
static void bst_pattern_editor_size_request	(GtkWidget		*widget,
						 GtkRequisition		*requisition);
static void bst_pattern_editor_size_allocate	(GtkWidget		*widget,
						 GtkAllocation		*allocation);
static void bst_pattern_editor_style_set	(GtkWidget		*widget,
						 GtkStyle		*previous_style);
static void bst_pattern_editor_state_changed	(GtkWidget		*widget,
						 guint			 previous_state);
static void bst_pattern_editor_realize		(GtkWidget		*widget);
static void bst_pattern_editor_unrealize	(GtkWidget		*widget);
static void bst_pattern_editor_map		(GtkWidget		*widget);
static void bst_pattern_editor_unmap		(GtkWidget		*widget);
static gint bst_pattern_editor_focus_in		(GtkWidget		*widget,
						 GdkEventFocus		*event);
static gint bst_pattern_editor_focus_out	(GtkWidget		*widget,
						 GdkEventFocus		*event);
static gint bst_pattern_editor_expose		(GtkWidget		*widget,
						 GdkEventExpose		*event);
static void bst_pattern_editor_draw_focus	(GtkWidget		*widget);
static void bst_pattern_editor_draw_main	(BstPatternEditor	*pe);
static void bst_pattern_editor_draw_tone	(BstPatternEditor	*pe,
						 guint			 channel,
						 guint			 row);
static void bst_pattern_editor_draw_grid	(BstPatternEditor	*pe,
						 guint			 first_channel,
						 guint			 first_row,
						 guint			 last_channel,
						 guint			 last_row);
static void bst_pattern_editor_draw_index	(BstPatternEditor	*pe,
						 guint			 row);
static void bst_pattern_editor_draw_headline	(BstPatternEditor	*pe,
						 guint			 channel);
static void bst_pattern_editor_refresh		(BstPatternEditor	*pe,
						 gboolean		 refresh_panel,
						 gboolean		 refresh_headline,
						 gboolean		 refresh_index);
static gint bst_pattern_editor_key_press	(GtkWidget		*widget,
						 GdkEventKey		*event);
static gint bst_pattern_editor_key_release	(GtkWidget		*widget,
						 GdkEventKey		*event);
static gint bst_pattern_editor_button_press	(GtkWidget		*widget,
						 GdkEventButton		*event);
static gint bst_pattern_editor_motion		(GtkWidget		*widget,
						 GdkEventMotion		*event);
static gint bst_pattern_editor_button_release	(GtkWidget		*widget,
						 GdkEventButton		*event);
static void bst_pattern_editor_draw		(GtkWidget		*widget,
						 GdkRectangle		*area);
static void bst_pattern_editor_channel_popup	(BstPatternEditor	*pe,
						 guint			 channel,
						 guint			 mouse_button,
						 guint32		 time);
static void bst_pattern_editor_release_pattern	(BstPatternEditor	*pe);
static void adjustments_value_changed		(GtkAdjustment		*adjustment,
						 BstPatternEditor	*pe);
static void bst_pattern_editor_selection_update (BstPatternEditor *pe,
						 guint		   channel,
						 guint		   row,
						 gint		   panel_sa_x,
						 gint		   panel_sa_y,
						 gboolean	   keep_selection,
						 gboolean	   subtract);
static void bst_pattern_editor_selection_moved	(BstPatternEditor *pe,
						 gint		   panel_sa_x,
						 gint		   panel_sa_y);
static void bst_pattern_editor_selection_done	(BstPatternEditor *pe);


/* --- static variables --- */
static BstPatternEditorClass *bst_pattern_editor_class = NULL;
static GtkContainerClass     *parent_class = NULL;
static guint		      pe_signals[LAST_SIGNAL] = { 0 };
static const gchar	     *class_rc_string =
( "style'BstPatternEditorClass-style'"
  "{"
  "font='-misc-fixed-*-*-*-*-*-130-*-*-*-*-*-*'\n"
  "fg[PRELIGHT]={1.,0.,0.}"
  "}"
  "widget_class'*BstPatternEditor'style'BstPatternEditorClass-style'"
  "\n");


/* --- functions --- */
GtkType
bst_pattern_editor_get_type (void)
{
  static GtkType pe_type = 0;
  
  if (!pe_type)
    {
      GtkTypeInfo pe_info =
      {
	"BstPatternEditor",
	sizeof (BstPatternEditor),
	sizeof (BstPatternEditorClass),
	(GtkClassInitFunc) bst_pattern_editor_class_init,
	(GtkObjectInitFunc) bst_pattern_editor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      pe_type = gtk_type_unique (GTK_TYPE_CONTAINER, &pe_info);
    }
  
  return pe_type;
}

static void
bst_pattern_editor_marshal_pattern_step (GtkObject	*object,
					 GtkSignalFunc	func,
					 gpointer	func_data,
					 GtkArg		*args)
{
  SignalPatternStep sfunc = (SignalPatternStep) func;
  
  sfunc (object,
	 GTK_VALUE_UINT (args[0]),
	 GTK_VALUE_INT (args[1]),
	 func_data);
}

static void
bst_pattern_editor_marshal_cell_clicked (GtkObject	*object,
					 GtkSignalFunc	func,
					 gpointer	func_data,
					 GtkArg		*args)
{
  SignalCellClicked sfunc = (SignalCellClicked) func;
  
  sfunc (object,
	 GTK_VALUE_UINT (args[0]),
	 GTK_VALUE_UINT (args[1]),
	 GTK_VALUE_UINT (args[2]),
	 GTK_VALUE_UINT (args[3]),
	 GTK_VALUE_UINT (args[4]),
	 GTK_VALUE_UINT (args[5]),
	 GTK_VALUE_UINT (args[6]),
	 func_data);
}

static void
bst_pattern_editor_class_init (BstPatternEditorClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;
  
  bst_pattern_editor_class = class;
  parent_class = gtk_type_class (GTK_TYPE_CONTAINER);
  
  object_class->shutdown = bst_pattern_editor_shutdown;
  object_class->destroy = bst_pattern_editor_destroy;
  object_class->finalize = bst_pattern_editor_finalize;
  
  widget_class->draw_focus = bst_pattern_editor_draw_focus;
  widget_class->size_request = bst_pattern_editor_size_request;
  widget_class->size_allocate = bst_pattern_editor_size_allocate;
  widget_class->realize = bst_pattern_editor_realize;
  widget_class->unrealize = bst_pattern_editor_unrealize;
  widget_class->style_set = bst_pattern_editor_style_set;
  widget_class->state_changed = bst_pattern_editor_state_changed;
  widget_class->draw = bst_pattern_editor_draw;
  widget_class->expose_event = bst_pattern_editor_expose;
  widget_class->focus_in_event = bst_pattern_editor_focus_in;
  widget_class->focus_out_event = bst_pattern_editor_focus_out;
  widget_class->map = bst_pattern_editor_map;
  widget_class->unmap = bst_pattern_editor_unmap;
  widget_class->key_press_event = bst_pattern_editor_key_press;
  widget_class->key_release_event = bst_pattern_editor_key_release;
  widget_class->button_press_event = bst_pattern_editor_button_press;
  widget_class->motion_notify_event = bst_pattern_editor_motion;
  widget_class->button_release_event = bst_pattern_editor_button_release;
  
  class->set_scroll_adjustments = bst_pattern_editor_set_scroll_adjustments;
  class->pea_ktab = g_hash_table_new (NULL, NULL);
  
  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new ("set_scroll_adjustments",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (BstPatternEditorClass, set_scroll_adjustments),
		    gtk_marshal_NONE__POINTER_POINTER,
		    GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
  pe_signals[SIGNAL_PATTERN_STEP] =
    gtk_signal_new ("pattern_step",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (BstPatternEditorClass, pattern_step),
		    bst_pattern_editor_marshal_pattern_step,
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_UINT,
		    GTK_TYPE_INT);
  pe_signals[SIGNAL_CELL_CLICKED] =
    gtk_signal_new ("cell_clicked",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (BstPatternEditorClass, cell_clicked),
		    bst_pattern_editor_marshal_cell_clicked,
		    GTK_TYPE_NONE, 7,
		    GTK_TYPE_UINT,
		    GTK_TYPE_UINT,
		    GTK_TYPE_UINT,
		    GTK_TYPE_UINT,
		    GTK_TYPE_UINT,
		    GTK_TYPE_UINT,
		    GTK_TYPE_UINT);
  gtk_object_class_add_signals (object_class, pe_signals, LAST_SIGNAL);
  
  gtk_rc_parse_string (class_rc_string);
}

static void
bst_pattern_editor_init (BstPatternEditor *pe)
{
  GtkWidget *widget;
  
  widget = GTK_WIDGET (pe);
  
  GTK_WIDGET_UNSET_FLAGS (pe, GTK_NO_WINDOW);
  GTK_WIDGET_SET_FLAGS (pe, GTK_CAN_FOCUS);
  
  pe->channel_grid = 1;
  pe->row_grid = 4;
  
  pe->char_width = 5;
  pe->char_height = 5;
  pe->char_descent = 2;
  
  pe->index_sa = NULL;
  pe->index = NULL;
  pe->headline_sa = NULL;
  pe->headline = NULL;
  pe->panel_sa = NULL;
  pe->panel = NULL;
  
  pe->popup_tag = 0;
  pe->channel_popup = NULL;
  
  pe->pattern = NULL;
  pe->instruments = NULL;
  pe->wrap_type = 0;
  pe->channel_page = 2;
  pe->row_page = 4;
  
  pe->next_moves_left = FALSE;
  pe->next_moves_right = FALSE;
  pe->next_moves_up = FALSE;
  pe->next_moves_down = TRUE;
  
  pe->in_selection = FALSE;
  pe->selection_subtract = FALSE;
  pe->saved_selection = NULL;
  pe->selection_channel = 0;
  pe->selection_row = 0;
  pe->selection_timer = 0;
  
  pe->focus_channel = 0;
  pe->focus_row = 0;
  pe->last_row = -1;
  pe->base_octave = 0;
  
  pe->vadjustment = NULL;
  pe->hadjustment = NULL;
  
  bst_pattern_editor_set_vadjustment (pe, NULL);
  bst_pattern_editor_set_hadjustment (pe, NULL);
  
  pe->ea_width = 0;
  pe->ea_get_width = NULL;
  pe->ea_draw = NULL;
  pe->ea_data = NULL;
  pe->ea_destroy = NULL;
}

static void
bst_pattern_editor_shutdown (GtkObject *object)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (BST_IS_PATTERN_EDITOR (object));
  
  pe = BST_PATTERN_EDITOR (object);
  
  if (pe->channel_popup)
    {
      gtk_widget_unref (pe->channel_popup);
      pe->channel_popup = NULL;
    }
  
  if (pe->pattern)
    bst_pattern_editor_release_pattern (pe);
  
  GTK_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bst_pattern_editor_destroy (GtkObject *object)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (BST_IS_PATTERN_EDITOR (object));
  
  pe = BST_PATTERN_EDITOR (object);
  
  bst_pattern_editor_set_effect_hooks (pe, NULL, NULL, NULL, NULL);
  
  if (pe->hadjustment)
    gtk_signal_disconnect_by_data (GTK_OBJECT (pe->hadjustment), pe);
  if (pe->vadjustment)
    gtk_signal_disconnect_by_data (GTK_OBJECT (pe->vadjustment), pe);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_pattern_editor_finalize (GtkObject *object)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (BST_IS_PATTERN_EDITOR (object));
  
  pe = BST_PATTERN_EDITOR (object);
  
  gtk_object_unref (GTK_OBJECT (pe->hadjustment));
  gtk_object_unref (GTK_OBJECT (pe->vadjustment));
  
  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bst_pattern_editor_set_hadjustment (BstPatternEditor *pe,
				    GtkAdjustment    *adjustment)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  
  if (pe->hadjustment && pe->hadjustment != adjustment)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (pe->hadjustment),
				     pe);
      gtk_object_unref (GTK_OBJECT (pe->hadjustment));
    }
  
  if (!adjustment)
    adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
						     0.0, 0.0, 0.0));
  
  if (pe->hadjustment != adjustment)
    {
      pe->hadjustment = adjustment;
      gtk_object_ref (GTK_OBJECT (pe->hadjustment));
      gtk_object_sink (GTK_OBJECT (pe->hadjustment));
      
      gtk_signal_connect (GTK_OBJECT (adjustment),
			  "changed",
			  adjustments_value_changed,
			  pe);
      gtk_signal_connect (GTK_OBJECT (adjustment),
			  "value_changed",
			  adjustments_value_changed,
			  pe);
      
      bst_pattern_editor_adjust_sas (pe, TRUE);
    }
}

void
bst_pattern_editor_set_vadjustment (BstPatternEditor *pe,
				    GtkAdjustment    *adjustment)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
  
  if (pe->vadjustment && pe->vadjustment != adjustment)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (pe->vadjustment),
				     pe);
      gtk_object_unref (GTK_OBJECT (pe->vadjustment));
    }
  
  if (!adjustment)
    adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0,
						     0.0, 0.0, 0.0));
  
  if (pe->vadjustment != adjustment)
    {
      pe->vadjustment = adjustment;
      gtk_object_ref (GTK_OBJECT (pe->vadjustment));
      gtk_object_sink (GTK_OBJECT (pe->vadjustment));
      
      gtk_signal_connect (GTK_OBJECT (adjustment),
			  "changed",
			  adjustments_value_changed,
			  pe);
      gtk_signal_connect (GTK_OBJECT (adjustment),
			  "value_changed",
			  adjustments_value_changed,
			  pe);
      
      bst_pattern_editor_adjust_sas (pe, TRUE);
    }
}

static void
bst_pattern_editor_set_scroll_adjustments (BstPatternEditor *pe,
					   GtkAdjustment    *hadjustment,
					   GtkAdjustment    *vadjustment)
{
  if (pe->hadjustment != hadjustment)
    bst_pattern_editor_set_hadjustment (pe, hadjustment);
  if (pe->vadjustment != vadjustment)
    bst_pattern_editor_set_vadjustment (pe, vadjustment);
}

static void
bst_pattern_editor_style_set (GtkWidget *widget,
			      GtkStyle	*previous_style)
{
  BstPatternEditor *pe;
  guint i;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  pe->char_width = 0;
  for (i = 0; i < 256; i++)
    {
      register guint width;
      
      width = gdk_char_width (widget->style->font, i);
      pe->char_width = MAX (pe->char_width, width);
    }
  pe->char_height = widget->style->font->ascent + widget->style->font->descent;
  pe->char_descent = widget->style->font->descent;
}

static void
bst_pattern_editor_state_changed (GtkWidget *widget,
				  guint	     previous_state)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  if (GTK_WIDGET_REALIZED (pe))
    {
      gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (pe));
      gdk_window_set_background (pe->index,
				 (GTK_WIDGET_IS_SENSITIVE (pe)
				  ? &widget->style->bg[GTK_STATE_SELECTED]
				  : &widget->style->bg[GTK_STATE_INSENSITIVE]));
      gdk_window_set_background (pe->headline,
				 (GTK_WIDGET_IS_SENSITIVE (pe)
				  ? &widget->style->bg[GTK_STATE_SELECTED]
				  : &widget->style->bg[GTK_STATE_INSENSITIVE]));
      gdk_window_set_background (pe->panel, &widget->style->base[GTK_WIDGET_STATE (pe)]);
      gdk_window_clear (widget->window);
      gdk_window_clear (pe->index);
      gdk_window_clear (pe->headline);
      gdk_window_clear (pe->panel);
    }
}

GtkWidget*
bst_pattern_editor_new (BsePattern *pattern)
{
  GtkWidget *widget;
  
  widget = gtk_type_new (bst_pattern_editor_get_type ());
  
  if (pattern)
    bst_pattern_editor_set_pattern (BST_PATTERN_EDITOR (widget), pattern);
  
  return widget;
}

static void
bst_pe_note_changed (BstPatternEditor *pe,
		     guint	       channel,
		     guint	       row,
		     BsePattern	      *pattern)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (pattern == pe->pattern);
  
  if (GTK_WIDGET_DRAWABLE (pe))
    {
      bst_pattern_editor_draw_tone (pe, channel, row);
      bst_pattern_editor_draw_grid (pe, channel, row, 0, 0);
    }
}

static void
bst_pe_pattern_changed (BstPatternEditor *pe)
{
  gtk_widget_queue_draw (GTK_WIDGET (pe));
}

static void
bst_pe_size_changed (BstPatternEditor *pe)
{
  guint focus_channel, focus_row;
  guint i;
  GList *list;
  
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  focus_channel = pe->focus_channel;
  focus_row = pe->focus_row;
  
  g_free (pe->instruments);
  pe->instruments = g_new (BseInstrument*, pe->pattern->n_channels);
  for (i = 0, list = BSE_SONG (bse_item_get_super (BSE_ITEM (pe->pattern)))->instruments;
       list && i < pe->pattern->n_channels;
       list = list->next, i++)
    pe->instruments[i] = list->data;
  while (i < pe->pattern->n_channels)
    pe->instruments[i++] = NULL;
  
  gtk_widget_queue_resize (GTK_WIDGET (pe));
  
  bst_pattern_editor_set_focus (pe, focus_channel, focus_row);
}

static void
bst_pattern_editor_fetch_pattern_sibling (BstPatternEditor *pe,
					  BseItem	   *container,
					  BsePattern	   *pattern)
{
  BseItem *item;
  guint seqid;
  
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  g_return_if_fail (pe->pattern == pattern);

  seqid = bse_item_get_seqid (BSE_ITEM (pattern));
  container = BSE_ITEM (pattern)->parent;

  g_return_if_fail (container != NULL);

  item = bse_container_get_item (BSE_CONTAINER (container), BSE_TYPE_PATTERN, seqid + 1);
  if (!item && seqid > 1)
    item = bse_container_get_item (BSE_CONTAINER (container), BSE_TYPE_PATTERN, seqid - 1);
  if (!item)
    bst_pattern_editor_release_pattern (pe);
  else
    bst_pattern_editor_set_pattern (pe, BSE_PATTERN (item));
}

static void
bst_pattern_editor_release_pattern (BstPatternEditor *pe)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (BSE_IS_PATTERN (pe->pattern));
  
  if (pe->in_selection)
    bst_pattern_editor_selection_done (pe);
  
  bse_object_remove_notifiers_by_func (pe->pattern,
				       bst_pattern_editor_release_pattern,
				       pe);
  bse_object_remove_notifiers_by_func (pe->pattern,
				       bst_pattern_editor_fetch_pattern_sibling,
				       pe);
  bse_object_remove_notifiers_by_func (pe->pattern,
				       bst_pe_size_changed,
				       pe);
  bse_object_remove_notifiers_by_func (pe->pattern,
				       bst_pe_note_changed,
				       pe);
  bse_object_remove_notifiers_by_func (pe->pattern,
				       bst_pe_pattern_changed,
				       pe);
  pe->pattern = NULL;
  g_free (pe->instruments);
  pe->instruments = NULL;
}

void
bst_pattern_editor_set_pattern (BstPatternEditor *pe,
				BsePattern	 *pattern)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (BSE_IS_PATTERN (pattern));
  
  if (pe->pattern != pattern)
    {
      if (pe->pattern)
	bst_pattern_editor_release_pattern (pe);
      
      pe->pattern = pattern;
      bse_object_add_data_notifier (pattern,
				    "destroy",
				    bst_pattern_editor_release_pattern,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "set_parent",
				    bst_pattern_editor_fetch_pattern_sibling,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "size_changed",
				    bst_pe_size_changed,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "note_changed",
				    bst_pe_note_changed,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "note_selected",
				    bst_pe_note_changed,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "note_unselected",
				    bst_pe_note_changed,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "seqid_changed",
				    bst_pe_pattern_changed,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "name_set",
				    bst_pe_pattern_changed,
				    pe);
      bse_object_add_data_notifier (pattern,
				    "param_changed",
				    bst_pe_pattern_changed,
				    pe);
      bst_pe_size_changed (pe);
    }
}

static gboolean
bst_pattern_editor_get_clamped_cell (BstPatternEditor *pe,
				     gint	       loc_x,
				     gint	       loc_y,
				     BstCellType      *cell_type_p,
				     gint	      *channel_p,
				     gint	      *row_p)
{
  gboolean ret_val;
  
  ret_val = bst_pattern_editor_get_cell (pe, loc_x, loc_y, cell_type_p, channel_p, row_p);
  if (channel_p)
    *channel_p = CLAMP (*channel_p, 0, N_CHANNELS (pe) - 1);
  if (row_p)
    *row_p = CLAMP (*row_p, 0, N_ROWS (pe) - 1);
  
  return ret_val;
}

gboolean
bst_pattern_editor_get_cell (BstPatternEditor *pe,
			     gint	       loc_x,
			     gint	       loc_y,
			     BstCellType      *cell_type_p,
			     gint	      *channel_p,
			     gint	      *row_p)
{
  gint x, y;
  gint row, channel;
  BstCellType cell_type;
  gboolean check_cell, within_panel;
  
  if (cell_type_p) *cell_type_p = BST_CELL_INVALID;
  if (channel_p) *channel_p = -1;
  if (row_p) *row_p = -1;
  
  g_return_val_if_fail (pe != NULL, FALSE);
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR (pe), FALSE);
  
  x = 0; /* horizontal offset */
  y = 0; /* vertical offset */
  
  /* let's go!
   */
  cell_type = BST_CELL_INVALID;
  check_cell = TRUE;
  within_panel = TRUE;
  
  /* figure row and strip to begin of first row
   */
  row = loc_y ? loc_y  / (gint) (GRID_BORDER + TONE_HEIGHT (pe)) : 0;
  if (loc_y < 0 || loc_y > PANEL_HEIGHT (pe) || row >= N_ROWS (pe))
    {
      check_cell = FALSE;
      within_panel = FALSE;
    }
  else
    {
      guint offset;
      
      /* check for row spaces
       */
      offset = loc_y % (GRID_BORDER + TONE_HEIGHT (pe) + GRID_BORDER);
      if (offset < GRID_BORDER || offset >= GRID_BORDER + TONE_HEIGHT (pe))
	check_cell = FALSE;
    }
  
  /* figure channel and strip to begin of first channel
   */
  channel = loc_x ? loc_x / (gint) (GRID_BORDER + TONE_WIDTH (pe)) : 0;
  if (loc_x < 0 || loc_x > PANEL_WIDTH (pe) || channel >= N_CHANNELS (pe))
    {
      check_cell = FALSE;
      within_panel = FALSE;
    }
  else
    {
      guint offset;
      
      /* check for channel spaces
       */
      offset = loc_x % (GRID_BORDER + TONE_WIDTH (pe) + GRID_BORDER);
      if (offset < GRID_BORDER || offset >= GRID_BORDER + TONE_WIDTH (pe))
	check_cell = FALSE;
    }
  
  /* offset into tone
   */
  if (check_cell)
    {
      loc_x -= channel * (GRID_BORDER + TONE_WIDTH (pe));
      loc_y -= row * (GRID_BORDER + TONE_HEIGHT (pe));
      x = GRID_BORDER + TONE_X_BORDER;
      y = GRID_BORDER + TONE_Y_BORDER;
      if (loc_y <= y ||
	  loc_y >= y + TONE_HEIGHT (pe))
	check_cell = FALSE;
    }
  
  /* check for note cell
   */
  if (check_cell &&
      loc_x >= x &&
      loc_x <= x + NOTE_WIDTH (pe))
    {
      cell_type = BST_CELL_NOTE;
      check_cell = FALSE;
    }
  else
    x += NOTE_WIDTH (pe) + CELL_SPACE;
  
  /* check for instrument cell
   */
  if (check_cell &&
      loc_x >= x &&
      loc_x <= x + INSTRUMENT_WIDTH (pe))
    {
      cell_type = BST_CELL_INSTRUMENT;
      check_cell = FALSE;
    }
  else
    x += INSTRUMENT_WIDTH (pe) + CELL_SPACE;
  
  /* finally, check for effect cell
   */
  if (check_cell &&
      loc_x >= x &&
      loc_x <= x + EFFECT_WIDTH (pe))
    cell_type = BST_CELL_EFFECT;
  
  /* return values
   */
  if (cell_type_p)
    *cell_type_p = cell_type;
  if (channel_p)
    *channel_p = channel;
  if (row_p)
    *row_p = row;
  
  return within_panel;
}

static void
bst_pattern_editor_size_request	(GtkWidget	*widget,
				 GtkRequisition *requisition)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  g_return_if_fail (requisition != NULL);
  
  pe = BST_PATTERN_EDITOR (widget);
  
  if (pe->ea_get_width)
    pe->ea_width = pe->ea_get_width (pe, pe->ea_data);
  else
    pe->ea_width = 0;
  
  requisition->width = PANEL_SA_X (pe) + PANEL_WIDTH (pe) + X_BORDER (pe);
  requisition->height = PANEL_SA_Y (pe) + PANEL_HEIGHT (pe) + Y_BORDER (pe);
}

static void
bst_pattern_editor_size_allocate (GtkWidget	*widget,
				  GtkAllocation *allocation)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  g_return_if_fail (allocation != NULL);
  
  pe = BST_PATTERN_EDITOR (widget);
  
  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = MIN (widget->requisition.width, allocation->width);
  widget->allocation.height = MIN (widget->requisition.height, allocation->height);
  pe->vadjustment->page_size = PANEL_SA_HEIGHT (pe);
  pe->vadjustment->page_increment = PANEL_SA_HEIGHT (pe) / 2;
  pe->vadjustment->step_increment = (TONE_HEIGHT (pe) + GRID_BORDER) * pe->row_grid;
  pe->vadjustment->lower = 0;
  pe->vadjustment->upper = PANEL_HEIGHT (pe);
  
  pe->hadjustment->page_size = PANEL_SA_WIDTH (pe);
  pe->hadjustment->page_increment = PANEL_SA_WIDTH (pe) / 2;
  pe->hadjustment->step_increment = TONE_WIDTH (pe) + GRID_BORDER;
  pe->hadjustment->lower = 0;
  pe->hadjustment->upper = PANEL_WIDTH (pe);
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      widget->allocation.x,
			      widget->allocation.y,
			      widget->allocation.width,
			      widget->allocation.height);
      gdk_window_move_resize (pe->index_sa,
			      INDEX_SA_X (pe),
			      INDEX_SA_Y (pe),
			      INDEX_SA_WIDTH (pe),
			      INDEX_SA_HEIGHT (pe));
      gdk_window_move_resize (pe->headline_sa,
			      HEADLINE_SA_X (pe),
			      HEADLINE_SA_Y (pe),
			      HEADLINE_SA_WIDTH (pe),
			      HEADLINE_SA_HEIGHT (pe));
      gdk_window_move_resize (pe->panel_sa,
			      PANEL_SA_X (pe),
			      PANEL_SA_Y (pe),
			      PANEL_SA_WIDTH (pe),
			      PANEL_SA_HEIGHT (pe));
      gdk_window_resize (pe->index,
			 INDEX_SA_WIDTH (pe),
			 PANEL_HEIGHT (pe));
      gdk_window_resize (pe->headline,
			 PANEL_WIDTH (pe),
			 HEADLINE_SA_HEIGHT (pe));
      gdk_window_resize (pe->panel,
			 PANEL_WIDTH (pe),
			 PANEL_HEIGHT (pe));
    }
  
  gtk_adjustment_changed (pe->hadjustment);
  gtk_adjustment_changed (pe->vadjustment);
  bst_pattern_editor_adjust_sas (pe, TRUE);
}

static void
adjustments_value_changed (GtkAdjustment    *adjustment,
			   BstPatternEditor *pe)
{
  if (GTK_WIDGET_REALIZED (pe))
    {
      gdk_window_move (pe->index,
		       0,
		       - pe->vadjustment->value);
      gdk_window_move (pe->headline,
		       - pe->hadjustment->value,
		       0);
      gdk_window_move (pe->panel,
		       - pe->hadjustment->value,
		       - pe->vadjustment->value);
    }
}

void
bst_pattern_editor_adjust_sas (BstPatternEditor *pe,
			       gboolean		 check_bounds)
{
  GtkWidget *widget;
  gfloat oh_value;
  gfloat ov_value;
  
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  check_bounds = check_bounds != 0;
  
  widget = GTK_WIDGET (pe);
  
  if (!pe->vadjustment || !pe->hadjustment)
    return;
  
  ov_value = pe->vadjustment->value;
  oh_value = pe->hadjustment->value;
  do
    {
      gfloat horz_value;
      gfloat vert_value;
      guint horz_bound;
      guint vert_bound;
      guint width = GRID_BORDER + TONE_WIDTH (pe) + GRID_BORDER;
      guint height = GRID_BORDER + TONE_HEIGHT (pe) + GRID_BORDER;
      
      if (!pe->in_selection)
	{
	  if (check_bounds)
	    {
	      horz_bound = 0;
	      vert_bound = 0;
	    }
	  else
	    {
	      horz_bound = pe->focus_channel;
	      vert_bound = pe->focus_row;
	    }
	}
      else /* pe->in_selection */
	{
	  if (check_bounds)
	    {
	      horz_bound = pe->focus_channel;
	      vert_bound = pe->focus_row;
	    }
	  else
	    {
	      horz_bound = pe->selection_channel;
	      vert_bound = pe->selection_row;
	    }
	}
      
      horz_bound *= GRID_BORDER + TONE_WIDTH (pe);
      vert_bound *= GRID_BORDER + TONE_HEIGHT (pe);
      horz_value = pe->hadjustment->value;
      vert_value = pe->vadjustment->value;
      
      if (horz_value > horz_bound)
	pe->hadjustment->value = horz_bound;
      if (horz_value + PANEL_SA_WIDTH (pe) < horz_bound + width)
	pe->hadjustment->value = horz_bound + width - PANEL_SA_WIDTH (pe);
      if (vert_value > vert_bound)
	pe->vadjustment->value = vert_bound;
      if (vert_value + PANEL_SA_HEIGHT (pe) < vert_bound + height)
	pe->vadjustment->value = vert_bound + height - PANEL_SA_HEIGHT (pe);
    }
  while (check_bounds-- > 0);
  
  if (ov_value != pe->vadjustment->value)
    gtk_adjustment_value_changed (pe->vadjustment);
  if (oh_value != pe->hadjustment->value)
    gtk_adjustment_value_changed (pe->hadjustment);
}

static void
bst_pattern_editor_realize (GtkWidget *widget)
{
  BstPatternEditor *pe;
  GdkWindowAttr attributes;
  gint attributes_mask;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  
  attributes.window_type = GDK_WINDOW_CHILD;
  
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, pe);
  
  attributes.x = INDEX_SA_X (pe);
  attributes.y = INDEX_SA_Y (pe);
  attributes.width = INDEX_SA_WIDTH (pe);
  attributes.height = INDEX_SA_HEIGHT (pe);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  pe->index_sa = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (pe->index_sa, pe);
  
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = INDEX_SA_WIDTH (pe);
  attributes.height = PANEL_HEIGHT (pe);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  pe->index = gdk_window_new (pe->index_sa, &attributes, attributes_mask);
  gdk_window_set_user_data (pe->index, pe);
  
  attributes.x = HEADLINE_SA_X (pe);
  attributes.y = HEADLINE_SA_Y (pe);
  attributes.width = HEADLINE_SA_WIDTH (pe);
  attributes.height = HEADLINE_SA_HEIGHT (pe);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  pe->headline_sa = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (pe->headline_sa, pe);
  
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = PANEL_HEIGHT (pe);
  attributes.height = HEADLINE_SA_HEIGHT (pe);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  pe->headline = gdk_window_new (pe->headline_sa, &attributes, attributes_mask);
  gdk_window_set_user_data (pe->headline, pe);
  
  attributes.x = PANEL_SA_X (pe);
  attributes.y = PANEL_SA_Y (pe);
  attributes.width = PANEL_SA_WIDTH (pe);
  attributes.height = PANEL_SA_HEIGHT (pe);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  pe->panel_sa = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (pe->panel_sa, pe);
  
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = PANEL_HEIGHT (pe);
  attributes.height = PANEL_HEIGHT (pe);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_BUTTON1_MOTION_MASK |
			    GDK_KEY_PRESS_MASK |
			    GDK_KEY_RELEASE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  pe->panel = gdk_window_new (pe->panel_sa, &attributes, attributes_mask);
  gdk_window_set_user_data (pe->panel, pe);
  
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, pe->index_sa, GTK_STATE_INSENSITIVE);
  gtk_style_set_background (widget->style, pe->headline_sa, GTK_STATE_INSENSITIVE);
  gtk_style_set_background (widget->style, pe->panel_sa, GTK_STATE_INSENSITIVE);
  gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (pe));
  gdk_window_set_background (pe->index,
			     (GTK_WIDGET_IS_SENSITIVE (pe)
			      ? &widget->style->bg[GTK_STATE_SELECTED]
			      : &widget->style->bg[GTK_STATE_INSENSITIVE]));
  gdk_window_set_background (pe->headline,
			     (GTK_WIDGET_IS_SENSITIVE (pe)
			      ? &widget->style->bg[GTK_STATE_SELECTED]
			      : &widget->style->bg[GTK_STATE_INSENSITIVE]));
  gdk_window_set_background (pe->panel, &widget->style->base[GTK_WIDGET_STATE (pe)]);
  
  bst_pattern_editor_adjust_sas (pe, FALSE);
}

static void
bst_pattern_editor_unrealize (GtkWidget *widget)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  gdk_window_set_user_data (pe->index_sa, NULL);
  gdk_window_destroy (pe->index_sa);
  pe->index_sa = NULL;
  gdk_window_set_user_data (pe->index, NULL);
  gdk_window_destroy (pe->index);
  pe->index = NULL;
  gdk_window_set_user_data (pe->headline_sa, NULL);
  gdk_window_destroy (pe->headline_sa);
  pe->headline_sa = NULL;
  gdk_window_set_user_data (pe->headline, NULL);
  gdk_window_destroy (pe->headline);
  pe->headline = NULL;
  gdk_window_set_user_data (pe->panel_sa, NULL);
  gdk_window_destroy (pe->panel_sa);
  pe->panel_sa = NULL;
  gdk_window_set_user_data (pe->panel, NULL);
  gdk_window_destroy (pe->panel);
  pe->panel = NULL;
  
  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
bst_pattern_editor_map (GtkWidget *widget)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  
  gdk_window_show (pe->headline_sa);
  gdk_window_show (pe->headline);
  gdk_window_show (pe->index_sa);
  gdk_window_show (pe->index);
  gdk_window_show (pe->panel_sa);
  gdk_window_show (pe->panel);
  gdk_window_show (widget->window);
}

static void
bst_pattern_editor_unmap (GtkWidget *widget)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  
  if (pe->in_selection)
    bst_pattern_editor_selection_done (pe);
  
  gdk_window_hide (widget->window);
}

static gint
bst_pattern_editor_focus_in (GtkWidget	   *widget,
			     GdkEventFocus *event)
{
  BstPatternEditor *pe;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  pe = BST_PATTERN_EDITOR (widget);
  GTK_WIDGET_SET_FLAGS (pe, GTK_HAS_FOCUS);
  
  gtk_widget_draw_focus (widget);
  
  return TRUE;
}

static gint
bst_pattern_editor_focus_out (GtkWidget	    *widget,
			      GdkEventFocus *event)
{
  BstPatternEditor *pe;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  pe = BST_PATTERN_EDITOR (widget);
  GTK_WIDGET_UNSET_FLAGS (pe, GTK_HAS_FOCUS);
  
  gtk_widget_draw_focus (widget);
  
  return TRUE;
}

static gint
bst_pattern_editor_expose (GtkWidget	  *widget,
			   GdkEventExpose *event)
{
  BstPatternEditor *pe;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  pe = BST_PATTERN_EDITOR (widget);
  
  if (GTK_WIDGET_DRAWABLE (pe))
    {
      gint b_c;
      gint e_c;
      gint b_r;
      gint e_r;
      guint c;
      guint r;
      
      bst_pattern_editor_get_clamped_cell (pe,
					   event->area.x,
					   event->area.y,
					   NULL,
					   &b_c,
					   &b_r);
      bst_pattern_editor_get_clamped_cell (pe,
					   event->area.x + event->area.width,
					   event->area.y + event->area.height,
					   NULL,
					   &e_c,
					   &e_r);
      
      if (event->window == pe->panel)
	{
	  for (c = b_c; c <= e_c; c++)
	    for (r = b_r; r <= e_r; r++)
	      bst_pattern_editor_draw_tone (pe, c, r);
	  bst_pattern_editor_draw_grid (pe, b_c, b_r, e_c + 1, e_r + 1);
	}
      else if (event->window == pe->index)
	{
	  for (r = b_r; r <= e_r; r++)
	    bst_pattern_editor_draw_index (pe, r);
	}
      else if (event->window == pe->headline)
	{
	  for (c = b_c; c <= e_c; c++)
	    bst_pattern_editor_draw_headline (pe, c);
	}
      else if (event->window == widget->window)
	bst_pattern_editor_draw_main (pe);
      else
	{
	  g_warning ("BstPatternEditor: unknown expose event, window=%p, widget=%p (%s)",
		     event->window,
		     gtk_get_event_widget ((GdkEvent*) event),
		     gtk_widget_get_name (gtk_get_event_widget ((GdkEvent*) event)));
	}
    }
  
  return TRUE;
}

static void
bst_pattern_editor_draw (GtkWidget    *widget,
			 GdkRectangle *area)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  g_return_if_fail (area != NULL);
  
  pe = BST_PATTERN_EDITOR (widget);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      bst_pattern_editor_draw_main (pe);
      bst_pattern_editor_refresh (pe, TRUE, TRUE, TRUE);
    }
}

static void
bst_pattern_editor_refresh (BstPatternEditor *pe,
			    gboolean	      refresh_panel,
			    gboolean	      refresh_headline,
			    gboolean	      refresh_index)
{
  gint x, y, width, height;
  gint b_c;
  gint e_c;
  gint b_r;
  gint e_r;
  guint c;
  guint r;
  
  gdk_window_get_position (pe->panel, &x, &y);
  gdk_window_get_size (pe->panel_sa, &width, &height);
  x = -x;
  y = -y;
  bst_pattern_editor_get_clamped_cell (pe,
				       x, y,
				       NULL,
				       &b_c, &b_r);
  bst_pattern_editor_get_clamped_cell (pe,
				       x + width,
				       y + height,
				       NULL,
				       &e_c, &e_r);
  
  if (refresh_headline)
    for (c = b_c; c <= e_c; c++)
      bst_pattern_editor_draw_headline (pe, c);
  if (refresh_index)
    for (r = b_r; r <= e_r; r++)
      bst_pattern_editor_draw_index (pe, r);
  if (refresh_panel)
    for (c = b_c; c <= e_c; c++)
      for (r = b_r; r <= e_r; r++)
	bst_pattern_editor_draw_tone (pe, c, r);
  if (refresh_panel)
    bst_pattern_editor_draw_grid (pe, b_c, b_r, e_c + 1, e_r + 1);
  gdk_flush ();
}

static void
bst_pattern_editor_draw_focus (GtkWidget *widget)
{
  BstPatternEditor *pe;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (widget));
  
  pe = BST_PATTERN_EDITOR (widget);
  
  if (GTK_WIDGET_DRAWABLE (pe))
    {
      bst_pattern_editor_draw_main (pe);
      bst_pattern_editor_draw_tone (pe, pe->focus_channel, pe->focus_row);
      bst_pattern_editor_draw_grid (pe, pe->focus_channel, pe->focus_row, 0, 0);
    }
}

static void
bst_pattern_editor_draw_main (BstPatternEditor *pe)
{
  GtkWidget *widget = GTK_WIDGET (pe);
  guint x, y, width, height, offset;
  GdkGC *fg_gc, *bg_gc, *focus_gc, *unfocus_gc;
  gchar buffer[64];
  
  if (GTK_WIDGET_IS_SENSITIVE (pe))
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_SELECTED];
      bg_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
    }
  else
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_INSENSITIVE];
      bg_gc = widget->style->bg_gc[GTK_STATE_INSENSITIVE];
    }
  focus_gc = widget->style->fg_gc[GTK_STATE_PRELIGHT];
  unfocus_gc = widget->style->bg_gc[GTK_WIDGET_STATE (pe)];
  
  /* draw shadow
   */
  gtk_draw_shadow (widget->style,
		   widget->window,
		   GTK_STATE_NORMAL,
		   GTK_SHADOW_IN,
		   OUTER_X_BORDER (pe),
		   OUTER_Y_BORDER (pe),
		   widget->allocation.width - 2 * OUTER_X_BORDER (pe),
		   widget->allocation.height - 2 * OUTER_Y_BORDER (pe));
  
  /* draw widget focus
   */
  gdk_draw_rectangle (widget->window,
		      GTK_WIDGET_HAS_FOCUS (pe) ? focus_gc : unfocus_gc,
		      FALSE,
		      X_BORDER (pe) - 1,
		      Y_BORDER (pe) - 1,
		      widget->allocation.width - 2 * (X_BORDER (pe) - 1) - 1,
		      widget->allocation.height - 2 * (Y_BORDER (pe) - 1) - 1);
  
  /* draw SeqId
   */
  x = INDEX_SA_X (pe);
  y = HEADLINE_SA_Y (pe);
  width = INDEX_SA_WIDTH (pe);
  height = HEADLINE_SA_HEIGHT (pe);
  gdk_draw_rectangle (widget->window,
		      bg_gc,
		      TRUE,
		      x,
		      y,
		      width - 1,
		      height - 1);
  g_snprintf (buffer, 64, "%u", bse_item_get_seqid (BSE_ITEM (pe->pattern)));
  x += INDEX_X (pe);
  width -= INDEX_X (pe);
  y += HEADLINE_Y (pe);
  height -= HEADLINE_Y (pe);
  offset = MAX (0, (width - gdk_string_measure (widget->style->font, buffer))) / 2;
  gdk_draw_string (widget->window,
		   widget->style->font,
		   fg_gc,
		   x + offset,
		   y,
		   buffer);
}

static void
bst_pattern_editor_draw_index (BstPatternEditor	*pe,
			       guint		 row)
{
  GtkWidget *widget = GTK_WIDGET (pe);
  guint y, i;
  gchar buffer[64];
  GdkGC *fg_gc, *bg_gc, *spaced_gc, *dark_gc;
  
  if (GTK_WIDGET_IS_SENSITIVE (pe))
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_SELECTED];
      bg_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
    }
  else
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_INSENSITIVE];
      bg_gc = widget->style->bg_gc[GTK_STATE_INSENSITIVE];
    }
  spaced_gc = widget->style->base_gc[GTK_WIDGET_STATE (pe)];
  dark_gc = widget->style->dark_gc[GTK_WIDGET_STATE (pe)];
  
  /* draw outlines
   */
  y = row * (GRID_BORDER + TONE_HEIGHT (pe));
  for (i = 0; i < GRID_BORDER; i++)
    gdk_draw_line (pe->index,
		   (row > 0 && row < N_ROWS (pe) && row % pe->row_grid == 0
		    ? dark_gc
		    : spaced_gc),
		   0, y + i,
		   INDEX_WIDTH (pe), y + i);
  row++;
  y = row * (GRID_BORDER + TONE_HEIGHT (pe));
  for (i = 0; i < GRID_BORDER; i++)
    gdk_draw_line (pe->index,
		   (row > 0 && row < N_ROWS (pe) && row % pe->row_grid == 0
		    ? dark_gc
		    : spaced_gc),
		   0, y + i,
		   INDEX_WIDTH (pe), y + i);
  row--;
  
  /* draw row index
   */
  g_snprintf (buffer, 64, "%03u", row + 1);
  gdk_draw_string (pe->index,
		   widget->style->font,
		   fg_gc,
		   INDEX_X (pe),
		   INDEX_Y (pe, row),
		   buffer);
}

static void
bst_pattern_editor_draw_headline (BstPatternEditor *pe,
				  guint		    channel)
{
  GtkWidget *widget = GTK_WIDGET (pe);
  guint x, i;
  gchar buffer[64];
  GdkGC *fg_gc, *bg_gc, *spaced_gc, *dark_gc;
  
  if (GTK_WIDGET_IS_SENSITIVE (pe))
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_SELECTED];
      bg_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
    }
  else
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_INSENSITIVE];
      bg_gc = widget->style->bg_gc[GTK_STATE_INSENSITIVE];
    }
  spaced_gc = widget->style->base_gc[GTK_WIDGET_STATE (pe)];
  dark_gc = widget->style->dark_gc[GTK_WIDGET_STATE (pe)];
  
  /* draw outlines
   */
  x = channel * (GRID_BORDER + TONE_WIDTH (pe));
  for (i = 0; i < GRID_BORDER; i++)
    gdk_draw_line (pe->headline,
		   (channel > 0 && channel < N_CHANNELS (pe) && channel % pe->channel_grid == 0
		    ? dark_gc
		    : spaced_gc),
		   x + i, 0,
		   x + i, HEADLINE_HEIGHT (pe));
  channel++;
  x = channel * (GRID_BORDER + TONE_WIDTH (pe));
  for (i = 0; i < GRID_BORDER; i++)
    gdk_draw_line (pe->headline,
		   (channel > 0 && channel < N_CHANNELS (pe) && channel % pe->channel_grid == 0
		    ? dark_gc
		    : spaced_gc),
		   x + i, 0,
		   x + i, HEADLINE_HEIGHT (pe));
  channel--;
  
  /* draw channel index
   */
  g_snprintf (buffer, 64, "Chnl %u", channel + 1);
  gdk_draw_string (pe->headline,
		   widget->style->font,
		   fg_gc,
		   HEADLINE_X (pe, channel),
		   HEADLINE_Y (pe),
		   buffer);
}

static void
bst_pattern_editor_draw_grid (BstPatternEditor *pe,
			      guint		first_channel,
			      guint		first_row,
			      guint		last_channel,
			      guint		last_row)
{
  GtkWidget *widget = GTK_WIDGET (pe);
  GdkGC *spaced_gc, *dark_gc, *focus_gc;
  guint c, r, i;
  
  g_return_if_fail (first_channel < N_CHANNELS (pe));
  g_return_if_fail (first_row < N_ROWS (pe));
  
  last_channel = CLAMP (last_channel, first_channel + 1, N_CHANNELS (pe));
  last_row = CLAMP (last_row, first_row + 1, N_ROWS (pe));
  spaced_gc = widget->style->base_gc[GTK_WIDGET_STATE (pe)];
  dark_gc = widget->style->dark_gc[GTK_WIDGET_STATE (pe)];
  focus_gc = widget->style->fg_gc[GTK_STATE_PRELIGHT];
  
  /* draw spaced lines
   */
  for (c = first_channel; c <= last_channel; c++)
    {
      guint x = c * (GRID_BORDER + TONE_WIDTH (pe));
      
      if (c > 0 && c < N_CHANNELS (pe) && c % pe->channel_grid == 0)
	continue;
      
      for (i = 0; i < GRID_BORDER; i++)
	gdk_draw_line (pe->panel, spaced_gc,
		       x + i,
		       first_row * (GRID_BORDER + TONE_HEIGHT (pe)),
		       x + i,
		       last_row * (GRID_BORDER + TONE_HEIGHT (pe)) + GRID_BORDER - 1);
    }
  for (r = first_row; r <= last_row; r++)
    {
      guint y = r * (GRID_BORDER + TONE_HEIGHT (pe));
      
      if (r > 0 && r < N_ROWS (pe) && r % pe->row_grid == 0)
	continue;
      
      for (i = 0; i < GRID_BORDER; i++)
	gdk_draw_line (pe->panel, spaced_gc,
		       first_channel * (GRID_BORDER + TONE_WIDTH (pe)),
		       y + i,
		       last_channel * (GRID_BORDER + TONE_WIDTH (pe)) + GRID_BORDER - 1,
		       y + i);
    }
  
  /* draw outlines
   */
  for (c = first_channel; c <= last_channel; c++)
    {
      guint x = c * (GRID_BORDER + TONE_WIDTH (pe));
      
      if (c < 1 || c >= N_CHANNELS (pe) || c % pe->channel_grid != 0)
	continue;
      
      for (i = 0; i < GRID_BORDER; i++)
	gdk_draw_line (pe->panel, dark_gc,
		       x + i,
		       first_row * (GRID_BORDER + TONE_HEIGHT (pe)),
		       x + i,
		       last_row * (GRID_BORDER + TONE_HEIGHT (pe)) + GRID_BORDER - 1);
    }
  for (r = first_row; r <= last_row; r++)
    {
      guint y = r * (GRID_BORDER + TONE_HEIGHT (pe));
      
      if (r < 1 || r >= N_ROWS (pe) || r % pe->row_grid != 0)
	continue;
      
      for (i = 0; i < GRID_BORDER; i++)
	gdk_draw_line (pe->panel, dark_gc,
		       first_channel * (GRID_BORDER + TONE_WIDTH (pe)),
		       y + i,
		       last_channel * (GRID_BORDER + TONE_WIDTH (pe)) + GRID_BORDER - 1,
		       y + i);
    }
  
  /* draw focus
   */
  if (GTK_WIDGET_HAS_FOCUS (pe))
    {
      guint x = pe->focus_channel * (GRID_BORDER + TONE_WIDTH (pe));
      guint y = pe->focus_row * (GRID_BORDER + TONE_HEIGHT (pe));
      
      for (i = 0; i < GRID_BORDER; i++)
	{
	  gdk_draw_line (pe->panel, focus_gc,
			 x + i,
			 y,
			 x + i,
			 y + GRID_BORDER + TONE_HEIGHT (pe) + GRID_BORDER - 1);
	  gdk_draw_line (pe->panel, focus_gc,
			 x + GRID_BORDER + TONE_WIDTH (pe) + i,
			 y,
			 x + GRID_BORDER + TONE_WIDTH (pe) + i,
			 y + GRID_BORDER + TONE_HEIGHT (pe) + GRID_BORDER - 1);
	  gdk_draw_line (pe->panel, focus_gc,
			 x + GRID_BORDER,
			 y + i,
			 x + GRID_BORDER + TONE_WIDTH (pe) - 1,
			 y + i);
	  gdk_draw_line (pe->panel, focus_gc,
			 x + GRID_BORDER,
			 y + GRID_BORDER + TONE_HEIGHT (pe) + i,
			 x + GRID_BORDER + TONE_WIDTH (pe) - 1,
			 y + GRID_BORDER + TONE_HEIGHT (pe) + i);
	}
      if (GRID_BORDER < 2) /* hm, hack alert */
	gdk_draw_rectangle (pe->panel, focus_gc, FALSE,
			    TONE_X (pe, pe->focus_channel),
			    TONE_Y (pe, pe->focus_row),
			    TONE_WIDTH (pe) - 1,
			    TONE_HEIGHT (pe) - 1);
    }
}

static void
bst_pattern_editor_draw_tone (BstPatternEditor *pe,
			      guint		channel,
			      guint		row)
{
  GtkWidget *widget = GTK_WIDGET (pe);
  BsePatternNote *note;
  guint tone_x, tone_y, tone_width, tone_height;
  GdkGC *fg_gc, *bg_gc, *light_gc;
  gchar buffer[64], *p;
  
  g_return_if_fail (channel < N_CHANNELS (pe));
  g_return_if_fail (row < N_ROWS (pe));
  
  note = bse_pattern_peek_note (pe->pattern, channel, row);
  if (note->selected)
    {
      fg_gc = widget->style->fg_gc[GTK_STATE_SELECTED];
      bg_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
    }
  else
    {
      fg_gc = widget->style->fg_gc[GTK_WIDGET_STATE (pe)];
      bg_gc = widget->style->base_gc[GTK_WIDGET_STATE (pe)];
    }
  light_gc = widget->style->dark_gc[GTK_WIDGET_STATE (pe)];
  
  /* base allocation
   */
  tone_width = TONE_WIDTH (pe);
  tone_height = TONE_HEIGHT (pe);
  tone_x = TONE_X (pe, channel);
  tone_y = TONE_Y (pe, row);
  
  /* clear/paint area
   */
  if (note->selected)
    gdk_draw_rectangle (pe->panel, bg_gc, TRUE,
			tone_x, tone_y,
			tone_width, tone_height);
  else
    gdk_window_clear_area (pe->panel, tone_x, tone_y, tone_width, tone_height);
  
  /* draw note
   */
  if (note->note != BSE_NOTE_VOID)
    {
      gchar letter;
      gboolean ht_flag;
      gint octave;
      
      bse_note_examine (note->note, &octave, NULL, &ht_flag, &letter);
      
      g_snprintf (buffer, 64,
		  NOTE_FMT /* %c%c%d */,
		  letter - ('a' - 'A'),
		  ht_flag ? '#' : ' ',
		  octave);
    }
  else
    g_snprintf (buffer, 64, NOTE_EMPTY);
  gdk_draw_string (pe->panel,
		   widget->style->font,
		   note->note != BSE_NOTE_VOID ? fg_gc : light_gc,
		   tone_x + NOTE_X (pe),
		   tone_y + NOTE_Y (pe),
		   buffer);
  
  /* draw instrument
   */
  if (note->instrument)
    g_snprintf (buffer, 64,
		INSTRUMENT_FMT,
		bse_item_get_seqid (BSE_ITEM (note->instrument)));
  else
    g_snprintf (buffer, 64, INSTRUMENT_EMPTY);
  buffer[4] = 0;
  p = buffer;
  while (*p == '0')
    *(p++) = '-';
  gdk_draw_string (pe->panel,
		   widget->style->font,
		   note->instrument ? fg_gc : light_gc,
		   tone_x + INSTRUMENT_X (pe),
		   tone_y + INSTRUMENT_Y (pe),
		   buffer);
  
  /* draw effect
   */
  if (pe->ea_draw)
    pe->ea_draw (pe,
		 channel, row,
		 pe->panel,
		 tone_x + EFFECT_X (pe),
		 tone_y + EFFECT_Y (pe),
		 EFFECT_WIDTH (pe),
		 EFFECT_HEIGHT (pe),
		 fg_gc,
		 bg_gc,
		 pe->ea_data);
}

void
bst_pattern_editor_set_octave (BstPatternEditor *pe,
			       gint		 octave)
{
  gint min_octave;
  gint max_octave;
  
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  bse_note_examine (BSE_MIN_NOTE, &min_octave, NULL, NULL, NULL);
  bse_note_examine (BSE_MAX_NOTE, &max_octave, NULL, NULL, NULL);
  
  pe->base_octave = CLAMP (octave, min_octave, max_octave);
}

void
bst_pattern_editor_set_focus (BstPatternEditor *pe,
			      guint		channel,
			      guint		row)
{
  GtkWidget *widget;
  guint old_channel, old_row;
  
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  widget = GTK_WIDGET (pe);
  
  if (channel >= pe->pattern->n_channels)
    channel = pe->pattern->n_channels - 1;
  if (row >= pe->pattern->n_rows)
    row = pe->pattern->n_rows - 1;
  
  old_channel = pe->focus_channel;
  old_row = pe->focus_row;
  pe->focus_channel = channel;
  pe->focus_row = row;
  
  if (old_channel != pe->focus_channel ||
      old_row != pe->focus_row)
    {
      // bse_pattern_select_note (pe->pattern, pe->focus_channel, pe->focus_row);
      bse_pattern_unselect_except (pe->pattern, pe->focus_channel, pe->focus_row);

      if (GTK_WIDGET_DRAWABLE (pe))
	{
	  if (old_channel < N_CHANNELS (pe) &&
	      old_row < N_ROWS (pe))
	    {
	      bst_pattern_editor_draw_tone (pe, old_channel, old_row);
	      bst_pattern_editor_draw_grid (pe, old_channel, old_row, 0, 0);
	    }
	  bst_pattern_editor_draw_grid (pe, pe->focus_channel, pe->focus_row, 0, 0);
	  bst_pattern_editor_draw_grid (pe, pe->focus_channel, pe->focus_row, 0, 0);
	}
    }

  bst_pattern_editor_adjust_sas (pe, FALSE);
}

static guint32*
save_selection (BstPatternEditor *pe,
		gboolean	  keep_selection)
{
  guint32 *barray;
  
  barray = g_new0 (guint32, (N_CHANNELS (pe) * N_ROWS (pe) + 31) / 32);
  if (keep_selection)
    {
      guint c, r;
      
      for (c = 0; c < N_CHANNELS (pe); c++)
	for (r = 0; r < N_ROWS (pe); r++)
	  {
	    BsePatternNote *note = bse_pattern_peek_note (pe->pattern, c, r);
	    
	    if (note->selected)
	      {
		guint n = N_CHANNELS (pe) * r + c;
		
		barray[n / 32] |= 1 << n % 32;
	      }
	  }
    }
  
  return barray;
}

static inline guint
test_selection (BstPatternEditor *pe,
		guint		  channel,
		guint		  row)
{
  guint n = N_CHANNELS (pe) * row + channel;
  
  return pe->saved_selection[n / 32] & 1 << n % 32;
}

static void
bst_pattern_editor_selection_update (BstPatternEditor *pe,
				     guint	       channel,
				     guint	       row,
				     gint	       panel_sa_x,
				     gint	       panel_sa_y,
				     gboolean	       keep_selection,
				     gboolean	       subtract)
{
  guint r, c;
  gboolean selection_started = FALSE;
  
  channel = MIN (channel, N_CHANNELS (pe) - 1);
  row = MIN (row, N_ROWS (pe) - 1);
  
  if (!pe->in_selection)
    {
      GdkCursor *cursor = gdk_cursor_new (GDK_FLEUR);
      gboolean failed = gdk_pointer_grab (pe->panel_sa, FALSE,
					  GDK_BUTTON_PRESS_MASK |
					  GDK_BUTTON_RELEASE_MASK |
					  GDK_POINTER_MOTION_MASK |
					  GDK_POINTER_MOTION_HINT_MASK,
					  NULL,
					  cursor,
					  GDK_CURRENT_TIME);
      gdk_cursor_destroy (cursor);
      if (failed)
	{
	  gdk_beep ();
	  return;
	}
      
      bse_object_lock (BSE_OBJECT (pe->pattern));
      
      pe->in_selection = TRUE;
      pe->selection_subtract = subtract != FALSE;
      pe->saved_selection = save_selection (pe, keep_selection);
      pe->selection_timer = 0;
      selection_started = TRUE;
    }
  
  if (pe->selection_channel != channel ||
      pe->selection_row != row ||
      selection_started)
    {
      guint b_c = MIN (pe->selection_channel, channel);
      guint b_r = MIN (pe->selection_row, row);
      guint e_c = MAX (pe->selection_channel, channel);
      guint e_r = MAX (pe->selection_row, row);
      
      if (selection_started)
	{
	  b_c = 0;
	  b_r = 0;
	  e_c = N_CHANNELS (pe) - 1;
	  e_r = N_ROWS (pe) - 1;
	}
      else
	{
	  b_c = MIN (pe->focus_channel, b_c);
	  b_r = MIN (pe->focus_row, b_r);
	  e_c = MAX (pe->focus_channel, e_c);
	  e_r = MAX (pe->focus_row, e_r);
	}
      
      pe->selection_channel = channel;
      pe->selection_row = row;
      
      for (c = b_c; c <= e_c; c++)
	for (r = b_r; r <= e_r; r++)
	  {
	    BsePatternNote *note = bse_pattern_peek_note (pe->pattern, c, r);
	    gboolean want_selection;
	    gboolean in_selection = (c >= MIN (pe->focus_channel, pe->selection_channel) &&
				     c <= MAX (pe->focus_channel, pe->selection_channel) &&
				     r >= MIN (pe->focus_row, pe->selection_row) &&
				     r <= MAX (pe->focus_row, pe->selection_row));
	    
	    if (pe->selection_subtract)
	      want_selection = !in_selection && test_selection (pe, c, r);
	    else
	      want_selection = in_selection || test_selection (pe, c, r);
	    
	    if (want_selection && !note->selected)
	      bse_pattern_select_note (pe->pattern, c, r);
	    else if (!want_selection && note->selected)
	      bse_pattern_unselect_note (pe->pattern, c, r);
	  }
      
      bst_pattern_editor_adjust_sas (pe, FALSE);
      bst_pattern_editor_selection_moved (pe, panel_sa_x, panel_sa_y);
    }
}

static gint
selection_timeout (gpointer data)
{
  BstPatternEditor *pe = BST_PATTERN_EDITOR (data);
  
  pe->selection_timer = 0;
  
  if (pe->in_selection)
    {
      gint x, y;
      
      gdk_window_get_pointer (pe->panel_sa, &x, &y, NULL);
      bst_pattern_editor_selection_update (pe,
					   pe->selection_timer_channel,
					   pe->selection_timer_row,
					   x, y,
					   0, 0);
    }
  
  return FALSE;
}

static void
bst_pattern_editor_selection_moved (BstPatternEditor *pe,
				    gint	      panel_sa_x,
				    gint	      panel_sa_y)
{
  if (pe->in_selection)
    {
      gint channel, row;
      gint sa_x, sa_y, sa_width, sa_height;
      BstCellType cell_type;
      
      gdk_window_get_position (pe->panel, &sa_x, &sa_y);
      gdk_window_get_size (pe->panel_sa, &sa_width, &sa_height);
      
      if (bst_pattern_editor_get_cell (pe,
				       panel_sa_x - sa_x,
				       panel_sa_y - sa_y,
				       &cell_type,
				       &channel, &row) &&
	  cell_type &&
	  panel_sa_x > 0 && panel_sa_x < sa_width &&
	  panel_sa_y > 0 && panel_sa_y < sa_height)
	{
	  if (pe->selection_timer)
	    {
	      gtk_timeout_remove (pe->selection_timer);
	      pe->selection_timer = 0;
	    }
	  
	  bst_pattern_editor_selection_update (pe,
					       channel, row,
					       panel_sa_x, panel_sa_y,
					       0, 0);
	}
      else if (panel_sa_x < 0 || panel_sa_x > sa_width ||
	       panel_sa_y < 0 || panel_sa_y > sa_height)
	{
	  pe->selection_timer_channel = MAX (0, channel);
	  pe->selection_timer_row = MAX (0, row);
	  
	  if (!pe->selection_timer)
	    pe->selection_timer = gtk_timeout_add (SELECTION_TIMEOUT,
						   selection_timeout,
						   pe);
	}
    }
}

static void
bst_pattern_editor_selection_done (BstPatternEditor *pe)
{
  if (pe->in_selection)
    {
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
      
      g_free (pe->saved_selection);
      pe->saved_selection = NULL;
      
      pe->selection_subtract = FALSE;
      pe->selection_channel = 0;
      pe->selection_row = 0;
      pe->in_selection = FALSE;
      
      if (pe->selection_timer)
	{
	  gtk_timeout_remove (pe->selection_timer);
	  pe->selection_timer = 0;
	}
      
      bse_object_unlock (BSE_OBJECT (pe->pattern));
      
      /* bst_pattern_editor_adjust_sas (pe, FALSE); */
    }
}

static gint
bst_pattern_editor_motion (GtkWidget	  *widget,
			   GdkEventMotion *event)
{
  BstPatternEditor *pe = BST_PATTERN_EDITOR (widget);
  gboolean handled = FALSE;
  
  if (event->window == pe->panel && !pe->in_selection)
    {
      gint channel, row;
      
      if (bst_pattern_editor_get_clamped_cell (pe, event->x, event->y, NULL, &channel, &row))
	{
	  gint x, y;
	  
	  gdk_window_get_pointer (pe->panel_sa, &x, &y, NULL);
	  bst_pattern_editor_selection_update (pe,
					       channel, row,
					       x, y,
					       event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK),
					       event->state & GDK_CONTROL_MASK);
	}
      
      handled = TRUE;
    }
  else if (event->window == pe->panel_sa && pe->in_selection)
    {
      gint x, y;
      
      gdk_window_get_pointer (pe->panel_sa, &x, &y, NULL);
      bst_pattern_editor_selection_moved (pe, x, y);
      
      handled = TRUE;
    }
  
  return handled;
}

static gint
bst_pattern_editor_button_press (GtkWidget	*widget,
				 GdkEventButton *event)
{
  BstPatternEditor *pe = BST_PATTERN_EDITOR (widget);
  gint focus_channel;
  gint focus_row;
  gboolean handled = FALSE;
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);
  
  if (pe->in_selection)
    bst_pattern_editor_selection_done (pe);
  
  if (event->window == pe->panel)
    {
      BstCellType cell_type;
      
      if (bst_pattern_editor_get_clamped_cell (pe,
					       event->x, event->y,
					       &cell_type,
					       &focus_channel, &focus_row))
	{
	  if (event->button == 1)
	    bst_pattern_editor_set_focus (pe, focus_channel, focus_row);
	  if (cell_type > BST_CELL_INVALID)
	    gtk_signal_emit (GTK_OBJECT (pe),
			     pe_signals[SIGNAL_CELL_CLICKED],
			     focus_channel,
			     focus_row,
			     cell_type,
			     (guint) event->x_root,
			     (guint) event->y_root,
			     (guint) event->button,
			     (guint) event->time);
	}
      handled = TRUE;
    }
  else if (event->window == pe->headline)
    {
      gint channel;
      
      if (bst_pattern_editor_get_clamped_cell (pe, event->x, 0, NULL, &channel, NULL) &&
	  event->button == 3 && channel < pe->pattern->n_channels)
	bst_pattern_editor_channel_popup (pe, channel, event->button, event->time);
      
      handled = TRUE;
    }
  
  return handled;
}

static gint
bst_pattern_editor_button_release (GtkWidget	  *widget,
				   GdkEventButton *event)
{
  BstPatternEditor *pe = BST_PATTERN_EDITOR (widget);
  gboolean handled = FALSE;
  
  if (event->button == 1)
    {
      bst_pattern_editor_selection_done (pe);
      handled = TRUE;
    }
  
  return handled;
}

static gint
bst_pattern_editor_key_press (GtkWidget	  *widget,
			      GdkEventKey *event)
{
  BstPatternEditor *pe;
  BstPEActionType pea = 0;
  guint16 modifier;
  guint16 masks[] = {
    BST_MOD_SCA, BST_MOD_SC0, BST_MOD_S0A, BST_MOD_S00,
    BST_MOD_0CA, BST_MOD_0C0, BST_MOD_00A, BST_MOD_000,
  };
  guint n_masks = sizeof (masks) / sizeof (masks[0]);
  guint i;
  BsePatternNote *pnote;
  BseInstrument *instrument;
  gint focus_channel;
  gint focus_row;
  gint note;
  gint difference;
  guint new_focus_channel;
  guint new_focus_row;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR (widget), FALSE);
  
  pe = BST_PATTERN_EDITOR (widget);
  
  modifier = event->state & BST_MOD_SCA;
  for (i = 0; !(pea & BST_PEA_TAG) && i < n_masks; i++)
    if (modifier == masks[i])
      {
	gpointer p = g_hash_table_lookup (BST_PATTERN_EDITOR_GET_CLASS (pe)->pea_ktab,
					  GUINT_TO_POINTER (event->keyval | ((modifier & masks[i]) << 16)));
	
	pea = GPOINTER_TO_UINT (p);
      }
  
  focus_channel = pe->focus_channel;
  focus_row = pe->focus_row;
  pnote = bse_pattern_peek_note (pe->pattern, focus_channel, focus_row);
  note = pnote->note;
  instrument = pnote->instrument;
  
  difference = 0;
  new_focus_channel = 0;
  new_focus_row = 0;
  
  if (pea & BST_PEA_TAG)
    {
      BstPEActionType wrap;
      
      if (pea & BST_PEA_NOTE_MASK)
	note = BSE_NOTE_GENERIC (pea & BST_PEA_NOTE_MASK, pe->base_octave);
      
      if (pea & BST_PEA_INSTRUMENT_MASK)
	{
	  if ((pea & BST_PEA_INSTRUMENT_MASK) == BST_PEA_INSTRUMENT_0F)
	    instrument = pe->instruments[focus_channel];
	  else
	    instrument = bse_song_get_instrument (BSE_SONG (bse_item_get_super (BSE_ITEM (pe->pattern))),
						  (pea & BST_PEA_INSTRUMENT_MASK) >> 8);
	}
      
      if (pea & BST_PEA_SET_INSTRUMENT_0F)
	{
	  pe->instruments[focus_channel] = bse_song_get_instrument (BSE_SONG (bse_item_get_super (BSE_ITEM (pe->pattern))),
								    (pea & BST_PEA_INSTRUMENT_MASK) >> 8);
	}
      
      switch (pea & BST_PEA_MOVE_MASK)
	{
	case  BST_PEA_MOVE_NEXT:
	  focus_channel -= (pe->next_moves_left != 0);
	  focus_channel += (pe->next_moves_right != 0);
	  focus_row -= (pe->next_moves_up != 0);
	  focus_row += (pe->next_moves_down != 0);
	  break;
	case  BST_PEA_MOVE_LEFT:
	  focus_channel--;
	  break;
	case  BST_PEA_MOVE_RIGHT:
	  focus_channel++;
	  break;
	case  BST_PEA_MOVE_UP:
	  focus_row--;
	  break;
	case  BST_PEA_MOVE_DOWN:
	  focus_row++;
	  break;
	case  BST_PEA_MOVE_PAGE_LEFT:
	  focus_channel -= pe->channel_page;
	  break;
	case  BST_PEA_MOVE_PAGE_RIGHT:
	  focus_channel += pe->channel_page;
	  break;
	case  BST_PEA_MOVE_PAGE_UP:
	  focus_row -= pe->row_page;
	  break;
	case  BST_PEA_MOVE_PAGE_DOWN:
	  focus_row += pe->row_page;
	  break;
	case  BST_PEA_MOVE_JUMP_LEFT:
	  focus_channel = 0;
	  break;
	case  BST_PEA_MOVE_JUMP_RIGHT:
	  focus_channel = pe->pattern->n_channels - 1;
	  break;
	case  BST_PEA_MOVE_JUMP_TOP:
	  focus_row = 0;
	  break;
	case  BST_PEA_MOVE_JUMP_BOTTOM:
	  focus_row = pe->pattern->n_rows - 1;
	  break;
	case  BST_PEA_MOVE_PREV_PATTERN:
	  difference = -1;
	  new_focus_channel = focus_channel;
	  new_focus_row = focus_row;
	  break;
	case  BST_PEA_MOVE_NEXT_PATTERN:
	  difference = +1;
	  new_focus_channel = focus_channel;
	  new_focus_row = focus_row;
	  break;
	}
      
      switch (pea & BST_PEA_OCTAVE_SHIFT_MASK)
	{
	case  BST_PEA_OCTAVE_SHIFT_UP:
	  if (pea & BST_PEA_AFFECT_BASE_OCTAVE)
	    bst_pattern_editor_set_octave (pe, pe->base_octave + 1);
	  else
	    note = BSE_NOTE_OCTAVE_UP (note);
	  break;
	case  BST_PEA_OCTAVE_SHIFT_DOWN:
	  if (pea & BST_PEA_AFFECT_BASE_OCTAVE)
	    bst_pattern_editor_set_octave (pe, pe->base_octave - 1);
	  else
	    note = BSE_NOTE_OCTAVE_DOWN (note);
	  break;
	case  BST_PEA_OCTAVE_SHIFT_UP2:
	  if (pea & BST_PEA_AFFECT_BASE_OCTAVE)
	    bst_pattern_editor_set_octave (pe, pe->base_octave + 2);
	  else
	    {
	      note = BSE_NOTE_OCTAVE_UP (note);
	      note = BSE_NOTE_OCTAVE_UP (note);
	    }
	  break;
	case  BST_PEA_OCTAVE_SHIFT_DOWN2:
	  if (pea & BST_PEA_AFFECT_BASE_OCTAVE)
	    bst_pattern_editor_set_octave (pe, pe->base_octave - 2);
	  else
	    {
	      note = BSE_NOTE_OCTAVE_DOWN (note);
	      note = BSE_NOTE_OCTAVE_DOWN (note);
	    }
	  break;
	default:
	  if (pea & BST_PEA_AFFECT_BASE_OCTAVE)
	    bst_pattern_editor_set_octave (pe, 0);
	}
      
      if (pea & BST_PEA_NOTE_RESET)
	note = BSE_NOTE_VOID;
      
      if (pea & BST_PEA_INSTRUMENT_RESET)
	instrument = NULL;
      
      if (pea & BST_PEA_WRAP_AS_CONFIG)
	wrap = pe->wrap_type;
      else
	wrap = pea;
      if (wrap & BST_PEA_WRAP_TO_PATTERN)
	wrap &= BST_PEA_WRAP_TO_PATTERN;
      else
	wrap &= BST_PEA_WRAP_TO_NOTE;
      
      if (focus_channel < 0)
	{
	  if (wrap == BST_PEA_WRAP_TO_NOTE)
	    focus_channel = pe->pattern->n_channels - 1;
	  else
	    focus_channel = 0;
	}
      if (focus_channel >= pe->pattern->n_channels)
	{
	  if (wrap == BST_PEA_WRAP_TO_NOTE)
	    focus_channel = 0;
	  else
	    focus_channel = pe->pattern->n_channels - 1;
	}
      if (focus_row < 0)
	{
	  if (wrap == BST_PEA_WRAP_TO_NOTE)
	    focus_row = pe->pattern->n_rows - 1;
	  else if (wrap == BST_PEA_WRAP_TO_PATTERN)
	    {
	      difference = -1;
	      new_focus_row = pe->pattern->n_rows + focus_row;
	      new_focus_channel = focus_channel;
	      focus_row = 0;
	    }
	  else
	    focus_row = 0;
	}
      if (focus_row >= pe->pattern->n_rows)
	{
	  if (wrap == BST_PEA_WRAP_TO_NOTE)
	    focus_row = 0;
	  else if (wrap == BST_PEA_WRAP_TO_PATTERN)
	    {
	      difference = +1;
	      new_focus_row = focus_row - pe->pattern->n_rows;
	      new_focus_channel = focus_channel;
	      focus_row = pe->pattern->n_rows - 1;
	    }
	  else
	    focus_row = pe->pattern->n_rows - 1;
	}
    }
  
  if (note != pnote->note)
    bse_pattern_set_note (pe->pattern,
			  pe->focus_channel,
			  pe->focus_row,
			  note);
  if (instrument != pnote->instrument)
    bse_pattern_set_instrument (pe->pattern,
				pe->focus_channel,
				pe->focus_row,
				instrument);
  
  if (focus_channel != pe->focus_channel ||
      focus_row != pe->focus_row)
    bst_pattern_editor_set_focus (pe, focus_channel, focus_row);
  
  if (difference != 0)
    {
      BsePattern *pattern;
      
      pattern = pe->pattern;
      gtk_signal_emit (GTK_OBJECT (pe),
		       pe_signals[SIGNAL_PATTERN_STEP],
		       bse_item_get_seqid (BSE_ITEM (pe->pattern)),
		       difference);
      if (pattern != pe->pattern)
	{
	  if (new_focus_channel != pe->focus_channel ||
	      new_focus_row != pe->focus_row)
	    bst_pattern_editor_set_focus (pe, new_focus_channel, new_focus_row);
	}
    }
  
  return pea & BST_PEA_TAG;
}

static gint
bst_pattern_editor_key_release (GtkWidget   *widget,
				GdkEventKey *event)
{
  BstPatternEditor *pe;
  gboolean handled;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR (widget), FALSE);
  
  pe = BST_PATTERN_EDITOR (widget);
  
  handled = FALSE;
  
  return handled;
}

static void
bst_pattern_editor_set_instrument (GtkWidget	    *item,
				   BstPatternEditor *pe)
{
  gpointer data;
  
  g_return_if_fail (item != NULL);
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  data = gtk_object_get_user_data (GTK_OBJECT (item));
  if (data)
    pe->instruments[pe->popup_tag] = BSE_INSTRUMENT (data);
  else
    pe->instruments[pe->popup_tag] = NULL;
}

static void
bst_pattern_editor_channel_popup (BstPatternEditor *pe,
				  guint		    channel,
				  guint		    mouse_button,
				  guint32	    time)
{
  GtkWidget *item;
  GList *list;
  
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  if (pe->channel_popup)
    gtk_widget_unref (pe->channel_popup);
  
  pe->popup_tag = channel;
  pe->channel_popup =
    gtk_widget_new (gtk_menu_get_type (),
		    NULL);
  gtk_widget_ref (pe->channel_popup);
  gtk_object_sink (GTK_OBJECT (pe->channel_popup));
  item = gtk_menu_item_new_with_label ("Instruments");
  gtk_widget_set (item,
		  "GtkObject::user_data", NULL,
		  "GtkWidget::sensitive", FALSE,
		  "GtkWidget::visible", TRUE,
		  "GtkWidget::parent", pe->channel_popup,
		  NULL);
  gtk_widget_lock_accelerators (item);
  item = gtk_menu_item_new ();
  gtk_widget_set (item,
		  "GtkObject::user_data", NULL,
		  "GtkWidget::sensitive", FALSE,
		  "GtkWidget::visible", TRUE,
		  "GtkWidget::parent", pe->channel_popup,
		  NULL);
  gtk_widget_lock_accelerators (item);
  item = gtk_menu_item_new_with_label ("<NONE>");
  gtk_widget_set (item,
		  "GtkObject::user_data", NULL,
		  "GtkObject::signal::activate", bst_pattern_editor_set_instrument, pe,
		  "GtkWidget::sensitive", TRUE,
		  "GtkWidget::visible", TRUE,
		  "GtkWidget::parent", pe->channel_popup,
		  NULL);
  gtk_widget_lock_accelerators (item);
  for (list = BSE_SONG (bse_item_get_super (BSE_ITEM (pe->pattern)))->instruments; list; list = list->next)
    {
      BseInstrument *instrument;
      gchar *string;
      gchar buffer[64];
      
      instrument = list->data;
      
      g_snprintf (buffer, 64, INSTRUMENT_FMT, bse_item_get_seqid (BSE_ITEM (instrument)));
      string = BSE_OBJECT_NAME (instrument);
      if (!string || *string == 0)
	string = BSE_OBJECT_NAME (instrument->input);
      string = g_strconcat (buffer, ") ", string, NULL);
      item = gtk_menu_item_new_with_label (string);
      gtk_widget_set (item,
		      "GtkObject::user_data", instrument,
		      "GtkObject::signal::activate", bst_pattern_editor_set_instrument, pe,
		      "GtkWidget::sensitive", TRUE,
		      "GtkWidget::visible", TRUE,
		      "GtkWidget::parent", pe->channel_popup,
		      NULL);
      gtk_widget_lock_accelerators (item);
      g_free (string);
    }
  gtk_menu_popup (GTK_MENU (pe->channel_popup),
		  NULL, NULL,
		  NULL, NULL,
		  mouse_button, time);
}

void
bst_pattern_editor_mark_row (BstPatternEditor	    *pe,
			     gint		     row)
{
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (row >= -1);
  g_return_if_fail (row < pe->pattern->n_rows);
  
  FIXME (i am bogus);
  
  if (pe->last_row >= 0)
    {
      bst_pattern_editor_draw_tone (pe, 0, pe->last_row);
    }
  pe->last_row = row;
  if (pe->last_row >= 0)
    {
      bst_pattern_editor_draw_tone (pe, 0, pe->last_row);
    }
}

void
bst_pattern_editor_dfl_stepper (BstPatternEditor *pe,
				guint		  current_seqid,
				gint		  difference)
{
  GList *list;
  
  g_return_if_fail (pe != NULL);
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  g_return_if_fail (bse_item_get_seqid (BSE_ITEM (pe->pattern)) == current_seqid);
  
  list = g_list_nth (BSE_SONG (bse_item_get_super (BSE_ITEM (pe->pattern)))->patterns, difference - 1 + current_seqid);
  
  if (list && list->data != (gpointer) pe->pattern)
    bst_pattern_editor_set_pattern (pe, list->data);
  else
    gdk_beep ();
}

void
bst_pattern_editor_set_effect_hooks (BstPatternEditor		 *pe,
				     BstPatternEffectAreaWidth	  ea_width,
				     BstPatternEffectAreaDraw	  ea_draw,
				     gpointer			  user_data,
				     GtkDestroyNotify		  ea_destroy)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR (pe));
  
  if (pe->ea_destroy)
    pe->ea_destroy (pe->ea_data);
  
  pe->ea_width = 0;
  pe->ea_get_width = ea_width;
  pe->ea_draw = ea_draw;
  pe->ea_data = user_data;
  pe->ea_destroy = ea_destroy;
}

void
bst_pattern_editor_class_set_key (BstPatternEditorClass	*class,
				  guint16		 keyval,
				  guint16		 modifier,
				  BstPEActionType	 pe_action)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR_CLASS (class));
  
  g_hash_table_insert (class->pea_ktab,
		       GUINT_TO_POINTER (keyval | ((modifier & BST_MOD_SCA) << 16)),
		       GUINT_TO_POINTER (pe_action | BST_PEA_TAG));
}

void
bst_pattern_editor_class_clear_keys (BstPatternEditorClass *class)
{
  g_return_if_fail (BST_IS_PATTERN_EDITOR_CLASS (class));
  
  g_hash_table_foreach_remove (class->pea_ktab,
			       (GHRFunc) gtk_true,
			       NULL);
}

static void
string_dump_pea (gpointer key,
		 gpointer value,
		 gpointer user_data)
{
  GSList **slist_p = user_data;
  BstPEActionType pea = GPOINTER_TO_UINT (value);
  guint keyval = GPOINTER_TO_UINT (key) & 0xffff;
  guint modifier = GPOINTER_TO_UINT (key) >> 16;
  GString *gstring = g_string_new ("");
  gchar *name;
  
  name = gdk_keyval_name (gdk_keyval_to_lower (keyval));
  
  /* modifier + keyvalname
   */
  g_string_sprintfa (gstring, "%-12.12s ", name);
  g_string_sprintfa (gstring, "%c%c%c ",
		     modifier & BST_MOD_S00 ? 'S' : '_',
		     modifier & BST_MOD_0C0 ? 'C' : '_',
		     modifier & BST_MOD_00A ? 'A' : '_');
  
  /* note
   */
  name = bse_note_to_string (BSE_NOTE_C (0) + (pea & BST_PEA_NOTE_MASK));
  name[0] = toupper (name[0]);
  g_string_sprintfa (gstring, "%-3.3s ", name[0] != 'v' ? name : "");
  g_free (name);

  /* octave
   */
  switch (pea & BST_PEA_MOVE_MASK)
    {
    case BST_PEA_OCTAVE_SHIFT_UP:	name = "+1";	break;
    case BST_PEA_OCTAVE_SHIFT_DOWN:	name = "-1";	break;
    case BST_PEA_OCTAVE_SHIFT_UP2:	name = "+2";	break;
    case BST_PEA_OCTAVE_SHIFT_DOWN2:	name = "-2";	break;
    default:				name = " 0";	break;
    }
  g_string_sprintfa (gstring, "%s ", name);

  /* decode instrument
   */
  if ((pea & BST_PEA_INSTRUMENT_MASK) == BST_PEA_INSTRUMENT_0F)
    g_string_append (gstring, "df ");
  else
    g_string_sprintfa (gstring, "%02u ", (pea & BST_PEA_INSTRUMENT_MASK) >> 8);
  
  /* movement
   */
  switch (pea & BST_PEA_MOVE_MASK)
    {
    case BST_PEA_MOVE_NEXT:		name = "|n| | | |"; break;
    case BST_PEA_MOVE_LEFT:		name = "|l| | | |"; break;
    case BST_PEA_MOVE_RIGHT:		name = "|r| | | |"; break;
    case BST_PEA_MOVE_UP:		name = "|u| | | |"; break;
    case BST_PEA_MOVE_DOWN:		name = "|d| | | |"; break;
    case BST_PEA_MOVE_PAGE_LEFT:	name = "| |L| | |"; break;
    case BST_PEA_MOVE_PAGE_RIGHT:	name = "| |R| | |"; break;
    case BST_PEA_MOVE_PAGE_UP:		name = "| |U| | |"; break;
    case BST_PEA_MOVE_PAGE_DOWN:	name = "| |D| | |"; break;
    case BST_PEA_MOVE_JUMP_LEFT:	name = "| | |L| |"; break;
    case BST_PEA_MOVE_JUMP_RIGHT:	name = "| | |R| |"; break;
    case BST_PEA_MOVE_JUMP_TOP:		name = "| | |T| |"; break;
    case BST_PEA_MOVE_JUMP_BOTTOM:	name = "| | |B| |"; break;
    case BST_PEA_MOVE_PREV_PATTERN:	name = "| | | |P|"; break;
    case BST_PEA_MOVE_NEXT_PATTERN:	name = "| | | |N|"; break;
    default:				name = "| | | | |"; break;
    }
  g_string_sprintfa (gstring, "%s ", name);

  /* flags
   */
  g_string_append_c (gstring, pea & BST_PEA_WRAP_TO_NOTE	? 'B' : '.');
  g_string_append_c (gstring, pea & BST_PEA_WRAP_TO_PATTERN	? 'P' : '.');
  g_string_append_c (gstring, pea & BST_PEA_NOTE_RESET		? 'N' : '.');
  g_string_append_c (gstring, pea & BST_PEA_INSTRUMENT_RESET	? 'I' : '.');
  g_string_append_c (gstring, pea & BST_PEA_SET_INSTRUMENT_0F	? 'F' : '.');
  g_string_append_c (gstring, pea & BST_PEA_AFFECT_BASE_OCTAVE	? 'D' : '.');
  g_string_append_c (gstring, pea & BST_PEA_WRAP_AS_CONFIG	? 'c' : '.');
  
  g_string_sprintfa (gstring, "\n");

  *slist_p = g_slist_prepend (*slist_p, g_strdup (gstring->str));
  g_string_free (gstring, TRUE);
}

static gint
list_str_cmp (gconstpointer v1,
	      gconstpointer v2)
{
  const gchar *s1 = v1;
  const gchar *s2 = v2;
  guint l1, l2;

  for (l1 = 0; s1[l1]; l1++)
    if (s1[l1] == ' ')
      break;
  for (l2 = 0; s2[l2]; l2++)
    if (s2[l2] == ' ')
      break;

  return (l1 == l2) ? strcmp (s1, s2) : l1 - l2;
}

GString*
bst_pattern_editor_class_keydump (BstPatternEditorClass *class)
{
  GString *gstring;
  GSList *node, *slist = NULL;
  
  g_return_val_if_fail (BST_IS_PATTERN_EDITOR_CLASS (class), NULL);
  
  gstring = g_string_new ("Pattern editor keytable:\n"
			  "========================\n"
			  "\n"
			  "             Modifier (Shift, Control, ALt)\n"
			  "             |   Note\n"
			  "             |   |    Octave shift\n"
			  "             |   |    | Instrument id (df=default instrument)\n"
			  "             |   |    | |  Note movement (Next, Left, Right, Up, Down)\n"
			  "             |   |    | |  | Page movement (Left, Right, Up, Down)\n"
			  "             |   |    | |  | | Jump to border (Left, Right, Top, Bottom)\n"
			  "             |   |    | |  | | | Switch Pattern (Next, Previous)\n"
			  "             |   |    | |  | | | |   Flag Values:\n"
			  "             |   |    | |  | | | |   | B - Wrap around left and right\n"
			  "             |   |    | |  | | | |   |     border\n"
			  "             |   |    | |  | | | |   | P - Wrap to previous/next pattern\n"
			  "             |   |    | |  | | | |   |     at top and bottom border\n"
			  "             |   |    | |  | | | |   | N - Reset (delete) current note\n"
			  "             |   |    | |  | | | |   | I - Reset (delete) current instrument\n"
			  "             |   |    | |  | | | |   | F - Set default instrument of\n"
			  "             |   |    | |  | | | |   |     current channel\n"
			  "             |   |    | |  | | | |   | D - Apply octave shift to the\n"
			  "             |   |    | |  | | | |   |     default octave of the pattern\n"
			  "KeyName      Mod Not Oc In Movement  Flags\n");
  
  g_hash_table_foreach (class->pea_ktab,
			string_dump_pea,
			&slist);

  slist = g_slist_sort (slist, list_str_cmp);

  for (node = slist; node; node = node->next)
    {
      g_string_append (gstring, node->data);
      g_free (node->data);
    }
  g_slist_free (slist);

  return gstring;
}
