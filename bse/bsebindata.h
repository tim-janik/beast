/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
 *
 * bsebindata.h: binary data container
 */
#ifndef __BSE_BIN_DATA_H__
#define __BSE_BIN_DATA_H__

#include        <bse/bseobject.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define	BSE_TYPE_BIN_DATA	       (BSE_TYPE_ID (BseBinData))
#define BSE_BIN_DATA(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_BIN_DATA, BseBinData))
#define BSE_BIN_DATA_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_BIN_DATA, BseBinDataClass))
#define BSE_IS_BIN_DATA(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_BIN_DATA))
#define BSE_IS_BIN_DATA_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_BIN_DATA))
#define BSE_BIN_DATA_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_BIN_DATA, BseBinDataClass))


/* --- Data types --- */
typedef enum
{
  BSE_BIN_DATA_RAW,
  BSE_BIN_DATA_SAMPLE_VALUES
} BseBinDataType;


/* --- BseBinData object --- */
struct _BseBinData
{
  BseObject		 parent_object;
  
  guint			 bits_per_value;
  guint		 	 n_values;
  guint			 n_bytes;
  guint8		*values;
  guint			 byte_padding;
};
struct _BseBinDataClass
{
  BseObjectClass parent_class;
};


/* --- prototypes --- */
BseErrorType	bse_bin_data_set_values_from_fd	(BseBinData	*bin_data,
						 gint		 fd,
						 glong		 offset,
						 guint		 n_bytes,
						 BseEndianType   byte_order);
void		bse_bin_data_init_values	(BseBinData	*bin_data,
						 guint		 bits_per_value,
						 guint		 n_values);
void		bse_bin_data_set_byte_padding	(BseBinData	*bin_data,
						 guint		 n_bytes);




G_END_DECLS

#endif /* __BSE_BIN_DATA_H__ */
