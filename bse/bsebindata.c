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
#include        "bsebindata.h"

#include        "bseglobals.h"
#include	<errno.h>
#include	<unistd.h>


enum
{
  PARAM_0,
  PARAM_N_BITS,
  PARAM_BYTE_SIZE
};


/* --- prototypes --- */
static void	bse_bin_data_class_init		(BseBinDataClass	*class);
static void	bse_bin_data_init		(BseBinData		*bin_data);
static void	bse_bin_data_destroy		(BseObject		*object);
static void     bse_bin_data_set_param          (BseBinData		*bin_data,
						 BseParam               *param);
static void     bse_bin_data_get_param 		(BseBinData        	*bin_data,
						 BseParam               *param);
static void	bse_bin_data_free_values	(BseBinData		*bin_data);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseBinData)
{
  static const BseTypeInfo bin_data_info = {
    sizeof (BseBinDataClass),
    
    (BseClassInitBaseFunc) NULL,
    (BseClassDestroyBaseFunc) NULL,
    (BseClassInitFunc) bse_bin_data_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseBinData),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_bin_data_init,
  };
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseBinData",
				   "Binary data container",
				   &bin_data_info);
}

static void
bse_bin_data_class_init (BseBinDataClass *class)
{
  BseObjectClass *object_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_OBJECT);
  object_class = BSE_OBJECT_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_bin_data_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_bin_data_get_param;
  object_class->destroy = bse_bin_data_destroy;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_N_BITS,
			      bse_param_spec_uint ("n_bits", "Value size in bits",
						   BSE_MIN_BIT_SIZE, BSE_MAX_BIT_SIZE,
						   8, BSE_DFL_BIN_DATA_BITS,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_BYTE_SIZE,
			      bse_param_spec_uint ("byte_size", "Value size in bytes",
						   BSE_MIN_BIT_SIZE / 8, BSE_MAX_BIT_SIZE / 8,
						   1, BSE_DFL_BIN_DATA_BITS / 8,
						   BSE_PARAM_READWRITE |
						   BSE_PARAM_SERVE_GUI));
}

static void
bse_bin_data_init (BseBinData *bin_data)
{
  bin_data->bits_per_value = BSE_DFL_BIN_DATA_BITS;
  bin_data->n_values = 0;
  bin_data->n_bytes = 0;
  bin_data->values = NULL;
}

static void
bse_bin_data_destroy (BseObject *object)
{
  BseBinData *bin_data;
  
  bin_data = BSE_BIN_DATA (object);
  
  bse_bin_data_free_values (bin_data);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_bin_data_set_param (BseBinData  *bin_data,
			BseParam    *param)
{
  if (bin_data->values)
    bse_bin_data_free_values (bin_data);
  switch (param->pspec->any.param_id)
    {
    case PARAM_N_BITS:
      bin_data->bits_per_value = param->value.v_uint;
      break;
    case PARAM_BYTE_SIZE:
      bin_data->bits_per_value = param->value.v_uint * 8;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (bin_data),
		 BSE_OBJECT_NAME (bin_data),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_bin_data_get_param (BseBinData *bin_data,
			BseParam   *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_N_BITS:
      param->value.v_uint = bin_data->bits_per_value;
      break;
    case PARAM_BYTE_SIZE:
      param->value.v_uint = bin_data->bits_per_value * 8;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (bin_data),
		 BSE_OBJECT_NAME (bin_data),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_bin_data_free_values (BseBinData *bin_data)
{
  g_free (bin_data->values);
  bin_data->values = NULL;
  bin_data->n_values = 0;
  bin_data->n_bytes = 0;
}

BseErrorType
bse_bin_data_set_values_from_fd (BseBinData   *bin_data,
				 gint          fd,
				 glong         offset,
				 guint         n_bytes,
				 BseEndianType byte_order)
{
  glong l;

  g_return_val_if_fail (BSE_IS_BIN_DATA (bin_data), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_OBJECT_IS_LOCKED (bin_data), BSE_ERROR_INTERNAL);

  bse_bin_data_free_values (bin_data);
  if (!n_bytes)
    return BSE_ERROR_NONE;

  g_return_val_if_fail (fd >= 0, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (offset >= 0, BSE_ERROR_INTERNAL);

  l = lseek (fd, 0, SEEK_END);
  if (l < 0)
    return BSE_ERROR_FILE_IO;
  if (l < offset + n_bytes)
    return BSE_ERROR_FILE_TOO_SHORT;

  if (lseek (fd, offset, SEEK_SET) != offset)
    return BSE_ERROR_FILE_IO;

  bin_data->n_values = n_bytes / ((bin_data->bits_per_value + 7) / 8);
  bin_data->n_bytes = bin_data->n_values * ((bin_data->bits_per_value + 7) / 8);
  bin_data->values = g_new (guint8, bin_data->n_bytes + 32);
  for (l = bin_data->n_bytes; l < bin_data->n_bytes + 32; l++)
    bin_data->values[l] = 0;

  do
    {
      l = read (fd, bin_data->values, bin_data->n_bytes);
    }
  while (l < 0 && errno == EINTR);

  if (l < bin_data->n_bytes)
    {
      /* FIXME: we could probably do better here */
      bse_bin_data_free_values (bin_data);

      return l < 1 ? BSE_ERROR_FILE_IO : BSE_ERROR_FILE_TOO_SHORT;
    }

  /* internally, we enforce LE representation of data
   */
  if (byte_order == BSE_BIG_ENDIAN &&
      bin_data->bits_per_value > 8)
    {
      guint16 *v16_f, *v16_l; /* first, last */

      v16_f = (gpointer) bin_data->values;
      v16_l = v16_f + bin_data->n_values;

      while (v16_f < v16_l)
	{
	  *v16_f = GUINT16_SWAP_LE_BE (*v16_f);
	  v16_f++;
	}
    }

  return BSE_ERROR_NONE;
}
