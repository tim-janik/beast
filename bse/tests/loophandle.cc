// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/gsldatahandle.hh>
#include <bse/gsldatautils.hh>
#include <bse/bse.hh>
#include <bse/bsemain.hh>
#include <stdlib.h>

/* --- loop handle: reference code --- */
typedef struct {
  GslDataHandle     dhandle;
  GslDataHandle	   *src_handle;	/* the data handle we loop */
  GslLong	    requested_first;
  GslLong	    requested_last;
  GslLong	    loop_start;
  GslLong	    loop_width;
} LoopHandleReference;

static BseErrorType
loop_handle_reference_open (GslDataHandle      *dhandle,
			    GslDataHandleSetup *setup)
{
  LoopHandleReference *lhandle = (LoopHandleReference*) dhandle;
  BseErrorType error;
  
  error = gsl_data_handle_open (lhandle->src_handle);
  if (error != BSE_ERROR_NONE)
    return error;
  
  *setup = lhandle->src_handle->setup; /* copies setup.xinfos by pointer */
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
  
  return BSE_ERROR_NONE;
}

static void
loop_handle_reference_close (GslDataHandle *dhandle)
{
  LoopHandleReference *lhandle = (LoopHandleReference*) dhandle;

  dhandle->setup.xinfos = NULL;     /* cleanup pointer reference */
  gsl_data_handle_close (lhandle->src_handle);
}

static void
loop_handle_reference_destroy (GslDataHandle *dhandle)
{
  LoopHandleReference *lhandle = (LoopHandleReference*) dhandle;
  
  gsl_data_handle_unref (lhandle->src_handle);
  
  gsl_data_handle_common_free (dhandle);
  sfi_delete_struct (LoopHandleReference, lhandle);
}

static GslLong
loop_handle_reference_read (GslDataHandle *dhandle,
			    GslLong        voffset,
			    GslLong        n_values,
			    gfloat        *values)
{
  LoopHandleReference *lhandle = (LoopHandleReference*) dhandle;
  
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

static int64
loop_handle_reference_get_state_length (GslDataHandle *dhandle)
{
  LoopHandleReference *lhandle = (LoopHandleReference*) dhandle;
  return gsl_data_handle_get_state_length (lhandle->src_handle);
}


static GslDataHandle*
gsl_data_handle_new_looped_reference (GslDataHandle *src_handle,
			              GslLong        loop_first,
			              GslLong        loop_last)
{
  static GslDataHandleFuncs loop_handle_reference_vtable = {
    loop_handle_reference_open,
    loop_handle_reference_read,
    loop_handle_reference_close,
    NULL,
    loop_handle_reference_get_state_length,
    loop_handle_reference_destroy,
  };
  LoopHandleReference *lhandle;
  gboolean success;
  
  g_return_val_if_fail (src_handle != NULL, NULL);
  g_return_val_if_fail (loop_first >= 0, NULL);
  g_return_val_if_fail (loop_last >= loop_first, NULL);
  
  lhandle = sfi_new_struct0 (LoopHandleReference, 1);
  success = gsl_data_handle_common_init (&lhandle->dhandle, NULL);
  if (success)
    {
      lhandle->dhandle.name = g_strdup_printf ("%s// #loop(0x%llx:0x%llx) /", src_handle->name, loop_first, loop_last);
      lhandle->dhandle.vtable = &loop_handle_reference_vtable;
      lhandle->src_handle = gsl_data_handle_ref (src_handle);
      lhandle->requested_first = loop_first;
      lhandle->requested_last = loop_last;
      lhandle->loop_start = 0;
      lhandle->loop_width = 0;
    }
  else
    {
      sfi_delete_struct (LoopHandleReference, lhandle);
      return NULL;
    }
  return &lhandle->dhandle;
}

const guint n_channels = 2;
const guint n_values   = 4096;

static void
check_loop (GslDataHandle *src_handle,
            GslLong loop_start,
	    GslLong loop_end)
{
  g_return_if_fail (loop_start >= 0);
  g_return_if_fail (loop_start < n_values);
  g_return_if_fail (loop_end > loop_start);
  g_return_if_fail (loop_end < n_values);

  GslDataHandle *loop_handle           = gsl_data_handle_new_looped (src_handle, loop_start, loop_end);
  GslDataHandle *loop_handle_reference = gsl_data_handle_new_looped_reference (src_handle, loop_start, loop_end);

  GslDataPeekBuffer peek_buffer		  = { +1 /* incremental direction */, 0, };
  GslDataPeekBuffer peek_buffer_reference = { +1 /* incremental direction */, 0, };
  sfi_info ("check_loop<%lld,%lld>", loop_start, loop_end);

  BseErrorType error;
  error = gsl_data_handle_open (loop_handle);
  if (error)
    {
      sfi_error ("loop_handle open failed: %s", bse_error_blurb (error));
      exit (1);
    }

  error = gsl_data_handle_open (loop_handle_reference);
  if (error)
    {
      sfi_error ("loop_handle_reference open failed: %s", bse_error_blurb (error));
      exit (1);
    }

  GslLong i;
  for (i = 0; i < n_values * 10; i++)
    {
      gfloat a = gsl_data_handle_peek_value (loop_handle, i, &peek_buffer);
      gfloat b = gsl_data_handle_peek_value (loop_handle_reference, i, &peek_buffer_reference);

      if (a != b)
	{
	  sfi_error ("bad read in loop<%lld,%lld> position %lld: a = %f, b = %f", loop_start, loop_end, i, a, b);
	  exit (1);
	}
    }
  gsl_data_handle_close (loop_handle);
  gsl_data_handle_close (loop_handle_reference);
  gsl_data_handle_unref (loop_handle);
  gsl_data_handle_unref (loop_handle_reference);
}

int
main (int argc, char **argv)
{
  gfloat      values[n_values * n_channels];

  guint i;
  for (i = 0; i < n_channels * n_values; i++)
    values[i] = 1.0 - rand() / (0.5 * RAND_MAX);

  /* init bse */
  bse_init_test (&argc, &argv, NULL);
  sfi_msg_allow ("misc");

  /* test loophandle against the reference implementation */
  GslDataHandle *mem_handle = gsl_data_handle_new_mem (n_channels, 32, 44100, 440, n_values, values, NULL /* don't free */);
  check_loop (mem_handle, 0, n_values - 1);
  check_loop (mem_handle, 17, 379);
  check_loop (mem_handle, 59, 1930);
  return 0;
}
