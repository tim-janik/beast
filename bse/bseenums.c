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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bseenums.h"

#include        <bse/bseexports.h>
#include	<errno.h>

/* --- prototypes --- */
extern void	bse_type_register_enums		(void);
static void	bse_enum_class_init		(BseEnumClass	*class,
						 gpointer	 class_data);
static void	bse_flags_class_init		(BseFlagsClass	*class,
						 gpointer	 class_data);


/* include generated enum value arrays and *.h files the enums steam from */
#include "bseenum_arrays.c"
/* special hook for bse_init() */
extern BseFlagsValue *bse_debug_key_flag_values;
extern guint          bse_debug_key_n_flag_values;
BseFlagsValue        *bse_debug_key_flag_values = bse_debug_flags_values;
guint                 bse_debug_key_n_flag_values = (sizeof (bse_debug_flags_values) /
						     sizeof (bse_debug_flags_values[0]));


/* --- functions --- */
void
bse_type_register_enums (void)
{
  static const struct {
    gchar *name;
    BseType parent_type;
    BseType *type_p;
    gpointer values;
  } enums[] = {
    /* include generated enum list */
#include "bseenum_list.c"
  };
  guint n_enums = sizeof (enums) / sizeof (enums[0]);
  BseTypeInfo info = {
    0 /* class_size */,
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) NULL /* class_init */,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    0 /* object_size */,
    0 /* n_preallocs */,
    (BseObjectInitFunc) NULL,
  };
  guint i;
  
  for (i = 0; i < n_enums; i++)
    {
      if (enums[i].parent_type == BSE_TYPE_ENUM)
	{
	  info.class_size = sizeof (BseEnumClass);
	  info.class_init = (BseClassInitFunc) bse_enum_class_init;
	}
      else if (enums[i].parent_type == BSE_TYPE_FLAGS)
	{
	  info.class_size = sizeof (BseFlagsClass);
	  info.class_init = (BseClassInitFunc) bse_flags_class_init;
	}
      else
	g_assert_not_reached ();
      
      info.class_data = enums[i].values;
      *(enums[i].type_p) = bse_type_register_static (enums[i].parent_type,
						     enums[i].name,
						     NULL,
						     &info);
    }
}

void
bse_enum_complete_info (const BseExportSpec *spec,
			BseTypeInfo         *info)
{
  const BseExportEnum *espec = &spec->s_enum;
  
  if (espec->parent_type == BSE_TYPE_ENUM)
    {
      info->class_size = sizeof (BseEnumClass);
      info->class_init = (BseClassInitFunc) bse_enum_class_init;
    }
  else if (espec->parent_type == BSE_TYPE_FLAGS)
    {
      info->class_size = sizeof (BseFlagsClass);
      info->class_init = (BseClassInitFunc) bse_flags_class_init;
    }
  else
    g_assert_not_reached ();
  
  info->class_data = espec->values;
}

