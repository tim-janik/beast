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
#include	"bsestream.h"


enum {
  PARAM_0,
  PARAM_FILE_NAME
};


/* --- prototypes --- */
static void     bse_stream_class_init    (BseStreamClass	*class);
static void     bse_stream_init          (BseStream             *stream);
static void     bse_stream_destroy       (BseObject             *object);
static void     bse_stream_set_param     (BseStream             *stream,
					  BseParam              *param);
static void     bse_stream_get_param     (BseStream             *stream,
					  BseParam              *param);
static guint	bse_stream_do_read_sv    (BseStream       	*stream,
					  guint			 n_values,
					  BseSampleValue	*values);
static guint	bse_stream_do_write_sv   (BseStream       	*stream,
					  guint			 n_values,
					  const BseSampleValue	*values);


/* --- variables --- */
static BseObjectClass    *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseStream)
{
  static const BseTypeInfo stream_info = {
    sizeof (BseStreamClass),

    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_stream_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseStream),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_stream_init,
  };
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseStream",
				   "Base type for streamed data handling",
				   &stream_info);
}

static void
bse_stream_class_init (BseStreamClass *class)
{
  BseObjectClass *object_class;

  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_stream_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_stream_get_param;
  object_class->destroy = bse_stream_destroy;

  class->open = NULL;
  class->close = NULL;
  class->start = NULL;
  class->stop = NULL;
  class->would_block = NULL;
  class->read = NULL;
  class->write = NULL;
  class->read_sv = bse_stream_do_read_sv;
  class->write_sv = bse_stream_do_write_sv;

  bse_object_class_add_param (object_class, NULL,
			      PARAM_FILE_NAME,
			      bse_param_spec_string ("file_name", "File Name",
						     NULL,
						     BSE_PARAM_DEFAULT));
}

static void
bse_stream_init (BseStream *stream)
{
  stream->file_name = NULL;
}

static void
bse_stream_destroy (BseObject *object)
{
  BseStream *stream;

  stream = BSE_STREAM (object);

  if (BSE_STREAM_READY (stream))
    bse_stream_stop (stream);

  if (BSE_STREAM_OPENED (stream))
    bse_stream_close (stream);

  if (stream->file_name)
    g_free (stream->file_name);

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_stream_set_param (BseStream *stream,
		      BseParam  *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_FILE_NAME:
      g_free (stream->file_name);
      stream->file_name = g_strdup (param->value.v_string);
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (stream),
		 BSE_OBJECT_NAME (stream),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_stream_get_param (BseStream *stream,
		      BseParam      *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_FILE_NAME:
      g_free (param->value.v_string);
      param->value.v_string = g_strdup (stream->file_name);
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
                 BSE_OBJECT_TYPE_NAME (stream),
		 BSE_OBJECT_NAME (stream),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

BseStream*
bse_stream_new (BseType          stream_type,
		const gchar     *first_param_name,
		...)
{
  BseStream *stream;
  va_list var_args;

  g_return_val_if_fail (bse_type_is_a (stream_type, BSE_TYPE_STREAM), NULL);

  va_start (var_args, first_param_name);
  stream = bse_object_new_valist (stream_type, first_param_name, var_args);
  va_end (var_args);

  return stream;
}

BseErrorType
bse_stream_open (BseStream *stream,
		 gboolean   read_access,
		 gboolean   write_access)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_STREAM_OPENED (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (read_access || write_access, BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (read_access && !BSE_STREAM_READABLE (stream))
    {
      BSE_STREAM_SET_ERROR (stream, BSE_ERROR_STREAM_READ_DENIED);

      return BSE_ERROR_STREAM_READ_DENIED;
    }
  if (write_access && !BSE_STREAM_WRITABLE (stream))
    {
      BSE_STREAM_SET_ERROR (stream, BSE_ERROR_STREAM_WRITE_DENIED);

      return BSE_ERROR_STREAM_WRITE_DENIED;
    }

  if (BSE_STREAM_GET_CLASS (stream)->open)
    BSE_STREAM_GET_CLASS (stream)->open (stream, read_access, write_access);
  else
    BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);

  return BSE_STREAM_ERROR (stream);
}

BseErrorType
bse_stream_close (BseStream *stream)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_OPENED (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (BSE_STREAM_READY (stream))
    bse_stream_stop (stream);

  if (BSE_STREAM_GET_CLASS (stream)->close)
    BSE_STREAM_GET_CLASS (stream)->close (stream);
  else
    BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);

  return BSE_STREAM_ERROR (stream);
}

BseErrorType
bse_stream_start (BseStream *stream)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_OPENED (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (!BSE_STREAM_READY (stream))
    {
      if (BSE_STREAM_GET_CLASS (stream)->start)
	BSE_STREAM_GET_CLASS (stream)->start (stream);
      else
	BSE_STREAM_SET_FLAG (stream, READY);
    }

  return BSE_STREAM_ERROR (stream);
}

BseErrorType
bse_stream_stop (BseStream *stream)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_OPENED (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (BSE_STREAM_READY (stream))
    {
      if (BSE_STREAM_GET_CLASS (stream)->stop)
	BSE_STREAM_GET_CLASS (stream)->stop (stream);
      else
	BSE_STREAM_UNSET_FLAG (stream, READY);
    }

  return BSE_STREAM_ERROR (stream);
}

gboolean
bse_stream_would_block (BseStream *stream,
			guint	   n_values)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), FALSE);
  g_return_val_if_fail (BSE_STREAM_READY (stream), FALSE);

  BSE_STREAM_RESET_ERROR (stream);

  if (BSE_STREAM_GET_CLASS (stream)->would_block)
    return BSE_STREAM_GET_CLASS (stream)->would_block (stream, n_values);
  else
    return FALSE;
}

