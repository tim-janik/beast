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
 *
 * bsepcmstream.h: bse pcm stream base class
 */
#ifndef	__BSE_PCM_STREAM_H__
#define	__BSE_PCM_STREAM_H__

#include	<bse/bsestream.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_PCM_STREAM		 (BSE_TYPE_ID (BsePcmStream))
#define BSE_PCM_STREAM(object)		 (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_PCM_STREAM, BsePcmStream))
#define BSE_PCM_STREAM_CLASS(class)	 (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_STREAM, BsePcmStreamClass))
#define BSE_IS_PCM_STREAM(object)	 (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_PCM_STREAM))
#define BSE_IS_PCM_STREAM_CLASS(class)	 (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_STREAM))
#define BSE_PCM_STREAM_GET_CLASS(object) ((BsePcmStreamClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* BsePcmStream attributes and flags
 */
typedef struct _BsePcmStreamAttribs BsePcmStreamAttribs;
struct _BsePcmStreamAttribs
{
  guint			n_channels;
  guint			play_frequency;
  guint			record_frequency;
  guint			fragment_size;
};
typedef enum
{
  BSE_PCMSA_NONE		   = 0,
  BSE_PCMSA_N_CHANNELS		   = 1 << 0,
  BSE_PCMSA_PLAY_FREQUENCY	   = 1 << 1,
  BSE_PCMSA_REC_FREQUENCY	   = 1 << 2,
  BSE_PCMSA_FRAGMENT_SIZE	   = 1 << 3,
  BSE_PCMSA_MASK		   = 0x0f  /* <skip> */
} BsePcmStreamAttribMask;


/* --- BsePcmStream object --- */
struct _BsePcmStream
{
  BseStream	bse_stream;
  
  /* Constant Values
   */
  guint max_channels;
  guint min_play_frequency;
  guint max_play_frequency;
  guint min_record_frequency;
  guint max_record_frequency;
  guint min_fragment_size;
  guint	max_fragment_size;
  
  /* Stream attributes
   */
  BsePcmStreamAttribs	attribs;
};
struct _BsePcmStreamClass
{
  BseStreamClass bse_stream_class;
  
  void	(*set_attribs)	(BsePcmStream	       *stream,
			 BsePcmStreamAttribMask mask,
			 BsePcmStreamAttribs   *attribs);
};


/* --- prototypes -- */
BseType	     bse_pcm_stream_default_type (void);
BseStream*   bse_pcm_stream_new		 (BseType		 pcm_stream_type,
					  const gchar		*first_param_name,
					  ...);
/* set and auto-update the attributes */
BseErrorType bse_pcm_stream_set_attribs	 (BsePcmStream		*pcm_stream,
					  BsePcmStreamAttribMask mask,
					  BsePcmStreamAttribs	*attribs);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_STREAM_H__ */
