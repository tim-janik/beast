/* BseSequencer - BSE Sequencer
 * Copyright (C) 2002 Tim Janik
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
 */
#ifndef __BSE_SEQUENCER_H__
#define __BSE_SEQUENCER_H__

#define  BSE_PLUGIN_NAME  "BseSequencer"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SEQUENCER              (type_id_sequencer)
#define BSE_SEQUENCER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SEQUENCER, BseSequencer))
#define BSE_SEQUENCER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SEQUENCER, BseSequencerClass))
#define BSE_IS_SEQUENCER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SEQUENCER))
#define BSE_IS_SEQUENCER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SEQUENCER))
#define BSE_SEQUENCER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SEQUENCER, BseSequencerClass))


/* --- BseSequencer source --- */
typedef struct _BseSequencer   BseSequencer;
typedef struct _BseSourceClass BseSequencerClass;
struct _BseSequencer
{
  BseSource        parent_object;

  gfloat	   counter;
  gint		   transpose;
  BswNoteSequence *sdata;
  guint		   n_freq_values;
  gfloat	  *freq_values;
};


/* --- channels --- */
enum
{
  BSE_SEQUENCER_OCHANNEL_FREQ,
  BSE_SEQUENCER_OCHANNEL_NOTE_SYNC,
  BSE_SEQUENCER_N_OCHANNELS
};



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SEQUENCER_H__ */
