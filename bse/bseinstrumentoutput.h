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
#ifndef __BSE_INSTRUMENT_OUTPUT_H__
#define __BSE_INSTRUMENT_OUTPUT_H__

#include <bse/bsesuboport.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_INSTRUMENT_OUTPUT		(BSE_TYPE_ID (BseInstrumentOutput))
#define BSE_INSTRUMENT_OUTPUT(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_INSTRUMENT_OUTPUT, BseInstrumentOutput))
#define BSE_INSTRUMENT_OUTPUT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_INSTRUMENT_OUTPUT, BseInstrumentOutputClass))
#define BSE_IS_OUTPUT(object)	                (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_INSTRUMENT_OUTPUT))
#define BSE_IS_OUTPUT_CLASS(class)	        (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_INSTRUMENT_OUTPUT))
#define BSE_INSTRUMENT_OUTPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_INSTRUMENT_OUTPUT, BseInstrumentOutputClass))


/* --- BseInstrumentOutput source --- */
typedef struct _BseInstrumentOutput      BseInstrumentOutput;
typedef struct _BseInstrumentOutputClass BseInstrumentOutputClass;
struct _BseInstrumentOutput
{
  BseSubOPort parent_object;
};
struct _BseInstrumentOutputClass
{
  BseSubOPortClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_LEFT,
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_RIGHT,
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_UNUSED,
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_DONE
};

G_END_DECLS

#endif /* __BSE_INSTRUMENT_OUTPUT_H__ */
