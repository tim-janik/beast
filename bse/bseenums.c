/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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

#include	"gslcommon.h"

#include	<errno.h>

/* --- prototypes --- */
extern void	bse_type_register_enums		(void);


/* include generated enum value arrays and *.h files the enums steam from */
#include "bseenum_arrays.c"

/* special hooks for bse_init() */
extern GFlagsValue *bse_debug_key_flag_values;
extern guint        bse_debug_key_n_flag_values;
GFlagsValue        *bse_debug_key_flag_values = bse_debug_flags_values;
guint               bse_debug_key_n_flag_values = (sizeof (bse_debug_flags_values) /
						   sizeof (bse_debug_flags_values[0]));


/* --- functions --- */
void
bse_type_register_enums (void)
{
  static const struct {
    gchar   *name;
    GType    parent_type;
    GType   *type_p;
    gpointer values;
  } enums[] = {
    /* include generated enum list */
#include "bseenum_list.c"
  };
  guint n_enums = sizeof (enums) / sizeof (enums[0]);
  guint i;
  
  for (i = 0; i < n_enums; i++)
    {
      if (enums[i].parent_type == G_TYPE_ENUM)
	{
	  *(enums[i].type_p) = g_enum_register_static (enums[i].name, enums[i].values);
	  g_value_register_transform_func (SFI_TYPE_CHOICE, *(enums[i].type_p), sfi_value_choice2enum_simple);
	  g_value_register_transform_func (*(enums[i].type_p), SFI_TYPE_CHOICE, sfi_value_enum2choice);
	}
      else if (enums[i].parent_type == G_TYPE_FLAGS)
	*(enums[i].type_p) = g_flags_register_static (enums[i].name, enums[i].values);
      else
	g_assert_not_reached ();
    }
}

/* BseErrorType is a static type */
static GEnumClass *bse_error_class = NULL;

gchar*
bse_error_name (BseErrorType error_value)
{
  GEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = g_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_name : NULL;
}

gchar*
bse_error_nick (BseErrorType error_value)
{
  GEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = g_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

gchar*
bse_error_blurb (BseErrorType error_value)
{
  GEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = g_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  switch (error_value)
    {
    case BSE_ERROR_NONE:	/* GSL */	return "Everything went well";
    case BSE_ERROR_UNIMPLEMENTED:		return "Functionality not implemented";
    case BSE_ERROR_DEVICE_NOT_AVAILABLE:	return "No device (driver) available";
    case BSE_ERROR_DEVICE_ASYNC:		return "Device not async capable";
    case BSE_ERROR_DEVICE_BUSY:			return "Device busy";
    case BSE_ERROR_DEVICE_GET_CAPS:		return "Failed to query device capabilities";
    case BSE_ERROR_DEVICE_CAPS_MISMATCH:	return "Device capabilities not sufficient";
    case BSE_ERROR_DEVICE_SET_CAPS:		return "Failed to set device capabilities";
      /* FIXME: remove date parsing errors */
    case BSE_ERROR_SOURCE_NO_SUCH_MODULE:	return "No such synthesis module";
    case BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL:	return "No such input channel";
    case BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL:	return "No such output channel";
    case BSE_ERROR_SOURCE_NO_SUCH_CONNECTION:	return "Input/Output channels not connected";
    case BSE_ERROR_SOURCE_ICHANNEL_IN_USE:	return "Input channel already in use";
    case BSE_ERROR_SOURCE_CHANNELS_CONNECTED:	return "Input/Output channels already connected";
    case BSE_ERROR_SOURCE_BAD_LOOPBACK:		return "Bad loopback";
    case BSE_ERROR_SOURCE_BUSY:			return "Synthesis module currently busy";
    case BSE_ERROR_SOURCE_TYPE_INVALID:		return "Invalid synthsis module type";
    case BSE_ERROR_PROC_NOT_FOUND:		return "No such procedure";
    case BSE_ERROR_PROC_BUSY:			return "Procedure currently busy"; /* recursion */
    case BSE_ERROR_PROC_PARAM_INVAL:		return "Procedure parameter invalid";
    case BSE_ERROR_PROC_EXECUTION:		return "Procedure execution failed";
    case BSE_ERROR_PROC_ABORT:			return "Procedure execution aborted";
    case BSE_ERROR_PARSE_ERROR:			return "Parsing error";
    case BSE_ERROR_SPAWN:			return "Failed to spawn child process";
    case BSE_ERROR_POS_ALLOC:			return "Position already allocated";
      /* various procedure errors */
    case BSE_ERROR_NO_ENTRY:			return "No such entry";
    case BSE_ERROR_NO_EVENT:			return "No such event";
    case BSE_ERROR_NO_TARGET:			return "No target";
    case BSE_ERROR_NOT_OWNER:			return "Ownership mismatch";
    case BSE_ERROR_INVALID_OFFSET:		return "Invalid offset";
    case BSE_ERROR_INVALID_DURATION:		return "Invalid duration";
    case BSE_ERROR_INVALID_OVERLAP:		return "Invalid overlap";
    default:
      if (error_value < GSL_ERROR_LAST)
	return (gchar*) gsl_strerror (error_value);
      break;
    }
  
  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

BseErrorType
bse_error_from_errno (gint            v_errno,
		      BseErrorType    fallback)
{
  return gsl_error_from_errno (v_errno, fallback);
}
