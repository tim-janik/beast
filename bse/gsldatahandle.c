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
#include "gsldatahandle.h"

#include "gslcommon.h"
#include "gsldatacache.h"
#include "gslfilehash.h"

#include <string.h>
#include <errno.h>


/* --- typedefs --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;
} ChainHandle;
typedef ChainHandle ReversedHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  GslLong	    cut_offset;
  GslLong	    n_cut_values;
} CutHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  GslLong	    paste_offset;
  GslLong	    n_paste_values;
  const gfloat	   *paste_values;
  void            (*free_values) (gpointer);
} InsertHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  GslLong	    loop_start;
  GslLong	    loop_width;
} LoopHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslDataCache	   *dcache;
  guint		    node_size;
} DCacheHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslWaveFormatType format;
  guint             byte_order;
  guint             boffset_reminder;
  GslHFile	   *hfile;
} WaveHandle;


/* --- standard functions --- */
#include <sys/types.h> // FIXME
#include <sys/stat.h> // FIXME
#include <unistd.h> // FIXME
gboolean
gsl_data_handle_common_init (GslDataHandle *dhandle,
			     const gchar   *file_name,
			     guint          bit_depth)
{
  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (dhandle->vtable == NULL, FALSE);
  g_return_val_if_fail (dhandle->name == NULL, FALSE);
  g_return_val_if_fail (dhandle->ref_count == 0, FALSE);
  g_return_val_if_fail (bit_depth > 0, FALSE);
  
  if (file_name)
    {
      struct stat statbuf = { 0, };
      
      if (stat (file_name, &statbuf) < 0 || statbuf.st_size < 1)
	return FALSE;
      
      dhandle->name = g_strdup (file_name);
      dhandle->n_values = statbuf.st_size;
    }
  else
    {
      dhandle->name = NULL;
      dhandle->n_values = 0;
    }
  dhandle->bit_depth = bit_depth;
  gsl_mutex_init (&dhandle->mutex);
  dhandle->ref_count = 1;
  dhandle->open_count = 0;
  
  return TRUE;
}

GslDataHandle*
gsl_data_handle_ref (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, NULL);
  g_return_val_if_fail (dhandle->ref_count > 0, NULL);
  
  GSL_SPIN_LOCK (&dhandle->mutex);
  dhandle->ref_count++;
  GSL_SPIN_UNLOCK (&dhandle->mutex);
  
  return dhandle;
}

void
gsl_data_handle_common_free (GslDataHandle *dhandle)
{
  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (dhandle->vtable != NULL);
  g_return_if_fail (dhandle->ref_count == 0);
  
  g_free (dhandle->name);
  dhandle->name = NULL;
  gsl_mutex_destroy (&dhandle->mutex);
}

void
gsl_data_handle_unref (GslDataHandle *dhandle)
{
  gboolean destroy;
  
  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (dhandle->ref_count > 0);
  
  GSL_SPIN_LOCK (&dhandle->mutex);
  dhandle->ref_count--;
  destroy = dhandle->ref_count == 0;
  GSL_SPIN_UNLOCK (&dhandle->mutex);
  if (destroy)
    dhandle->vtable->destroy (dhandle);
}

GslErrorType
gsl_data_handle_open (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (dhandle->ref_count > 0, GSL_ERROR_INTERNAL);

  GSL_SPIN_LOCK (&dhandle->mutex);
  if (dhandle->open_count == 0)
    {
      GslErrorType error = dhandle->vtable->open (dhandle);

      if (error)
	{
	  GSL_SPIN_UNLOCK (&dhandle->mutex);
	  return error;
	}
      dhandle->ref_count++;
      dhandle->open_count++;
    }
  else
    dhandle->open_count++;
  GSL_SPIN_UNLOCK (&dhandle->mutex);
  
  return GSL_ERROR_NONE;
}

void
gsl_data_handle_close (GslDataHandle *dhandle)
{
  gboolean need_unref;
  
  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (dhandle->ref_count > 0);
  g_return_if_fail (dhandle->open_count > 0);
  
  GSL_SPIN_LOCK (&dhandle->mutex);
  dhandle->open_count--;
  need_unref = !dhandle->open_count;
  if (!dhandle->open_count)
    dhandle->vtable->close (dhandle);
  GSL_SPIN_UNLOCK (&dhandle->mutex);
  if (need_unref)
    gsl_data_handle_unref (dhandle);
}

