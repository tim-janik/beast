/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 * bsenullstream.h: /dev/null stream implementation
 */
#ifndef	__BSE_NULL_STREAM_H__
#define	__BSE_NULL_STREAM_H__

#include	<bse/bsestream.h>
#include	<bse/bsetext.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_NULL_STREAM              (BSE_TYPE_ID (BseNullStream))
#define BSE_NULL_STREAM(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_NULL_STREAM, BseNullStream))
#define BSE_NULL_STREAM_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_NULL_STREAM, BseNullStreamClass))
#define BSE_IS_NULL_STREAM(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_NULL_STREAM))
#define BSE_IS_NULL_STREAM_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_NULL_STREAM))
#define BSE_NULL_STREAM_GET_CLASS(object) ((BseNullStreamClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseNullStream object --- */
struct _BseNullStream
{
  BseStream	bse_stream;
};
struct _BseNullStreamClass
{
  BseStreamClass bse_stream_class;
};


/* --- prototypes -- */
BseStream*   bse_null_stream_new	(void);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_NULL_STREAM_H__ */
