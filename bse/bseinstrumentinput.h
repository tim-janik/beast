/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2003 Tim Janik
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
#ifndef __BSE_INSTRUMENT_INPUT_H__
#define __BSE_INSTRUMENT_INPUT_H__

#include <bse/bsesubiport.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_INSTRUMENT_INPUT		(BSE_TYPE_ID (BseInstrumentInput))
#define BSE_INSTRUMENT_INPUT(object)	       (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_INSTRUMENT_INPUT, BseInstrumentInput))
#define BSE_INSTRUMENT_INPUT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_INSTRUMENT_INPUT, BseInstrumentInputClass))
#define BSE_IS_INPUT(object)		       (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_INSTRUMENT_INPUT))
#define BSE_IS_INPUT_CLASS(class)	       (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_INSTRUMENT_INPUT))
#define BSE_INSTRUMENT_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_INSTRUMENT_INPUT, BseInstrumentInputClass))


/* --- BseInstrumentInput source --- */
typedef struct _BseInstrumentInput      BseInstrumentInput;
typedef struct _BseInstrumentInputClass BseInstrumentInputClass;
struct _BseInstrumentInput
{
  BseSubIPort parent_object;
};
struct _BseInstrumentInputClass
{
  BseSubIPortClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_INSTRUMENT_INPUT_OCHANNEL_FREQUENCY,
  BSE_INSTRUMENT_INPUT_OCHANNEL_GATE,
  BSE_INSTRUMENT_INPUT_OCHANNEL_VELOCITY,
  BSE_INSTRUMENT_INPUT_OCHANNEL_AFTERTOUCH
};


G_END_DECLS

#endif /* __BSE_INSTRUMENT_INPUT_H__ */
