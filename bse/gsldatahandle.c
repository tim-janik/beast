/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik
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
  guint             n_channels;
  guint             bit_depth;
  gfloat            mix_freq;
  gfloat            osc_freq;
  GslLong           n_values;
  const gfloat     *values;
  void            (*free_values) (gpointer);
} MemHandle;
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
  GslLong	    tail_cut;
} CutHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  GslLong	    requested_paste_offset;
  GslLong	    paste_offset;
  GslLong	    n_paste_values;
  guint		    paste_bit_depth;
  const gfloat	   *paste_values;
  void            (*free_values) (gpointer);
} InsertHandle;
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  GslLong	    requested_first;
  GslLong	    requested_last;
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
  GslHFile	   *hfile;
  GslLong	    byte_offset;
  guint             byte_order;
  guint		    n_channels;
  gfloat            mix_freq;
  gfloat            osc_freq;
  GslWaveFormatType format;
  guint		    add_zoffset : 1;
  GslLong	    requested_offset;
  GslLong	    requested_length;
} WaveHandle;


/* --- standard functions --- */
gboolean
gsl_data_handle_common_init (GslDataHandle *dhandle,
			     const gchar   *file_name)
{
  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (dhandle->vtable == NULL, FALSE);
  g_return_val_if_fail (dhandle->name == NULL, FALSE);
  g_return_val_if_fail (dhandle->ref_count == 0, FALSE);
  
  dhandle->name = g_strdup (file_name);
  sfi_mutex_init (&dhandle->mutex);
  dhandle->ref_count = 1;
  dhandle->open_count = 0;
  g_datalist_init (&dhandle->qdata);
  memset (&dhandle->setup, 0, sizeof (dhandle->setup));
  
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
  
  g_datalist_clear (&dhandle->qdata);
  g_free (dhandle->name);
  dhandle->name = NULL;
  sfi_mutex_destroy (&dhandle->mutex);
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
    {
      g_return_if_fail (dhandle->open_count == 0);
      dhandle->vtable->destroy (dhandle);
    }
}

void
gsl_data_handle_override (GslDataHandle    *dhandle,
                          gint              bit_depth,
                          gfloat            mix_freq,
                          gfloat            osc_freq)
{
  gfloat *fp;

  g_return_if_fail (dhandle != NULL);

  if (bit_depth <= 0)
    g_datalist_set_data (&dhandle->qdata, "bse-bit-depth", NULL);
  else
    g_datalist_set_data (&dhandle->qdata, "bse-bit-depth", GINT_TO_POINTER (MIN (bit_depth, 32)));

  if (mix_freq <= 0)
    g_datalist_set_data (&dhandle->qdata, "bse-mix-freq", NULL);
  else
    {
      fp = g_new (gfloat, 1);
      *fp = mix_freq;
      g_datalist_set_data_full (&dhandle->qdata, "bse-mix-freq", fp, g_free);
    }

  if (osc_freq <= 0)
    g_datalist_set_data (&dhandle->qdata, "bse-osc-freq", NULL);
  else
    {
      fp = g_new (gfloat, 1);
      *fp = osc_freq;
      g_datalist_set_data_full (&dhandle->qdata, "bse-osc-freq", fp, g_free);
    }
}

