/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bsestorage.h: BSE storage for (re-)storing objects and certain values
 */
#ifndef	__BSE_STORAGE_H__
#define	__BSE_STORAGE_H__

#include	<bse/bseobject.h>
#include	<gsl/gsldefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- macros --- */
#define	BSE_IS_STORAGE(st)		(((BseStorage*) (st)) != NULL)
#define	BSE_STORAGE_FLAGS(st)		(((BseStorage*) (st))->flags)
#define	BSE_STORAGE_SET_FLAGS(st,f)	(BSE_STORAGE_FLAGS (st) |= (f))
#define	BSE_STORAGE_UNSET_FLAGS(st,f)	(BSE_STORAGE_FLAGS (st) &= ~(f))
#define	BSE_STORAGE_READABLE(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_FLAG_READABLE) != 0)
#define	BSE_STORAGE_WRITABLE(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_FLAG_WRITABLE) != 0)
#define	BSE_STORAGE_NEEDS_BREAK(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_FLAG_NEEDS_BREAK) != 0)
#define	BSE_STORAGE_AT_BOL(st)		((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_FLAG_AT_BOL) != 0)
#define	BSE_STORAGE_PUT_DEFAULTS(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_FLAG_PUT_DEFAULTS) != 0)
#define	BSE_STORAGE_SELF_CONTAINED(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_FLAG_SELF_CONTAINED) != 0)

/* --- BseStorage flags --- */
typedef enum			/*< skip >*/
{
  BSE_STORAGE_FLAG_READABLE	  = 1 << 0,
  BSE_STORAGE_FLAG_WRITABLE	  = 1 << 1,
  BSE_STORAGE_FLAG_NEEDS_BREAK	  = 1 << 2,
  BSE_STORAGE_FLAG_AT_BOL	  = 1 << 3,
  BSE_STORAGE_FLAG_PUT_DEFAULTS	  = 1 << 4,
  BSE_STORAGE_FLAG_SELF_CONTAINED = 1 << 5
} BseStorageFlags;


/* --- BseStorage --- */
typedef struct _BseStorageBBlock  BseStorageBBlock;
struct _BseStorage
{
  BseStorageFlags	 flags;
  guint			 indent_width;
  
  /* reading */
  GScanner		*scanner;
  gint			 fd;
  glong			 bin_offset;
  gpointer		 resolver_fd;
  BsePathResolver	 resolver;
  gpointer		 resolver_data;
  
  /* writing */
  GSList		*indent;
  BseStorageBBlock	*wblocks;	/* keeps ref */
  GString		*gstring;
};


/* --- prototypes -- */
BseStorage*	bse_storage_new			(void);
BseStorage*	bse_storage_from_scanner	(GScanner	*scanner);
void		bse_storage_destroy		(BseStorage	*storage);
void		bse_storage_prepare_write	(BseStorage	*storage,
						 gboolean        store_defaults);
BseErrorType	bse_storage_input_file		(BseStorage	*storage,
						 const gchar	*file_name);
void		bse_storage_reset		(BseStorage	*storage);


/* --- writing --- */
void		bse_storage_push_level		(BseStorage	*storage);
void		bse_storage_pop_level		(BseStorage	*storage);
void		bse_storage_puts		(BseStorage	*storage,
						 const gchar	*string);
void		bse_storage_putc		(BseStorage	*storage,
						 gchar		 character);
void		bse_storage_printf		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2, 3);
void		bse_storage_handle_break	(BseStorage	*storage);
void		bse_storage_break		(BseStorage	*storage);
void		bse_storage_needs_break		(BseStorage	*storage);
void		bse_storage_put_param		(BseStorage	*storage,
						 GValue		*value,
						 GParamSpec	*pspec);
void		bse_storage_put_wave_handle	(BseStorage	*storage,
						 guint		 significant_bits,
						 GslDataHandle	*handle,
						 GslLong	 voffset,
						 GslLong	 vlength);
void		bse_storage_flush_fd		(BseStorage	*storage,
						 gint		 fd);
     

/* --- reading --- */
void		bse_storage_set_path_resolver	(BseStorage	*storage,
						 BsePathResolver resolver,
						 gpointer        func_data);
gboolean	bse_storage_input_eof		(BseStorage	*storage);
void		bse_storage_error		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
void		bse_storage_unexp_token		(BseStorage	*storage,
						 GTokenType	 expected_token);
void		bse_storage_warn		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
GTokenType	bse_storage_warn_skip		(BseStorage	*storage,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
GTokenType	bse_storage_skip_statement	(BseStorage	*storage);
GTokenType	bse_storage_parse_rest		(BseStorage     *storage,
						 BseTryStatement try_statement,
						 gpointer        func_data,
						 gpointer        user_data);
GTokenType	bse_storage_parse_note		(BseStorage	*storage,
						 gint		*note,
						 gchar           bbuffer[BSE_BBUFFER_SIZE]);
GTokenType	bse_storage_parse_wave_handle	(BseStorage	*storage,
						 GslDataHandle **data_handle_p);
GTokenType	bse_storage_parse_param_value	(BseStorage	*storage,
						 GValue		*value,
						 GParamSpec	*pspec);


/* --- helpers --- */
#define bse_storage_scanner_parse_or_return(scanner, token)  G_STMT_START{ \
  guint _t = (token); \
  if (g_scanner_get_next_token (scanner) != _t) \
    return _t; \
}G_STMT_END
#define bse_storage_scanner_peek_or_return(scanner, token)   G_STMT_START{ \
  GScanner *__s = (scanner); guint _t = (token); \
  if (g_scanner_peek_next_token (__s) != _t) { \
    g_scanner_get_next_token (__s); /* advance position for error-handler */ \
    return _t; \
  } \
}G_STMT_END

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_STORAGE_H__ */
