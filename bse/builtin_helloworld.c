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
static GType   type_id_hello_world = 0;
static void
hello_world_setup (BseProcedureClass *proc,
		   GParamSpec       **in_pspecs,
		   GParamSpec       **out_pspecs)
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
		  GValue            *in_values,
		  GValue            *out_values)
{
  /* issue a message */
  g_message ("Hello World");

  return BSE_ERROR_NONE;
}

/* --- randomizer --- */
#include <stdlib.h>
static GType   type_id_randomizer = 0;
static void
randomizer_setup (BseProcedureClass *proc,
		  GParamSpec       **in_pspecs,
		  GParamSpec       **out_pspecs)
{
  proc->help      = ("Randomizer will take on integer as input parameter, "
		     "use it as seed value and return a string containing "
		     "a random number. The deeper sense behind this plugin "
		     "is to test out the parameter passing functionality.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(in_pspecs++) = b_param_spec_bool ("use-seed", "Use seed value?", NULL,
				      FALSE, B_PARAM_PROCEDURE);
  *(in_pspecs++) = b_param_spec_int ("seed", "Random Seed", NULL,
				     -32768, 32767, 42, 1, B_PARAM_PROCEDURE);
  
  /* output parameters */
  *(out_pspecs++) = b_param_spec_string ("text", "Random Text", NULL,
					 NULL, B_PARAM_DEFAULT);
}

static BseErrorType
randomizer_exec (BseProcedureClass *proc,
		 GValue            *in_values,
		 GValue            *out_values)
{
  /* extract parameter values */
  gint use_seed = b_value_get_bool (in_values++);
  gint seed     = b_value_get_int (in_values++);
  gchar *string;

  /* check parameters */
  if (use_seed && seed == 0)
    return BSE_ERROR_PROC_PARAM_INVAL;

  /* perform our duty */
  if (use_seed)
    srand (seed);

  string = g_strdup_printf ("Random Number: %d", rand ());

  /* set output parameters */
  b_value_set_string (out_values++, string);
  g_free (string);

  return BSE_ERROR_NONE;
}


/* --- progressor --- */
#include <stdlib.h>
static GType   type_id_progressor = 0;
static void
progressor_setup (BseProcedureClass *proc,
                  GParamSpec       **in_pspecs,
		  GParamSpec       **out_pspecs)
{
  proc->help      = ("Progressor takes two seed values and then starts progressing. "
		     "It doesn't do anything particularly usefull, other than "
		     "sharing CPU time with the main program from time to time.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";
  
  /* input parameters */
  *(in_pspecs++) = b_param_spec_uint ("n-iterations", "Iterations", "Number of total Iterations",
				      0, 1024*1024, 1000, 512, B_PARAM_PROCEDURE);
  *(in_pspecs++) = b_param_spec_uint ("n-wait-spins", "Cycles", "Number of internal iterations",
				      0, 1024*1024, 1000, 512, B_PARAM_PROCEDURE);
  *(in_pspecs++) = b_param_spec_bool ("update-progress", "Update Progress Value?", NULL,
				      TRUE, B_PARAM_PROCEDURE);

  /* output parameters */
  *(out_pspecs++) = b_param_spec_string ("text1", "Status1", NULL,
					 NULL, B_PARAM_DEFAULT);
  *(out_pspecs++) = b_param_spec_string ("text2", "Status2", NULL,
					 NULL, B_PARAM_DEFAULT);
}

static BseErrorType
progressor_exec (BseProcedureClass *proc,
                 GValue            *in_values,
		 GValue            *out_values)
{
  /* extract parameter values */
  guint n_iter  = b_value_get_uint (in_values++);
  guint n_spin  = b_value_get_uint (in_values++);
  gint progress = b_value_get_bool (in_values++);
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
  b_value_set_string (out_values++, "Hooh, that was good!");
  b_value_set_string (out_values++, "Now gimme a break...");

  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_hello_world, "hello-world",
    "Hello World - First Plugin ever executed by BSE", 0,
    hello_world_setup, hello_world_exec, NULL,
    "/Proc/Toys/Hello World",
  },
  { &type_id_randomizer, "randomizer",
    "Randomizer takes a seed value and returns a random string", 0,
    randomizer_setup, randomizer_exec, NULL,
    "/Proc/Toys/Randomizer",
  },
  { &type_id_progressor, "progressor",
    "Progressor takes two seed values and then starts progressing", 0,
    progressor_setup, progressor_exec, NULL,
    "/Proc/Toys/Progressor",
  },
  { NULL, },
};
BSE_EXPORTS_END;
