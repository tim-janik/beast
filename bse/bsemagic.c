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
#include	"bsemagic.h"
#include	"bsesong.h"
#include	"bsesongsequencer.h"
#include	<string.h>
#include	<unistd.h>
#include	<errno.h>


/* --- functions --- */
/* identify a BSE data file
 * enforce a string of type
 * ";BSE-Data V1 " 10*<oct-value> " <anything> "\n"
 * FIXME: then check the version numbering
 * magic entry:
 * #------------------------------------------------------------------------------
 * # BSE Data - Tim Janik <timj@gtk.org>
 * 0       string          (BSE-Data\040        BSE File, sound related data
 * >10     string          =V1\040              - version 1
 */
static inline BseErrorType
bse_magic_identify_bse_string_o (const gchar   *string,
				 BseMagicFlags *flags_p)
{
  gchar *expect;
  guint len;
  guint i;
  guint flags;
  guint length;

  if (flags_p)
    *flags_p = 0;
  g_return_val_if_fail (string != NULL, BSE_ERROR_INTERNAL);

  length = strlen (string);

  /* read ";BSE-Data V1 "
   */
  expect = ";BSE-Data V1 ";
  len = strlen (expect);
  if (length <= len)
    return BSE_ERROR_FILE_TOO_SHORT;
  if (strncmp (string, expect, len) != 0)
    return BSE_ERROR_HEADER_CORRUPT;
  string += len;
  length -= len;

  /* check octal values
   */
  len = 10;
  if (length <= len)
    return BSE_ERROR_FILE_TOO_SHORT;
  flags = 0;
  for (i = 0; i < len; i++)
    {
      if (string[i] >= '0' || string[i] <= '7')
	flags |= (string[i] - '0') << (9 - i);
      else
	return BSE_ERROR_HEADER_CORRUPT;
    }
  string += len;
  length -= len;
  
  /* skip to newline
   */
  do
    {
      if (!*string)
	return BSE_ERROR_FILE_TOO_SHORT;
    }
  while (*(string++) != '\n');

  if (flags_p)
    *flags_p = flags;

  return BSE_ERROR_NONE;
}

BseErrorType
bse_magic_identify_bse_fd (gint           fd,
			   BseMagicFlags *flags)
{
  guint l;
  gchar buffer[1025];
  BseErrorType error;
  
  if (flags)
    *flags = 0;
  g_return_val_if_fail (fd >= 0, BSE_ERROR_INTERNAL);

  do
    l = read (fd, buffer, 1024);
  while (l < 0 && errno == EINTR);

  if (l <= 0)
    return l < 0 ? BSE_ERROR_FILE_IO : BSE_ERROR_FILE_TOO_SHORT;

  buffer[l] = 0;

  error = bse_magic_identify_bse_string_o (buffer, flags);
  lseek (fd, 0, SEEK_SET);

  return error;
}

BseErrorType
bse_magic_identify_bse_string (const gchar   *string,
			       BseMagicFlags *flags)
{
  return bse_magic_identify_bse_string_o (string, flags);
}