BseErrorType
bse_stream_read (BseStream *stream,
		 guint      n_bytes,
		 guint8    *bytes)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_READABLE (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_READY (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (n_bytes)
    {
      g_return_val_if_fail (bytes != NULL, BSE_ERROR_INTERNAL);

      if (BSE_STREAM_GET_CLASS (stream)->read)
	BSE_STREAM_GET_CLASS (stream)->read (stream, n_bytes, bytes);
      else
	BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);
    }

  return BSE_STREAM_ERROR (stream);
}

BseErrorType
bse_stream_write (BseStream    *stream,
		  guint         n_bytes,
		  const guint8 *bytes)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_WRITABLE (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_READY (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (n_bytes)
    {
      g_return_val_if_fail (bytes != NULL, BSE_ERROR_INTERNAL);

      if (BSE_STREAM_GET_CLASS (stream)->write)
	BSE_STREAM_GET_CLASS (stream)->write (stream, n_bytes, bytes);
      else
	BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);
    }
  
  return BSE_STREAM_ERROR (stream);
}

BseErrorType
bse_stream_read_sv (BseStream	   *stream,
		    guint           n_values,
		    BseSampleValue *values)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_READABLE (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_READY (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (n_values)
    {
      g_return_val_if_fail (values != NULL, BSE_ERROR_INTERNAL);

      if (BSE_STREAM_GET_CLASS (stream)->read_sv)
	BSE_STREAM_GET_CLASS (stream)->read_sv (stream, n_values, values);
      else
	BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);
    }

  return BSE_STREAM_ERROR (stream);
}

static guint
bse_stream_do_read_sv (BseStream      *stream,
		       guint	       n_values,
		       BseSampleValue *values)
{
  if (BSE_STREAM_GET_CLASS (stream)->read)
    return BSE_STREAM_GET_CLASS (stream)->read (stream,
						n_values * sizeof (BseSampleValue),
						(guint8*) values);
  else
    {
      BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);

      return 0;
    }
}

BseErrorType
bse_stream_write_sv (BseStream	          *stream,
		     guint                 n_values,
		     const BseSampleValue *values)
{
  g_return_val_if_fail (BSE_IS_STREAM (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_WRITABLE (stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_READY (stream), BSE_ERROR_INTERNAL);

  BSE_STREAM_RESET_ERROR (stream);

  if (n_values)
    {
      g_return_val_if_fail (values != NULL, BSE_ERROR_INTERNAL);

      if (BSE_STREAM_GET_CLASS (stream)->write_sv)
	BSE_STREAM_GET_CLASS (stream)->write_sv (stream, n_values, values);
      else
	BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);
    }
  
  return BSE_STREAM_ERROR (stream);
}

static guint
bse_stream_do_write_sv (BseStream            *stream,
			guint		      n_values,
			const BseSampleValue *values)
{
  if (BSE_STREAM_GET_CLASS (stream)->write)
    return BSE_STREAM_GET_CLASS (stream)->write (stream,
						 n_values * sizeof (BseSampleValue),
						 (const guint8*) values);
  else
    {
      BSE_STREAM_SET_ERROR (stream, BSE_ERROR_UNIMPLEMENTED);

      return 0;
    }
}
