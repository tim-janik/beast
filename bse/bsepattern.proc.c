/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * bsepattern.proc.c: BsePattern procedure implementations
 */
#include        <bse/bseplugin.h>
#include	<bse/bseprocedure.h>

#include        <bse/bsepattern.h>
#include        <bse/bseinstrument.h>
#include        <stdlib.h>	/* rand() */


/* --- BSE types --- */
static GType   type_id_clear_content = 0;
static GType   type_id_random_fill = 0;
static GType   type_id_select_invert = 0;
static GType   type_id_select_all = 0;
static GType   type_id_select_none = 0;


/* --- clear-content --- */
static void
clear_content_setup (BseProcedureClass *proc,
		     GParamSpec	      **in_pspecs,
		     GParamSpec	      **out_pspecs)
{
  proc->help      = ("Reset note and instrument contents of the selection"
		     "to none");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
clear_content_exec (BseProcedureClass *proc,
		    GValue	      *in_values,
		    GValue	      *out_values)
{
  /* extract parameter values */
  BsePattern *pattern	= (BsePattern*) g_value_get_object (in_values++);
  guint c, r;

  /* check parameters */
  if (!BSE_IS_PATTERN (pattern))
    return BSE_ERROR_PROC_PARAM_INVAL;

  /* FIXME: start undo */

  /* iterate over the whole pattern, affecting only selected notes */
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = bse_pattern_peek_note (pattern, c, r);

	if (note->selected)
	  bse_pattern_modify_note (pattern, c, r, BSE_NOTE_VOID, NULL);
      }
  
  /* FIXME: end undo */

  /* set output parameters */

  return BSE_ERROR_NONE;
}


/* --- random-fill --- */
static void
random_fill_setup (BseProcedureClass *proc,
		   GParamSpec	    **in_pspecs,
		   GParamSpec	    **out_pspecs)
{
  proc->help      = ("Reset note and instrument contents of the selection"
		     "to none");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(in_pspecs++) = bse_param_spec_int ("seed_value", "Random Seed Value",
				     "Enter any number here, it will be used "
				     "as seed value for the note generator",
				     0, 1000, 1, 1,
				     BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
random_fill_exec (BseProcedureClass *proc,
		  GValue            *in_values,
		  GValue            *out_values)
{
  /* extract parameter values */
  BsePattern *pattern	    = (BsePattern*) g_value_get_object (in_values++);
  gint        seed_value    = g_value_get_int (in_values++);
  guint c, r;

  /* check parameters */
  if (!BSE_IS_PATTERN (pattern))
    return BSE_ERROR_PROC_PARAM_INVAL;

  /* initialize from seed value */
  srand (seed_value);

  /* FIXME: start undo */

  /* iterate over the whole pattern, affecting only selected notes */
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = bse_pattern_peek_note (pattern, c, r);

	if (note->selected)
	  bse_pattern_modify_note (pattern,
				   c, r,
				   BSE_MIN_NOTE + rand () % (BSE_MAX_NOTE - BSE_MIN_NOTE + 1),
				   note->instrument);
      }

  /* FIXME: end undo */

  /* set output parameters */

  return BSE_ERROR_NONE;
}

/* --- multi-select --- */
enum {
  SELECT_INVERT,
  SELECT_ALL,
  SELECT_NONE
};
static void
multi_select_setup (BseProcedureClass *proc,
		    GParamSpec       **in_pspecs,
		    GParamSpec       **out_pspecs)
{
  switch (proc->private_id)
    {
    case SELECT_INVERT:
      proc->help  = ("Invert the current selection");
      break;
    case SELECT_ALL:
      proc->help  = ("Select all notes");
      break;
    case SELECT_NONE:
      proc->help  = ("Unselect all notes");
      break;
    }
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
multi_select_exec (BseProcedureClass *proc,
		   GValue            *in_values,
		   GValue            *out_values)
{
  /* extract parameter values */
  BsePattern *pattern = (BsePattern*) g_value_get_object (in_values++);
  guint32 *selection;
  
  /* check parameters */
  if (!BSE_IS_PATTERN (pattern))
    return BSE_ERROR_PROC_PARAM_INVAL;
  
  /* iterate over the whole selection */
  selection = bse_pattern_selection_new (pattern->n_channels, pattern->n_rows);
  switch (proc->private_id)
    {
      guint c, r;
      
    case SELECT_NONE:
      bse_pattern_selection_fill (selection, FALSE);
      break;
    case SELECT_ALL:
      bse_pattern_selection_fill (selection, TRUE);
      break;
    case SELECT_INVERT:
      bse_pattern_save_selection (pattern, selection);
      for (c = 0; c < BSE_PATTERN_SELECTION_N_CHANNELS (selection); c++)
	for (r = 0; r < BSE_PATTERN_SELECTION_N_ROWS (selection); r++)
	  {
	    if (BSE_PATTERN_SELECTION_TEST (selection, c, r))
	      BSE_PATTERN_SELECTION_UNMARK (selection, c, r);
	    else
	      BSE_PATTERN_SELECTION_MARK (selection, c, r);
	  }
      break;
    }
  bse_pattern_restore_selection (pattern, selection);
  bse_pattern_selection_free (selection);
  
  /* no output parameters */
  
  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_clear_content, "BsePattern+clear-content",
    "Reset note and instrument contents", 0,
    clear_content_setup, clear_content_exec, NULL,
    "/Method/BsePattern/Edit/Clear",
  },
  { &type_id_random_fill, "BsePattern+random-fill",
    "Fill the selection with random notes", 0,
    random_fill_setup, random_fill_exec, NULL,
    "/Method/BsePattern/Tools/Fill Random",
  },
  { &type_id_select_invert, "BsePattern+select-invert",
    "Invert the selection", SELECT_INVERT,
    multi_select_setup, multi_select_exec, NULL,
    "/Method/BsePattern/Select/Invert",
  },
  { &type_id_select_all, "BsePattern+select-all",
    "Select everything", SELECT_ALL,
    multi_select_setup, multi_select_exec, NULL,
    "/Method/BsePattern/Select/All",
  },
  { &type_id_select_none, "BsePattern+select-none",
    "Unselect everything", SELECT_NONE,
    multi_select_setup, multi_select_exec, NULL,
    "/Method/BsePattern/Select/None",
  },
  { NULL, },
};
BSE_EXPORTS_END;