GslLong
gsl_data_handle_read (GslDataHandle *dhandle,
		      GslLong        value_offset,
		      GslLong        n_values,
		      gfloat        *values)
{
  GslLong l;
  
  g_return_val_if_fail (dhandle != NULL, -1);
  g_return_val_if_fail (dhandle->ref_count > 0, -1);
  g_return_val_if_fail (dhandle->open_count > 0, -1);
  g_return_val_if_fail (value_offset >= 0, -1);
  if (n_values < 1)
    return 0;
  g_return_val_if_fail (values != NULL, -1);
  g_return_val_if_fail (value_offset < dhandle->n_values, -1);
  
  n_values = MIN (n_values, dhandle->n_values - value_offset);
  GSL_SPIN_LOCK (&dhandle->mutex);
  l = dhandle->vtable->read (dhandle, value_offset, n_values, values);
  GSL_SPIN_UNLOCK (&dhandle->mutex);
  
  return l;
}


/* --- chain handle --- */
static GslErrorType
chain_handle_open (GslDataHandle *data_handle)
{
  ChainHandle *chandle = (ChainHandle*) data_handle;
  
  if (chandle->src_handle)
    return gsl_data_handle_open (chandle->src_handle);
  else
    return GSL_ERROR_NONE;
}

static void
chain_handle_close (GslDataHandle *data_handle)
{
  ChainHandle *chandle = (ChainHandle*) data_handle;
  
  if (chandle->src_handle)
    gsl_data_handle_close (chandle->src_handle);
}


