/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_SONG_BUS_H__
#define __BSE_SONG_BUS_H__

#include <bse/bsesubsynth.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_SONG_BUS               (BSE_TYPE_ID (BseSongBus))
#define BSE_SONG_BUS(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SONG_BUS, BseSongBus))
#define BSE_SONG_BUS_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SONG_BUS, BseSongBusClass))
#define BSE_IS_SONG_BUS(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SONG_BUS))
#define BSE_IS_SONG_BUS_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SONG_BUS))
#define BSE_SONG_BUS_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SONG_BUS, BseSongBusClass))


/* --- BseSongBus source --- */
typedef struct _BseSongBus      BseSongBus;
typedef struct _BseSongBusClass BseSongBusClass;
struct _BseSongBus
{
  BseSubSynth   parent_object;
};
struct _BseSongBusClass
{
  BseSubSynthClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_SONG_BUS_ICHANNEL_LEFT,
  BSE_SONG_BUS_ICHANNEL_RIGHT,
  BSE_SONG_BUS_N_ICHANNELS
};
enum
{
  BSE_SONG_BUS_OCHANNEL_LEFT,
  BSE_SONG_BUS_OCHANNEL_RIGHT,
  BSE_SONG_BUS_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_SONG_BUS_H__ */
