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
 * bsefilestream.h: bse stream implementation for unix files
 */
#ifndef	__BSE_FILE_STREAM_H__
#define	__BSE_FILE_STREAM_H__

#include	<bse/bsestream.h>
#include	<bse/bsetext.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_FILE_STREAM              (BSE_TYPE_ID (BseFileStream))
#define BSE_FILE_STREAM(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_FILE_STREAM, BseFileStream))
#define BSE_FILE_STREAM_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_FILE_STREAM, BseFileStreamClass))
#define BSE_IS_FILE_STREAM(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_FILE_STREAM))
#define BSE_IS_FILE_STREAM_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_FILE_STREAM))
#define BSE_FILE_STREAM_GET_CLASS(object) ((BseFileStreamClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- object & class member/convenience macros --- */
#define BSE_STREAM_SET_FLAG(stream, f)   (BSE_OBJECT_FLAGS (stream) |= BSE_STREAMF_ ## f)
#define BSE_STREAM_UNSET_FLAG(stream, f) (BSE_OBJECT_FLAGS (stream) &= ~(BSE_STREAMF_ ## f))
#define BSE_STREAM_ERROR(stream)         (((BseStream*) (stream))->last_error)
#define BSE_STREAM_RESET_ERROR(stream)   (BSE_STREAM_ERROR (stream) = BSE_ERROR_NONE)
#define BSE_STREAM_SET_ERROR(stream, e)  (BSE_STREAM_ERROR (stream) = (e))
#define BSE_STREAM_OPENED(stream)        ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_OPENED) != 0)
#define BSE_STREAM_READY(stream)         ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_READY) != 0)
#define BSE_STREAM_READABLE(stream)      ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_READABLE) != 0)
#define BSE_STREAM_WRITABLE(stream)      ((BSE_OBJECT_FLAGS (stream) & BSE_STREAMF_WRITABLE) != 0)

#define	BSE_FILE_STREAM_STDIN	((gchar*) 42 + 0)
#define	BSE_FILE_STREAM_STDOUT	((gchar*) 42 + 1)
#define	BSE_FILE_STREAM_STDERR	((gchar*) 42 + 2)


/* --- BseFileStream object --- */
struct _BseFileStream
{
  BseStream	bse_stream;

  guint	fake_close : 1;

  gpointer file;
};
struct _BseFileStreamClass
{
  BseStreamClass bse_stream_class;
};


/* --- prototypes -- */
BseStream*   bse_file_stream_new	(const gchar	*file_name);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_FILE_STREAM_H__ */
