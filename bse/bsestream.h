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
 * bsestream.h: streaming base object
 */
#ifndef	__BSE_STREAM_H__
#define	__BSE_STREAM_H__

#include	<bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- object type macros --- */
#define BSE_TYPE_STREAM              (BSE_TYPE_ID (BseStream))
#define BSE_STREAM(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_STREAM, BseStream))
#define BSE_STREAM_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_STREAM, BseStreamClass))
#define BSE_IS_STREAM(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_STREAM))
#define BSE_IS_STREAM_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_STREAM))
#define BSE_STREAM_GET_CLASS(object) ((BseStreamClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- object & class member/convenience macros --- */
#define BSE_STREAM_SET_FLAG(stream, f)   (BSE_OBJECT_FLAGS (stream) |= BSE_STREAMF_ ## f)
#define BSE_STREAM_UNSET_FLAG(stream, f) (BSE_OBJECT_FLAGS (stream) &= ~(BSE_STREAMF_ ## f))
#define	BSE_STREAM_ERROR(stream)	 (((BseStream*) (stream))->last_error)
#define	BSE_STREAM_RESET_ERROR(stream)	 (BSE_STREAM_ERROR (stream) = BSE_ERROR_NONE)
#define	BSE_STREAM_SET_ERROR(stream, e)	 (BSE_STREAM_ERROR (stream) = (e))
#define	BSE_STREAM_OPENED(stream)	 ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_OPENED) != 0)
#define	BSE_STREAM_READY(stream)	 ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_READY) != 0)
#define	BSE_STREAM_READABLE(stream)	 ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_READABLE) != 0)
#define	BSE_STREAM_WRITABLE(stream)	 ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_WRITABLE) != 0)


/* --- BseStream flags --- */
typedef enum
{
  BSE_STREAMF_OPENED		= 1 << (BSE_OBJECT_FLAGS_USER_SHIFT + 0),
  BSE_STREAMF_READY		= 1 << (BSE_OBJECT_FLAGS_USER_SHIFT + 1),
  BSE_STREAMF_READABLE		= 1 << (BSE_OBJECT_FLAGS_USER_SHIFT + 2),
  BSE_STREAMF_WRITABLE		= 1 << (BSE_OBJECT_FLAGS_USER_SHIFT + 3)
} BseStreamFlags;


/* --- BseStream object --- */
struct _BseStream
{
  BseObject	bse_object;

  BseErrorType  last_error;

  gchar         *file_name;
};
struct _BseStreamClass
{
  BseObjectClass	bse_object_class;

  void		(*open)         (BseStream       *stream,
				 gboolean         read_access,
				 gboolean         write_access);
  void		(*close)        (BseStream	 *stream);
  void		(*start)        (BseStream       *stream);
  void		(*stop)	        (BseStream       *stream);
  gboolean      (*would_block)	(BseStream       *stream,
				 guint            n_values);
  guint		(*read)		(BseStream       *stream,
				 guint            n_bytes,
				 guint8		 *bytes);
  guint		(*write)	(BseStream       *stream,
				 guint            n_bytes,
				 const guint8	 *bytes);
  guint		(*read_sv)	(BseStream       *stream,
				 guint            n_values,
				 BseSampleValue  *values);
  guint		(*write_sv)	(BseStream       *stream,
				 guint            n_values,
				 const BseSampleValue  *values);
};



/* --- prototypes -- */
BseStream*	bse_stream_new		(BseType	 	 stream_type,
					 const gchar	 	*first_param_name,
					 ...);
BseErrorType	bse_stream_open		(BseStream	 	*stream,
					 gboolean	 	 read_access,
					 gboolean	 	 write_access);
BseErrorType	bse_stream_close	(BseStream	 	*stream);
BseErrorType	bse_stream_start	(BseStream	 	*stream);
BseErrorType	bse_stream_stop		(BseStream	 	*stream);
gboolean	bse_stream_would_block	(BseStream	 	*stream,
					 guint		 	 n_values);
BseErrorType	bse_stream_read		(BseStream	 	*stream,
					 guint           	 n_bytes,
					 guint8          	*bytes);
BseErrorType	bse_stream_write	(BseStream	 	*stream,
					 guint		 	 n_bytes,
					 const guint8	 	*bytes);
BseErrorType	bse_stream_read_sv	(BseStream	 	*stream,
					 guint		 	 n_values,
					 BseSampleValue	 	*values);
BseErrorType	bse_stream_write_sv	(BseStream	 	*stream,
					 guint		 	 n_values,
					 const BseSampleValue	 *values);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_STREAM_H__ */
