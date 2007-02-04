/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2003 Tim Janik
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
#ifndef __BSE_SUB_SYNTH_H__
#define __BSE_SUB_SYNTH_H__

#include <bse/bsesource.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_SUB_SYNTH		(BSE_TYPE_ID (BseSubSynth))
#define BSE_SUB_SYNTH(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SUB_SYNTH, BseSubSynth))
#define BSE_SUB_SYNTH_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SUB_SYNTH, BseSubSynthClass))
#define BSE_IS_SUB_SYNTH(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SUB_SYNTH))
#define BSE_IS_SUB_SYNTH_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SUB_SYNTH))
#define BSE_SUB_SYNTH_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SUB_SYNTH, BseSubSynthClass))

/* --- BseSubSynth source --- */
struct _BseSubSynth
{
  BseSource        parent_object;

  BseSNet	  *snet;
  gchar		 **input_ports;
  gchar		 **output_ports;
  guint            midi_channel;
  guint            null_shortcut : 1;
};
struct _BseSubSynthClass
{
  BseSourceClass     parent_class;
};


/* --- prototypes --- */
/* whether to shortcut inputs with outputs for snet==NULL */
void    bse_sub_synth_set_null_shortcut  (BseSubSynth     *self,
                                          gboolean         enabled);
/* override midi_channel for snet, or if midi_channel==0 inherit from parent */
void    bse_sub_synth_set_midi_channel   (BseSubSynth     *self,
                                          guint            midi_channel);


G_END_DECLS

#endif /* __BSE_SUB_SYNTH_H__ */
