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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- macros --- */
#define	BSE_IS_STORAGE(st)		(((BseStorage*) (st)) != NULL)
#define	BSE_STORAGE_FLAGS(st)		(((BseStorage*) (st))->flags)
#define	BSE_STORAGE_SET_FLAGS(st,f)	(BSE_STORAGE_FLAGS (st) |= (f))
#define	BSE_STORAGE_UNSET_FLAGS(st,f)	(BSE_STORAGE_FLAGS (st) &= ~(f))
#define	BSE_STORAGE_READABLE(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_READABLE) != 0)
#define	BSE_STORAGE_WRITABLE(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_WRITABLE) != 0)
#define	BSE_STORAGE_NEEDS_BREAK(st)	((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_NEEDS_BREAK) != 0)
#define	BSE_STORAGE_AT_BOL(st)		((BSE_STORAGE_FLAGS (st) & BSE_STORAGE_AT_BOL) != 0)

/* --- BseStorage flags --- */
typedef enum			/* <skip> */
{
  BSE_STORAGE_READABLE		= 1 << 0,
  BSE_STORAGE_WRITABLE		= 1 << 1,
  BSE_STORAGE_NEEDS_BREAK	= 1 << 2,
  BSE_STORAGE_AT_BOL		= 1 << 3
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
  BseStorageBBlock	*rblocks;	/* keeps ref */
  gpointer		 resolver_fd;
  BsePathResolver	 resolver;
  gpointer		 resolver_data;
  
  /* writing */
  GSList		*indent;
  BseStorageBBlock	*wblocks;	/* keeps ref */
  GString		*gstring;
};
struct _BseStorageBBlock
{
  BseBinData	   *bdata;
  BseStorageBBlock *next;
  guint		    offset;
  guint		    length;
};


/* --- prototypes -- */
BseStorage*	bse_storage_new			(void);
BseStorage*	bse_storage_from_scanner	(GScanner	*scanner);
void		bse_storage_destroy		(BseStorage	*storage);
void		bse_storage_prepare_write	(BseStorage	*storage);
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
						 BseParam	*param);
void		bse_storage_put_bin_data	(BseStorage	*storage,
						 BseBinData	*bdata);
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
						 guint		*note,
						 gchar           bbuffer[BSE_BBUFFER_SIZE]);
GTokenType	bse_storage_parse_bin_data	(BseStorage	*storage,
						 BseBinData    **bdata_p);
GTokenType	bse_storage_parse_param_value	(BseStorage	*storage,
						 BseParam	*param);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_STORAGE_H__ */
