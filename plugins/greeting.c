/* Greeting - BSE Plugin to greet the world
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
 * greeting.c: implement a simplistic greeting procedure
 */
#include        <bse/bseplugin.h>

#include	<bse/bseprocedure.h>



/* --- Greeting --- */
static BseType type_id_greeting = 0;
static BseType type_id_greeting_again = 0;
static void
greeting_setup (BseProcedureClass *proc,
		BseParamSpec     **ipspecs,
		BseParamSpec     **opspecs)
{
  proc->help      = ("Greeting greets the outside world. It takes an "
		     "unsigned integer as input parameter "
		     "and interprets it as the current year for its "
		     "return value - a greeting message.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_uint ("year", "Curent Year",
				      1980, 2100, 1, 1999, BSE_PARAM_PROCEDURE);
  
  /* output parameters */
  *(opspecs++) = bse_param_spec_string ("greeting", "Greeting",
					NULL, BSE_PARAM_DEFAULT);
}

static BseErrorType
greeting_exec (BseProcedureClass *proc,
	       BseParam          *iparams,
	       BseParam          *oparams)
{
  /* extract parameter values */
  guint year = (iparams++)->value.v_uint;
  gchar *string;
  
  /* check parameters */
  
  /* construct greeting */
  string = g_strdup_printf ("Hello, nice to be loaded into a library in %d.", year);
  
  /* set output parameters */
  (oparams++)->value.v_string = string;
  
  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_greeting, "greeting",
    "Greeting greets the outside world",
    greeting_setup, greeting_exec, NULL,
    "/Proc/Test/Greeting",
  },
  { &type_id_greeting_again, "greeting-again",
    "Greeting greets the outside world",
    greeting_setup, greeting_exec, NULL,
    "/Proc/Test/Greeting again",
  },
  { NULL, },
};
BSE_EXPORTS_END;