/* --- reversed handle --- */
static void
reverse_handle_destroy (GslDataHandle *data_handle)
{
  ReversedHandle *rhandle = (ReversedHandle*) data_handle;
  
  gsl_data_handle_unref (rhandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (ReversedHandle, rhandle);
}

static GslLong
reverse_handle_read (GslDataHandle *data_handle,
		     GslLong        voffset,
		     GslLong        n_values,
		     gfloat        *values)
{
  ReversedHandle *rhandle = (ReversedHandle*) data_handle;
  GslLong left, new_offset = data_handle->n_values - (voffset + n_values);
  gfloat *t, *p = values;
  
  g_assert (new_offset >= 0);
  
  left = n_values;
  do
    {
      GslLong l = gsl_data_handle_read (rhandle->src_handle, new_offset, left, p);
      
      if (l < 0)
	return l;	/* pass on errors */
      
      new_offset += l;
      left -= l;
      p += l;
    }
  while (left > 0);
  
  p = values;
  t = values + n_values - 1;
  while (p < t)
    {
      gfloat v = *t;
      
      *t-- = *p;
      *p++ = v;
    }
  return n_values;
}

GslDataHandle*
gsl_data_handle_new_reverse (GslDataHandle *src_handle)
{
  static GslDataHandleFuncs reverse_handle_vtable = {
    chain_handle_open,
    reverse_handle_read,
    chain_handle_close,
    reverse_handle_destroy,
  };
  ReversedHandle *rhandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  
  rhandle = gsl_new_struct0 (ReversedHandle, 1);
  success = gsl_data_handle_common_init (&rhandle->dhandle, NULL, src_handle->bit_depth);
  if (success)
    {
      rhandle->dhandle.name = g_strconcat (src_handle->name, "// #reversed /", NULL);
      rhandle->dhandle.vtable = &reverse_handle_vtable;
      rhandle->dhandle.n_values = src_handle->n_values;
      rhandle->src_handle = gsl_data_handle_ref (src_handle);
    }
  else
    {
      gsl_delete_struct (ReversedHandle, rhandle);
      return NULL;
    }
  return &rhandle->dhandle;
}


/* --- cut handle --- */
static void
cut_handle_destroy (GslDataHandle *data_handle)
{
  CutHandle *chandle = (CutHandle*) data_handle;
  
  gsl_data_handle_unref (chandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (CutHandle, chandle);
}

static GslLong
cut_handle_read (GslDataHandle *data_handle,
		 GslLong        voffset,
		 GslLong        n_values,
		 gfloat        *values)
{
  CutHandle *chandle = (CutHandle*) data_handle;
  GslLong orig_n_values = n_values;
  
  if (voffset < chandle->cut_offset)
    {
      GslLong l = MIN (chandle->cut_offset - voffset, n_values);
      
      l = gsl_data_handle_read (chandle->src_handle, voffset, l, values);
      if (l < 0)
	return l;	/* pass on errors */
      n_values -= l;
      values += l;
      voffset += l;
    }
  
  if (voffset >= chandle->cut_offset)
    {
      GslLong l = gsl_data_handle_read (chandle->src_handle, voffset + chandle->n_cut_values, n_values, values);
      
      if (l < 0 && orig_n_values == n_values)
	return l;       /* pass on errors */
      else if (l < 0)
	l = 0;

      n_values -= l;
    }
  
  return orig_n_values - n_values;
}

static GslDataHandle*
gsl_data_handle_new_translate (GslDataHandle *src_handle,
			       GslLong        cut_offset,
			       GslLong        n_cut_values,
			       GslLong	      tail_cut)
{
  static GslDataHandleFuncs cut_handle_vtable = {
    chain_handle_open,
    cut_handle_read,
    chain_handle_close,
    cut_handle_destroy,
  };
  CutHandle *chandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  g_return_val_if_fail (cut_offset >= 0 && n_cut_values >= 0 && tail_cut >= 0, NULL);
  g_return_val_if_fail (cut_offset < src_handle->n_values, NULL);
  g_return_val_if_fail (cut_offset + n_cut_values + tail_cut < src_handle->n_values, NULL);
  
  chandle = gsl_new_struct0 (CutHandle, 1);
  success = gsl_data_handle_common_init (&chandle->dhandle, NULL, src_handle->bit_depth);
  if (success)
    {
      chandle->dhandle.name = g_strconcat (src_handle->name, "// #translate /", NULL);
      chandle->dhandle.vtable = &cut_handle_vtable;
      chandle->dhandle.n_values = src_handle->n_values - n_cut_values - tail_cut;
      chandle->src_handle = gsl_data_handle_ref (src_handle);
      chandle->cut_offset = n_cut_values ? cut_offset : 0;
      chandle->n_cut_values = n_cut_values;
    }
  else
    {
      gsl_delete_struct (CutHandle, chandle);
      return NULL;
    }
  return &chandle->dhandle;
}

/**
 * gsl_data_handle_new_cut
 * @src_handle:   source GslDataHandle
 * @cut_offset:   offset of gap into @src_handle
 * @n_cut_values: length of gap in @src_handle
 * @RETURNS: a newly created data handle
 *
 * Create a new data handle containing the contents of @src_handle
 * minus @n_cut_values at offset @cut_offset.
 */
GslDataHandle*
gsl_data_handle_new_cut (GslDataHandle *src_handle,
			 GslLong        cut_offset,
			 GslLong        n_cut_values)
{
  return gsl_data_handle_new_translate (src_handle, cut_offset, n_cut_values, 0);
}

/**
 * gsl_data_handle_new_crop
 * @src_handle: source GslDataHandle
 * @n_head_cut: number of values to cut at data handle head
 * @n_tail_cut: number of values to cut at data handle tail
 * @RETURNS: a newly created data handle
 *
 * Create a new data handle containing the contents of @src_handle
 * minus @n_head_cut values at the start and @n_tail_cut values at
 * the end.
 */
GslDataHandle*
gsl_data_handle_new_crop (GslDataHandle *src_handle,
			  GslLong        n_head_cut,
			  GslLong        n_tail_cut)
{
  return gsl_data_handle_new_translate (src_handle, 0, n_head_cut, n_tail_cut);
}


/* --- insert handle --- */
static void
insert_handle_destroy (GslDataHandle *data_handle)
{
  InsertHandle *ihandle = (InsertHandle*) data_handle;
  void (*free_values) (gpointer) = ihandle->free_values;
  const gfloat *paste_values = ihandle->paste_values;
  
  if (ihandle->src_handle)
    gsl_data_handle_unref (ihandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  ihandle->paste_values = NULL;
  ihandle->free_values = NULL;
  gsl_delete_struct (InsertHandle, ihandle);
  
  if (free_values)
    free_values ((gpointer) paste_values);
}

static GslLong
insert_handle_read (GslDataHandle *data_handle,
		    GslLong        voffset,
		    GslLong        n_values,
		    gfloat        *values)
{
  InsertHandle *ihandle = (InsertHandle*) data_handle;
  GslLong l, orig_n_values = n_values;
  
  if (voffset < ihandle->paste_offset)
    {
      l = MIN (n_values, ihandle->paste_offset - voffset);
      l = gsl_data_handle_read (ihandle->src_handle, voffset, l, values);
      if (l < 0)
	return l;       /* pass on errors */
      
      voffset += l;
      n_values -= l;
      values += l;
    }
  
  if (voffset >= ihandle->paste_offset && voffset < ihandle->paste_offset + ihandle->n_paste_values)
    {
      l = MIN (n_values, ihandle->paste_offset + ihandle->n_paste_values - voffset);
      memcpy (values, ihandle->paste_values + voffset - ihandle->paste_offset, l * sizeof (values[0]));
      voffset += l;
      n_values -= l;
      values += l;
    }
  
  if (voffset >= ihandle->paste_offset + ihandle->n_paste_values && n_values)
    {
      l = gsl_data_handle_read (ihandle->src_handle, voffset - ihandle->n_paste_values, n_values, values);
      if (l < 0 && orig_n_values == n_values)
	return l;       /* pass on errors */
      else if (l < 0)
	l = 0;
      
      n_values -= l;
    }
  
  return orig_n_values - n_values;
}

GslDataHandle*
gsl_data_handle_new_insert (GslDataHandle *src_handle,
			    guint          paste_bit_depth,
			    GslLong        insertion_offset,
			    GslLong        n_paste_values,
			    const gfloat  *paste_values,
			    void         (*free) (gpointer values))
{
  static GslDataHandleFuncs insert_handle_vtable = {
    chain_handle_open,
    insert_handle_read,
    chain_handle_close,
    insert_handle_destroy,
  };
  InsertHandle *ihandle;
  gboolean success;
  
  g_return_val_if_fail (n_paste_values >= 0, NULL);
  if (!src_handle)
    {
      g_return_val_if_fail (insertion_offset == 0, NULL);
      g_return_val_if_fail (n_paste_values > 0, NULL);
    }
  else
    {
      if (insertion_offset < 0)
	insertion_offset = src_handle->n_values;
      g_return_val_if_fail (insertion_offset <= src_handle->n_values, NULL);
    }
  if (n_paste_values)
    g_return_val_if_fail (paste_values != NULL, NULL);
  
  ihandle = gsl_new_struct0 (InsertHandle, 1);
  if (src_handle)
    paste_bit_depth = MAX (src_handle->bit_depth, paste_bit_depth);
  success = gsl_data_handle_common_init (&ihandle->dhandle, NULL, paste_bit_depth);
  if (success)
    {
      ihandle->dhandle.name = g_strconcat (src_handle ? src_handle->name : "", "// #insert /", NULL);
      ihandle->dhandle.vtable = &insert_handle_vtable;
      ihandle->dhandle.n_values = n_paste_values + (src_handle ? src_handle->n_values : 0);
      ihandle->src_handle = src_handle ? gsl_data_handle_ref (src_handle) : NULL;
      ihandle->paste_offset = insertion_offset;
      ihandle->n_paste_values = n_paste_values;
      ihandle->paste_values = paste_values;
      ihandle->free_values = free;
    }
  else
    {
      gsl_delete_struct (InsertHandle, ihandle);
      return NULL;
    }
  return &ihandle->dhandle;
}


/* --- loop handle --- */
static void
loop_handle_destroy (GslDataHandle *data_handle)
{
  LoopHandle *lhandle = (LoopHandle*) data_handle;
  
  gsl_data_handle_unref (lhandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (LoopHandle, lhandle);
}

static GslLong
loop_handle_read (GslDataHandle *data_handle,
		  GslLong        voffset,
		  GslLong        n_values,
		  gfloat        *values)
{
  LoopHandle *lhandle = (LoopHandle*) data_handle;
  
  if (voffset < lhandle->loop_start)
    return gsl_data_handle_read (lhandle->src_handle, voffset,
				 MIN (lhandle->loop_start - voffset, n_values),
				 values);
  else
    {
      GslLong noffset = voffset - lhandle->loop_start;
      
      noffset %= lhandle->loop_width;
      
      return gsl_data_handle_read (lhandle->src_handle,
				   lhandle->loop_start + noffset,
				   MIN (lhandle->loop_width - noffset, n_values),
				   values);
    }
}

GslDataHandle*
gsl_data_handle_new_looped (GslDataHandle *src_handle,
			    GslLong        loop_start,
			    GslLong        loop_end)
{
  static GslDataHandleFuncs loop_handle_vtable = {
    chain_handle_open,
    loop_handle_read,
    chain_handle_close,
    loop_handle_destroy,
  };
  LoopHandle *lhandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  g_return_val_if_fail (loop_start >= 0, NULL);
  g_return_val_if_fail (loop_start <= loop_end, NULL);
  g_return_val_if_fail (loop_end < src_handle->n_values, NULL);
  
  lhandle = gsl_new_struct0 (LoopHandle, 1);
  success = gsl_data_handle_common_init (&lhandle->dhandle, NULL, src_handle->bit_depth);
  if (success)
    {
      lhandle->dhandle.name = g_strdup_printf ("%s// #loop(0x%lx:0x%lx) /", src_handle->name, loop_start, loop_end);
      lhandle->dhandle.vtable = &loop_handle_vtable;
      lhandle->dhandle.n_values = GSL_MAXLONG;
      lhandle->src_handle = gsl_data_handle_ref (src_handle);
      lhandle->loop_start = loop_start;
      lhandle->loop_width = loop_end - loop_start + 1;
    }
  else
    {
      gsl_delete_struct (LoopHandle, lhandle);
      return NULL;
    }
  return &lhandle->dhandle;
}


/* --- dcache handle --- */
static void
dcache_handle_destroy (GslDataHandle *data_handle)
{
  DCacheHandle *dhandle = (DCacheHandle*) data_handle;
  
  gsl_data_cache_unref (dhandle->dcache);
  
  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (DCacheHandle, dhandle);
}

static GslErrorType
dcache_handle_open (GslDataHandle *data_handle)
{
  DCacheHandle *dhandle = (DCacheHandle*) data_handle;
  
  gsl_data_cache_open (dhandle->dcache);
  
  return 0;	/* FIXME: should catch messages here and return error */
}

static void
dcache_handle_close (GslDataHandle *data_handle)
{
  DCacheHandle *dhandle = (DCacheHandle*) data_handle;
  
  gsl_data_cache_close (dhandle->dcache);
}

static GslLong
dcache_handle_read (GslDataHandle *data_handle,
		    GslLong        voffset,
		    GslLong        n_values,
		    gfloat        *values)
{
  DCacheHandle *dhandle = (DCacheHandle*) data_handle;
  GslDataCacheNode *node;
  
  node = gsl_data_cache_ref_node (dhandle->dcache, voffset, TRUE);
  voffset -= node->offset;
  n_values = MIN (n_values, dhandle->node_size - voffset);
  memcpy (values, node->data + voffset, sizeof (values[0]) * n_values);
  
  return n_values;
}

GslDataHandle*
gsl_data_handle_new_dcached (GslDataCache *dcache)
{
  static GslDataHandleFuncs dcache_handle_vtable = {
    dcache_handle_open,
    dcache_handle_read,
    dcache_handle_close,
    dcache_handle_destroy,
  };
  DCacheHandle *dhandle;
  gboolean success;
  
  g_return_val_if_fail (dcache != NULL, NULL);
  
  dhandle = gsl_new_struct0 (DCacheHandle, 1);
  success = gsl_data_handle_common_init (&dhandle->dhandle, NULL, dcache->dhandle->bit_depth);
  if (success)
    {
      dhandle->dhandle.name = g_strdup_printf ("%s// #dcache /", dcache->dhandle->name);
      dhandle->dhandle.vtable = &dcache_handle_vtable;
      dhandle->dhandle.n_values = dcache->dhandle->n_values;
      dhandle->dcache = gsl_data_cache_ref (dcache);
      dhandle->node_size = GSL_DATA_CACHE_NODE_SIZE (dcache) + dcache->padding;
    }
  else
    {
      gsl_delete_struct (DCacheHandle, dhandle);
      return NULL;
    }
  return &dhandle->dhandle;
}


/* --- wave handle --- */
static inline const guint G_GNUC_CONST
wave_format_bit_depth (const GslWaveFormatType format)
{
  switch (format)
    {
    case GSL_WAVE_FORMAT_UNSIGNED_8:
    case GSL_WAVE_FORMAT_SIGNED_8:
      return 8;
    case GSL_WAVE_FORMAT_UNSIGNED_12:
    case GSL_WAVE_FORMAT_SIGNED_12:
      return 12;
    case GSL_WAVE_FORMAT_UNSIGNED_16:
    case GSL_WAVE_FORMAT_SIGNED_16:
      return 16;
    case GSL_WAVE_FORMAT_FLOAT:
      return 32;
    default:
      return 0;
    }
}
#define	wave_format_byte_width(f)	((wave_format_bit_depth (f) + 7) / 8)

static void
wave_handle_destroy (GslDataHandle *data_handle)
{
  WaveHandle *whandle = (WaveHandle*) data_handle;

  gsl_data_handle_common_free (data_handle);
  gsl_delete_struct (WaveHandle, whandle);
}

static GslErrorType
wave_handle_open (GslDataHandle *data_handle)
{
  WaveHandle *whandle = (WaveHandle*) data_handle;

  whandle->hfile = gsl_hfile_open (whandle->dhandle.name);
  if (!whandle->hfile)
    return gsl_error_from_errno (errno, GSL_ERROR_OPEN_FAILED);
  else
    return GSL_ERROR_NONE;
}

static void
wave_handle_close (GslDataHandle *data_handle)
{
  WaveHandle *whandle = (WaveHandle*) data_handle;
  
  gsl_hfile_close (whandle->hfile);
  whandle->hfile = NULL;
}

static GslLong
wave_handle_read (GslDataHandle *data_handle,
		  GslLong        voffset,
		  GslLong        n_values,
		  gfloat        *values)
{
  WaveHandle *whandle = (WaveHandle*) data_handle;
  gpointer buffer = values;
  GslLong l, i, byte_offset;

  byte_offset = voffset * wave_format_byte_width (whandle->format);	/* float offset into bytes */
  byte_offset += whandle->boffset_reminder;				/* remaining byte offset */

  switch (whandle->format)
    {
      guint8 *u8; gint8 *s8; guint16 *u16; guint32 *u32;
    case GSL_WAVE_FORMAT_UNSIGNED_8:
      u8 = buffer; u8 += n_values * 3;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values, u8);
      if (l < 1)
	return l;
      for (i = 0; i < l; i++)
	{
	  int v = u8[i] - 128;
	  values[i] = v * (1. / 128.);
	}
      break;
    case GSL_WAVE_FORMAT_SIGNED_8:
      s8 = buffer; s8 += n_values * 3;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values, s8);
      if (l < 1)
	return l;
      for (i = 0; i < l; i++)
	values[i] = s8[i] * (1. / 128.);
      break;
    case GSL_WAVE_FORMAT_SIGNED_12:
    case GSL_WAVE_FORMAT_UNSIGNED_12:
    case GSL_WAVE_FORMAT_SIGNED_16:
    case GSL_WAVE_FORMAT_UNSIGNED_16:
      u16 = buffer; u16 += n_values;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values << 1, u16);
      if (l < 2)
	return l < 0 ? l : 0;
      l >>= 1;
      switch (whandle->format)
	{
	case GSL_WAVE_FORMAT_UNSIGNED_16:
	  if (whandle->byte_order != G_BYTE_ORDER)
	    for (i = 0; i < l; i++)
	      {
		int v = GUINT16_SWAP_LE_BE (u16[i]); v -= 32768;
		values[i] = v * (1. / 32768.);
	      }
	  else /* whandle->byte_order == G_BYTE_ORDER */
	    for (i = 0; i < l; i++)
	      {
		int v = u16[i]; v -= 32768;
		values[i] = v * (1. / 32768.);
	      }
	  break;
	case GSL_WAVE_FORMAT_UNSIGNED_12:
	  if (whandle->byte_order != G_BYTE_ORDER)
	    for (i = 0; i < l; i++)
	      {
		int v = GUINT16_SWAP_LE_BE (u16[i]); v &= 0x0fff; v -= 4096;
		values[i] = v * (1. / 4096.);
	      }
	  else /* whandle->byte_order == G_BYTE_ORDER */
	    for (i = 0; i < l; i++)
	      {
		int v = u16[i]; v &= 0x0fff; v -= 4096;
		values[i] = v * (1. / 4096.);
	      }
	  break;
	case GSL_WAVE_FORMAT_SIGNED_16:
	  if (whandle->byte_order != G_BYTE_ORDER)
	    for (i = 0; i < l; i++)
	      {
		gint16 v = GUINT16_SWAP_LE_BE (u16[i]);
		values[i] = v * (1. / 32768.);
	      }
	  else /* whandle->byte_order == G_BYTE_ORDER */
	    for (i = 0; i < l; i++)
	      {
		gint16 v = u16[i];
		values[i] = v * (1. / 32768.);
	      }
	  break;
	case GSL_WAVE_FORMAT_SIGNED_12:
	  if (whandle->byte_order != G_BYTE_ORDER)
	    for (i = 0; i < l; i++)
	      {
		gint16 v = GUINT16_SWAP_LE_BE (u16[i]);
		values[i] = CLAMP (v, -4096, 4096) * (1. / 4096.);
	      }
	  else /* whandle->byte_order == G_BYTE_ORDER */
	    for (i = 0; i < l; i++)
	      {
		gint16 v = u16[i];
		values[i] = CLAMP (v, -4096, 4096) * (1. / 4096.);
	      }
	  break;
	default:
	  g_assert_not_reached ();
	}
      break;
    case GSL_WAVE_FORMAT_FLOAT:
      u32 = buffer;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values << 2, u32);
      if (l < 4)
	return l < 0 ? l : 0;
      l >>= 2;
      if (whandle->byte_order != G_BYTE_ORDER)
	for (i = 0; i < l; i++)
	  u32[i] = GUINT32_SWAP_LE_BE (u32[i]);
      break;
    default:
      l = -1;
      g_assert_not_reached ();
    }
  
  return l;
}

