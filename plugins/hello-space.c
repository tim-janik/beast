/* hello-space - BSE Test Plugin
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
 * hello-space.c: implement a simplistic hello world type o'thing
 */
#include        <bse/bseplugin.h>

#include	<bse/bseprocedure.h>



/* --- hello_world --- */
static BseType type_id_hello_space = 0;
static void
hello_space_setup (BseProcedureClass *proc,
		   BseParamSpec     **ipspecs,
		   BseParamSpec     **opspecs)
{
  proc->help      = ("Hello Space - Simplistic Hello World type o' thing. "
		     "Its purpose is to say \"Hello Space\". "
		     "This plugin takes no input or output parameters.");
  proc->author    = "Tim Janik <timj@gtk.org>";
  proc->copyright = "Tim Janik <timj@gtk.org>";
  proc->date      = "1999";

  /* no input/output parameters */
}

static BseErrorType
hello_space_exec (BseProcedureClass *proc,
		  BseParam          *iparams,
		  BseParam          *oparams)
{
  /* issue a message */
  g_message ("Hello Space");

  return BSE_ERROR_NONE;
}


/* --- Export to BSE --- */
#include "./icons/any.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_PROCEDURES = {
  { &type_id_hello_space, "hello-space",
    "Hello Space - Simplistic Hello World type o' thing.",
    hello_space_setup, hello_space_exec, NULL,
    "/Proc/Test/Hello Space",
    { TEST_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      TEST_IMAGE_WIDTH, TEST_IMAGE_HEIGHT,
      TEST_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
