/* XmlAntiSpace - Eliminate whitespaces from XML markup
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#include <glib.h>


#define BUFFER_SIZE        (4000)

#define	INHERIT		(2)

/* --- structures --- */
typedef struct {
  gboolean tab2space;
  gboolean newline2space;
  gboolean compress_spaces;
  gboolean del2newline;
  gboolean kill_leading_space;
  gboolean kill_trailing_space;
  gboolean no_leading_spaces;
  gboolean no_trailing_spaces;
  gboolean back2newline;
} XasRule;
typedef struct _XasTag XasTag;
struct _XasTag {
  gchar   *name;
  XasRule  rule;
  XasTag  *next;
};
typedef struct {
  gint   fd;
  gint   last;
  gchar  buffer[BUFFER_SIZE + 1];
  gchar *text;
  gchar *bound;
} XasInput;
typedef struct {
  gint     fd;
  gint     last;
  gchar    buffer[BUFFER_SIZE + 1];
  gchar   *p;
  gchar	  *space_pipe;
  guint    skip_spaces;
  gboolean literal;
  gboolean tab2space;
  gboolean newline2space;
  gboolean compress_spaces;
  gboolean del2newline;
} XasOutput;