static GslDataHandle*
raw_wave_handle_new_cached (const gchar      *file_name,
			    GslWaveFormatType format,
			    guint	      byte_order,
			    guint	      boffset_reminder)
{
  static GslDataHandleFuncs wave_handle_vtable = {
    wave_handle_open,
    wave_handle_read,
    wave_handle_close,
    wave_handle_destroy,
  };
  WaveHandle *whandle;
  gboolean success;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (format > GSL_WAVE_FORMAT_NONE && format < GSL_WAVE_FORMAT_LAST, NULL);
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);
  g_return_val_if_fail (boffset_reminder < wave_format_byte_width (format), NULL);

  whandle = gsl_new_struct0 (WaveHandle, 1);
  success = gsl_data_handle_common_init (&whandle->dhandle, file_name, wave_format_bit_depth (format));
  if (success)
    {
      GslLong l;
      
      whandle->dhandle.vtable = &wave_handle_vtable;
      whandle->format = format;
      whandle->byte_order = byte_order;
      whandle->boffset_reminder = boffset_reminder;
      whandle->hfile = NULL;
      
      /* convert size into n_values, i.e. float length */
      l = whandle->dhandle.n_values;
      if (l > whandle->boffset_reminder)
	{
	  l -= whandle->boffset_reminder;
	  l /= wave_format_byte_width (format);
	  if (l > 0)
	    whandle->dhandle.n_values = l;
	  else
	    success = FALSE;
	}
      else
	success = FALSE;

      if (!success)
	gsl_data_handle_common_free (&whandle->dhandle);
    }
  if (!success)
    {
      gsl_delete_struct (WaveHandle, whandle);
      return NULL;
    }

  return &whandle->dhandle;
}

