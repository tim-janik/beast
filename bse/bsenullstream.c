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
 */
#include	"bsenullstream.h"
#include	<errno.h>



/* --- stream prototypes --- */
static void     bse_null_stream_class_init      (BseNullStreamClass *class);
static void     bse_null_stream_text_iface_init (BseTextInterface   *iface);
static void     bse_null_stream_init            (BseNullStream      *null_stream);
static void     bse_null_stream_destroy         (BseObject          *object);
static void	bse_null_stream_open	   	(BseStream	    *stream,
						 gboolean	     read_access,
						 gboolean	     write_access);
static void	bse_null_stream_close	   	(BseStream	    *stream);
static void	bse_null_stream_start	   	(BseStream	    *stream);
static void	bse_null_stream_stop	   	(BseStream	    *stream);
static gboolean	bse_null_stream_would_block	(BseStream	    *stream,
						 guint	    	     n_values);
static guint	bse_null_stream_read		(BseStream	    *stream,
						 guint		     n_bytes,
						 guint8		    *bytes);
static guint	bse_null_stream_write		(BseStream	    *stream,
						 guint	    	     n_bytes,
						 const guint8	    *bytes);


/* --- variables --- */
static BseStreamClass   *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseNullStream)
{
  BseTypeInfo null_stream_info = {
    sizeof (BseNullStreamClass),

    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_null_stream_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseNullStream),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_null_stream_init,
  };
  BseInterfaceInfo null_stream_text_iface_info =
  {
    (BseClassInitFunc) bse_null_stream_text_iface_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
  };
  BseType new_type;

  new_type = bse_type_register_static (BSE_TYPE_STREAM,
				       "BseNullStream",
				       "Streamed data handling from/to null device",
				       &null_stream_info);
  bse_type_add_interface (new_type,
			  BSE_TYPE_TEXT,
			  &null_stream_text_iface_info);

  return new_type;
}

static void
bse_null_stream_class_init (BseNullStreamClass *class)
{
  BseObjectClass *object_class;
  BseStreamClass *stream_class;

  parent_class = bse_type_class_peek (BSE_TYPE_STREAM);
  object_class = BSE_OBJECT_CLASS (class);
  stream_class = BSE_STREAM_CLASS (class);

  object_class->destroy = bse_null_stream_destroy;

  stream_class->open = bse_null_stream_open;
  stream_class->close = bse_null_stream_close;
  stream_class->would_block = bse_null_stream_would_block;
  stream_class->read = bse_null_stream_read;
  stream_class->write = bse_null_stream_write;
}

static void
bse_null_stream_text_iface_init (BseTextInterface *iface)
{
  iface->write_chars = (gpointer) bse_stream_write;
}

static void
bse_null_stream_init (BseNullStream *null_stream)
{
  BseStream *stream;

  stream = BSE_STREAM (null_stream);

  BSE_STREAM_SET_FLAG (null_stream, READABLE);
  BSE_STREAM_SET_FLAG (null_stream, WRITABLE);
}

static void
bse_null_stream_destroy (BseObject *object)
{
  BseNullStream *null_stream;
  BseStream *stream;

  null_stream = BSE_NULL_STREAM (object);
  stream = BSE_STREAM (object);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

BseStream*
bse_null_stream_new (void)
{
  return bse_stream_new (BSE_TYPE_NULL_STREAM, "file-name", "Null", NULL);
}

static void
bse_null_stream_open (BseStream *stream,
		      gboolean	 read_access,
		      gboolean	 write_access)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  BSE_STREAM_SET_FLAG (null_stream, OPENED);

  errno = 0;
}

static void
bse_null_stream_close (BseStream *stream)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  BSE_STREAM_UNSET_FLAG (null_stream, OPENED);

  errno = 0;
}

static void
bse_null_stream_start (BseStream *stream)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  BSE_STREAM_SET_FLAG (null_stream, READY);

  errno = 0;
}

static void
bse_null_stream_stop (BseStream *stream)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  BSE_STREAM_UNSET_FLAG (null_stream, READY);

  errno = 0;
}

static gboolean
bse_null_stream_would_block (BseStream *stream,
			     guint	n_values)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  errno = 0;

  return FALSE;
}

static guint
bse_null_stream_read (BseStream	*stream,
		      guint	 n_bytes,
		      guint8	*bytes)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  memset (bytes, 0, n_bytes);

  errno = 0;

  return n_bytes;
}

static guint
bse_null_stream_write (BseStream    *stream,
		       guint	     n_bytes,
		       const guint8 *bytes)
{
  BseNullStream *null_stream;

  null_stream = BSE_NULL_STREAM (stream);

  errno = 0;

  return n_bytes;
}
