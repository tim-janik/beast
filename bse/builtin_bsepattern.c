/* Count Notes - Test BSE Plugin
 * Copyright (C) 1999 Tim Janik
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
 * bsepattern.c: implement basic BsePattern procedures
 */
#include        <bse/bseplugin.h>

#include	<bse/bseprocedure.h>
#include        <bse/bsepattern.h>
#include        <bse/bseinstrument.h>


/* --- BSE types --- */
static BseType type_id_set_note = 0;
static BseType type_id_set_instrument = 0;


/* --- set-note --- */
static void
set_note_setup (BseProcedureClass *proc,
		BseParamSpec     **ipspecs,
		BseParamSpec     **opspecs)
{
  proc->help      = ("Set the note to play for a specific note within a "
		     "pattern, specified through its channel/row coordinates\n"
		     "Input Parameters: Pattern - the pattern to operate on\n"
		     "                  Channel\n"
		     "                  Row\n"
		     "                  Note to play\n"
		     "Output Parameter: none\n");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", "Pattern", NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("channel", "Channel", NULL,
                                      0, BSE_MAX_N_CHANNELS - 1, 1, 0, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("row", "Row", NULL,
                                      0, BSE_MAX_N_ROWS - 1, 1, 0, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_note ("note", "Note", "Note to play",
                                      BSE_MIN_NOTE, BSE_MAX_NOTE,
				      1, BSE_NOTE_VOID, TRUE,
				      BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
set_note_exec (BseProcedureClass *proc,
	       BseParam          *iparams,
	       BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern	= (BsePattern*) (iparams++)->value.v_item;
  guint channel		= (iparams++)->value.v_uint;
  guint row		= (iparams++)->value.v_uint;
  gint note_val		= (iparams++)->value.v_note;
  BsePatternNote *note;

  /* check parameters */
  if (!pattern ||
      channel >= pattern->n_channels ||
      row >= pattern->n_rows)
    return BSE_ERROR_PROC_PARAM_INVAL;

  /* proceed */
  note = bse_pattern_peek_note (pattern, channel, row);
  if (note->note == note_val)
    return BSE_ERROR_PROC_NEEDLESS;

  /* FIXME: start undo */
  bse_pattern_modify_note (pattern,
			   channel, row,
			   note_val,
			   note->instrument,
			   note->selected);
  /* FIXME: end undo */

  /* set output parameters */

  return BSE_ERROR_NONE;
}


/* --- set-instrument --- */
static void
set_instrument_setup (BseProcedureClass *proc,
		      BseParamSpec     **ipspecs,
		      BseParamSpec     **opspecs)
{
  proc->help      = ("Set the instrument to play for a specific note within a "
		     "pattern, specified through its channel/row coordinates.\n"
		     "Input Parameters: Pattern - the pattern to operate on\n"
		     "                  Channel\n"
		     "                  Row\n"
		     "                  Instrument to play\n"
		     "Output Parameter: none\n");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", "Pattern", NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("channel", "Channel", NULL,
                                      0, BSE_MAX_N_CHANNELS - 1, 1, 0, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_uint ("row", "Row", NULL,
                                      0, BSE_MAX_N_ROWS - 1, 1, 0, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_item ("instrument", "Instrument", "Instrument to play",
                                      BSE_TYPE_INSTRUMENT, BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
set_instrument_exec (BseProcedureClass *proc,
		     BseParam          *iparams,
		     BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern	= (BsePattern*) (iparams++)->value.v_item;
  guint channel		= (iparams++)->value.v_uint;
  guint row		= (iparams++)->value.v_uint;
  BseInstrument* instrument = (BseInstrument*) (iparams++)->value.v_item;
  BsePatternNote *note;

  /* check parameters */
  if (!pattern ||
      channel >= pattern->n_channels ||
      row >= pattern->n_rows)
    return BSE_ERROR_PROC_PARAM_INVAL;

  /* proceed */
  note = bse_pattern_peek_note (pattern, channel, row);
  if (note->instrument == instrument)
    return BSE_ERROR_PROC_NEEDLESS;
  
  /* FIXME: start undo */
  bse_pattern_modify_note (pattern,
			   channel, row,
			   note->note,
			   instrument,
			   note->selected);
  /* FIXME: end undo */

  /* set output parameters */

  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_set_note, "BsePattern::set-note",
    "Set a specific note to play",
    set_note_setup, set_note_exec, NULL,
    "/Method/BsePattern/Set Note",
  },
  { &type_id_set_instrument, "BsePattern::set-instrument",
    "Set a specific instrument to play",
    set_instrument_setup, set_instrument_exec, NULL,
    "/Method/BsePattern/Set Instrument",
  },
  { NULL, },
};
BSE_EXPORTS_END;