GslDataHandle*
gsl_wave_handle_new (const gchar      *file_name,
		     GslWaveFormatType format,
		     guint             byte_order,
		     GslLong           byte_offset,
		     GslLong           n_values)
{
  GslDataHandle *whandle, *thandle;
  guint fwidth, boffset_reminder;
  GslLong offset;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (format > GSL_WAVE_FORMAT_NONE && format < GSL_WAVE_FORMAT_LAST, NULL);
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);
  g_return_val_if_fail (byte_offset >= 0, NULL);
  g_return_val_if_fail (n_values >= 1 || n_values == -1, NULL);

  fwidth = wave_format_byte_width (format);
  boffset_reminder = byte_offset % fwidth;
  whandle = raw_wave_handle_new_cached (file_name, format, byte_order, boffset_reminder);
  if (!whandle)
    return NULL;

  offset = (byte_offset - boffset_reminder) / fwidth;
  if (n_values == -1)
    n_values = whandle->n_values - offset;

  if (offset + n_values > whandle->n_values)
    {
      gsl_data_handle_unref (whandle);
      return NULL;
    }

  /* do we need a translation handle? */
  if (offset == 0 && whandle->n_values == n_values)
    return whandle;

  thandle = gsl_data_handle_new_translate (whandle,
					   0, offset,
					   whandle->n_values - offset - n_values);
  gsl_data_handle_unref (whandle);
  g_assert (thandle->n_values == n_values);	/* paranoid */
  
  return thandle;
}

