/* BSE - Better Sound Engine
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bseenums.h"
#include "gslcommon.h"
#include <errno.h>

/* include generated enum value arrays and *.h files the enums steam from */
#include "bseenum_arrays.cc"

/* --- functions --- */
void
bse_type_register_enums (void)
{
  static const struct {
    const char *name;
    GType       parent_type;
    GType      *type_p;
    void       *values;
  } enums[] = {
    /* include generated enum list */
#include "bseenum_list.cc"
  };
  uint n_enums = sizeof (enums) / sizeof (enums[0]);
  uint i;
  
  for (i = 0; i < n_enums; i++)
    {
      if (enums[i].parent_type == G_TYPE_ENUM)
	{
	  *(enums[i].type_p) = g_enum_register_static (enums[i].name, (GEnumValue*) enums[i].values);
	  g_value_register_transform_func (SFI_TYPE_CHOICE, *(enums[i].type_p), sfi_value_choice2enum_simple);
	  g_value_register_transform_func (*(enums[i].type_p), SFI_TYPE_CHOICE, sfi_value_enum2choice);
	}
      else if (enums[i].parent_type == G_TYPE_FLAGS)
	*(enums[i].type_p) = g_flags_register_static (enums[i].name, (GFlagsValue*) enums[i].values);
      else
	g_assert_not_reached ();
    }
}

/* BseErrorType is a static type */
static GEnumClass *bse_error_class = NULL;

const char*
bse_error_name (BseErrorType error_value)
{
  GEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_name : NULL;
}

