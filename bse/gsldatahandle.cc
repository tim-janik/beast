// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gsldatahandle.hh"
#include "gslcommon.hh"
#include "gsldatacache.hh"
#include "gsldatautils.hh"
#include "gslfilehash.hh"
#include <string.h>
#include <errno.h>
/* --- typedefs --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle; /* structure layout mirrored by various structs */
} ChainHandle;
typedef ChainHandle ReversedHandle;
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
  new (&dhandle->spinlock) Bse::Spinlock();
  dhandle->ref_count = 1;
  dhandle->open_count = 0;
  memset (&dhandle->setup, 0, sizeof (dhandle->setup));
  return TRUE;
}
GslDataHandle*
gsl_data_handle_ref (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, NULL);
  g_return_val_if_fail (dhandle->ref_count > 0, NULL);
  dhandle->spinlock.lock();
  dhandle->ref_count++;
  dhandle->spinlock.unlock();
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
  dhandle->spinlock.~Spinlock();
}
void
gsl_data_handle_unref (GslDataHandle *dhandle)
{
  gboolean destroy;
  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (dhandle->ref_count > 0);
  dhandle->spinlock.lock();
  dhandle->ref_count--;
  destroy = dhandle->ref_count == 0;
  dhandle->spinlock.unlock();
  if (destroy)
    {
      g_return_if_fail (dhandle->open_count == 0);
      dhandle->vtable->destroy (dhandle);
    }
}
BseErrorType
gsl_data_handle_open (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (dhandle->ref_count > 0, BSE_ERROR_INTERNAL);
  dhandle->spinlock.lock();
  if (dhandle->open_count == 0)
    {
      GslDataHandleSetup setup = { 0, };
      BseErrorType error = dhandle->vtable->open (dhandle, &setup);
      if (!error && (setup.n_values < 0 ||
		     setup.n_channels < 1))
	{
	  sfi_warning ("invalid parameters in data handle open() (%p()): nv=%lld nc=%u",
                       dhandle->vtable->open, setup.n_values, setup.n_channels);
	  dhandle->vtable->close (dhandle);
	  error = BSE_ERROR_FORMAT_INVALID;
	}
      if (error)
	{
	  dhandle->spinlock.unlock();
          if (setup.xinfos)
            g_warning ("%s: leaking xinfos after open() (%p)", "GslDataHandle", dhandle->vtable->open);
	  return error;
	}
      dhandle->ref_count++;
      dhandle->open_count++;
      dhandle->setup = setup;
    }
  else
    dhandle->open_count++;
  dhandle->spinlock.unlock();
  return BSE_ERROR_NONE;
}
void
gsl_data_handle_close (GslDataHandle *dhandle)
{
  gboolean need_unref;
  g_return_if_fail (dhandle != NULL);
  g_return_if_fail (dhandle->ref_count > 0);
  g_return_if_fail (dhandle->open_count > 0);
  dhandle->spinlock.lock();
  dhandle->open_count--;
  need_unref = !dhandle->open_count;
  if (!dhandle->open_count)
    {
      dhandle->vtable->close (dhandle);
      if (dhandle->setup.xinfos)
        g_warning ("%s: leaking xinfos after close() (%p)", "GslDataHandle", dhandle->vtable->close);
      memset (&dhandle->setup, 0, sizeof (dhandle->setup));
    }
  dhandle->spinlock.unlock();
  if (need_unref)
    gsl_data_handle_unref (dhandle);
}
int64
gsl_data_handle_read (GslDataHandle *dhandle,
		      int64          value_offset,
		      int64          n_values,
		      gfloat        *values)
{
  int64 l;
  g_return_val_if_fail (dhandle != NULL, -1);
  g_return_val_if_fail (dhandle->open_count > 0, -1);
  g_return_val_if_fail (value_offset >= 0, -1);
  if (n_values < 1)
    return 0;
  g_return_val_if_fail (values != NULL, -1);
  g_return_val_if_fail (value_offset < dhandle->setup.n_values, -1);
  n_values = MIN (n_values, dhandle->setup.n_values - value_offset);
  dhandle->spinlock.lock();
  l = dhandle->vtable->read (dhandle, value_offset, n_values, values);
  dhandle->spinlock.unlock();
  return l;
}
GslDataHandle*
gsl_data_handle_get_source (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, NULL);
  dhandle->spinlock.lock();
  GslDataHandle *src_handle = dhandle->vtable->get_source ? dhandle->vtable->get_source (dhandle) : NULL;
  dhandle->spinlock.unlock();
  return src_handle;
}
/**
 * @param data_handle	a DataHandle
 * @return		the state length of the data handle
 *
 * Most data handles produce output samples from an input data handle.
 * Some of them, like filtering and resampling datahandles, have an internal
 * state which means that the value of one input sample affects not only one
 * output sample, but some samples before and/or some samples after the
 * "corresponding" output sample.
 *
 * Often the state is symmetric, so that the number of output samples affected
 * before and after the "corresponding" output sample is the same. Then the
 * function returns this number. If the state is asymmetric, this function
 * shall return the maximum of the two numbers.
 *
 * If multiple data handles are nested (for instance when resampling a
 * filtered signal), the function propagates the state length, so that the
 * accumulated state length of all operations together is returned.
 *
 * Note: This function can only be used while the data handle is opened.
 *
 * This function is MT-safe and may be called from any thread.
 */
