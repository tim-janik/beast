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
 *
 * bsetext.h: text streaming interface
 */
#ifndef	__BSE_TEXT_H__
#define	__BSE_TEXT_H__

#include	<bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- interface type macros --- */
#define BSE_TYPE_TEXT                  (BSE_TYPE_ID (BseText))
#define BSE_TEXT(object)               (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_TEXT, BseText))
#define BSE_IS_TEXT(object)            (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_TEXT))
#define BSE_TEXT_GET_INTERFACE(object) (BSE_OBJECT_GET_INTERFACE ((object), BSE_TYPE_TEXT, BseTextInterface))


/* --- object & class member/convenience macros --- */
#define BSE_TEXT_SET_FLAG(text, f)   (bse_text_set_flag ((BseText*) (text), 1, BSE_TEXTF_ ## f))
#define BSE_TEXT_UNSET_FLAG(text, f) (bse_text_set_flag ((BseText*) (text), 0, BSE_TEXTF_ ## f))
#define	BSE_TEXT_NEEDS_BREAK(text)   ((bse_text_flags ((BseText*) (text)) & BSE_TEXTF_NEEDS_BREAK) != 0)
#define	BSE_TEXT_AT_BOL(text)	     ((bse_text_flags ((BseText*) (text)) & BSE_TEXTF_AT_BOL) != 0)


/* --- BseText flags --- */
typedef enum
{
  BSE_TEXTF_NEEDS_BREAK		= 1 << 0,
  BSE_TEXTF_AT_BOL		= 1 << 1
} BseTextFlags;


/* --- BseText interface --- */
struct _BseTextInterface
{
  BseTypeInterfaceClass	interface;

  void	(*write_chars)	(BseText     *text,
			 guint        n_chars,
			 const gchar *chars);
};


/* --- prototypes -- */
void		bse_text_set_flag		(BseText	 *text,
						 gboolean	  set_unset,
						 BseTextFlags	  flag);
BseTextFlags	bse_text_flags			(BseText	 *text);
void		bse_text_push_indent		(BseText	 *text,
						 const gchar	 *indent);
void		bse_text_pop_indent		(BseText	 *text);
const gchar*	bse_text_get_indent		(BseText	 *text);
void		bse_text_printf			(BseText	 *text,
						 const gchar	 *format,
						 ...) G_GNUC_PRINTF (2, 3);
void		bse_text_puts			(BseText	 *text,
						 const gchar	 *string);
void		bse_text_putc			(BseText	 *text,
						 gchar	 	  character);
void		bse_text_indent			(BseText	 *text);
void		bse_text_handle_break		(BseText	 *text);
void		bse_text_break			(BseText	 *text);
void		bse_text_needs_break		(BseText	 *text);
void		bse_text_put_param		(BseText	 *text,
						 BseParam	 *param);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_TEXT_H__ */