GslErrorType
gsl_data_handle_open (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, GSL_ERROR_INTERNAL);
  g_return_val_if_fail (dhandle->ref_count > 0, GSL_ERROR_INTERNAL);

  GSL_SPIN_LOCK (&dhandle->mutex);
  if (dhandle->open_count == 0)
    {
      GslDataHandleSetup setup = { 0, };
      GslErrorType error;
      gfloat *fp;
      gint i;

      error = dhandle->vtable->open (dhandle, &setup);
      if (!error && (setup.n_values < 0 ||
		     setup.n_channels < 1 ||
		     setup.bit_depth < 1 ||
                     setup.mix_freq < 3999 ||
                     setup.osc_freq <= 0))
	{
	  g_warning ("internal error in data handle open() (%p): nv=%ld nc=%u bd=%u mf=%g of=%g",
		     dhandle->vtable->open, setup.n_values, setup.n_channels,
                     setup.bit_depth, setup.mix_freq, setup.osc_freq);
	  dhandle->vtable->close (dhandle);
	  error = GSL_ERROR_INTERNAL;
	}
      if (error)
	{
	  GSL_SPIN_UNLOCK (&dhandle->mutex);
	  return error;
	}
      dhandle->ref_count++;
      dhandle->open_count++;
      dhandle->setup = setup;
      i = GPOINTER_TO_INT (g_datalist_get_data (&dhandle->qdata, "bse-bit-depth"));
      if (i > 0)
        dhandle->setup.bit_depth = i;
      fp = g_datalist_get_data (&dhandle->qdata, "bse-mix-freq");
      if (fp)
        dhandle->setup.mix_freq = *fp;
      fp = g_datalist_get_data (&dhandle->qdata, "bse-osc-freq");
      if (fp)
        dhandle->setup.osc_freq = *fp;
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
    {
      dhandle->vtable->close (dhandle);
      memset (&dhandle->setup, 0, sizeof (dhandle->setup));
    }
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
  g_return_val_if_fail (dhandle->open_count > 0, -1);
  g_return_val_if_fail (value_offset >= 0, -1);
  if (n_values < 1)
    return 0;
  g_return_val_if_fail (values != NULL, -1);
  g_return_val_if_fail (value_offset < dhandle->setup.n_values, -1);
  
  n_values = MIN (n_values, dhandle->setup.n_values - value_offset);
  GSL_SPIN_LOCK (&dhandle->mutex);
  l = dhandle->vtable->read (dhandle, value_offset, n_values, values);
  GSL_SPIN_UNLOCK (&dhandle->mutex);
  
  return l;
}

GslLong
gsl_data_handle_length (GslDataHandle *dhandle)
{
  GslLong l;

  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);

  GSL_SPIN_LOCK (&dhandle->mutex);
  l = dhandle->open_count ? dhandle->setup.n_values : 0;
  GSL_SPIN_UNLOCK (&dhandle->mutex);

  return l;
}

guint
gsl_data_handle_n_channels (GslDataHandle *dhandle)
{
  guint n;

  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);

  GSL_SPIN_LOCK (&dhandle->mutex);
  n = dhandle->open_count ? dhandle->setup.n_channels : 0;
  GSL_SPIN_UNLOCK (&dhandle->mutex);

  return n;
}

guint
gsl_data_handle_bit_depth (GslDataHandle *dhandle)
{
  guint n;

  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);

  GSL_SPIN_LOCK (&dhandle->mutex);
  n = dhandle->open_count ? dhandle->setup.bit_depth : 0;
  GSL_SPIN_UNLOCK (&dhandle->mutex);

  return n;
}

gfloat
gsl_data_handle_mix_freq (GslDataHandle *dhandle)
{
  gfloat f;

  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);

  GSL_SPIN_LOCK (&dhandle->mutex);
  f = dhandle->open_count ? dhandle->setup.mix_freq : 0;
  GSL_SPIN_UNLOCK (&dhandle->mutex);

  return f;
}

gfloat
gsl_data_handle_osc_freq (GslDataHandle *dhandle)
{
  gfloat f;

  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);

  GSL_SPIN_LOCK (&dhandle->mutex);
  f = dhandle->open_count ? dhandle->setup.osc_freq : 0;
  GSL_SPIN_UNLOCK (&dhandle->mutex);

  return f;
}

const gchar*
gsl_data_handle_name (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, NULL);

  return dhandle->name;
}

typedef struct {
  GslDataHandleRecurse ufunc;
  gpointer             udata;
} RecurseData;

static void
data_handle_recurse_func (GslDataHandle *dhandle,
			  gpointer       data)
{
  RecurseData *rdata = data;

  GSL_SPIN_LOCK (&dhandle->mutex);
  rdata->ufunc (dhandle, rdata->udata);
  if (dhandle->vtable->recurse)
    dhandle->vtable->recurse (dhandle, data_handle_recurse_func, data);
  GSL_SPIN_UNLOCK (&dhandle->mutex);
}

static void
data_handle_recurse (GslDataHandle       *dhandle,
		     GslDataHandleRecurse ufunc,
		     gpointer             udata)
{
  RecurseData rdata;
  rdata.ufunc = ufunc;
  rdata.udata = udata;
  data_handle_recurse_func (dhandle, &rdata);
}

static void
query_needs_cache (GslDataHandle *dhandle,
		   gpointer       data)
{
  gboolean *needs_cache = data;

  if (dhandle->vtable->ojob)
    {
      gboolean implemented, dhandle_needs_cache = FALSE;
      implemented = dhandle->vtable->ojob (dhandle, GSL_DATA_HANDLE_NEEDS_CACHE, &dhandle_needs_cache);
      if (implemented)
	*needs_cache |= dhandle_needs_cache;
    }
}

