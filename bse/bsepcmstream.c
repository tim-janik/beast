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
 */
#include	"bsepcmstream.h"


/* --- prototypes --- */
static void	bse_pcm_stream_class_init    (BsePcmStreamClass	     *class);
static void	bse_pcm_stream_init	     (BsePcmStream	     *pcm_stream);
static void	bse_pcm_stream_destroy	     (BseObject		     *object);


/* --- variables --- */
static BseStreamClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BsePcmStream)
{
  BseTypeInfo pcm_stream_info = {
    sizeof (BsePcmStreamClass),
    
    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_pcm_stream_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BsePcmStream),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_pcm_stream_init,
  };
  
  return bse_type_register_static (BSE_TYPE_STREAM,
				   "BsePcmStream",
				   "Base type for streamed pcm data handling",
				   &pcm_stream_info);
}

static void
bse_pcm_stream_class_init (BsePcmStreamClass *class)
{
  BseObjectClass *object_class;
  BseStreamClass *stream_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_STREAM);
  object_class = BSE_OBJECT_CLASS (class);
  stream_class = BSE_STREAM_CLASS (class);
  
  object_class->destroy = bse_pcm_stream_destroy;
  
  class->set_attribs = NULL;
}

static void
bse_pcm_stream_init (BsePcmStream *pcm_stream)
{
  pcm_stream->max_channels = 0;
  pcm_stream->min_play_frequency = 0;
  pcm_stream->max_play_frequency = 0;
  pcm_stream->min_record_frequency = 0;
  pcm_stream->max_record_frequency = 0;
  pcm_stream->min_fragment_size = 0;
  pcm_stream->max_fragment_size = 0;
  
  pcm_stream->attribs.n_channels = pcm_stream->max_channels;
  pcm_stream->attribs.play_frequency = pcm_stream->max_play_frequency;
  pcm_stream->attribs.record_frequency = pcm_stream->max_record_frequency;
  pcm_stream->attribs.fragment_size = 0;
}

static void
bse_pcm_stream_destroy (BseObject *object)
{
  BsePcmStream *pcm_stream;
  BseStream *stream;
  
  pcm_stream = BSE_PCM_STREAM (object);
  stream = BSE_STREAM (object);
  
  if (BSE_STREAM_READY (pcm_stream))
    bse_stream_stop (stream);
  
  if (BSE_STREAM_OPENED (pcm_stream))
    bse_stream_close (stream);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

BseStream*
bse_pcm_stream_new (BseType	 pcm_stream_type,
		    const gchar *first_param_name,
		    ...)
{
  BseStream *pcm_stream;
  va_list var_args;
  
  g_return_val_if_fail (bse_type_is_a (pcm_stream_type, BSE_TYPE_PCM_STREAM), NULL);
  
  va_start (var_args, first_param_name);
  pcm_stream = bse_object_new_valist (pcm_stream_type, first_param_name, var_args);
  va_end (var_args);
  
  return pcm_stream;
}

BseErrorType
bse_pcm_stream_set_attribs (BsePcmStream	  *pcm_stream,
			    BsePcmStreamAttribMask mask,
			    BsePcmStreamAttribs	  *attribs)
{
  g_return_val_if_fail (BSE_IS_PCM_STREAM (pcm_stream), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (attribs != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_STREAM_OPENED (pcm_stream), BSE_ERROR_INTERNAL);
  
  BSE_STREAM_RESET_ERROR (pcm_stream);
  
  mask &= BSE_PCMSA_MASK;
  
  if (mask)
    {
      if (BSE_PCM_STREAM_GET_CLASS (pcm_stream)->set_attribs)
	BSE_PCM_STREAM_GET_CLASS (pcm_stream)->set_attribs (pcm_stream, mask, attribs);
      else
	BSE_STREAM_SET_ERROR (pcm_stream, BSE_ERROR_UNIMPLEMENTED);
    }
  
  return BSE_STREAM_ERROR (pcm_stream);
}

#include	"bsepcmstream-oss.h"

BseType
bse_pcm_stream_default_type (void)
{
  return BSE_TYPE_PCM_STREAM_OSS;
}