const gchar*
gsl_wave_format_to_string (GslWaveFormatType format)
{
  switch (format)
    {
    case GSL_WAVE_FORMAT_UNSIGNED_8:    return "unsigned_8";
    case GSL_WAVE_FORMAT_SIGNED_8:      return "signed_8";
    case GSL_WAVE_FORMAT_UNSIGNED_12:   return "unsigned_12";
    case GSL_WAVE_FORMAT_SIGNED_12:     return "signed_12";
    case GSL_WAVE_FORMAT_UNSIGNED_16:   return "unsigned_16";
    case GSL_WAVE_FORMAT_SIGNED_16:     return "signed_16";
    case GSL_WAVE_FORMAT_FLOAT:         return "float";
    case GSL_WAVE_FORMAT_NONE:
    case GSL_WAVE_FORMAT_LAST:
    default:
      g_return_val_if_fail (format >= GSL_WAVE_FORMAT_UNSIGNED_8 && format <= GSL_WAVE_FORMAT_FLOAT, NULL);
      return NULL;
    }
}

GslWaveFormatType
gsl_wave_format_from_string (const gchar *string)
{
  gboolean is_unsigned = FALSE;

  g_return_val_if_fail (string != NULL, GSL_WAVE_FORMAT_NONE);

  while (*string == ' ')
    string++;
  if (strncasecmp (string, "float", 5) == 0)
    return GSL_WAVE_FORMAT_FLOAT;
  if ((string[0] == 'u' || string[0] == 'U') &&
      (string[1] == 'n' || string[1] == 'N'))
    {
      is_unsigned = TRUE;
      string += 2;
    }
  if (strncasecmp (string, "signed", 6) != 0)
    return GSL_WAVE_FORMAT_NONE;
  string += 6;
  if (string[0] != '-' && string[0] != '_')
    return GSL_WAVE_FORMAT_NONE;
  string += 1;
  if (string[0] == '8')
    return is_unsigned ? GSL_WAVE_FORMAT_UNSIGNED_8 : GSL_WAVE_FORMAT_SIGNED_8;
  if (string[0] != '1')
    return GSL_WAVE_FORMAT_NONE;
  string += 1;
  if (string[0] == '2')
    return is_unsigned ? GSL_WAVE_FORMAT_UNSIGNED_12 : GSL_WAVE_FORMAT_SIGNED_12;
  if (string[0] == '6')
    return is_unsigned ? GSL_WAVE_FORMAT_UNSIGNED_16 : GSL_WAVE_FORMAT_SIGNED_16;
  return GSL_WAVE_FORMAT_NONE;
}
