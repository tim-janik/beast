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
#include	"bsetext.h"


/* --- functions --- */
BSE_BUILTIN_TYPE (BseText)
{
  BseTypeInfo text_info = {
    sizeof (BseTextInterface),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    
    /* non interface type stuff */
    NULL, NULL, NULL, 0, 0, NULL,
  };
  
  return bse_type_register_static (BSE_TYPE_INTERFACE,
				   "BseText",
				   "Interface type for text related streams",
				   &text_info);
}

typedef struct _BseTextData BseTextData;
struct _BseTextData
{
  BseTextFlags flags;
  GSList *indent_list;
};

static void
bse_text_data_free (gpointer fdata)
{
  BseTextData *data = fdata;
  GSList *slist;
  
  for (slist = data->indent_list; slist; slist = slist->next)
    g_free (slist->data);
  g_slist_free (data->indent_list);
  g_free (data);
}

static gpointer
bse_text_data_new (BseObject *object)
{
  BseTextData *data;
  
  data = g_new (BseTextData, 1);
  data->flags = BSE_TEXTF_AT_BOL;
  data->indent_list = NULL;
  
  return data;
}

void
bse_text_set_flag (BseText     *text,
		   gboolean     set_unset,
		   BseTextFlags flag)
{
  BseTextData *data;
  
  g_return_if_fail (BSE_IS_TEXT (text));
  
  data = bse_object_ensure_interface_data (BSE_OBJECT (text), BSE_TYPE_TEXT,
					   bse_text_data_new, bse_text_data_free);
  
  if (set_unset)
    data->flags |= flag;
  else
    data->flags &= ~flag;
}

BseTextFlags
bse_text_flags (BseText *text)
{
  BseTextData *data;
  
  g_return_val_if_fail (BSE_IS_TEXT (text), 0);
  
  data = bse_object_ensure_interface_data (BSE_OBJECT (text), BSE_TYPE_TEXT,
					   bse_text_data_new, bse_text_data_free);
  
  return data->flags;
}

void
bse_text_push_indent (BseText     *text,
		      const gchar *indent)
{
  BseTextData *data;
  
  g_return_if_fail (BSE_IS_TEXT (text));
  
  data = bse_object_ensure_interface_data (BSE_OBJECT (text), BSE_TYPE_TEXT,
					   bse_text_data_new, bse_text_data_free);
  
  data->indent_list = g_slist_prepend (data->indent_list,
				       g_strconcat ((data->indent_list ?
						     data->indent_list->data : ""),
						    indent,
						    NULL));
}

void
bse_text_pop_indent (BseText *text)
{
  BseTextData *data;
  
  g_return_if_fail (BSE_IS_TEXT (text));
  
  data = bse_object_ensure_interface_data (BSE_OBJECT (text), BSE_TYPE_TEXT,
					   bse_text_data_new, bse_text_data_free);
  
  if (data->indent_list)
    {
      GSList *tmp;
      
      tmp = data->indent_list;
      data->indent_list = data->indent_list->next;
      g_free (tmp->data);
      g_slist_free_1 (tmp);
    }
}

const gchar*
bse_text_get_indent (BseText *text)
{
  BseTextData *data;
  
  g_return_val_if_fail (BSE_IS_TEXT (text), NULL);
  
  data = bse_object_ensure_interface_data (BSE_OBJECT (text), BSE_TYPE_TEXT,
					   bse_text_data_new, bse_text_data_free);
  
  return data->indent_list ? data->indent_list->data : NULL;
}

void
bse_text_puts (BseText     *text,
	       const gchar *string)
{
  g_return_if_fail (BSE_IS_TEXT (text));
  
  if (string)
    {
      guint l;
      BseTextInterface *iface;
      
      l = strlen (string);
      iface = BSE_TEXT_GET_INTERFACE (text);

      BSE_NOTIFY (text, write_chars, NOTIFY (OBJECT, l, string, DATA));
      iface->write_chars (text, l, string);
      
      if (string[l - 1] == '\n')
	BSE_TEXT_SET_FLAG (text, AT_BOL);
      else
	BSE_TEXT_UNSET_FLAG (text, AT_BOL);
    }
}

void
bse_text_putc (BseText   *text,
	       gchar	  character)
{
  BseTextInterface *iface;
  
  g_return_if_fail (BSE_IS_TEXT (text));
  
  iface = BSE_TEXT_GET_INTERFACE (text);
  
  BSE_NOTIFY (text, write_chars, NOTIFY (OBJECT, 1, &character, DATA));
  iface->write_chars (text, 1, &character);
  
  if (character == '\n')
    BSE_TEXT_SET_FLAG (text, AT_BOL);
  else
    BSE_TEXT_UNSET_FLAG (text, AT_BOL);
}

