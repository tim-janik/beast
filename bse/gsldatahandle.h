/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#ifndef __GSL_DATA_HANDLE_H__
#define __GSL_DATA_HANDLE_H__

#include <gsl/gsldefs.h>
#include <gsl/gslcommon.h>	/* GslErrorType */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- macros --- */
#define	GSL_DATA_HANDLE_OPENED(handle)	(((GslDataHandle*) (handle))->open_count > 0)


/* --- typedefs & structures --- */
struct _GslDataHandle
{
  /* constant members */
  GslDataHandleFuncs *vtable;
  gchar		     *name;
  GTime		      mtime;
  GslLong	      n_values;		/* length in floats */
  /* common members */
  guint		      bit_depth;	/* number of significant bits */
  GslMutex	      mutex;
  guint		      ref_count;
  guint		      open_count;
};
struct _GslDataHandleFuncs
{
  gint		(*open)			(GslDataHandle		*data_handle);
  GslLong	(*read)			(GslDataHandle		*data_handle,
					 GslLong		 voffset, /* in values */
					 GslLong		 n_values,
					 gfloat			*values);
  void		(*close)		(GslDataHandle		*data_handle);
  void          (*destroy)		(GslDataHandle		*handle);
};



/* --- standard functions --- */
GslDataHandle*	  gsl_data_handle_ref		(GslDataHandle	  *dhandle);
void		  gsl_data_handle_unref		(GslDataHandle	  *dhandle);
gint /*errno*/	  gsl_data_handle_open		(GslDataHandle	  *dhandle);
void		  gsl_data_handle_close		(GslDataHandle	  *dhandle);
GslLong		  gsl_data_handle_read		(GslDataHandle	  *data_handle,
						 GslLong	   value_offset,
						 GslLong	   n_values,
						 gfloat		  *values);
GslDataHandle*	  gsl_data_handle_new_translate	(GslDataHandle	  *src_handle,
						 GslLong	   cut_offset,
						 GslLong	   n_cut_values,
						 GslLong	   tail_cut);
GslDataHandle*	  gsl_data_handle_new_reversed	(GslDataHandle	  *src_handle);
GslDataHandle*	  gsl_data_handle_new_insert	(GslDataHandle	  *src_handle,
						 GslLong	   insertion_offset,
						 GslLong	   n_paste_values,
						 const gfloat	  *paste_values,
						 guint             paste_bit_depth);
GslDataHandle*	  gsl_data_handle_new_merge	(GslDataHandle	  *src_handle,
						 GslLong	   insertion_offset,
						 GslLong	   n_insertions,
						 GslDataHandle	  *merge_handle,
						 GslLong	   merge_value_offset);
/* cheap and inefficient, testpurpose only */
GslDataHandle*	  gsl_data_handle_new_looped	(GslDataHandle	  *src_handle,
						 GslLong	   loop_start,
						 GslLong	   loop_end);
GslDataHandle*	  gsl_data_handle_new_dcached	(GslDataCache	  *dcache);


/* --- wave specific functions --- */
typedef enum    /*< skip >*/
{
  GSL_WAVE_FORMAT_NONE,
  GSL_WAVE_FORMAT_UNSIGNED_8,
  GSL_WAVE_FORMAT_SIGNED_8,
  GSL_WAVE_FORMAT_UNSIGNED_12,
  GSL_WAVE_FORMAT_SIGNED_12,
  GSL_WAVE_FORMAT_UNSIGNED_16,
  GSL_WAVE_FORMAT_SIGNED_16,
  GSL_WAVE_FORMAT_FLOAT,
  GSL_WAVE_FORMAT_LAST
} GslWaveFormatType;

GslDataHandle*	  gsl_wave_handle_new_cached	(const gchar      *file_name,
						 GTime             mtime, /* may be 0 */
						 GslWaveFormatType format,
						 guint             byte_order,
						 guint             boffset_reminder);
GslDataHandle*	  gsl_wave_handle_new		(const gchar	  *file_name,
						 GTime		   mtime, /* may be 0 */
						 GslWaveFormatType format,
						 guint		   byte_order,
						 GslLong	   byte_offset,
						 GslLong	   n_values);


/* --- auxillary functions --- */
gboolean	  gsl_data_handle_common_init	(GslDataHandle	  *dhandle,
						 const gchar	  *file_name,
						 guint		   bit_depth);
void		  gsl_data_handle_common_free	(GslDataHandle	  *dhandle);


						 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_DATA_HANDLE_H__ */
