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

/* collect a single argument value from a va_list.
 * this is implemented as a huge macro <shrug>, because we can't
 * pass va_list variables by reference on some systems.
 * the corresponding prototype would be:
 * static inline gchar*
 * bse_param_collect_value (BseParam *param,
 *		            va_list   var_args);
 */
/* beware that BSE_PARAM_COLLECT_VALUE() will not free the
 * old value in the param, so call bse_param_free_value() in
 * advance if desired.
 */
#define	BSE_PARAM_COLLECT_VALUE(param, var_args, _error)	\
G_STMT_START { \
  gchar *__error_msg; \
  BseType fundamental_type; \
\
  fundamental_type = BSE_FUNDAMENTAL_TYPE ((param)->pspec->type); \
\
  __error_msg = NULL; \
  switch (fundamental_type) \
    { \
    case BSE_TYPE_INVALID: \
      __error_msg = g_strdup ("invalid untyped argument"); \
      break; \
\
    case BSE_TYPE_NONE: \
      /* we just ignore this type, since it arithmetically just requires \
       * us to not move the var_args pointer any further. callers need to \
       * check for the validity of BSE_TYPE_NONE themselves. \
       * \
       * __error_msg = g_strdup ("invalid argument type `void'"); \
       */ \
      break; \
\
      /* everything smaller than an int is guarranteed to be \
       * passed as an int \
       */ \
    case BSE_TYPE_PARAM_BOOL: \
      (param)->value.v_bool = va_arg ((var_args), gint); \
      break; \
    case BSE_TYPE_PARAM_INT: \
      (param)->value.v_int = va_arg ((var_args), gint); \
      break; \
    case BSE_TYPE_PARAM_UINT: \
      (param)->value.v_uint = va_arg ((var_args), guint); \
      break; \
    case BSE_TYPE_PARAM_ENUM: \
      (param)->value.v_enum = va_arg ((var_args), gint); \
      break; \
    case BSE_TYPE_PARAM_FLAGS: \
      (param)->value.v_flags = va_arg ((var_args), guint); \
      break; \
    case BSE_TYPE_PARAM_NOTE: \
      (param)->value.v_note = va_arg ((var_args), guint); \
      break; \
    case BSE_TYPE_PARAM_INDEX_2D: \
      (param)->value.v_index_2d = va_arg ((var_args), BseIndex2D); \
      break; \
\
      /* floats are always passed as doubles \
       */ \
    case BSE_TYPE_PARAM_FLOAT: \
      (param)->value.v_float = va_arg ((var_args), gdouble); \
      break; \
    case BSE_TYPE_PARAM_DOUBLE: \
      (param)->value.v_double = va_arg ((var_args), gdouble); \
      break; \
\
      /* we collect longs as glongs since they differ in size with \
       * integers on some platforms \
       */ \
    case BSE_TYPE_PARAM_TIME: \
      (param)->value.v_time = va_arg ((var_args), BseTime); \
      break; \
\
      /* collect pointer values \
       */ \
    case BSE_TYPE_PARAM_STRING: \
      (param)->value.v_string = g_strdup (va_arg ((var_args), gchar*)); \
      break; \
    case BSE_TYPE_PARAM_DOTS: \
      (param)->value.v_dots = g_memdup (va_arg ((var_args), BseDot*), \
                                        sizeof (BseDot) * (param)->pspec->s_dots.n_dots); \
      break; \
    case BSE_TYPE_PARAM_ITEM: \
      (param)->value.v_item = va_arg ((var_args), BseItem*); \
      if ((param)->value.v_item) \
        bse_object_ref (BSE_OBJECT ((param)->value.v_item)); \
      break; \
\
    default: \
      __error_msg = g_strconcat ("unsupported argument type `", \
			         bse_type_name ((param)->pspec->type), \
			         "'", \
			         NULL); \
      break; \
    } \
   \
  (_error) = __error_msg; /* return __error_msg; */ \
} G_STMT_END
