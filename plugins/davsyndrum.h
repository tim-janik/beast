/* DavSynDrum - DAV Drum Synthesizer
 * Copyright (c) 1999, 2000 David A. Bartold, 2003 Tim Janik
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
#ifndef __DAV_SYNDRUM_H__
#define __DAV_SYNDRUM_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

G_BEGIN_DECLS


/* --- object type macros --- */
#define DAV_TYPE_SYN_DRUM              (BSE_EXPORT_TYPE_ID (DavSynDrum))
#define DAV_SYN_DRUM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_SYN_DRUM, DavSynDrum))
#define DAV_SYN_DRUM_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_SYN_DRUM, DavSynDrumClass))
#define DAV_IS_SYN_DRUM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_SYN_DRUM))
#define DAV_IS_SYN_DRUM_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_SYN_DRUM))
#define DAV_SYN_DRUM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DAV_TYPE_SYN_DRUM, DavSynDrumClass))

/* --- DavSynDrum source --- */
typedef struct {
  gfloat freq;
  gfloat trigger_vel;
  gfloat ratio;
  gfloat res;
} DavSynDrumParams;
typedef struct {
  BseSource        parent_object;
  DavSynDrumParams params;
  gfloat           half;
  gboolean         force_trigger;
} DavSynDrum;
typedef struct {
  BseSourceClass parent_class;
} DavSynDrumClass;
typedef struct {
  DavSynDrumParams params;
  gfloat last_trigger_level;
  gfloat spring_vel;
  gfloat spring_pos;
  gfloat env;
  gfloat freq_rad;
  gfloat freq_shift;
} DavSynDrumModule;

/* --- channels --- */
enum
{
  DAV_SYN_DRUM_ICHANNEL_FREQ,
  DAV_SYN_DRUM_ICHANNEL_RATIO,
  DAV_SYN_DRUM_ICHANNEL_TRIGGER,
  DAV_SYN_DRUM_N_ICHANNELS
};
enum
{
  DAV_SYN_DRUM_OCHANNEL_MONO,
  DAV_SYN_DRUM_N_OCHANNELS
};

G_END_DECLS

#endif /* __DAV_SYNDRUM_H__ */
