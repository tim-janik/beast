/* Hello World - First Plugin ever executed by BSE
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
 * helloworld.c: Say "Hello World" to BSE and the general outside
 */
#include        <bse/bseplugin.h>

#include	<bse/bseprocedure.h>



/* --- hello_world --- */
static BseType type_id_hello_world = 0;
static void
hello_world_setup (BseProcedureClass *proc,
		   BseParamSpec     **ipspecs,
		   BseParamSpec     **opspecs)
{
  proc->help      = ("Hello World - First Plugin ever executed by BSE. "
		     "Its purpose is to say \"Hello World\" to BSE and the "
		     "general outside. This plugin takes no input or output"
		     "parameters.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* no input/output parameters */
}

static BseErrorType
hello_world_exec (BseProcedureClass *proc,
		  BseParam          *iparams,
		  BseParam          *oparams)
{
  /* issue a message */
  g_message ("Hello World");

  return BSE_ERROR_NONE;
}

/* --- randomizer --- */
#include <stdlib.h>
static BseType type_id_randomizer = 0;
static void
randomizer_setup (BseProcedureClass *proc,
		  BseParamSpec    **ipspecs,
		  BseParamSpec    **opspecs)
{
  proc->help      = ("Randomizer will take on integer as input parameter, "
		     "use it as seed value and return a string containing "
		     "a random number. The deeper sense behind this plugin "
		     "is to test out the parameter passing functionality.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_bool ("use-seed", "Use seed value?", NULL,
				      FALSE, BSE_PARAM_PROCEDURE);
  *(ipspecs++) = bse_param_spec_int ("seed", "Random Seed", NULL,
				     -32768, 32767, 1, 42, BSE_PARAM_PROCEDURE);

  /* output parameters */
  *(opspecs++) = bse_param_spec_string ("text", "Random Text", NULL,
					NULL, BSE_PARAM_DEFAULT);
}

static BseErrorType
randomizer_exec (BseProcedureClass *proc,
		 BseParam          *iparams,
		 BseParam          *oparams)
{
  /* extract parameter values */
  gint use_seed = (iparams++)->value.v_bool;
  gint seed = (iparams++)->value.v_int;
  gchar *string;

  /* check parameters */
  if (!seed)
    return BSE_ERROR_PROC_PARAM_INVAL;

  /* perform our duty */
  if (use_seed)
    srand (seed);

  string = g_strdup_printf ("Random Number: %d", rand ());

  /* set output parameters */
  (oparams++)->value.v_string = string;

  return BSE_ERROR_NONE;
}


/* --- progressor --- */
#include <stdlib.h>
static BseType type_id_progressor = 0;
static void
progressor_setup (BseProcedureClass *proc,
		  BseParamSpec     **ipspecs,
		  BseParamSpec     **opspecs)
{
  proc->help      = ("Progressor takes two seed values and then starts progressing. "
		     "It doesn't do anything particularly usefull, other than "
		     "sharing CPU time with the main program from time to time.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(ipspecs++) = bse_param_spec_uint ("n-iterations", "Iterations", "Number of total Iterations",
				      0, 1024*1024, 512, 1000, BSE_PARAM_PROCEDURE);
  *(ipspecs++) = bse_param_spec_uint ("n-wait-spins", "Cycles", "Number of internal iterations",
				      0, 1024*1024, 512, 1000, BSE_PARAM_PROCEDURE);
  *(ipspecs++) = bse_param_spec_bool ("update-progress", "Update Progress Value?", NULL,
				      TRUE, BSE_PARAM_PROCEDURE);

  /* output parameters */
  *(opspecs++) = bse_param_spec_string ("text1", "Status1", NULL,
					NULL, BSE_PARAM_DEFAULT);
  *(opspecs++) = bse_param_spec_string ("text2", "Status2", NULL,
					NULL, BSE_PARAM_DEFAULT);
}

static BseErrorType
progressor_exec (BseProcedureClass *proc,
		 BseParam          *iparams,
		 BseParam          *oparams)
{
  /* extract parameter values */
  guint n_iter = (iparams++)->value.v_uint;
  guint n_spin = (iparams++)->value.v_uint;
  gint progress = (iparams++)->value.v_bool;
  gfloat total = n_iter;

  /* check parameters */
  /* return BSE_ERROR_PROC_PARAM_INVAL; */

  /* perform our duty */
  while (n_iter--)
    {
      guint n = n_spin;

      while (n--)
	{
	  g_free (g_strdup ("x"));
	}

      if (progress)
	{
	  if (bse_procedure_update (proc, 1.0 - ((gfloat) n_iter) / total))
	    return BSE_ERROR_PROC_ABORT;
	}
      else
	{
	  if (bse_procedure_share (proc))
	    return BSE_ERROR_PROC_ABORT;
	}
    }

  /* set output parameters */
  (oparams++)->value.v_string = g_strdup ("Hooh, that was good!");
  (oparams++)->value.v_string = g_strdup ("Now gimme a break...");

  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_hello_world, "hello-world",
    "Hello World - First Plugin ever executed by BSE",
    hello_world_setup, hello_world_exec, NULL,
    "/Proc/Test/Hello World",
  },
  { &type_id_randomizer, "randomizer",
    "Randomizer takes a seed value and returns a random string",
    randomizer_setup, randomizer_exec, NULL,
    "/Proc/Test/Randomizer",
  },
  { &type_id_progressor, "progressor",
    "Progressor takes two seed values and then starts progressing",
    progressor_setup, progressor_exec, NULL,
    "/Proc/Test/Progressor",
  },
  { NULL, },
};
BSE_EXPORTS_END;
