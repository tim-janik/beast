/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2001 Tim Janik
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
 * bsesubsynth.h: BSE module adapter for sub synthesis networks
 */
#ifndef __BSE_SUB_SYNTH_H__
#define __BSE_SUB_SYNTH_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SUB_SYNTH		(BSE_TYPE_ID (BseSubSynth))
#define BSE_SUB_SYNTH(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_SYNTH, BseSubSynth))
#define BSE_SUB_SYNTH_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_SYNTH, BseSubSynthClass))
#define BSE_IS_SUB_SYNTH(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_SYNTH))
#define BSE_IS_SUB_SYNTH_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_SYNTH))
#define BSE_SUB_SYNTH_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_SYNTH, BseSubSynthClass))

#define BSE_SUB_SYNTH_N_IOPORTS (8)

/* --- BseSubSynth source --- */
typedef struct _BseSubSynth      BseSubSynth;
typedef struct _BseSubSynthClass BseSubSynthClass;
struct _BseSubSynth
{
  BseSource       parent_object;

  BseSNet	*snet;
  gchar		*input_ports[BSE_SUB_SYNTH_N_IOPORTS];
  gchar		*output_ports[BSE_SUB_SYNTH_N_IOPORTS];
};
struct _BseSubSynthClass
{
  BseSourceClass     parent_class;
};


/* --- prototypes --- */
void	bse_sub_synth_set_snet (BseSubSynth	*sub_synth,
				BseSNet		*snet);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SUB_SYNTH_H__ */
