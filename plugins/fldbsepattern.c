/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2000 Dave Seidel
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
		   BseParamSpec     **ipspecs,
		   BseParamSpec     **opspecs)
{
  proc->help      = ("Delete current note and shift remaining notes up");
  proc->author    = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->copyright = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", NULL, NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("focus_channel", NULL, NULL,
				      0, BSE_MAX_N_CHANNELS - 1, 1, 0, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("focus_row", NULL, NULL,
				      0, BSE_MAX_N_ROWS - 1, 1, 0, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
delete_note_exec (BseProcedureClass *proc,
		  BseParam          *iparams,
		  BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern = (BsePattern*) (iparams++)->value.v_item;
  guint c             = (iparams++)->value.v_uint;
  guint r             = (iparams++)->value.v_uint;
  
  /* check parameters */
  if (!pattern)
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
		   BseParamSpec     **ipspecs,
		   BseParamSpec     **opspecs)
{
  proc->help      = ("Insert new note and shift remaining notes down");
  proc->author    = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->copyright = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", NULL, NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("focus_channel", NULL, NULL,
				      0, BSE_MAX_N_CHANNELS - 1, 1, 0, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("focus_row", NULL, NULL,
				      0, BSE_MAX_N_ROWS - 1, 1, 0, BSE_PARAM_DEFAULT);
#if 0
  *(ipspecs++) = bse_param_spec_note ("note", "Note", "Note to play",
                                      BSE_MIN_NOTE, BSE_MAX_NOTE,
				      1, BSE_NOTE_VOID, TRUE,
				      BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_item ("instrument", "Instrument", "Instrument to use",
                                      BSE_TYPE_INSTRUMENT, BSE_PARAM_DEFAULT);
#endif
  /* output parameters */
}

static BseErrorType
insert_note_exec (BseProcedureClass *proc,
		  BseParam          *iparams,
		  BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern       = (BsePattern*) (iparams++)->value.v_item;
  guint cur_c               = (iparams++)->value.v_uint;
  guint cur_r               = (iparams++)->value.v_uint;
  gint note_val	            = BSE_NOTE_VOID; /* (iparams++)->value.v_note; */
  BseInstrument* instrument = NULL; /* (BseInstrument*) (iparams++)->value.v_item; */
  
  guint r;
  
  /* check parameters */
  if (!pattern)
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
			BseParamSpec     **ipspecs,
			BseParamSpec     **opspecs)
{
  proc->help      = ("Reset instrument for selected notes");
  proc->author    = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->copyright = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", NULL, NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
clear_instrument_exec (BseProcedureClass *proc,
		       BseParam          *iparams,
		       BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern       = (BsePattern*) (iparams++)->value.v_item;
  guint c, r;
  
  /* check parameters */
  if (!pattern)
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
		       BseParamSpec     **ipspecs,
		       BseParamSpec     **opspecs)
{
  proc->help      = ("Reset instrument for selected notes");
  proc->author    = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->copyright = "Dave Seidel <feldspar@beast.gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", NULL, NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_item ("instrument", "Instrument", "Instrument to use",
                                      BSE_TYPE_INSTRUMENT, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
fill_instrument_exec (BseProcedureClass *proc,
		      BseParam          *iparams,
		      BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern       = (BsePattern*) (iparams++)->value.v_item;
  BseInstrument* instrument = (BseInstrument*) (iparams++)->value.v_item;
  guint c, r;
  
  /* check parameters */
  if (!pattern)
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
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
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