const gchar*
bse_enum_type_register (const gchar *name,
			BseType      parent_type,
			BsePlugin   *plugin,
			BseType     *ret_type)
{
  BseType type;

  g_return_val_if_fail (ret_type != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  *ret_type = 0;
  g_return_val_if_fail (name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (parent_type != 0, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (plugin != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));

  type = bse_type_from_name (name);
  if (type)
    return "Enum Type already registered";
  if (parent_type != BSE_TYPE_ENUM && parent_type != parent_type)
    return "Parent type neither enum nor flags";

  type = bse_type_register_dynamic (parent_type,
				    name,
				    NULL,
				    plugin);
  *ret_type = type;

  return NULL;
}

static void
bse_enum_class_init (BseEnumClass *class,
		     gpointer	   class_data)
{
  BseEnumValue *values;
  
  class->values = class_data;
  class->n_values = 0;
  class->minimum = 0;
  class->maximum = 0;
  if (class->values)
    for (values = class->values; values->value_name; values++)
      {
	class->n_values++;
	class->minimum = MIN (class->minimum, values->value);
	class->maximum = MAX (class->maximum, values->value);
      }
}

static void
bse_flags_class_init (BseFlagsClass *class,
		      gpointer	     class_data)
{
  BseFlagsValue *values;
  
  class->values = class_data;
  class->n_values = 0;
  class->mask = 0;
  if (class->values)
    for (values = class->values; values->value_name; values++)
      {
	class->n_values++;
	class->mask |= values->value;
      }
}

BseEnumValue*
bse_enum_get_value (BseEnumClass *enum_class,
		    gint	  value)
{
  g_return_val_if_fail (enum_class != NULL, NULL);
  g_return_val_if_fail (BSE_IS_ENUM_CLASS (enum_class), NULL);
  
  if (enum_class->n_values)
    {
      BseEnumValue *enum_value;
      
      for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
	if (enum_value->value == value)
	  return enum_value;
    }
  
  return NULL;
}

BseEnumValue*
bse_enum_get_value_by_name (BseEnumClass *enum_class,
			    const gchar	 *name)
{
  g_return_val_if_fail (enum_class != NULL, NULL);
  g_return_val_if_fail (BSE_IS_ENUM_CLASS (enum_class), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  if (enum_class->n_values)
    {
      BseEnumValue *enum_value;
      
      for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
	if (strcmp (name, enum_value->value_name) == 0)
	  return enum_value;
    }
  
  return NULL;
}

BseEnumValue*
bse_enum_get_value_by_nick (BseEnumClass *enum_class,
			    const gchar	 *nick)
{
  g_return_val_if_fail (enum_class != NULL, NULL);
  g_return_val_if_fail (BSE_IS_ENUM_CLASS (enum_class), NULL);
  g_return_val_if_fail (nick != NULL, NULL);
  
  if (enum_class->n_values)
    {
      BseEnumValue *enum_value;
      
      for (enum_value = enum_class->values; enum_value->value_nick; enum_value++)
	if (strcmp (nick, enum_value->value_nick) == 0)
	  return enum_value;
    }
  
  return NULL;
}

BseFlagsValue*
bse_flags_get_first_value (BseFlagsClass *flags_class,
			   guint	  value)
{
  g_return_val_if_fail (flags_class != NULL, NULL);
  g_return_val_if_fail (BSE_IS_FLAGS_CLASS (flags_class), NULL);
  
  if (flags_class->n_values)
    {
      BseFlagsValue *flags_value;
      
      for (flags_value = flags_class->values; flags_value->value_name; flags_value++)
	if ((flags_value->value & value) > 0)
	  return flags_value;
    }
  
  return NULL;
}

BseFlagsValue*
bse_flags_get_value_by_name (BseFlagsClass *flags_class,
			     const gchar   *name)
{
  g_return_val_if_fail (flags_class != NULL, NULL);
  g_return_val_if_fail (BSE_IS_FLAGS_CLASS (flags_class), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  if (flags_class->n_values)
    {
      BseFlagsValue *flags_value;
      
      for (flags_value = flags_class->values; flags_value->value_name; flags_value++)
	if (strcmp (name, flags_value->value_name) == 0)
	  return flags_value;
    }
  
  return NULL;
}

BseFlagsValue*
bse_flags_get_value_by_nick (BseFlagsClass *flags_class,
			     const gchar  *nick)
{
  g_return_val_if_fail (flags_class != NULL, NULL);
  g_return_val_if_fail (BSE_IS_FLAGS_CLASS (flags_class), NULL);
  g_return_val_if_fail (nick != NULL, NULL);
  
  if (flags_class->n_values)
    {
      BseFlagsValue *flags_value;
      
      for (flags_value = flags_class->values; flags_value->value_nick; flags_value++)
	if (strcmp (nick, flags_value->value_nick) == 0)
	  return flags_value;
    }
  
  return NULL;
}

/* BseErrorType is a static type */
static BseEnumClass *bse_error_class = NULL;

gchar*
bse_error_name (BseErrorType error_value)
{
  BseEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = bse_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  ev = bse_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_name : NULL;
}

gchar*
bse_error_nick (BseErrorType error_value)
{
  BseEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = bse_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  ev = bse_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

gchar*
bse_error_blurb (BseErrorType error_value)
{
  BseEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = bse_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  switch (error_value)
    {
    case BSE_ERROR_NONE:			return "Everything went well";
    case BSE_ERROR_IGNORE:			return "Temporary headache...";
    case BSE_ERROR_UNKNOWN:			return "Unknown error";
    case BSE_ERROR_INTERNAL:			return "Internal error (please report)";
    case BSE_ERROR_UNIMPLEMENTED:		return "Functionality not imlemented";
    case BSE_ERROR_IO:				return "Device/file I/O error";
    case BSE_ERROR_PERMS:			return "Insufficient permissions";
    case BSE_ERROR_FILE_EXISTS:			return "File exists";
    case BSE_ERROR_FILE_NOT_FOUND:		return "File not found";
    case BSE_ERROR_FILE_TOO_SHORT:		return "File too short";
    case BSE_ERROR_FILE_TOO_LONG:		return "File too long";
    case BSE_ERROR_FORMAT_UNSUPPORTED:		return "Format not supported";
    case BSE_ERROR_FORMAT_TOO_NEW:		return "Format too new";
    case BSE_ERROR_FORMAT_TOO_OLD:		return "Format too old";
    case BSE_ERROR_HEADER_CORRUPT:		return "Header corrupt";
    case BSE_ERROR_SUB_HEADER_CORRUPT:		return "Sub-header corrupt";
    case BSE_ERROR_DATA_CORRUPT:		return "Data corrupt";
    case BSE_ERROR_BINARY_DATA_CORRUPT:		return "Binary data corrupt";
    case BSE_ERROR_DEVICE_ASYNC:		return "Device not async capable";
    case BSE_ERROR_DEVICE_BUSY:			return "Device busy";
    case BSE_ERROR_DEVICE_GET_CAPS:		return "Failed to query device capabilities";
    case BSE_ERROR_DEVICE_CAPS_MISMATCH:	return "Device capabilities not sufficient";
    case BSE_ERROR_DEVICE_SET_CAPS:		return "Failed to set device capabilities";
    case BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL:	return "No such input channel";
    case BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL:	return "No such output channel";
    case BSE_ERROR_SOURCE_BAD_LOOPBACK:		return "Bad loopback";
    case BSE_ERROR_SOURCE_ICHANNEL_IN_USE:	return "Input channel already in use";
    case BSE_ERROR_SOURCE_TOO_MANY_ITRACKS:	return "Too many input tracks required";
    case BSE_ERROR_SOURCE_TOO_MANY_OTRACKS:	return "Too many output tracks supplied";
    case BSE_ERROR_PROC_BUSY: /* recursion */	return "Procedure currently busy";
    case BSE_ERROR_PROC_PARAM_INVAL:		return "Procedure parameter invalid";
    case BSE_ERROR_PROC_EXECUTION:		return "Procedure execution failed";
    case BSE_ERROR_PROC_NEEDLESS:		return "Procedure had nothing to do";
    case BSE_ERROR_PROC_ABORT:			return "Procedure execution aborted";
    default:
      break;
    }
  
  ev = bse_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

BseErrorType
bse_error_from_errno (gint            v_errno,
		      BseErrorType    fallback)
{
  switch (v_errno)
    {
    case EBUSY:		return BSE_ERROR_DEVICE_BUSY;
    case EISDIR:
    case EACCES:
    case EPERM:
    case EROFS:		return BSE_ERROR_DEVICE_PERMS;
    case ELOOP:
    case ENOENT:	return BSE_ERROR_FILE_NOT_FOUND;
    case EEXIST:	return BSE_ERROR_FILE_EXISTS;
    case EIO:		return BSE_ERROR_FILE_IO;
    case EBADF:		return BSE_ERROR_INTERNAL;
    default:		return fallback;
    }      
}
