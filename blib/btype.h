/* BLib - BSE/BSI helper library
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
 *
 * btype.h: BLib types
 */
#ifndef __B_TYPE_H__
#define __B_TYPE_H__

#include        <blib/glib-includes.h>	// FIXME: <glib.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- typedefs --- */
typedef gulong		BTime;
typedef struct _BDot	BDot;
typedef	guint		BCid;	/* component ids */


/* --- BLib types --- */
extern GType B_TYPE_PARAM_INT;
extern GType B_TYPE_PARAM_UINT;
extern GType B_TYPE_PARAM_FLOAT;
extern GType B_TYPE_PARAM_DOUBLE;
extern GType B_TYPE_PARAM_TIME;
extern GType B_TYPE_PARAM_NOTE;
extern GType B_TYPE_PARAM_DOTS;


/* --- param flags --- */
typedef enum
{
  B_PARAM_READABLE	      = G_PARAM_READABLE,
  B_PARAM_WRITABLE	      = G_PARAM_WRITABLE,
  B_PARAM_MASK		      = G_PARAM_MASK,	/* 0x000f */

  /* intention */
  B_PARAM_SERVE_GUI           = 1 << 4,
  B_PARAM_SERVE_STORAGE       = 1 << 5,
  B_PARAM_SERVE_MASK          = 0x00f0,

  /* GUI hints */
  B_PARAM_HINT_RDONLY         = 1 <<  8,
  B_PARAM_HINT_RADIO          = 1 <<  9,
  B_PARAM_HINT_SCALE          = 1 << 10,
  B_PARAM_HINT_DIAL           = 1 << 11,
  B_PARAM_HINT_CHECK_NULL     = 1 << 12,
  B_PARAM_HINT_MASK           = 0xff00,

  /* aliases */
  B_PARAM_READWRITE   = B_PARAM_READABLE | B_PARAM_WRITABLE,
  B_PARAM_GUI         = B_PARAM_READWRITE | B_PARAM_SERVE_GUI,
  B_PARAM_STORAGE     = B_PARAM_READWRITE | B_PARAM_SERVE_STORAGE,
  B_PARAM_DEFAULT     = B_PARAM_GUI | B_PARAM_STORAGE,
  B_PARAM_PROCEDURE   = B_PARAM_DEFAULT
} BParamFlags;


/* --- Sequential param ids --- */
typedef enum
{
  B_SEQ_PARAM_BOOL	=	G_TYPE_PARAM_BOOL,
  B_SEQ_PARAM_INT	=	G_TYPE_PARAM_INT,
  B_SEQ_PARAM_UINT	=	G_TYPE_PARAM_UINT,
  B_SEQ_PARAM_ENUM	=	G_TYPE_PARAM_ENUM,
  B_SEQ_PARAM_FLAGS	=	G_TYPE_PARAM_FLAGS,
  B_SEQ_PARAM_FLOAT	=	G_TYPE_PARAM_FLOAT,
  B_SEQ_PARAM_DOUBLE	=	G_TYPE_PARAM_DOUBLE,
  B_SEQ_PARAM_STRING	=	G_TYPE_PARAM_STRING,
  B_SEQ_PARAM_OBJECT	=	G_TYPE_PARAM_OBJECT,
  B_SEQ_PARAM_TIME,
  B_SEQ_PARAM_NOTE,
  B_SEQ_PARAM_DOTS
} BSeqParamType;


/* --- prototypes --- */
guint	b_seq_param_from_type	(GType	param_type);


/* --- implementation details --- */
extern const gchar *blib_log_domain_blib;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __B_TYPE_H__ */

