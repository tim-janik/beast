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


/* --- prototypes --- */
extern void	bse_type_register_enums		(void);
static void	bse_enum_class_init		(BseEnumClass	*class,
						 gpointer	 class_data);
static void	bse_flags_class_init		(BseFlagsClass	*class,
						 gpointer	 class_data);


/* include generated enum value arrays and *.h files the enums steam from */
#include "bseenum_arrays.c"


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
    case BSE_ERROR_IGNORE:			return "Something went wrong...";
    case BSE_ERROR_UNKNOWN:			return "Unknown error";
    case BSE_ERROR_INTERNAL:			return "Internal error";
    case BSE_ERROR_UNIMPLEMENTED:		return "Functionality not imlemented";
    case BSE_ERROR_FILE_IO:			return "File I/O error";
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
    case BSE_ERROR_STREAM_INVALID:		return "Stream handle invalid";
    case BSE_ERROR_STREAM_SUSPENDED:		return "Stream suspended";
    case BSE_ERROR_STREAM_DEVICE_BUSY:		return "Stream device busy";
    case BSE_ERROR_STREAM_READ_DENIED:		return "Stream read access denied";
    case BSE_ERROR_STREAM_READ_FAILED:		return "Filed to read from stream";
    case BSE_ERROR_STREAM_WRITE_DENIED:		return "Stream write access denied";
    case BSE_ERROR_STREAM_WRITE_FAILED:		return "Writing to stream failed";
    case BSE_ERROR_STREAM_IO:			return "Stream I/O error";
    case BSE_ERROR_STREAM_GET_ATTRIB:		return "Failed to get stream attribute";
    case BSE_ERROR_STREAM_SET_ATTRIB:		return "Failed to set stream attribute";
    case BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL:	return "No such input channel";
    case BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL:	return "No such output channel";
    case BSE_ERROR_SOURCE_ICHANNEL_IN_USE:	return "Input channel already in use";
    case BSE_ERROR_SOURCE_TOO_MANY_ITRACKS:	return "Too many input tracks required";
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
