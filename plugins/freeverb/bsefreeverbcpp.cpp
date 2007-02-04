/* BseFreeVerbCpp - Free Verb C++ Wrapper for BSE
 * Copyright (C) 2002 Tim Janik
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
 */
#include "bsefreeverbcpp.h"

#include "revmodel.hpp"


G_BEGIN_DECLS

/* --- functions --- */
void
bse_free_verb_cpp_create (BseFreeVerbCpp *cpp)
{
  revmodel *rmod = new revmodel ();

  cpp->obj = rmod;
}

void
bse_free_verb_cpp_configure (BseFreeVerbCpp    *cpp,
			     BseFreeVerbConfig *config)
{
  revmodel *rmod = (revmodel*) cpp->obj;

  rmod->setroomsize (config->room_size);
  rmod->setdamp (config->damp);
  rmod->setwet (config->wet);
  rmod->setdry (config->dry);
  rmod->setwidth (config->width);
  rmod->setmode (initialmode);
}

void
bse_free_verb_cpp_process (BseFreeVerbCpp *cpp,
			   guint	   n_values,
			   const gfloat   *ileft,
			   const gfloat   *iright,
			   gfloat         *oleft,
			   gfloat         *oright)
{
  revmodel *rmod = (revmodel*) cpp->obj;

  rmod->processreplace ((float*) ileft, (float*) iright, oleft, oright, n_values, 1);
}

void
bse_free_verb_cpp_destroy (BseFreeVerbCpp *cpp)
{
  revmodel *rmod = (revmodel*) cpp->obj;

  delete rmod;
}

void
bse_free_verb_cpp_defaults (BseFreeVerbConfig    *config,
			    BseFreeVerbConstants *constants)
{
  if (config)
    {
      config->room_size = initialroom;
      config->damp = initialdamp;
      config->wet = initialwet;
      config->dry = initialdry;
      config->width = initialwidth;
    }
  if (constants)
    {
      constants->room_offset = offsetroom;
      constants->room_scale = scaleroom;
      constants->damp_scale = 100.0;
      constants->wet_scale = scalewet;
      constants->dry_scale = scaledry;
      constants->width_scale = 100.0;
    }
}

void
bse_free_verb_cpp_save_config (BseFreeVerbCpp    *cpp,
			       BseFreeVerbConfig *config)
{
  cpp->saved_config = *config;
}

void
bse_free_verb_cpp_restore_config (BseFreeVerbCpp    *cpp,
				  BseFreeVerbConfig *config)
{
  *config = cpp->saved_config;
}

G_END_DECLS
