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
 * glib-extra.h: this file covers stuff that's missing from GLib 1.2.x
 */
#ifndef __GLIB_EXTRA_H__
#define __GLIB_EXTRA_H__

#include	<glib.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- string functions --- */
gchar*  g_strcanon              (gchar         *string,
				 const gchar   *extra_valid_chars,
				 gchar          subsitutor);
gchar*  g_strdup_quoted         (const gchar   *string);


/* --- lists & slists --- */
GSList* g_slist_insert_before   (GSList        *slist,
				 GSList        *sibling,
				 gpointer       data);
GList*  g_list_insert_before    (GList         *list,
				 GList         *sibling,
				 gpointer       data);
GSList*	g_slist_remove_any	(GSList	       *list,
				 gpointer	data);


/* --- signal queue --- */
typedef gboolean (*GUSignalFunc) (gint8          usignal,
			 	  gpointer       data);
guint   g_usignal_add            (gint8          usignal,
				  GUSignalFunc   function,
				  gpointer       data);
guint   g_usignal_add_full       (gint           priority,
				  gint8          usignal,
				  GUSignalFunc   function,
				  gpointer       data,
				  GDestroyNotify destroy);
void    g_usignal_notify         (gint8          usignal);


/* --- pattern matching --- */
typedef enum
{
  G_MATCH_ALL,       /* "*A?A*" */
  G_MATCH_ALL_TAIL,  /* "*A?AA" */
  G_MATCH_HEAD,      /* "AAAA*" */
  G_MATCH_TAIL,      /* "*AAAA" */
  G_MATCH_EXACT,     /* "AAAAA" */
  G_MATCH_LAST
} GMatchType;
typedef struct _GPatternSpec	GPatternSpec;
struct _GPatternSpec
{
  GMatchType match_type;
  guint      pattern_length;
  gchar     *pattern;
  gchar     *pattern_reversed;
};
GPatternSpec* g_pattern_spec_new       (const gchar  *pattern);
void          g_pattern_spec_free      (GPatternSpec *pspec);
gboolean      g_pattern_match          (GPatternSpec *pspec,
					guint         string_length,
					const gchar  *string,
					const gchar  *string_reversed);
gboolean      g_pattern_match_string   (GPatternSpec *pspec,
					const gchar  *string);
gboolean      g_pattern_match_simple   (const gchar  *pattern,
					const gchar  *string);

#if !GLIB_CHECK_VERSION (1, 3, 1)
#define G_STRINGIFY(macro_or_string)    _G_STRINGIFY_INTERNAL (macro_or_string)
#define _G_STRINGIFY_INTERNAL(contents) #contents
typedef struct _GTrashStack GTrashStack;
struct _GTrashStack { GTrashStack *next; };
static inline void g_trash_stack_push (GTrashStack **stack_p, gpointer data_p)
{ GTrashStack *data = data_p; data->next = *stack_p; *stack_p = data; }
static inline gpointer g_trash_stack_pop (GTrashStack **stack_p)
{ GTrashStack *data; data = *stack_p; if (data)
  { *stack_p = data->next; data->next = NULL; } return data; }
static inline gpointer g_trash_stack_peek (GTrashStack **stack_p)
{ GTrashStack *data; data = *stack_p; return data; }
static inline guint g_trash_stack_height (GTrashStack **stack_p)
{ GTrashStack *data; guint i = 0; for (data = *stack_p; data; data = data->next)
  i++; return i; }
static inline gpointer
_bse_datalist_id_remove_no_notify (GData **datalist,
				   GQuark  key_id)
{
  gpointer ret_data = g_datalist_id_get_data (datalist, key_id);
  g_datalist_id_remove_no_notify (datalist, key_id);
  return ret_data;
}
#define g_datalist_id_remove_no_notify _bse_datalist_id_remove_no_notify     
#endif /* !GLIB_CHECK_VERSION (1, 3, 1) */

#ifdef  __GNUC__
#define G_STRLOC	__FILE__ ":" G_STRINGIFY (__LINE__) ":" __PRETTY_FUNCTION__ "()"
#else
#define G_STRLOC	__FILE__ ":" G_STRINGIFY (__LINE__)
#endif




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GLIB_EXTRA_H__ */