gboolean
gsl_data_handle_needs_cache (GslDataHandle *dhandle)
{
  gboolean needs_cache = FALSE;

  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (dhandle->ref_count > 0, FALSE);

  data_handle_recurse (dhandle, query_needs_cache, &needs_cache);

  return needs_cache;
}


/* --- const memory handle --- */
static GslErrorType
mem_handle_open (GslDataHandle      *dhandle,
		 GslDataHandleSetup *setup)
{
  MemHandle *mhandle = (MemHandle*) dhandle;

  setup->n_values = mhandle->n_values;
  setup->n_channels = mhandle->n_channels;
  setup->bit_depth = mhandle->bit_depth;
  setup->mix_freq = mhandle->mix_freq;
  setup->osc_freq = mhandle->osc_freq;

  return GSL_ERROR_NONE;
}

static void
mem_handle_close (GslDataHandle *dhandle)
{
  // MemHandle *mhandle = (MemHandle*) dhandle;
}

static void
mem_handle_destroy (GslDataHandle *dhandle)
{
  MemHandle *mhandle = (MemHandle*) dhandle;
  void (*free_values) (gpointer) = mhandle->free_values;
  const gfloat *mem_values = mhandle->values;

  gsl_data_handle_common_free (dhandle);
  mhandle->values = NULL;
  mhandle->free_values = NULL;
  sfi_delete_struct (MemHandle, mhandle);
  
  if (free_values)
    free_values ((gpointer) mem_values);
}

static GslLong
mem_handle_read (GslDataHandle *dhandle,
		 GslLong        voffset,
		 GslLong        n_values,
		 gfloat        *values)
{
  MemHandle *mhandle = (MemHandle*) dhandle;

  g_return_val_if_fail (voffset + n_values <= mhandle->n_values, -1);

  memcpy (values, mhandle->values + voffset, n_values * sizeof (values[0]));

  return n_values;
}

GslDataHandle*
gsl_data_handle_new_mem (guint         n_channels,
			 guint         bit_depth,
                         gfloat        mix_freq,
                         gfloat        osc_freq,
			 GslLong       n_values,
			 const gfloat *values,
			 void        (*free) (gpointer values))
{
  static GslDataHandleFuncs mem_handle_vtable = {
    mem_handle_open,
    mem_handle_read,
    mem_handle_close,
    NULL,
    mem_handle_destroy,
  };
  MemHandle *mhandle;
  gboolean success;

  g_return_val_if_fail (n_channels > 0, NULL);
  g_return_val_if_fail (bit_depth > 0, NULL);
  g_return_val_if_fail (mix_freq >= 4000, NULL);
  g_return_val_if_fail (osc_freq > 0, NULL);
  g_return_val_if_fail (n_values >= n_channels, NULL);
  if (n_values)
    g_return_val_if_fail (values != NULL, NULL);
  
  mhandle = sfi_new_struct0 (MemHandle, 1);
  success = gsl_data_handle_common_init (&mhandle->dhandle, NULL);
  if (success)
    {
      mhandle->dhandle.name = g_strconcat ("// #memory /", NULL);
      mhandle->dhandle.vtable = &mem_handle_vtable;
      mhandle->n_channels = n_channels;
      mhandle->bit_depth = bit_depth;
      mhandle->mix_freq = mix_freq;
      mhandle->osc_freq = osc_freq;
      mhandle->n_values = n_values / mhandle->n_channels;
      mhandle->n_values *= mhandle->n_channels;
      mhandle->values = values;
      mhandle->free_values = free;
    }
  else
    {
      sfi_delete_struct (MemHandle, mhandle);
      return NULL;
    }
  return &mhandle->dhandle;
}


/* --- chain handle --- */
static GslErrorType
chain_handle_open (GslDataHandle      *dhandle,
		   GslDataHandleSetup *setup)
{
  ChainHandle *chandle = (ChainHandle*) dhandle;
  GslErrorType error;

  error = gsl_data_handle_open (chandle->src_handle);
  if (error != GSL_ERROR_NONE)
    return error;
  *setup = chandle->src_handle->setup;

  return GSL_ERROR_NONE;
}

