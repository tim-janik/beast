/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bsestandardsynths.h"

#include "bsesnet.h"
#include "bsestandardosc.h"
#include "bsesubinstrument.h"
#include "bsesubkeyboard.h"


/* --- typedefs & structures --- */
typedef struct {
  BseStandardSynth	 synth;
  BseSource		*input;
  BseSource		*osc;
  BseSource		*env;
  BseSource		*output;
} Piano;


/* --- prototypes --- */
static BseStandardSynth*	project_get_standard_synth	(BseProject	  *project,
								 const gchar	  *name);
static void			project_set_standard_synth	(BseProject	  *project,
								 const gchar	  *name,
								 BseStandardSynth *synth);


/* --- functions --- */
static BseStandardSynth*
project_get_standard_synth (BseProject  *project,
			    const gchar *name)
{
  BseStandardSynth *synth;

  synth = g_object_get_data (G_OBJECT (project), name);

  return synth;
}

static void
project_set_standard_synth (BseProject       *project,
			    const gchar      *name,
			    BseStandardSynth *synth)
{
  g_object_set_data_full (G_OBJECT (project), name, synth, g_free);
  bse_container_add_item (BSE_CONTAINER (project), BSE_ITEM (synth->snet));
}

BseStandardSynth*
bse_project_standard_piano (BseProject *project)
{
  BseStandardSynth *synth;

  synth = project_get_standard_synth (project, "bse-standard-piano");
  if (!synth)
    {
      Piano *piano = g_new0 (Piano, 1);

      synth = &piano->synth;
      synth->snet = g_object_new (BSE_TYPE_SNET, "name", "bse-intern-standard-piano", NULL);
      BSE_OBJECT_SET_FLAGS (synth->snet, BSE_ITEM_FLAG_STORAGE_IGNORE);
      project_set_standard_synth (project, "bse-standard-piano", synth);
      piano->input = bse_container_new_item (BSE_CONTAINER (synth->snet), BSE_TYPE_SUB_KEYBOARD, NULL);
      piano->osc = bse_container_new_item (BSE_CONTAINER (synth->snet), BSE_TYPE_STANDARD_OSC, NULL);
      bse_source_set_input (piano->osc, BSE_STANDARD_OSC_ICHANNEL_FREQ,
			    piano->input, BSE_SUB_KEYBOARD_OCHANNEL_FREQUENCY);
      bse_source_set_input (piano->osc, BSE_STANDARD_OSC_ICHANNEL_SYNC,
			    piano->input, BSE_SUB_KEYBOARD_OCHANNEL_GATE);
      piano->env = bse_container_new_item (BSE_CONTAINER (synth->snet), g_type_from_name ("BseMult"), NULL); // FIXME
      bse_source_set_input (piano->env, 0,
			    piano->input, BSE_SUB_KEYBOARD_OCHANNEL_GATE);
      bse_source_set_input (piano->env, 1,
			    piano->osc, BSE_STANDARD_OSC_OCHANNEL_OSC);
      piano->output = bse_container_new_item (BSE_CONTAINER (synth->snet), BSE_TYPE_SUB_INSTRUMENT, NULL);
      bse_source_set_input (piano->output, 0,
			    piano->env, 0);
      bse_source_set_input (piano->output, 1,
			    piano->env, 0);
    }
  return synth;
}
