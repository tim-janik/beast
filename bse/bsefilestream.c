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
#include	"bsefilestream.h"
#include	<stdio.h>
#include	<errno.h>



/* --- stream prototypes --- */
static void     bse_file_stream_class_init      (BseFileStreamClass *class);
static void     bse_file_stream_text_iface_init (BseTextInterface   *iface);
static void     bse_file_stream_init            (BseFileStream      *file_stream);
static void     bse_file_stream_destroy         (BseObject          *object);
static void	bse_file_stream_open	   	(BseStream	    *stream,
						 gboolean	     read_access,
						 gboolean	     write_access);
static void	bse_file_stream_close	   	(BseStream	    *stream);
static void	bse_file_stream_start	   	(BseStream	    *stream);
static void	bse_file_stream_stop	   	(BseStream	    *stream);
static gboolean	bse_file_stream_would_block	(BseStream	    *stream,
						 guint	    	     n_values);
static guint	bse_file_stream_read		(BseStream	    *stream,
						 guint		     n_bytes,
						 guint8		    *bytes);
static guint	bse_file_stream_write		(BseStream	    *stream,
						 guint	    	     n_bytes,
						 const guint8	    *bytes);


/* --- variables --- */
static BseStreamClass   *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseFileStream)
{
  BseTypeInfo file_stream_info = {
    sizeof (BseFileStreamClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_file_stream_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseFileStream),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_file_stream_init,
  };
  BseInterfaceInfo file_stream_text_iface_info = {
    (BseClassInitFunc) bse_file_stream_text_iface_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
  };
  BseType new_type;
  
  new_type = bse_type_register_static (BSE_TYPE_STREAM,
				       "BseFileStream",
				       "Streamed data handling from/to files",
				       &file_stream_info);
  bse_type_add_interface (new_type,
			  BSE_TYPE_TEXT,
			  &file_stream_text_iface_info);

  return new_type;
}

static void
bse_file_stream_class_init (BseFileStreamClass *class)
{
  BseObjectClass *object_class;
  BseStreamClass *stream_class;

  parent_class = bse_type_class_peek (BSE_TYPE_STREAM);
  object_class = BSE_OBJECT_CLASS (class);
  stream_class = BSE_STREAM_CLASS (class);

  object_class->destroy = bse_file_stream_destroy;

  stream_class->open = bse_file_stream_open;
  stream_class->close = bse_file_stream_close;
  stream_class->would_block = bse_file_stream_would_block;
  stream_class->read = bse_file_stream_read;
  stream_class->write = bse_file_stream_write;
  stream_class->start = bse_file_stream_start;
  stream_class->stop = bse_file_stream_stop;
}

static void
bse_file_stream_text_iface_init (BseTextInterface *iface)
{
  iface->write_chars = (gpointer) bse_stream_write;
}

static void
bse_file_stream_init (BseFileStream *file_stream)
{
  BseStream *stream;

  stream = BSE_STREAM (file_stream);

  BSE_STREAM_SET_FLAG (file_stream, READABLE);
  BSE_STREAM_SET_FLAG (file_stream, WRITABLE);

  file_stream->fake_close = FALSE;
  file_stream->file = NULL;
}

static void
bse_file_stream_destroy (BseObject *object)
{
  BseFileStream *file_stream;
  BseStream *stream;

  file_stream = BSE_FILE_STREAM (object);
  stream = BSE_STREAM (object);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

BseStream*
bse_file_stream_new (const gchar *file_name)
{
  if (file_name == BSE_FILE_STREAM_STDIN)
    file_name = "/:0";
  if (file_name == BSE_FILE_STREAM_STDOUT)
    file_name = "/:1";
  if (file_name == BSE_FILE_STREAM_STDERR)
    file_name = "/:2";
    
  return bse_stream_new (BSE_TYPE_FILE_STREAM, "file-name", file_name, NULL);
}

static void
bse_file_stream_open (BseStream *stream,
		      gboolean	 read_access,
		      gboolean	 write_access)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);


  if (stream->file_name[0] == '/' &&
      stream->file_name[1] == ':' &&
      stream->file_name[2] >= '0' &&
      stream->file_name[2] <= '2')
    {
      if (stream->file_name[2] == '0' && read_access && !write_access)
	file_stream->file = stdin;
      else if (stream->file_name[2] == '1' && write_access && !read_access)
	file_stream->file = stdout;
      else if (stream->file_name[2] == '2' && write_access && !read_access)
	file_stream->file = stderr;

      if (file_stream->file)
	file_stream->fake_close = TRUE;
    }
  else
    {
      const gchar *access = NULL;

      if (read_access && write_access)
	access = "w+";
      else if (read_access)
	access = "r";
      else if (write_access)
	access = "w";
      file_stream->file = fopen (stream->file_name, access);
    }

  if (file_stream->file && fileno (file_stream->file) >= 0)
    BSE_STREAM_SET_FLAG (file_stream, OPENED);
  else
    {
      file_stream->file = NULL;
      file_stream->fake_close = FALSE;
      
      switch (errno)
	{
	case EINVAL: BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_INTERNAL); break;
	case EISDIR: BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_STREAM_WRITE_DENIED); break;
	case ETXTBSY: BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_STREAM_WRITE_DENIED); break;
	case EACCES: BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_STREAM_WRITE_DENIED); break;
	default: BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_STREAM_IO); break;
	}
    }
  
  errno = 0;
}

static void
bse_file_stream_close (BseStream *stream)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);

  if (BSE_STREAM_WRITABLE (file_stream->file))
    fflush (file_stream->file);
  
  if (!file_stream->fake_close)
    {
      fclose (file_stream->file);
      if (errno)
	BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_INTERNAL);
    }

  file_stream->file = NULL;
  file_stream->fake_close = FALSE;
  
  BSE_STREAM_UNSET_FLAG (file_stream, OPENED);

  errno = 0;
}

static void
bse_file_stream_start (BseStream *stream)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);

  BSE_STREAM_SET_FLAG (file_stream, READY);

  errno = 0;
}

static void
bse_file_stream_stop (BseStream *stream)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);

  if (BSE_STREAM_WRITABLE (file_stream->file))
    fflush (file_stream->file);

  BSE_STREAM_UNSET_FLAG (file_stream, READY);

  errno = 0;
}

static gboolean
bse_file_stream_would_block (BseStream *stream,
			     guint	n_values)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);

  errno = 0;

  return FALSE;
}

static guint
bse_file_stream_read (BseStream	*stream,
		      guint	 n_bytes,
		      guint8	*bytes)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);

  n_bytes = fread (bytes, 1, n_bytes, file_stream->file);

  if (errno)
    BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_STREAM_READ_FAILED);
  
  errno = 0;

  return n_bytes;
}

static guint
bse_file_stream_write (BseStream    *stream,
		       guint	     n_bytes,
		       const guint8 *bytes)
{
  BseFileStream *file_stream;

  file_stream = BSE_FILE_STREAM (stream);

  n_bytes = fwrite (bytes, 1, n_bytes, file_stream->file);

  if (errno)
    BSE_STREAM_SET_ERROR (file_stream, BSE_ERROR_STREAM_WRITE_FAILED);

  errno = 0;

  return n_bytes;
}
