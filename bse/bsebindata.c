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
						 guint          	 param_id,
						 GValue         	*value,
						 GParamSpec     	*pspec,
						 const gchar    	*trailer);
static void     bse_bin_data_get_param 		(BseBinData        	*bin_data,
						 guint          	 param_id,
						 GValue         	*value,
						 GParamSpec     	*pspec,
						 const gchar    	*trailer);
static void	bse_bin_data_free_values	(BseBinData		*bin_data);


/* --- variables --- */
static GTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseBinData)
{
  static const GTypeInfo bin_data_info = {
    sizeof (BseBinDataClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_bin_data_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseBinData),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_bin_data_init,
  };
  
  return bse_type_register_static (BSE_TYPE_OBJECT,
				   "BseBinData",
				   "BSE binary data container",
				   &bin_data_info);
}

static void
bse_bin_data_class_init (BseBinDataClass *class)
{
  GObjectClass *gobject_class;
  BseObjectClass *object_class;
  
  parent_class = g_type_class_peek (BSE_TYPE_OBJECT);
  gobject_class = G_OBJECT_CLASS (class);
  object_class = BSE_OBJECT_CLASS (class);
  
  gobject_class->set_param = (GObjectSetParamFunc) bse_bin_data_set_param;
  gobject_class->get_param = (GObjectGetParamFunc) bse_bin_data_get_param;

  object_class->destroy = bse_bin_data_destroy;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_N_BITS,
			      b_param_spec_uint ("n_bits", "# Bits", "Value size in bits",
						 BSE_MIN_BIT_SIZE, BSE_MAX_BIT_SIZE,
						 BSE_DFL_BIN_DATA_BITS, 8,
						 B_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_BYTE_SIZE,
			      b_param_spec_uint ("byte_size", "Byte Size", "Value size in bytes",
						 BSE_MIN_BIT_SIZE / 8, BSE_MAX_BIT_SIZE / 8,
						 BSE_DFL_BIN_DATA_BITS / 8, 1,
						 B_PARAM_READWRITE |
						 B_PARAM_SERVE_GUI));
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
			guint        param_id,
			GValue      *value,
			GParamSpec  *pspec,
			const gchar *trailer)
{
  if (bin_data->values)
    bse_bin_data_free_values (bin_data);
  switch (param_id)
    {
    case PARAM_N_BITS:
      bin_data->bits_per_value = b_value_get_uint (value);
      break;
    case PARAM_BYTE_SIZE:
      bin_data->bits_per_value = b_value_get_uint (value);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (bin_data, param_id, pspec);
      break;
    }
}

static void
bse_bin_data_get_param (BseBinData  *bin_data,
                        guint        param_id,
			GValue      *value,
			GParamSpec  *pspec,
			const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_N_BITS:
      b_value_set_uint (value, bin_data->bits_per_value);
      break;
    case PARAM_BYTE_SIZE:
      b_value_set_uint (value, bin_data->bits_per_value * 8);
      break;
    default:
      G_WARN_INVALID_PARAM_ID (bin_data, param_id, pspec);
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
  bse_object_param_changed (BSE_OBJECT (bin_data), "n_bits");
  bse_object_param_changed (BSE_OBJECT (bin_data), "byte_size");
  bin_data->values = g_new (guint8, bin_data->n_bytes);

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

  /* if necessary, convert LE/BE
   */
  if (byte_order != G_BYTE_ORDER && bin_data->bits_per_value > 8)
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

void
bse_bin_data_init_values (BseBinData *bin_data,
			  guint       bits_per_value,
			  guint       n_values)
{
  g_return_if_fail (BSE_IS_BIN_DATA (bin_data));
  g_return_if_fail (!BSE_OBJECT_IS_LOCKED (bin_data));

  bse_bin_data_free_values (bin_data);

  bin_data->bits_per_value = CLAMP (bits_per_value, BSE_MIN_BIT_SIZE, BSE_MIN_BIT_SIZE);
  bin_data->n_values = n_values;
  bin_data->n_bytes = bin_data->n_values * ((bin_data->bits_per_value + 7) / 8);
  bse_object_param_changed (BSE_OBJECT (bin_data), "n_bits");
  bse_object_param_changed (BSE_OBJECT (bin_data), "byte_size");
  bin_data->values = g_new0 (guint8, bin_data->n_bytes + 32);
}
