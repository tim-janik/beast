/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bseinstrument.h: BSE instrument definition
 */
#ifndef __BSE_INSTRUMENT_H__
#define __BSE_INSTRUMENT_H__

#include	<bse/bseitem.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE type macros --- */
#define BSE_TYPE_INSTRUMENT		 (BSE_TYPE_ID (BseInstrument))
#define BSE_INSTRUMENT(object)		 (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_INSTRUMENT, BseInstrument))
#define BSE_INSTRUMENT_CLASS(class)	 (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_INSTRUMENT, BseInstrumentClass))
#define BSE_IS_INSTRUMENT(object)	 (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_INSTRUMENT))
#define BSE_IS_INSTRUMENT_CLASS(class)	 (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_INSTRUMENT))
#define BSE_INSTRUMENT_GET_CLASS(object) ((BseInstrumentClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseInstrument --- */
typedef struct _BseEnvelope BseEnvelope;
struct _BseEnvelope
{
  guint	 delay_time;
  guint	 attack_time;
  gfloat attack_level;
  guint	 decay_time;
  gfloat sustain_level;
  guint	 sustain_time;
  gfloat release_level;
  guint	 release_time;
};
struct _BseInstrument
{
  BseItem	parent_object;
  
  BseInstrumentType  type;
  
  /* sample specific fields */
  BseSample	*sample;
  guint		 interpolation : 1;

  guint		 polyphony : 1;
  gfloat	 volume_factor;
  gint		 balance;
  gint		 transpose;
  gint		 fine_tune;

  BseEnvelope	 env;
};
struct _BseInstrumentClass
{
  BseItemClass parent_class;
};


/* --- prototypes -- */
void		bse_instrument_set_sample	(BseInstrument	*instrument,
						 BseSample	*sample);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_INSTRUMENT_H__ */