static void
chain_handle_recurse (GslDataHandle       *dhandle,
		      GslDataHandleRecurse recurse,
		      gpointer		   data)
{
  ChainHandle *chandle = (ChainHandle*) dhandle;

  recurse (chandle->src_handle, data);
}

static void
chain_handle_close (GslDataHandle *dhandle)
{
  ChainHandle *chandle = (ChainHandle*) dhandle;
  
  gsl_data_handle_close (chandle->src_handle);
}


/* --- reversed handle --- */
static void
reverse_handle_destroy (GslDataHandle *data_handle)
{
  ReversedHandle *rhandle = (ReversedHandle*) data_handle;
  
  gsl_data_handle_unref (rhandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  sfi_delete_struct (ReversedHandle, rhandle);
}

static GslLong
reverse_handle_read (GslDataHandle *dhandle,
		     GslLong        voffset,
		     GslLong        n_values,
		     gfloat        *values)
{
  ReversedHandle *rhandle = (ReversedHandle*) dhandle;
  GslLong left, new_offset = dhandle->setup.n_values - (voffset + n_values);
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
    chain_handle_recurse,
    reverse_handle_destroy,
  };
  ReversedHandle *rhandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  
  rhandle = sfi_new_struct0 (ReversedHandle, 1);
  success = gsl_data_handle_common_init (&rhandle->dhandle, NULL);
  if (success)
    {
      rhandle->dhandle.name = g_strconcat (src_handle->name, "// #reversed /", NULL);
      rhandle->dhandle.vtable = &reverse_handle_vtable;
      rhandle->src_handle = gsl_data_handle_ref (src_handle);
    }
  else
    {
      sfi_delete_struct (ReversedHandle, rhandle);
      return NULL;
    }
  return &rhandle->dhandle;
}


/* --- cut handle --- */
static GslErrorType
cut_handle_open (GslDataHandle      *dhandle,
		 GslDataHandleSetup *setup)
{
  CutHandle *chandle = (CutHandle*) dhandle;
  GslErrorType error;

  error = gsl_data_handle_open (chandle->src_handle);
  if (error != GSL_ERROR_NONE)
    return error;
  *setup = chandle->src_handle->setup;
  setup->n_values -= MIN (setup->n_values, chandle->tail_cut);
  setup->n_values -= MIN (setup->n_values, chandle->n_cut_values);

  return GSL_ERROR_NONE;
}

