/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2000 Dave Seidel
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
 * fldbsepattern.c: BsePattern procedures from feldspar
 */

#include        <bse/bseplugin.h>
#include	<bse/bseprocedure.h>
#include        <bse/bsepattern.h>
#include        <bse/bseinstrument.h>


/* --- BSE types --- */
static GType   type_id_delete_note = 0;
static GType   type_id_insert_note = 0;
static GType   type_id_clear_instrument = 0;
static GType   type_id_fill_instrument = 0;


/* --- delete-note --- */
static void
delete_note_setup (BseProcedureClass *proc,
		   GParamSpec       **in_pspecs,
		   GParamSpec       **out_pspecs)
{
  proc->help      = ("Delete current note and shift remaining notes up");
  proc->author    = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->copyright = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(in_pspecs++) = bse_param_spec_uint ("focus_channel", "Focus Channel", NULL,
					0, 256 - 1, 0, 1, BSE_PARAM_DEFAULT);
  *(in_pspecs++) = bse_param_spec_uint ("focus_row", "Focus Row", NULL,
					0, 256 - 1, 0, 1, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
delete_note_exec (BseProcedureClass *proc,
		  GValue            *in_values,
		  GValue            *out_values)
{
  /* extract parameter values */
  BsePattern *pattern = (BsePattern*) g_value_get_object (in_values++);
  guint c             = g_value_get_uint (in_values++);
  guint r             = g_value_get_uint (in_values++);
  
  /* check parameters */
  if (!BSE_IS_PATTERN (pattern))
    return BSE_ERROR_PROC_PARAM_INVAL;
  
  /* FIXME: start undo */
  
  /* if it's the last note in the channel, just clear it */
  if (r == pattern->n_rows - 1)
    bse_pattern_modify_note (pattern, c, r, BSE_NOTE_VOID, NULL);
  else
    {
      /* iterate over the current channel starting with the current note,
	 replacing each note with the contents of the following note */
      for (; r+1 < pattern->n_rows; r++)
	{
	  BsePatternNote *note = bse_pattern_peek_note (pattern, c, r + 1);
	  
	  bse_pattern_modify_note (pattern, c, r, note->note, note->instrument);
	}
      /* clear the last note in the channel */
      bse_pattern_modify_note (pattern, c, r, BSE_NOTE_VOID, NULL);
    }
  
  /* FIXME: end undo */
  
  /* set output parameters */
  
  return BSE_ERROR_NONE;
}


/* --- insert-note --- */
static void
insert_note_setup (BseProcedureClass *proc,
                   GParamSpec       **in_pspecs,
		   GParamSpec       **out_pspecs)
{
  proc->help      = ("Insert new note and shift remaining notes down");
  proc->author    = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->copyright = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(in_pspecs++) = bse_param_spec_uint ("focus_channel", "Focus Channel", NULL,
					0, 256 - 1, 0, 1, BSE_PARAM_DEFAULT);
  *(in_pspecs++) = bse_param_spec_uint ("focus_row", "Focus Row", NULL,
				      0, 256 - 1, 0, 1, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
insert_note_exec (BseProcedureClass *proc,
                  GValue            *in_values,
		  GValue            *out_values)
{
  /* extract parameter values */
  BsePattern *pattern       = (BsePattern*) g_value_get_object (in_values++);
  guint cur_c               = g_value_get_uint (in_values++);
  guint cur_r               = g_value_get_uint (in_values++);
  gint note_val	            = BSE_NOTE_VOID;
  BseInstrument* instrument = NULL;
  guint r;
  
  /* check parameters */
  if (!BSE_IS_PATTERN (pattern))
    return BSE_ERROR_PROC_PARAM_INVAL;
  
  /* FIXME: start undo */
  
  /* start at the end of the channel and work back, */
  /* replacing each note with its predecessor until */
  /* we get to the current row                      */
  for (r = pattern->n_rows - 1; r > cur_r; r--)
    {
      BsePatternNote *note = bse_pattern_peek_note (pattern, cur_c, r-1);
      
      bse_pattern_modify_note (pattern, cur_c, r, note->note, note->instrument);
    }
  /* now insert the user-specified note in the current row */
  bse_pattern_modify_note (pattern, cur_c, cur_r, note_val, instrument);
  
  /* FIXME: end undo */
  
  /* set output parameters */
  
  return BSE_ERROR_NONE;
}


/* --- clear-instrument --- */
static void
clear_instrument_setup (BseProcedureClass *proc,
			GParamSpec       **in_pspecs,
			GParamSpec       **out_pspecs)
{
  proc->help      = ("Reset instrument for selected notes");
  proc->author    = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->copyright = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
clear_instrument_exec (BseProcedureClass *proc,
		       GValue            *in_values,
		       GValue            *out_values)
{
  /* extract parameter values */
  BsePattern *pattern = (BsePattern*) g_value_get_object (in_values++);
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
	  bse_pattern_set_instrument (pattern, c, r, NULL);
      }
  
  /* FIXME: end undo */
  
  /* set output parameters */
  
  return BSE_ERROR_NONE;
}


/* --- fill-instrument --- */
static void
fill_instrument_setup (BseProcedureClass *proc,
		       GParamSpec       **in_pspecs,
		       GParamSpec       **out_pspecs)
{
  proc->help      = ("Reset instrument for selected notes");
  proc->author    = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->copyright = "Dave Seidel <feldspar@beast.testbit.eu>";
  proc->date      = "2000";
  
  /* input parameters */
  *(in_pspecs++) = g_param_spec_object ("pattern", "Pattern", NULL,
					BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(in_pspecs++) = g_param_spec_object ("instrument", "Instrument", "Instrument to use",
					BSE_TYPE_INSTRUMENT, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
fill_instrument_exec (BseProcedureClass *proc,
		      GValue            *in_values,
		      GValue            *out_values)
{
  /* extract parameter values */
  BsePattern *pattern       = (BsePattern*) g_value_get_object (in_values++);
  BseInstrument *instrument = (BseInstrument*) g_value_get_object (in_values++);
  guint c, r;
  
  /* check parameters */
  if (!BSE_IS_PATTERN (pattern) || (instrument && !BSE_IS_INSTRUMENT (instrument)))
    return BSE_ERROR_PROC_PARAM_INVAL;
  
  /* FIXME: start undo */
  
  /* iterate over the whole pattern, affecting only selected notes */
  for (c = 0; c < pattern->n_channels; c++)
    for (r = 0; r < pattern->n_rows; r++)
      {
	BsePatternNote *note = bse_pattern_peek_note (pattern, c, r);
	
	if (note->selected)
	  bse_pattern_set_instrument (pattern, c, r, instrument);
      }
  
  /* FIXME: end undo */
  
  /* set output parameters */
  
  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN ();
BSE_EXPORT_PROCEDURES = {
  { &type_id_delete_note, "BsePattern+delete-note",
    "Delete the current note", 0,
    delete_note_setup, delete_note_exec, NULL,
    "/Method/BsePattern/Edit/Delete Note",
  },
  { &type_id_insert_note, "BsePattern+insert-note",
    "Insert new note at current cell", 0,
    insert_note_setup, insert_note_exec, NULL,
    "/Method/BsePattern/Edit/Insert Note",
  },
  { &type_id_clear_instrument, "BsePattern+clear-instrument",
    "Reset the instrument for the selected notes", 0,
    clear_instrument_setup, clear_instrument_exec, NULL,
    "/Method/BsePattern/Edit/Clear Instrument",
  },
  { &type_id_fill_instrument, "BsePattern+fill-instrument",
    "Set the instrument for the selected notes", 0,
    fill_instrument_setup, fill_instrument_exec, NULL,
    "/Method/BsePattern/Edit/Fill Instrument",
  },
  { NULL, },
};
BSE_EXPORTS_END;