void
bse_text_printf (BseText	*text,
		 const gchar *format,
		 ...)
{
  gchar *buffer;
  va_list args;
  
  g_return_if_fail (BSE_IS_TEXT (text));
  g_return_if_fail (format != NULL);
  
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  
  bse_text_puts (text, buffer);
  
  g_free (buffer);
}

void
bse_text_indent (BseText *text)
{
  g_return_if_fail (BSE_IS_TEXT (text));
  
  bse_text_puts (text, bse_text_get_indent (text));
}

void
bse_text_break (BseText *text)
{
  g_return_if_fail (BSE_IS_TEXT (text));
  
  bse_text_putc (text, '\n');
  BSE_TEXT_UNSET_FLAG (text, NEEDS_BREAK);
  bse_text_indent (text);
}

void
bse_text_handle_break (BseText *text)
{
  g_return_if_fail (BSE_IS_TEXT (text));
  
  if (BSE_TEXT_NEEDS_BREAK (text))
    bse_text_break (text);
}

void
bse_text_needs_break (BseText *text)
{
  g_return_if_fail (BSE_IS_TEXT (text));
  
  BSE_TEXT_SET_FLAG (text, NEEDS_BREAK);
}

void
bse_text_put_param (BseText *text,
		    BseParam  *param)
{
  g_return_if_fail (BSE_IS_TEXT (text));
  
  bse_text_putc (text, '(');
  bse_text_puts (text, param->pspec->any.name);
  bse_text_putc (text, ' ');
  
  switch (BSE_FUNDAMENTAL_TYPE (param->pspec->type))
    {
      gchar *string;
      BseEnumValue *ev;
      BseFlagsValue *fv;
      
    case BSE_TYPE_PARAM_BOOL:
      /* bse_text_puts (text, 
       *                (param->value.v_bool ?
       *	         param->pspec->s_bool.true_identifier :
       *	         param->pspec->s_bool.false_identifier));
       */
      bse_text_puts (text, param->value.v_bool ? "'t" : "'f");
      break;
    case BSE_TYPE_PARAM_INT:
      bse_text_printf (text, "%d", param->value.v_int);
      break;
    case BSE_TYPE_PARAM_UINT:
      bse_text_printf (text, "%u", param->value.v_uint);
      break;
    case BSE_TYPE_PARAM_ENUM:
      ev = bse_enum_get_value (param->pspec->s_enum.enum_class, param->value.v_enum);
      if (ev)
	bse_text_puts (text, ev->value_nick);
      else
	bse_text_printf (text, "%d", param->value.v_enum);
      break;
    case BSE_TYPE_PARAM_FLAGS:
      fv = bse_flags_get_first_value (param->pspec->s_flags.flags_class, param->value.v_flags);
      if (fv)
	{
	  guint value;
	  guint i = 0;
	  
	  value = param->value.v_flags & ~fv->value;
	  while (fv)
	    {
	      value &= ~fv->value;
	      
	      if (i++)
		bse_text_putc (text, ' ');
	      
	      bse_text_puts (text, fv->value_nick);
	      
	      fv = bse_flags_get_first_value (param->pspec->s_flags.flags_class, value);
	    }
	  if (value)
	    bse_text_printf (text, " %u", value);
	}
      else
	bse_text_printf (text, "%u", param->value.v_flags);
      break;
    case BSE_TYPE_PARAM_FLOAT:
      bse_text_printf (text, "%f", param->value.v_float);
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      bse_text_printf (text, "%e", param->value.v_double);
      break;
    case BSE_TYPE_PARAM_TIME:
      string = bse_time_to_str (bse_time_to_gmt (param->value.v_time));
      bse_text_putc (text, '"');
      bse_text_puts (text, string);
      bse_text_putc (text, '"');
      g_free (string);
      break;
    case BSE_TYPE_PARAM_NOTE:
      string = bse_note_to_string (param->value.v_note);
      bse_text_putc (text, '\'');
      bse_text_puts (text, string);
      g_free (string);
      break;
    case BSE_TYPE_PARAM_STRING:
      if (param->value.v_string)
	{
	  string = g_strdup_quoted (param->value.v_string);
	  bse_text_puts (text, string);
	  g_free (string);
	}
      else
	bse_text_puts (text, "nil");
      break;
    default:
      bse_text_putc (text, '?');
      g_warning ("invalid parameter type `%s' (%d)",
		 bse_type_name (param->pspec->type),
		 param->pspec->type);
      break;
    }
  
  bse_text_putc (text, ')');
}