const char*
bse_error_nick (BseErrorType error_value)
{
  GEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

const char*
bse_error_blurb (BseErrorType error_value)
{
  GEnumValue *ev;
  
  if (!bse_error_class)
    bse_error_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_ERROR_TYPE);
  
  switch (error_value)
    {
    case BSE_ERROR_NONE:                        return _("Everything went well");
    case BSE_ERROR_INTERNAL:                    return _("Internal error (please report)");
    case BSE_ERROR_UNKNOWN:                     return _("Unknown error");
    case BSE_ERROR_IO:                          return _("Input/output error");
    case BSE_ERROR_PERMS:                       return _("Insufficient permission");
      /* file errors */
    case BSE_ERROR_FILE_BUSY:                   return _("Device or resource busy");
    case BSE_ERROR_FILE_EXISTS:                 return _("File exists already");
    case BSE_ERROR_FILE_EOF:                    return _("Premature EOF");
    case BSE_ERROR_FILE_EMPTY:                  return _("File empty");
    case BSE_ERROR_FILE_NOT_FOUND:              return _("No such file, device or directory");
    case BSE_ERROR_FILE_IS_DIR:                 return _("Is a directory");
    case BSE_ERROR_FILE_OPEN_FAILED:            return _("Open failed");
    case BSE_ERROR_FILE_SEEK_FAILED:            return _("Seek failed");
    case BSE_ERROR_FILE_READ_FAILED:            return _("Read failed");
    case BSE_ERROR_FILE_WRITE_FAILED:           return _("Write failed");
      /* out of resource conditions */
    case BSE_ERROR_MANY_FILES:                  return _("Too many open files");
    case BSE_ERROR_NO_FILES:                    return _("Too many open files in system");
    case BSE_ERROR_NO_SPACE:                    return _("No space left on device");
    case BSE_ERROR_NO_MEMORY:                   return _("Out of memory");
      /* content errors */
    case BSE_ERROR_NO_HEADER:                   return _("Failed to detect (start of) header");
    case BSE_ERROR_NO_SEEK_INFO:                return _("Failed to retrieve seek information");
    case BSE_ERROR_NO_DATA:                     return _("No data available");
    case BSE_ERROR_DATA_CORRUPT:                return _("Data corrupt");
    case BSE_ERROR_WRONG_N_CHANNELS:            return _("Wrong number of channels");
    case BSE_ERROR_FORMAT_INVALID:              return _("Invalid format");
    case BSE_ERROR_FORMAT_UNKNOWN:              return _("Unknown format");
    case BSE_ERROR_DATA_UNMATCHED:              return _("Requested data values unmatched");
      /* miscellaneous errors */
    case BSE_ERROR_TEMP:                        return _("Temporary error");
    case BSE_ERROR_WAVE_NOT_FOUND:              return _("No such wave");
    case BSE_ERROR_CODEC_FAILURE:               return _("CODEC failure");
    case BSE_ERROR_UNIMPLEMENTED:		return _("Functionality not implemented");
    case BSE_ERROR_INVALID_PROPERTY:	        return _("Invalid object property");
    case BSE_ERROR_INVALID_MIDI_CONTROL:	return _("Invalid MIDI control type");
    case BSE_ERROR_PARSE_ERROR:			return _("Parsing error");
    case BSE_ERROR_SPAWN:			return _("Failed to spawn child process");
      /* Device errors */
    case BSE_ERROR_DEVICE_NOT_AVAILABLE:	return _("No device (driver) available");
    case BSE_ERROR_DEVICE_ASYNC:		return _("Device not async capable");
    case BSE_ERROR_DEVICE_BUSY:			return _("Device busy");
    case BSE_ERROR_DEVICE_FORMAT:               return _("Failed to configure device format");
    case BSE_ERROR_DEVICE_BUFFER:               return _("Failed to configure device buffer");
    case BSE_ERROR_DEVICE_LATENCY:              return _("Failed to configure device latency");
    case BSE_ERROR_DEVICE_CHANNELS:             return _("Failed to configure number of device channels");
    case BSE_ERROR_DEVICE_FREQUENCY:            return _("Failed to configure device frequency");
    case BSE_ERROR_DEVICES_MISMATCH:            return _("Device configurations mismatch");
      /* BseSource errors */
    case BSE_ERROR_SOURCE_NO_SUCH_MODULE:	return _("No such synthesis module");
    case BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL:	return _("No such input channel");
    case BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL:	return _("No such output channel");
    case BSE_ERROR_SOURCE_NO_SUCH_CONNECTION:	return _("Input/Output channels not connected");
    case BSE_ERROR_SOURCE_PRIVATE_ICHANNEL:	return _("Input channel is private");
    case BSE_ERROR_SOURCE_ICHANNEL_IN_USE:	return _("Input channel already in use");
    case BSE_ERROR_SOURCE_CHANNELS_CONNECTED:	return _("Input/Output channels already connected");
    case BSE_ERROR_SOURCE_CONNECTION_INVALID:	return _("Invalid synthesis module connection");
    case BSE_ERROR_SOURCE_PARENT_MISMATCH:	return _("Parent mismatch");
    case BSE_ERROR_SOURCE_BAD_LOOPBACK:		return _("Bad loopback");
    case BSE_ERROR_SOURCE_BUSY:			return _("Synthesis module currently busy");
    case BSE_ERROR_SOURCE_TYPE_INVALID:		return _("Invalid synthsis module type");
      /* BseProcedure errors */
    case BSE_ERROR_PROC_NOT_FOUND:		return _("No such procedure");
    case BSE_ERROR_PROC_BUSY:			return _("Procedure currently busy"); /* recursion */
    case BSE_ERROR_PROC_PARAM_INVAL:		return _("Procedure parameter invalid");
    case BSE_ERROR_PROC_EXECUTION:		return _("Procedure execution failed");
    case BSE_ERROR_PROC_ABORT:			return _("Procedure execution aborted");
      /* various procedure errors */
    case BSE_ERROR_NO_ENTRY:			return _("No such entry");
    case BSE_ERROR_NO_EVENT:			return _("No such event");
    case BSE_ERROR_NO_TARGET:			return _("No target");
    case BSE_ERROR_NOT_OWNER:			return _("Ownership mismatch");
    case BSE_ERROR_INVALID_OFFSET:		return _("Invalid offset");
    case BSE_ERROR_INVALID_DURATION:		return _("Invalid duration");
    case BSE_ERROR_INVALID_OVERLAP:		return _("Invalid overlap");
    }
  
  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

BseErrorType
bse_error_from_errno (int             v_errno,
		      BseErrorType    fallback)
{
  return gsl_error_from_errno (v_errno, fallback);
}