static void
cut_handle_destroy (GslDataHandle *data_handle)
{
  CutHandle *chandle = (CutHandle*) data_handle;
  
  gsl_data_handle_unref (chandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  sfi_delete_struct (CutHandle, chandle);
}

static GslLong
cut_handle_read (GslDataHandle *dhandle,
		 GslLong        voffset,
		 GslLong        n_values,
		 gfloat        *values)
{
  CutHandle *chandle = (CutHandle*) dhandle;
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
  
  if (voffset >= chandle->cut_offset && n_values)
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
    cut_handle_open,
    cut_handle_read,
    chain_handle_close,
    chain_handle_recurse,
    cut_handle_destroy,
  };
  CutHandle *chandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  g_return_val_if_fail (cut_offset >= 0 && n_cut_values >= 0 && tail_cut >= 0, NULL);
  
  chandle = sfi_new_struct0 (CutHandle, 1);
  success = gsl_data_handle_common_init (&chandle->dhandle, NULL);
  if (success)
    {
      chandle->dhandle.name = g_strconcat (src_handle->name, "// #translate /", NULL);
      chandle->dhandle.vtable = &cut_handle_vtable;
      chandle->src_handle = gsl_data_handle_ref (src_handle);
      chandle->cut_offset = n_cut_values ? cut_offset : 0;
      chandle->n_cut_values = n_cut_values;
      chandle->tail_cut = tail_cut;
    }
  else
    {
      sfi_delete_struct (CutHandle, chandle);
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
static GslErrorType
insert_handle_open (GslDataHandle      *dhandle,
		    GslDataHandleSetup *setup)
{
  InsertHandle *ihandle = (InsertHandle*) dhandle;
  GslErrorType error;

  error = gsl_data_handle_open (ihandle->src_handle);
  if (error != GSL_ERROR_NONE)
    return error;
  *setup = ihandle->src_handle->setup;
  ihandle->paste_offset = ihandle->requested_paste_offset < 0 ? setup->n_values : ihandle->requested_paste_offset;
  if (setup->n_values < ihandle->paste_offset)
    setup->n_values = ihandle->paste_offset + ihandle->n_paste_values;
  else
    setup->n_values += ihandle->n_paste_values;
  setup->bit_depth = MAX (setup->bit_depth, ihandle->paste_bit_depth);

  return GSL_ERROR_NONE;
}

static void
insert_handle_destroy (GslDataHandle *data_handle)
{
  InsertHandle *ihandle = (InsertHandle*) data_handle;
  void (*free_values) (gpointer) = ihandle->free_values;
  const gfloat *paste_values = ihandle->paste_values;
  
  gsl_data_handle_unref (ihandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  ihandle->paste_values = NULL;
  ihandle->free_values = NULL;
  sfi_delete_struct (InsertHandle, ihandle);
  
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
  
  if (voffset < ihandle->src_handle->setup.n_values &&
      voffset < ihandle->paste_offset)
    {
      l = MIN (n_values, MIN (ihandle->paste_offset, ihandle->src_handle->setup.n_values) - voffset);
      l = gsl_data_handle_read (ihandle->src_handle, voffset, l, values);
      if (l < 0)
	return l;       /* pass on errors */
      
      voffset += l;
      n_values -= l;
      values += l;
    }

  if (n_values && voffset >= ihandle->src_handle->setup.n_values && voffset < ihandle->paste_offset)
    {
      l = MIN (n_values, ihandle->paste_offset - voffset);
      memset (values, 0, l * sizeof (values[0]));
      voffset += l;
      n_values -= l;
      values += l;
    }

  if (n_values && voffset >= ihandle->paste_offset && voffset < ihandle->paste_offset + ihandle->n_paste_values)
    {
      l = MIN (n_values, ihandle->paste_offset + ihandle->n_paste_values - voffset);
      memcpy (values, ihandle->paste_values + voffset - ihandle->paste_offset, l * sizeof (values[0]));
      voffset += l;
      n_values -= l;
      values += l;
    }
  
  if (n_values && voffset >= ihandle->paste_offset + ihandle->n_paste_values)
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
    insert_handle_open,
    insert_handle_read,
    chain_handle_close,
    chain_handle_recurse,
    insert_handle_destroy,
  };
  InsertHandle *ihandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  g_return_val_if_fail (n_paste_values >= 0, NULL);
  if (n_paste_values)
    g_return_val_if_fail (paste_values != NULL, NULL);
  
  ihandle = sfi_new_struct0 (InsertHandle, 1);
  success = gsl_data_handle_common_init (&ihandle->dhandle, NULL);
  if (success)
    {
      ihandle->dhandle.name = g_strconcat (src_handle ? src_handle->name : "", "// #insert /", NULL);
      ihandle->dhandle.vtable = &insert_handle_vtable;
      ihandle->src_handle = gsl_data_handle_ref (src_handle);
      ihandle->requested_paste_offset = insertion_offset;
      ihandle->paste_offset = 0;
      ihandle->n_paste_values = n_paste_values;
      ihandle->paste_bit_depth = paste_bit_depth;
      ihandle->paste_values = paste_values;
      ihandle->free_values = free;
    }
  else
    {
      sfi_delete_struct (InsertHandle, ihandle);
      return NULL;
    }
  return &ihandle->dhandle;
}


/* --- loop handle --- */
static GslErrorType
loop_handle_open (GslDataHandle      *dhandle,
		  GslDataHandleSetup *setup)
{
  LoopHandle *lhandle = (LoopHandle*) dhandle;
  GslErrorType error;

  error = gsl_data_handle_open (lhandle->src_handle);
  if (error != GSL_ERROR_NONE)
    return error;

  *setup = lhandle->src_handle->setup;
  if (setup->n_values > lhandle->requested_last)
    {
      lhandle->loop_start = lhandle->requested_first;
      lhandle->loop_width = lhandle->requested_last - lhandle->requested_first + 1;
      setup->n_values = GSL_MAXLONG;
    }
  else	/* cannot loop */
    {
      lhandle->loop_start = setup->n_values;
      lhandle->loop_width = 0;
    }

  return GSL_ERROR_NONE;
}

static void
loop_handle_destroy (GslDataHandle *data_handle)
{
  LoopHandle *lhandle = (LoopHandle*) data_handle;
  
  gsl_data_handle_unref (lhandle->src_handle);
  
  gsl_data_handle_common_free (data_handle);
  sfi_delete_struct (LoopHandle, lhandle);
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
			    GslLong        loop_first,
			    GslLong        loop_last)
{
  static GslDataHandleFuncs loop_handle_vtable = {
    loop_handle_open,
    loop_handle_read,
    chain_handle_close,
    chain_handle_recurse,
    loop_handle_destroy,
  };
  LoopHandle *lhandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  g_return_val_if_fail (loop_first >= 0, NULL);
  g_return_val_if_fail (loop_last >= loop_first, NULL);
  
  lhandle = sfi_new_struct0 (LoopHandle, 1);
  success = gsl_data_handle_common_init (&lhandle->dhandle, NULL);
  if (success)
    {
      lhandle->dhandle.name = g_strdup_printf ("%s// #loop(0x%lx:0x%lx) /", src_handle->name, loop_first, loop_last);
      lhandle->dhandle.vtable = &loop_handle_vtable;
      lhandle->src_handle = gsl_data_handle_ref (src_handle);
      lhandle->requested_first = loop_first;
      lhandle->requested_last = loop_last;
      lhandle->loop_start = 0;
      lhandle->loop_width = 0;
    }
  else
    {
      sfi_delete_struct (LoopHandle, lhandle);
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
  sfi_delete_struct (DCacheHandle, dhandle);
}

static GslErrorType
dcache_handle_open (GslDataHandle      *data_handle,
		    GslDataHandleSetup *setup)
{
  DCacheHandle *dhandle = (DCacheHandle*) dhandle;
  GslErrorType error;

  error = gsl_data_handle_open (dhandle->dcache->dhandle);
  if (error != GSL_ERROR_NONE)
    return error;
  gsl_data_cache_open (dhandle->dcache);
  *setup = dhandle->dcache->dhandle->setup;
  gsl_data_handle_close (dhandle->dcache->dhandle);

  return GSL_ERROR_NONE;
}

static void
dcache_handle_recurse (GslDataHandle       *data_handle,
		       GslDataHandleRecurse recurse,
		       gpointer             data)
{
  DCacheHandle *dhandle = (DCacheHandle*) data_handle;

  recurse (dhandle->dcache->dhandle, data);
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
    dcache_handle_recurse,
    dcache_handle_destroy,
  };
  DCacheHandle *dhandle;
  gboolean success;
  
  g_return_val_if_fail (dcache != NULL, NULL);
  
  dhandle = sfi_new_struct0 (DCacheHandle, 1);
  success = gsl_data_handle_common_init (&dhandle->dhandle, NULL);
  if (success)
    {
      dhandle->dhandle.name = g_strdup_printf ("%s// #dcache /", dcache->dhandle->name);
      dhandle->dhandle.vtable = &dcache_handle_vtable;
      dhandle->dcache = gsl_data_cache_ref (dcache);
      dhandle->node_size = GSL_DATA_CACHE_NODE_SIZE (dcache) + dcache->padding;
    }
  else
    {
      sfi_delete_struct (DCacheHandle, dhandle);
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

guint
gsl_wave_format_bit_depth (GslWaveFormatType format)
{
  return wave_format_bit_depth (format);
}

guint
gsl_wave_format_byte_width (GslWaveFormatType format)
{
  return wave_format_byte_width (format);
}

static void
wave_handle_destroy (GslDataHandle *data_handle)
{
  WaveHandle *whandle = (WaveHandle*) data_handle;

  gsl_data_handle_common_free (data_handle);
  sfi_delete_struct (WaveHandle, whandle);
}

static GslErrorType
wave_handle_open (GslDataHandle      *data_handle,
		  GslDataHandleSetup *setup)
{
  WaveHandle *whandle = (WaveHandle*) data_handle;

  whandle->hfile = gsl_hfile_open (whandle->dhandle.name);
  if (!whandle->hfile)
    return gsl_error_from_errno (errno, GSL_ERROR_OPEN_FAILED);
  else
    {
      GslLong l, fwidth = wave_format_byte_width (whandle->format);
      whandle->byte_offset = whandle->requested_offset;
      if (whandle->add_zoffset)
	{
	  GslLong zoffset = gsl_hfile_zoffset (whandle->hfile);
	  if (zoffset >= 0)
	    whandle->byte_offset += zoffset + 1;
	}
      /* convert size into n_values, i.e. float length */
      l = whandle->hfile->n_bytes;
      l -= MIN (l, whandle->byte_offset);
      if (l >= fwidth)
	{
	  l /= fwidth;
	  if (whandle->requested_length < 0)
	    setup->n_values = l;
	  else
	    setup->n_values = MIN (l, whandle->requested_length);
	}
      else
	setup->n_values = 0;
      setup->n_channels = whandle->n_channels;
      setup->bit_depth = wave_format_bit_depth (whandle->format);
      setup->mix_freq = whandle->mix_freq;
      setup->osc_freq = whandle->osc_freq;
      return GSL_ERROR_NONE;
    }
}

static void
wave_handle_close (GslDataHandle *dhandle)
{
  WaveHandle *whandle = (WaveHandle*) dhandle;
  
  gsl_hfile_close (whandle->hfile);
  whandle->hfile = NULL;
}

static gboolean
wave_handle_ojob (GslDataHandle    *dhandle,
		  GslDataHandleOJob ojob,
		  gpointer          data)
{
  switch (ojob)
    {
      gboolean *needs_cache;
    case GSL_DATA_HANDLE_NEEDS_CACHE:
      needs_cache = data;
      *needs_cache = TRUE;
      return TRUE;	/* case implemented */
    default:
      return FALSE;	/* unimplemented cases */
    }
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
  byte_offset += whandle->byte_offset;

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

GslDataHandle*
gsl_wave_handle_new (const gchar      *file_name,
		     guint             n_channels,
		     GslWaveFormatType format,
		     guint             byte_order,
                     gfloat            mix_freq,
                     gfloat            osc_freq,
		     GslLong           byte_offset,
		     GslLong           n_values)
{
  static GslDataHandleFuncs wave_handle_vtable = {
    wave_handle_open,
    wave_handle_read,
    wave_handle_close,
    NULL,
    wave_handle_destroy,
    wave_handle_ojob,
  };
  WaveHandle *whandle;

  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (format > GSL_WAVE_FORMAT_NONE && format < GSL_WAVE_FORMAT_LAST, NULL);
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);
  g_return_val_if_fail (mix_freq >= 4000, NULL);
  g_return_val_if_fail (osc_freq > 0, NULL);
  g_return_val_if_fail (byte_offset >= 0, NULL);
  g_return_val_if_fail (n_channels >= 1, NULL);
  g_return_val_if_fail (n_values >= 1 || n_values == -1, NULL);

  whandle = sfi_new_struct0 (WaveHandle, 1);
  if (gsl_data_handle_common_init (&whandle->dhandle, file_name))
    {
      whandle->dhandle.vtable = &wave_handle_vtable;
      whandle->n_channels = n_channels;
      whandle->format = format;
      whandle->byte_order = byte_order;
      whandle->mix_freq = mix_freq;
      whandle->osc_freq = osc_freq;
      whandle->requested_offset = byte_offset;
      whandle->requested_length = n_values;
      whandle->hfile = NULL;
      return &whandle->dhandle;
    }
  else
    {
      sfi_delete_struct (WaveHandle, whandle);
      return NULL;
    }
}

GslDataHandle*
gsl_wave_handle_new_zoffset (const gchar      *file_name,
			     guint             n_channels,
			     GslWaveFormatType format,
			     guint             byte_order,
                             gfloat            mix_freq,
                             gfloat            osc_freq,
                             GslLong           byte_offset,
			     GslLong           byte_size)
{
  GslDataHandle *dhandle = gsl_wave_handle_new (file_name, n_channels, format,
						byte_order, mix_freq, osc_freq, byte_offset,
						byte_size / wave_format_byte_width (format));
  if (dhandle)
    ((WaveHandle*) dhandle)->add_zoffset = TRUE;
  return dhandle;
}

const gchar*
gsl_wave_format_to_string (GslWaveFormatType format)
{
  switch (format)
    {
    case GSL_WAVE_FORMAT_UNSIGNED_8:    return "unsigned-8";
    case GSL_WAVE_FORMAT_SIGNED_8:      return "signed-8";
    case GSL_WAVE_FORMAT_UNSIGNED_12:   return "unsigned-12";
    case GSL_WAVE_FORMAT_SIGNED_12:     return "signed-12";
    case GSL_WAVE_FORMAT_UNSIGNED_16:   return "unsigned-16";
    case GSL_WAVE_FORMAT_SIGNED_16:     return "signed-16";
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
