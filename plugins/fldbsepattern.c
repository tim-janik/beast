/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2000 Olaf Hoehmann and Tim Janik
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
 * fldbsepattern.c: test implementation of BsePattern procedures
 */
#include        <bse/bseplugin.h>

#include	<bse/bseprocedure.h>
#include        <bse/bsepattern.h>
#include        <bse/bseinstrument.h>
#include	<stdlib.h> /* for rand() */


/* --- BSE types --- */
static BseType type_id_test_content = 0;


/* --- test-content --- */
static void
test_content_setup (BseProcedureClass *proc,
		    BseParamSpec     **ipspecs,
		    BseParamSpec     **opspecs)
{
  proc->help      = ("Reset note and instrument contents of the selection"
		     "to none");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "2000";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_item ("pattern", "Pattern", NULL,
				      BSE_TYPE_PATTERN, BSE_PARAM_DEFAULT);
  *(ipspecs++) = bse_param_spec_int ("seed_value", "Random Seed Value",
				     "Enter any number here, it will be used "
				     "as seed value for the note generator",
				     0, 1000, 1, 1,
				     BSE_PARAM_DEFAULT);
  /* output parameters */
}

static BseErrorType
test_content_exec (BseProcedureClass *proc,
		   BseParam          *iparams,
		   BseParam          *oparams)
{
  /* extract parameter values */
  BsePattern *pattern	    = (BsePattern*) (iparams++)->value.v_item;
  gint        seed_value    = (iparams++)->value.v_int;
  guint c, r;
  
  /* check parameters */
  if (!pattern)
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


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_test_content, "BsePattern::test-content",
    "Random fill notes", 0,
    test_content_setup, test_content_exec, NULL,
    "/Method/BsePattern/Test Content",
  },
  { NULL, },
};
BSE_EXPORTS_END;
