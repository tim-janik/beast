/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_TEXT_TOOLS_H__
#define __BST_TEXT_TOOLS_H__

#include        "bstdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum /*< skip >*/
{
  BST_TEXT_VIEW_PARSE_TSM	= 1 << 0, /* TagSpanMarkup */
  BST_TEXT_VIEW_MONO_SPACED	= 1 << 1,
  BST_TEXT_VIEW_CENTER          = 1 << 2,
  BST_TEXT_VIEW_NO_WRAP         = 1 << 3,
  BST_TEXT_VIEW_SHEET_BG        = 1 << 4
} BstTextViewFlags;

void		bst_text_view_enter_browse_mode	(GtkTextView		*tview);
void		bst_text_view_leave_browse_mode	(GtkTextView		*tview);
void		bst_text_view_cursor_to_start	(GtkTextView		*tview);
void		bst_text_view_cursor_to_end	(GtkTextView		*tview);
GtkWidget*	bst_scroll_text_create		(BstTextViewFlags	 flags,
						 const gchar		*string);
GtkWidget*	bst_scroll_text_from_file	(BstTextViewFlags	 flags,
						 const gchar		*file_name);
void		bst_scroll_text_set		(GtkWidget		*sctext,
						 const gchar		*string);
void		bst_scroll_text_set_tsm		(GtkWidget		*sctext,
						 const gchar		*string);
void		bst_scroll_text_clear		(GtkWidget		*sctext);
void		bst_scroll_text_push_indent	(GtkWidget		*sctext,
						 const gchar		*spaces);
void		bst_scroll_text_append		(GtkWidget		*sctext,
						 const gchar		*string);
void		bst_scroll_text_append_tsm	(GtkWidget		*sctext,
						 const gchar		*string);
void		bst_scroll_text_append_file	(GtkWidget		*sctext,
						 const gchar    	*file_name);
void		bst_scroll_text_append_file_tsm	(GtkWidget		*sctext,
						 const gchar    	*file_name);
void		bst_scroll_text_aprintf		(GtkWidget		*sctext,
						 const gchar		*text_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_scroll_text_aprintf_tsm	(GtkWidget		*sctext,
						 const gchar		*text_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_scroll_text_pop_indent	(GtkWidget		*sctext);
GtkTextView*	bst_scroll_text_get_text_view	(GtkWidget		*sctext);
void		bst_text_add_tsm_path		(const gchar		*path);
void	bst_text_buffer_cursor_to_start			(GtkTextBuffer	*tbuffer);
void	bst_text_buffer_cursor_to_end			(GtkTextBuffer	*tbuffer);
void	bst_text_buffer_append_from_string		(GtkTextBuffer	*tbuffer,
							 gboolean	 parse_tsm,
							 guint		 indent_margin,
							 guint		 text_length,
							 const gchar	*text);
void	bst_text_buffer_append_from_file		(GtkTextBuffer	*tbuffer,
							 gboolean	 parse_tsm,
							 guint		 indent_margin,
							 const gchar	*file_name);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_TEXT_TOOLS_H__ */