int64
gsl_data_handle_get_state_length (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, -1);
  g_return_val_if_fail (dhandle->open_count > 0, -1);
  dhandle->spinlock.lock();
  int64 state_length = dhandle->vtable->get_state_length ? dhandle->vtable->get_state_length (dhandle) : 0;
  dhandle->spinlock.unlock();
  return state_length;
}
int64
gsl_data_handle_length (GslDataHandle *dhandle)
{
  int64 l;
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  dhandle->spinlock.lock();
  l = dhandle->open_count ? dhandle->setup.n_values : 0;
  dhandle->spinlock.unlock();
  return l;
}
guint
gsl_data_handle_n_channels (GslDataHandle *dhandle)
{
  guint n;
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  dhandle->spinlock.lock();
  n = dhandle->open_count ? dhandle->setup.n_channels : 0;
  dhandle->spinlock.unlock();
  return n;
}
guint
gsl_data_handle_bit_depth (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  return dhandle->setup.bit_depth;
}
gfloat
gsl_data_handle_mix_freq (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  return dhandle->setup.mix_freq;
}
gfloat
gsl_data_handle_osc_freq (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  dhandle->spinlock.lock();
  gfloat f = bse_xinfos_get_float (dhandle->setup.xinfos, "osc-freq");
  dhandle->spinlock.unlock();
  return f;
}
gfloat
gsl_data_handle_volume (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  dhandle->spinlock.lock();
  gfloat f = bse_xinfos_get_float (dhandle->setup.xinfos, "volume");
  dhandle->spinlock.unlock();
  /* no (or invalid) volume setting means that we replay without scaling */
  if (f <= 0 || f > 1.0)
    f = 1.0;
  return f;
}
gfloat
gsl_data_handle_fine_tune (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (dhandle->open_count > 0, 0);
  dhandle->spinlock.lock();
  gfloat f = bse_xinfos_get_float (dhandle->setup.xinfos, "fine-tune");
  dhandle->spinlock.unlock();
  return f;
}
const gchar*
gsl_data_handle_name (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, NULL);
  return dhandle->name;
}
gboolean
gsl_data_handle_needs_cache (GslDataHandle *dhandle)
{
  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (dhandle->ref_count > 0, FALSE);
  g_return_val_if_fail (dhandle->open_count > 0, FALSE);
  return dhandle->setup.needs_cache;
}
/* --- const memory handle --- */
typedef struct {
  GslDataHandle     dhandle;
  guint             n_channels;
  int64             n_values;
  const gfloat     *values;
  void            (*free_values) (gpointer);
  gchar           **xinfos;
  gfloat            mix_freq;
  guint             bit_depth : 8;
} MemHandle;
static BseErrorType
mem_handle_open (GslDataHandle      *dhandle,
		 GslDataHandleSetup *setup)
{
  MemHandle *mhandle = (MemHandle*) dhandle;
  setup->n_values = mhandle->n_values;
  setup->n_channels = mhandle->n_channels;
  setup->xinfos = mhandle->xinfos;
  setup->mix_freq = mhandle->mix_freq;
  setup->bit_depth = mhandle->bit_depth;
  return BSE_ERROR_NONE;
}
static void
mem_handle_close (GslDataHandle *dhandle)
{
  // MemHandle *mhandle = (MemHandle*) dhandle;
  dhandle->setup.xinfos = NULL;
}
static void
mem_handle_destroy (GslDataHandle *dhandle)
{
  MemHandle *mhandle = (MemHandle*) dhandle;
  void (*free_values) (gpointer) = mhandle->free_values;
  const gfloat *mem_values = mhandle->values;
  g_strfreev (mhandle->xinfos);
  gsl_data_handle_common_free (dhandle);
  mhandle->values = NULL;
  mhandle->free_values = NULL;
  sfi_delete_struct (MemHandle, mhandle);
  if (free_values)
    free_values ((gpointer) mem_values);
}
static int64
mem_handle_read (GslDataHandle *dhandle,
		 int64          voffset,
		 int64          n_values,
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
			 int64         n_values,
			 const gfloat *values,
			 void        (*free) (gpointer values))
{
  static GslDataHandleFuncs mem_handle_vtable = {
    mem_handle_open,
    mem_handle_read,
    mem_handle_close,
    NULL,
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
      mhandle->n_values = n_values / mhandle->n_channels;
      mhandle->n_values *= mhandle->n_channels;
      mhandle->values = values;
      mhandle->free_values = free;
      mhandle->xinfos = bse_xinfos_add_float (mhandle->xinfos, "osc-freq", osc_freq);
      mhandle->mix_freq = mix_freq;
      mhandle->bit_depth = bit_depth;
    }
  else
    {
      sfi_delete_struct (MemHandle, mhandle);
      return NULL;
    }
  return &mhandle->dhandle;
}
/* --- xinfo handle --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle    *src_handle;
  SfiRing          *remove_xinfos; /* list of "xinfo=" stubs */
  SfiRing          *added_xinfos;  /* list of valid xinfos */
  guint             clear_xinfos : 1;
} XInfoHandle;
static SfiRing*
ring_remove_dups (SfiRing        *ring,
                  SfiCompareFunc  cmp,
                  gpointer        data,
                  GDestroyNotify  data_destroy)
{
  SfiRing *rcopy = sfi_ring_copy (ring);
  /* sort (stable, keeping order) */
  ring = sfi_ring_sort (ring, cmp, data);
  /* remove dups (preserves first element from dup list) */
  ring = sfi_ring_uniq_free_deep (ring, cmp, data, data_destroy);
  /* restore original order */
  ring = sfi_ring_reorder (ring, rcopy);
  sfi_ring_free (rcopy);
  return ring;
}
static BseErrorType
xinfo_handle_open (GslDataHandle      *dhandle,
                   GslDataHandleSetup *setup)
{
  XInfoHandle *chandle = (XInfoHandle*) dhandle;
  GslDataHandle *src_handle = chandle->src_handle;
  BseErrorType error = gsl_data_handle_open (src_handle);
  if (error != BSE_ERROR_NONE)
    return error;
  *setup = src_handle->setup;
  setup->xinfos = NULL;
  guint i;
  /* collect xinfos to copy over */
  SfiRing *sxinfos = NULL;
  if (!chandle->clear_xinfos && src_handle->setup.xinfos)
    {
      for (i = 0; src_handle->setup.xinfos[i]; i++)
        sxinfos = sfi_ring_append (sxinfos, (gchar*) src_handle->setup.xinfos[i]);
    }
  /* override by deleting xinfos */
  if (sxinfos)
    sxinfos = sfi_ring_concat (sfi_ring_copy (chandle->remove_xinfos), sxinfos);
  /* override by added xinfos */
  sxinfos = sfi_ring_concat (sfi_ring_copy (chandle->added_xinfos), sxinfos);
  /* remove dups (preserves first element from dup list) */
  sxinfos = ring_remove_dups (sxinfos, (SfiCompareFunc) bse_xinfo_stub_compare, NULL, NULL);
  /* copy over non-empty xinfos */
  if (sxinfos)
    {
      setup->xinfos = g_new (gchar*, sfi_ring_length (sxinfos) + 1);
      i = 0;
      while (sxinfos)
        {
          const char *xinfo = (const char*) sfi_ring_pop_head (&sxinfos);
          const char *e = strchr (xinfo, '=');
          if (e[1]) /* non-empty xinfo */
            setup->xinfos[i++] = g_strdup (xinfo);
        }
      setup->xinfos[i] = NULL;
    }
  return BSE_ERROR_NONE;
}
static int64 
xinfo_handle_read (GslDataHandle *dhandle,
                   int64          voffset,
                   int64          n_values,
                   gfloat        *values)
{
  XInfoHandle *chandle = (XInfoHandle*) dhandle;
  return gsl_data_handle_read (chandle->src_handle, voffset, n_values, values);
}
static void
xinfo_handle_close (GslDataHandle *dhandle)
{
  XInfoHandle *chandle = (XInfoHandle*) dhandle;
  g_strfreev (dhandle->setup.xinfos);
  dhandle->setup.xinfos = NULL;
  gsl_data_handle_close (chandle->src_handle);
}
static void
xinfo_handle_destroy (GslDataHandle *dhandle)
{
  XInfoHandle *chandle = (XInfoHandle*) dhandle;
  sfi_ring_free_deep (chandle->remove_xinfos, g_free);
  sfi_ring_free_deep (chandle->added_xinfos, g_free);
  gsl_data_handle_unref (chandle->src_handle);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (XInfoHandle, chandle);
}
static GslDataHandle*
xinfo_get_source_handle (GslDataHandle *dhandle)
{
  XInfoHandle *chandle = (XInfoHandle*) dhandle;
  return chandle->src_handle;
}
static int64
xinfo_get_state_length (GslDataHandle *dhandle)
{
  XInfoHandle *chandle = (XInfoHandle*) dhandle;
  return gsl_data_handle_get_state_length (chandle->src_handle);
}
static GslDataHandle*
xinfo_data_handle_new (GslDataHandle *src_handle,
                       gboolean       clear_xinfos,
                       SfiRing       *remove_xinfos,
                       SfiRing       *added_xinfos)
{
  static GslDataHandleFuncs xinfo_handle_vtable = {
    xinfo_handle_open,
    xinfo_handle_read,
    xinfo_handle_close,
    xinfo_get_source_handle,
    xinfo_get_state_length,
    xinfo_handle_destroy,
  };
  SfiRing *dest_added = NULL, *dest_remove = NULL;
  gboolean dest_clear_xinfos = FALSE;
  /* if src_handle is a xinfo handle, collapse handle chain */
  if (src_handle->vtable == &xinfo_handle_vtable)
    {
      XInfoHandle *src_chandle = (XInfoHandle*) src_handle;
      src_handle = src_chandle->src_handle; /* unchain */
      if (!clear_xinfos)
        {
          /* copy over old added xinfos */
          dest_added = sfi_ring_copy_deep (src_chandle->added_xinfos, (SfiRingDataFunc) g_strdup, NULL);
        }
      if (!clear_xinfos)
        {
          /* copy over remove xinfos */
          dest_remove = sfi_ring_copy_deep (src_chandle->remove_xinfos, (SfiRingDataFunc) g_strdup, NULL);
          /* override with old added xinfos */
          dest_remove = sfi_ring_concat (sfi_ring_copy_deep (src_chandle->added_xinfos, (SfiRingDataFunc) g_strdup, NULL), dest_remove);
        }
      dest_clear_xinfos = src_chandle->clear_xinfos;
    }
  /* setup added xinfos */
  if (clear_xinfos)
    dest_added = sfi_ring_copy_deep (added_xinfos, (SfiRingDataFunc) g_strdup, NULL);
  else
    {
      /* override with new removals */
      dest_added = sfi_ring_concat (sfi_ring_copy_deep (remove_xinfos, (SfiRingDataFunc) g_strdup, NULL), dest_added);
      /* override with newly added xinfos */
      dest_added = sfi_ring_concat (sfi_ring_copy_deep (added_xinfos, (SfiRingDataFunc) g_strdup, NULL), dest_added);
    }
  /* remove dups (preserves first element from dup list) */
  dest_added = ring_remove_dups (dest_added, (SfiCompareFunc) bse_xinfo_stub_compare, NULL, g_free);
  /* filter non-empty xinfos */
  SfiRing *ring = NULL;
  while (dest_added)
    {
      char *xinfo = (char*) sfi_ring_pop_head (&dest_added);
      const char *e = strchr (xinfo, '=');
      if (e[1]) /* non-empty xinfo */
        ring = sfi_ring_append (ring, xinfo);
      else
        g_free (xinfo);
    }
  dest_added = ring;
  /* setup remove xinfos */
  if (!clear_xinfos)
    {
      /* override with new removals */
      dest_remove = sfi_ring_concat (sfi_ring_copy_deep (remove_xinfos, (SfiRingDataFunc) g_strdup, NULL), dest_remove);
      /* override with newly added xinfos */
      dest_remove = sfi_ring_concat (sfi_ring_copy_deep (added_xinfos, (SfiRingDataFunc) g_strdup, NULL), dest_remove);
    }
  /* remove dups (preserves first element from dup list) */
  dest_remove = ring_remove_dups (dest_remove, (SfiCompareFunc) bse_xinfo_stub_compare, NULL, g_free);
  /* filter empty xinfos */
  ring = NULL;
  while (dest_remove)
    {
      char *xinfo = (char*) sfi_ring_pop_head (&dest_remove);
      const char *e = strchr (xinfo, '=');
      if (!e[1]) /* empty xinfo */
        ring = sfi_ring_append (ring, xinfo);
      else
        g_free (xinfo);
    }
  dest_remove = ring;
  /* setup clear_xinfos */
  clear_xinfos |= dest_clear_xinfos;
  /* cleanup and swap */
  sfi_ring_free_deep (remove_xinfos, g_free);
  sfi_ring_free_deep (added_xinfos, g_free);
  remove_xinfos = dest_remove;
  added_xinfos = dest_added;
  /* create handle */
  XInfoHandle *chandle = sfi_new_struct0 (XInfoHandle, 1);
  gboolean success = gsl_data_handle_common_init (&chandle->dhandle, NULL);
  if (success)
    {
      chandle->dhandle.name = g_strconcat (src_handle->name,
                                           "// #xinfo",
                                           clear_xinfos ? "-cleared" : "",
                                           remove_xinfos ? "-removed" : "",
                                           added_xinfos ? "-added" : "",
                                           " /", NULL);
      chandle->dhandle.vtable = &xinfo_handle_vtable;
      chandle->src_handle = gsl_data_handle_ref (src_handle);
      chandle->clear_xinfos = clear_xinfos;
      if (chandle->clear_xinfos)
        {
          chandle->remove_xinfos = NULL;
          sfi_ring_free_deep (remove_xinfos, g_free);
        }
      else
        chandle->remove_xinfos = remove_xinfos;
      chandle->added_xinfos = added_xinfos;
    }
  else
    {
      sfi_ring_free_deep (remove_xinfos, g_free);
      sfi_ring_free_deep (added_xinfos, g_free);
      sfi_delete_struct (XInfoHandle, chandle);
      return NULL;
    }
  return &chandle->dhandle;
}
GslDataHandle*
gsl_data_handle_new_add_xinfos (GslDataHandle *src_handle,
                                gchar        **xinfos)
{
  SfiRing *added_xinfos = NULL;
  guint i;
  for (i = 0; xinfos && xinfos[i]; i++)
    {
      const gchar *xinfo = xinfos[i];
      const gchar *e = strchr (xinfo, '=');
      if (e && e[1]) /* non-empty xinfo */
        added_xinfos = sfi_ring_append (added_xinfos, g_strdup (xinfo));
    }
  return xinfo_data_handle_new (src_handle, FALSE, NULL, added_xinfos);
}
GslDataHandle*
gsl_data_handle_new_remove_xinfos (GslDataHandle *src_handle,
                                   gchar        **xinfos)
{
  SfiRing *remove_xinfos = NULL;
  guint i;
  for (i = 0; xinfos && xinfos[i]; i++)
    {
      const gchar *xinfo = xinfos[i];
      const gchar *e = strchr (xinfo, '=');
      if (!e && xinfo[0])     /* empty xinfo without '=' */
        remove_xinfos = sfi_ring_append (remove_xinfos, g_strconcat (xinfo, "=", NULL));
      if (e && !e[1])           /* empty xinfo with "=" */
        remove_xinfos = sfi_ring_append (remove_xinfos, g_strdup (xinfo));
    }
  return xinfo_data_handle_new (src_handle, FALSE, remove_xinfos, NULL);
}
GslDataHandle*
gsl_data_handle_new_clear_xinfos (GslDataHandle *src_handle)
{
  return xinfo_data_handle_new (src_handle, TRUE, NULL, NULL);
}
/* --- chain handle --- */
static BseErrorType
chain_handle_open (GslDataHandle      *dhandle,
		   GslDataHandleSetup *setup)
{
  ChainHandle *chandle = (ChainHandle*) dhandle;
  BseErrorType error;
  error = gsl_data_handle_open (chandle->src_handle);
  if (error != BSE_ERROR_NONE)
    return error;
  *setup = chandle->src_handle->setup; /* copies setup.xinfos by pointer */
  return BSE_ERROR_NONE;
}
static void
chain_handle_close (GslDataHandle *dhandle)
{
  ChainHandle *chandle = (ChainHandle*) dhandle;
  dhandle->setup.xinfos = NULL;     /* cleanup pointer reference */
  gsl_data_handle_close (chandle->src_handle);
}
static int64
chain_handle_get_state_length (GslDataHandle *dhandle)
{
  ChainHandle *chandle = (ChainHandle*) dhandle;
  return gsl_data_handle_get_state_length (chandle->src_handle);
}
/* --- reversed handle --- */
static void
reverse_handle_destroy (GslDataHandle *dhandle)
{
  ReversedHandle *rhandle = (ReversedHandle*) dhandle;
  gsl_data_handle_unref (rhandle->src_handle);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (ReversedHandle, rhandle);
}
static int64
reverse_handle_read (GslDataHandle *dhandle,
		     int64          voffset,
		     int64          n_values,
		     gfloat        *values)
{
  ReversedHandle *rhandle = (ReversedHandle*) dhandle;
  int64  left, new_offset = dhandle->setup.n_values - (voffset + n_values);
  gfloat *t, *p = values;
  g_assert (new_offset >= 0);
  left = n_values;
  do
    {
      int64 l = gsl_data_handle_read (rhandle->src_handle, new_offset, left, p);
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
    NULL,
    chain_handle_get_state_length,
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
/* --- scale handle --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  double            factor;
} ScaledHandle;
static void
scale_handle_destroy (GslDataHandle *dhandle)
{
  ScaledHandle *shandle = (ScaledHandle*) dhandle;
  gsl_data_handle_unref (shandle->src_handle);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (ScaledHandle, shandle);
}
static int64
scale_handle_read (GslDataHandle *dhandle,
                   int64          voffset,
                   int64          n_values,
                   gfloat        *values)
{
  ScaledHandle *shandle = (ScaledHandle*) dhandle;
  int64 i, l = gsl_data_handle_read (shandle->src_handle, voffset, n_values, values);
  for (i = 0; i < l; i++)
    values[i] *= shandle->factor;
  return l;
}
GslDataHandle*
gsl_data_handle_new_scale (GslDataHandle *src_handle,
                           double         factor)
{
  static GslDataHandleFuncs scale_handle_vtable = {
    chain_handle_open,
    scale_handle_read,
    chain_handle_close,
    NULL,
    chain_handle_get_state_length,
    scale_handle_destroy,
  };
  ScaledHandle *shandle;
  gboolean success;
  g_return_val_if_fail (src_handle != NULL, NULL);
  shandle = sfi_new_struct0 (ScaledHandle, 1);
  success = gsl_data_handle_common_init (&shandle->dhandle, NULL);
  if (success)
    {
      shandle->dhandle.name = g_strconcat (src_handle->name, "// #scaled /", NULL);
      shandle->dhandle.vtable = &scale_handle_vtable;
      shandle->src_handle = gsl_data_handle_ref (src_handle);
      shandle->factor = factor;
    }
  else
    {
      sfi_delete_struct (ScaledHandle, shandle);
      return NULL;
    }
  return &shandle->dhandle;
}
/* --- cut handle --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  int64 	    cut_offset;
  int64 	    n_cut_values;
  int64 	    tail_cut;
} CutHandle;
static BseErrorType
cut_handle_open (GslDataHandle      *dhandle,
		 GslDataHandleSetup *setup)
{
  CutHandle *chandle = (CutHandle*) dhandle;
  BseErrorType error;
  error = gsl_data_handle_open (chandle->src_handle);
  if (error != BSE_ERROR_NONE)
    return error;
  *setup = chandle->src_handle->setup; /* copies setup.xinfos by pointer */
  setup->n_values -= MIN (setup->n_values, chandle->tail_cut);
  setup->n_values -= MIN (setup->n_values, chandle->n_cut_values);
  return BSE_ERROR_NONE;
}
static void
cut_handle_destroy (GslDataHandle *dhandle)
{
  CutHandle *chandle = (CutHandle*) dhandle;
  gsl_data_handle_unref (chandle->src_handle);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (CutHandle, chandle);
}
static int64
cut_handle_read (GslDataHandle *dhandle,
		 int64          voffset,
		 int64          n_values,
		 gfloat        *values)
{
  CutHandle *chandle = (CutHandle*) dhandle;
  int64 orig_n_values = n_values;
  if (voffset < chandle->cut_offset)
    {
      int64 l = MIN (chandle->cut_offset - voffset, n_values);
      l = gsl_data_handle_read (chandle->src_handle, voffset, l, values);
      if (l < 0)
	return l;	/* pass on errors */
      n_values -= l;
      values += l;
      voffset += l;
    }
  if (voffset >= chandle->cut_offset && n_values)
    {
      int64 l = gsl_data_handle_read (chandle->src_handle, voffset + chandle->n_cut_values, n_values, values);
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
			       int64          cut_offset,
			       int64          n_cut_values,
			       int64	      tail_cut)
{
  static GslDataHandleFuncs cut_handle_vtable = {
    cut_handle_open,
    cut_handle_read,
    chain_handle_close,
    NULL,
    chain_handle_get_state_length,
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
 * @param src_handle   source GslDataHandle
 * @param cut_offset   offset of gap into @a src_handle
 * @param n_cut_values length of gap in @a src_handle
 * @return             a newly created data handle
 *
 * Create a new data handle containing the contents of @a src_handle
 * minus @a n_cut_values at offset @a cut_offset.
 */
GslDataHandle*
gsl_data_handle_new_cut (GslDataHandle *src_handle,
			 int64          cut_offset,
			 int64          n_cut_values)
{
  return gsl_data_handle_new_translate (src_handle, cut_offset, n_cut_values, 0);
}
/**
 * @param src_handle source GslDataHandle
 * @param n_head_cut number of values to cut at data handle head
 * @param n_tail_cut number of values to cut at data handle tail
 * @return           a newly created data handle
 *
 * Create a new data handle containing the contents of @a src_handle
 * minus @a n_head_cut values at the start and @a n_tail_cut values at
 * the end.
 */
GslDataHandle*
gsl_data_handle_new_crop (GslDataHandle *src_handle,
			  int64          n_head_cut,
			  int64          n_tail_cut)
{
  return gsl_data_handle_new_translate (src_handle, 0, n_head_cut, n_tail_cut);
}
/* --- insert handle --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  int64 	    requested_paste_offset;
  int64 	    paste_offset;
  int64 	    n_paste_values;
  guint		    paste_bit_depth;
  const gfloat	   *paste_values;
  void            (*free_values) (gpointer);
} InsertHandle;
static BseErrorType
insert_handle_open (GslDataHandle      *dhandle,
		    GslDataHandleSetup *setup)
{
  InsertHandle *ihandle = (InsertHandle*) dhandle;
  BseErrorType error;
  error = gsl_data_handle_open (ihandle->src_handle);
  if (error != BSE_ERROR_NONE)
    return error;
  *setup = ihandle->src_handle->setup; /* copies setup.xinfos by pointer */
  ihandle->paste_offset = ihandle->requested_paste_offset < 0 ? setup->n_values : ihandle->requested_paste_offset;
  if (setup->n_values < ihandle->paste_offset)
    setup->n_values = ihandle->paste_offset + ihandle->n_paste_values;
  else
    setup->n_values += ihandle->n_paste_values;
  guint n = gsl_data_handle_bit_depth (ihandle->src_handle);
  setup->bit_depth = MAX (n, ihandle->paste_bit_depth);
  return BSE_ERROR_NONE;
}
static void
insert_handle_close (GslDataHandle *dhandle)
{
  InsertHandle *ihandle = (InsertHandle*) dhandle;
  dhandle->setup.xinfos = NULL;
  gsl_data_handle_close (ihandle->src_handle);
}
static void
insert_handle_destroy (GslDataHandle *dhandle)
{
  InsertHandle *ihandle = (InsertHandle*) dhandle;
  void (*free_values) (gpointer) = ihandle->free_values;
  const gfloat *paste_values = ihandle->paste_values;
  gsl_data_handle_unref (ihandle->src_handle);
  gsl_data_handle_common_free (dhandle);
  ihandle->paste_values = NULL;
  ihandle->free_values = NULL;
  sfi_delete_struct (InsertHandle, ihandle);
  if (free_values)
    free_values ((gpointer) paste_values);
}
static int64
insert_handle_read (GslDataHandle *dhandle,
		    int64          voffset,
		    int64          n_values,
		    gfloat        *values)
{
  InsertHandle *ihandle = (InsertHandle*) dhandle;
  int64 l, orig_n_values = n_values;
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
static int64
insert_handle_get_state_length (GslDataHandle *dhandle)
{
  InsertHandle *ihandle = (InsertHandle*) dhandle;
  return gsl_data_handle_get_state_length (ihandle->src_handle);
}
GslDataHandle*
gsl_data_handle_new_insert (GslDataHandle *src_handle,
			    guint          paste_bit_depth,
			    int64          insertion_offset,
			    int64          n_paste_values,
			    const gfloat  *paste_values,
			    void         (*free) (gpointer values))
{
  static GslDataHandleFuncs insert_handle_vtable = {
    insert_handle_open,
    insert_handle_read,
    insert_handle_close,
    NULL,
    insert_handle_get_state_length,
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
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* mirror ChainHandle */
  int64 	    requested_first;
  int64 	    requested_last;
  int64 	    loop_start;
  int64 	    loop_width;
} LoopHandle;
static BseErrorType
loop_handle_open (GslDataHandle      *dhandle,
		  GslDataHandleSetup *setup)
{
  LoopHandle *lhandle = (LoopHandle*) dhandle;
  BseErrorType error;
  error = gsl_data_handle_open (lhandle->src_handle);
  if (error != BSE_ERROR_NONE)
    return error;
  *setup = lhandle->src_handle->setup; /* copies setup.xinfos by pointer */
  if (setup->n_values > lhandle->requested_last)
    {
      lhandle->loop_start = lhandle->requested_first;
      lhandle->loop_width = lhandle->requested_last - lhandle->requested_first + 1;
      setup->n_values = G_MAXINT64;
    }
  else	/* cannot loop */
    {
      lhandle->loop_start = setup->n_values;
      lhandle->loop_width = 0;
    }
  return BSE_ERROR_NONE;
}
static void
loop_handle_destroy (GslDataHandle *dhandle)
{
  LoopHandle *lhandle = (LoopHandle*) dhandle;
  gsl_data_handle_unref (lhandle->src_handle);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (LoopHandle, lhandle);
}
static int64
loop_handle_read (GslDataHandle *dhandle,
		  int64          voffset,
		  int64          n_values,
		  gfloat        *values)
{
  LoopHandle *lhandle = (LoopHandle*) dhandle;
  if (voffset < lhandle->loop_start)
    return gsl_data_handle_read (lhandle->src_handle, voffset,
				 MIN (lhandle->loop_start - voffset, n_values),
				 values);
  else
    {
      int64 noffset = voffset - lhandle->loop_start;
      noffset %= lhandle->loop_width;
      return gsl_data_handle_read (lhandle->src_handle,
				   lhandle->loop_start + noffset,
				   MIN (lhandle->loop_width - noffset, n_values),
				   values);
    }
}
GslDataHandle*
gsl_data_handle_new_looped (GslDataHandle *src_handle,
			    int64          loop_first,
			    int64          loop_last)
{
  static GslDataHandleFuncs loop_handle_vtable = {
    loop_handle_open,
    loop_handle_read,
    chain_handle_close,
    NULL,
    chain_handle_get_state_length,
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
      lhandle->dhandle.name = g_strdup_printf ("%s// #loop(0x%llx:0x%llx) /", src_handle->name, loop_first, loop_last);
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
typedef struct {
  GslDataHandle     dhandle;
  GslDataCache	   *dcache;
  guint		    node_size;
} DCacheHandle;
static void
dcache_handle_destroy (GslDataHandle *dhandle)
{
  DCacheHandle *chandle = (DCacheHandle*) dhandle;
  gsl_data_cache_unref (chandle->dcache);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (DCacheHandle, chandle);
}
static BseErrorType
dcache_handle_open (GslDataHandle      *dhandle,
		    GslDataHandleSetup *setup)
{
  DCacheHandle *chandle = (DCacheHandle*) dhandle;
  BseErrorType error;
  error = gsl_data_handle_open (chandle->dcache->dhandle);
  if (error != BSE_ERROR_NONE)
    return error;
  gsl_data_cache_open (chandle->dcache);
  *setup = chandle->dcache->dhandle->setup; /* copies setup.xinfos by pointer */
  gsl_data_handle_close (chandle->dcache->dhandle);
  return BSE_ERROR_NONE;
}
static void
dcache_handle_close (GslDataHandle *dhandle)
{
  DCacheHandle *chandle = (DCacheHandle*) dhandle;
  dhandle->setup.xinfos = NULL;     /* cleanup pointer reference */
  gsl_data_cache_close (chandle->dcache);
}
static int64
dcache_handle_read (GslDataHandle *dhandle,
		    int64          voffset,
		    int64          n_values,
		    gfloat        *values)
{
  DCacheHandle *chandle = (DCacheHandle*) dhandle;
  GslDataCacheNode *node;
  node = gsl_data_cache_ref_node (chandle->dcache, voffset, GSL_DATA_CACHE_DEMAND_LOAD);
  voffset -= node->offset;
  n_values = MIN (n_values, chandle->node_size - voffset);
  memcpy (values, node->data + voffset, sizeof (values[0]) * n_values);
  return n_values;
}
static GslDataHandle*
dcache_handle_get_source_handle (GslDataHandle *dhandle)
{
  DCacheHandle *chandle = (DCacheHandle*) dhandle;
  return chandle->dcache->dhandle;
}
static int64
dcache_handle_get_state_length (GslDataHandle *dhandle)
{
  DCacheHandle *chandle = (DCacheHandle*) dhandle;
  return gsl_data_handle_get_state_length (chandle->dcache->dhandle);
}
GslDataHandle*
gsl_data_handle_new_dcached (GslDataCache *dcache)
{
  static GslDataHandleFuncs dcache_handle_vtable = {
    dcache_handle_open,
    dcache_handle_read,
    dcache_handle_close,
    dcache_handle_get_source_handle,
    dcache_handle_get_state_length,
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
typedef struct {
  GslDataHandle     dhandle;
  GslHFile	   *hfile;
  int64	            byte_offset;
  guint             byte_order;
  guint		    n_channels;
  GslWaveFormatType format;
  guint		    add_zoffset : 1;
  int64	            requested_offset;
  int64	            requested_length;
  gchar           **xinfos;
  gfloat            mix_freq;
} WaveHandle;
static inline guint G_GNUC_CONST
wave_format_bit_depth (const GslWaveFormatType format)
{
  switch (format)
    {
    case GSL_WAVE_FORMAT_UNSIGNED_8:
    case GSL_WAVE_FORMAT_SIGNED_8:
      return 8;
    case GSL_WAVE_FORMAT_ALAW:
    case GSL_WAVE_FORMAT_ULAW:
      return 11;
    case GSL_WAVE_FORMAT_UNSIGNED_12:
    case GSL_WAVE_FORMAT_SIGNED_12:
      return 12;
    case GSL_WAVE_FORMAT_UNSIGNED_16:
    case GSL_WAVE_FORMAT_SIGNED_16:
      return 16;
    case GSL_WAVE_FORMAT_SIGNED_24:
    case GSL_WAVE_FORMAT_SIGNED_24_PAD4:
      return 24;
    case GSL_WAVE_FORMAT_SIGNED_32:
      return 32;
    case GSL_WAVE_FORMAT_FLOAT:
      return 32;
    default:
      return 0;
    }
}
static inline guint G_GNUC_CONST
wave_format_byte_width (const GslWaveFormatType format)
{
  switch (format)
    {
    case GSL_WAVE_FORMAT_ALAW:
    case GSL_WAVE_FORMAT_ULAW:
      return 1;
    default:
      return (wave_format_bit_depth (format) + 7) / 8;
    }
}                   
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
wave_handle_destroy (GslDataHandle *dhandle)
{
  WaveHandle *whandle = (WaveHandle*) dhandle;
  g_strfreev (whandle->xinfos);
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (WaveHandle, whandle);
}
static BseErrorType
wave_handle_open (GslDataHandle      *dhandle,
		  GslDataHandleSetup *setup)
{
  WaveHandle *whandle = (WaveHandle*) dhandle;
  whandle->hfile = gsl_hfile_open (whandle->dhandle.name);
  if (!whandle->hfile)
    return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
  else
    {
      int64 l, fwidth = wave_format_byte_width (whandle->format);
      whandle->byte_offset = whandle->requested_offset;
      if (whandle->add_zoffset)
	{
	  int64 zoffset = gsl_hfile_zoffset (whandle->hfile);
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
      setup->xinfos = whandle->xinfos;
      setup->bit_depth = wave_format_bit_depth (whandle->format);
      setup->mix_freq = whandle->mix_freq;
#ifndef __linux__
      /* linux does proper caching and WAVs are easily readable */
      setup->needs_cache = TRUE;
#endif
      return BSE_ERROR_NONE;
    }
}
static void
wave_handle_close (GslDataHandle *dhandle)
{
  WaveHandle *whandle = (WaveHandle*) dhandle;
  dhandle->setup.xinfos = NULL;
  gsl_hfile_close (whandle->hfile);
  whandle->hfile = NULL;
}
static int64
wave_handle_read (GslDataHandle *dhandle,
		  int64          voffset,
		  int64          n_values,
		  gfloat        *values)
{
  WaveHandle *whandle = (WaveHandle*) dhandle;
  gpointer buffer = values;
  int64 l, byte_offset;
  byte_offset = voffset * wave_format_byte_width (whandle->format);	/* float offset into bytes */
  byte_offset += whandle->byte_offset;
  switch (whandle->format)
    {
      guint8 *u8; gint8 *s8; guint16 *u16; guint32 *u32; gint32 *s32;
    case GSL_WAVE_FORMAT_UNSIGNED_8:
      u8 = (guint8*) buffer; u8 += n_values * 3;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values, u8);
      if (l < 1)
	return l;
      gsl_conv_to_float (whandle->format, whandle->byte_order, u8, values, l);
      break;
    case GSL_WAVE_FORMAT_SIGNED_8:
    case GSL_WAVE_FORMAT_ALAW:
    case GSL_WAVE_FORMAT_ULAW:
      s8 = (gint8*) buffer; s8 += n_values * 3;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values, s8);
      if (l < 1)
	return l;
      gsl_conv_to_float (whandle->format, whandle->byte_order, s8, values, l);
      break;
    case GSL_WAVE_FORMAT_SIGNED_12:
    case GSL_WAVE_FORMAT_UNSIGNED_12:
    case GSL_WAVE_FORMAT_SIGNED_16:
    case GSL_WAVE_FORMAT_UNSIGNED_16:
      u16 = (guint16*) buffer; u16 += n_values;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values << 1, u16);
      if (l < 2)
	return l < 0 ? l : 0;
      l >>= 1;
      gsl_conv_to_float (whandle->format, whandle->byte_order, u16, values, l);
      break;
    case GSL_WAVE_FORMAT_SIGNED_24:
      s8 = (gint8*) buffer; s8 += n_values * 1;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values * 3, s8);
      if (l < 3)
	return l < 0 ? l : 0;
      l /= 3;
      gsl_conv_to_float (whandle->format, whandle->byte_order, s8, values, l);
      break;
    case GSL_WAVE_FORMAT_SIGNED_24_PAD4:
      s32 = (gint32*) buffer;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values * 4, s32);
      if (l < 4)
	return l < 0 ? l : 0;
      l /= 4;
      gsl_conv_to_float (whandle->format, whandle->byte_order, s32, values, l);
      break;
    case GSL_WAVE_FORMAT_SIGNED_32:
      s32 = (gint32*) buffer;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values * 4, s32);
      if (l < 4)
        return l < 0 ? l : 0;
      l /= 4;
      gsl_conv_to_float (whandle->format, whandle->byte_order, s32, values, l);
      break;
    case GSL_WAVE_FORMAT_FLOAT:
      u32 = (guint32*) buffer;
      l = gsl_hfile_pread (whandle->hfile, byte_offset, n_values << 2, u32);
      if (l < 4)
	return l < 0 ? l : 0;
      l >>= 2;
      gsl_conv_to_float (whandle->format, whandle->byte_order, u32, values, l);
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
		     int64             byte_offset,
		     int64             n_values,
                     gchar           **xinfos)
{
  static GslDataHandleFuncs wave_handle_vtable = {
    wave_handle_open,
    wave_handle_read,
    wave_handle_close,
    NULL,
    NULL,
    wave_handle_destroy,
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
      whandle->requested_offset = byte_offset;
      whandle->requested_length = n_values;
      whandle->hfile = NULL;
      whandle->xinfos = bse_xinfos_dup_consolidated (xinfos, FALSE);
      whandle->mix_freq = mix_freq;
      whandle->xinfos = bse_xinfos_add_float (whandle->xinfos, "osc-freq", osc_freq);
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
                             int64             byte_offset,
			     int64             byte_size,
                             gchar           **xinfos)
{
  GslDataHandle *dhandle = gsl_wave_handle_new (file_name, n_channels, format,
						byte_order, mix_freq, osc_freq, byte_offset,
						byte_size / wave_format_byte_width (format), xinfos);
  if (dhandle)
    ((WaveHandle*) dhandle)->add_zoffset = TRUE;
  return dhandle;
}
const gchar*
gsl_wave_format_to_string (GslWaveFormatType format)
{
  switch (format)
    {
    case GSL_WAVE_FORMAT_UNSIGNED_8:      return "unsigned-8";
    case GSL_WAVE_FORMAT_SIGNED_8:        return "signed-8";
    case GSL_WAVE_FORMAT_ALAW:            return "alaw";
    case GSL_WAVE_FORMAT_ULAW:            return "ulaw";
    case GSL_WAVE_FORMAT_UNSIGNED_12:     return "unsigned-12";
    case GSL_WAVE_FORMAT_SIGNED_12:       return "signed-12";
    case GSL_WAVE_FORMAT_UNSIGNED_16:     return "unsigned-16";
    case GSL_WAVE_FORMAT_SIGNED_16:       return "signed-16";
    case GSL_WAVE_FORMAT_SIGNED_24:       return "signed-24";
    case GSL_WAVE_FORMAT_SIGNED_24_PAD4:  return "signed-24-pad";
    case GSL_WAVE_FORMAT_SIGNED_32:       return "signed-32";
    case GSL_WAVE_FORMAT_FLOAT:           return "float";
    default:
      g_return_val_if_fail (format > GSL_WAVE_FORMAT_NONE && format < GSL_WAVE_FORMAT_LAST, NULL);
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
  if (strncasecmp (string, "alaw", 5) == 0)
    return GSL_WAVE_FORMAT_ALAW;
  if (strncasecmp (string, "ulaw", 5) == 0)
    return GSL_WAVE_FORMAT_ULAW;
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
  if (string[0] == '1' && string[1] == '2')
    return is_unsigned ? GSL_WAVE_FORMAT_UNSIGNED_12 : GSL_WAVE_FORMAT_SIGNED_12;
  if (string[0] == '1' && string[1] == '6')
    return is_unsigned ? GSL_WAVE_FORMAT_UNSIGNED_16 : GSL_WAVE_FORMAT_SIGNED_16;
  if (string[0] == '3' && string[1] == '2' && !is_unsigned)
    return GSL_WAVE_FORMAT_SIGNED_32;
  if (string[0] == '2' && string[1] == '4' && !is_unsigned)
    {
      if (string[2] == '-' && string[3] == 'p' && string[4] == 'a' && string[5] == 'd')
        return GSL_WAVE_FORMAT_SIGNED_24_PAD4;
      else
        return GSL_WAVE_FORMAT_SIGNED_24;
    }
  return GSL_WAVE_FORMAT_NONE;
}
