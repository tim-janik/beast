/* SFI - Synthesis Fusion Kit Interface
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
#ifndef __SFI_PRIMITIVES_H__
#define __SFI_PRIMITIVES_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS


/* --- SfiBBlock primitive type --- */
struct _SfiBBlock {
  guint   ref_count;
  guint   n_bytes;
  guint8 *bytes;
};
SfiBBlock* sfi_bblock_new	   (void);
SfiBBlock* sfi_bblock_new_sized	   (guint	     size);
SfiBBlock* sfi_bblock_ref	   (SfiBBlock	    *bblock);
void	   sfi_bblock_unref	   (SfiBBlock	    *bblock);
void	   sfi_bblock_resize	   (SfiBBlock       *bblock,
				    guint            size);
SfiBBlock* sfi_bblock_copy_deep	   (const SfiBBlock *bblock);
#define	   sfi_bblock_copy_shallow sfi_bblock_ref
void	   sfi_bblock_append	   (SfiBBlock	    *bblock,
				    guint            n_bytes,
				    const guint8    *bytes);
void	   sfi_bblock_append1	   (SfiBBlock	    *bblock,
				    guint8	     byte0);
guint	   sfi_bblock_length	   (const SfiBBlock *bblock);
guint8*	   sfi_bblock_get	   (const SfiBBlock *bblock);


/* --- SfiFBlock primitive type --- */
struct _SfiFBlock {
  guint     ref_count;
  guint     n_values;
  gfloat   *values;
  GFreeFunc freefunc;
};
SfiFBlock* sfi_fblock_new	   (void);
SfiFBlock* sfi_fblock_new_sized	   (guint	     size);
SfiFBlock* sfi_fblock_new_foreign  (guint            n_values,
                                    gfloat          *values,
                                    GFreeFunc        freefunc);
SfiFBlock* sfi_fblock_ref	   (SfiFBlock	    *fblock);
void	   sfi_fblock_unref	   (SfiFBlock	    *fblock);
void	   sfi_fblock_resize	   (SfiFBlock	    *fblock,
				    guint	     size);
SfiFBlock* sfi_fblock_copy_deep	   (const SfiFBlock *fblock);
#define	   sfi_fblock_copy_shallow sfi_fblock_ref
void	   sfi_fblock_append	   (SfiFBlock	    *fblock,
				    guint            n_values,
				    const gfloat    *values);
void	   sfi_fblock_append1	   (SfiFBlock	    *fblock,
				    gfloat	     float0);
guint	   sfi_fblock_length	   (const SfiFBlock *fblock);
gfloat*	   sfi_fblock_get	   (const SfiFBlock *fblock);


/* --- SfiSeq primitive type --- */
struct _SfiSeq {
  guint   ref_count;
  guint   n_elements;
  GValue *elements;
};
SfiSeq*	 sfi_seq_new		(void);
SfiSeq*	 sfi_seq_ref		(SfiSeq		 *seq);
void	 sfi_seq_unref		(SfiSeq		 *seq);
SfiSeq*	 sfi_seq_copy_deep	(const SfiSeq	 *seq);
#define	 sfi_seq_copy_shallow	sfi_seq_ref
void	 sfi_seq_append		(SfiSeq		 *seq,
				 const GValue	 *value);
GValue*	 sfi_seq_append_empty	(SfiSeq		 *seq,
				 GType            value_type);
void	 sfi_seq_clear		(SfiSeq		 *seq);
guint	 sfi_seq_length		(const SfiSeq	 *seq);
GValue*	 sfi_seq_get		(const SfiSeq	 *seq,
				 guint		  index);
gboolean sfi_seq_check		(SfiSeq		 *seq,
				 GType		  element_type);
gboolean sfi_seq_validate       (SfiSeq          *seq,
                                 GParamSpec      *pspec);
/* convenience */
void     sfi_seq_append_bool	(SfiSeq          *seq,
				 SfiBool	  v_bool);
void     sfi_seq_append_int	(SfiSeq          *seq,
				 SfiInt	  	  v_int);
void     sfi_seq_append_num	(SfiSeq          *seq,
				 SfiNum		  v_num);
void     sfi_seq_append_real	(SfiSeq          *seq,
				 SfiReal	  v_real);
void     sfi_seq_append_string	(SfiSeq          *seq,
				 const gchar	 *string);
void     sfi_seq_append_choice	(SfiSeq          *seq,
				 const gchar	 *choice);
void     sfi_seq_append_bblock	(SfiSeq          *seq,
				 SfiBBlock	 *bblock);
void     sfi_seq_append_fblock	(SfiSeq          *seq,
				 SfiFBlock	 *fblock);
void     sfi_seq_append_pspec	(SfiSeq          *seq,
				 GParamSpec	 *pspec);
void     sfi_seq_append_seq	(SfiSeq          *seq,
				 SfiSeq		 *v_seq);
void     sfi_seq_append_rec	(SfiSeq          *seq,
				 SfiRec		 *rec);
void     sfi_seq_append_proxy	(SfiSeq          *seq,
				 SfiProxy	  proxy);
SfiBool	     sfi_seq_get_bool	(SfiSeq		*seq,
				 guint		 index);
SfiInt	     sfi_seq_get_int	(SfiSeq		*seq,
				 guint           index);
SfiNum	     sfi_seq_get_num	(SfiSeq		*seq,
				 guint           index);
SfiReal	     sfi_seq_get_real	(SfiSeq		*seq,
				 guint           index);
const gchar* sfi_seq_get_string (SfiSeq		*seq,
				 guint           index);
const gchar* sfi_seq_get_choice (SfiSeq		*seq,
				 guint           index);
SfiBBlock*   sfi_seq_get_bblock	(SfiSeq		*seq,
				 guint           index);
SfiFBlock*   sfi_seq_get_fblock	(SfiSeq		*seq,
				 guint           index);
GParamSpec*  sfi_seq_get_pspec	(SfiSeq		*seq,
				 guint           index);
SfiSeq*	     sfi_seq_get_seq	(SfiSeq		*seq,
				 guint           index);
SfiRec*	     sfi_seq_get_rec	(SfiSeq		*seq,
				 guint           index);
SfiProxy     sfi_seq_get_proxy	(SfiSeq		*seq,
				 guint           index);
/* conversion convenience */
gchar**	     sfi_seq_to_strv	(SfiSeq		*seq);
SfiSeq*	     sfi_seq_from_strv	(gchar	       **strv);
SfiSeq*	     sfi_seq_from_cstrv	(const gchar   **strv);


/* --- SfiRec primitive type --- */
struct _SfiRec {
  guint    ref_count;
  guint    n_fields;
  guint	   sorted : 1;
  GValue  *fields;
  gchar  **field_names;
};
SfiRec*	   sfi_rec_new		(void);
SfiRec*	   sfi_rec_ref		(SfiRec		 *rec);
void	   sfi_rec_unref	(SfiRec          *rec);
SfiRec*	   sfi_rec_copy_deep	(SfiRec		 *rec);
#define	   sfi_rec_copy_shallow	sfi_rec_ref
void       sfi_rec_swap_fields  (SfiRec          *rec,
				 SfiRec		 *swapper);
gboolean   sfi_rec_validate	(SfiRec		 *rec,
				 SfiRecFields	  fields);
void       sfi_rec_clear        (SfiRec          *rec);
void       sfi_rec_set          (SfiRec          *rec,
				 const gchar     *field_name,
				 const GValue    *value);
GValue*    sfi_rec_get          (SfiRec          *rec,
				 const gchar     *field_name);
GValue*    sfi_rec_forced_get   (SfiRec          *rec,
                                 const gchar     *field_name,
                                 GType            gtype);
guint      sfi_rec_n_fields     (const SfiRec    *rec);
GValue*    sfi_rec_field        (const SfiRec    *rec,
				 guint            index);
gboolean   sfi_rec_check	(SfiRec		 *rec,
				 SfiRecFields	  rfields);
void	   sfi_rec_sort		(SfiRec          *rec);
/* convenience */
void       sfi_rec_set_bool	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiBool	  v_bool);
void       sfi_rec_set_int	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiInt	  	  v_int);
void       sfi_rec_set_num	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiNum		  v_num);
void       sfi_rec_set_real	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiReal	  v_real);
void       sfi_rec_set_string	(SfiRec          *rec,
				 const gchar     *field_name,
				 const gchar	 *string);
void       sfi_rec_set_choice	(SfiRec          *rec,
				 const gchar     *field_name,
				 const gchar	 *choice);
void       sfi_rec_set_bblock	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiBBlock	 *bblock);
void       sfi_rec_set_fblock	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiFBlock	 *fblock);
void       sfi_rec_set_pspec	(SfiRec          *rec,
				 const gchar     *field_name,
				 GParamSpec	 *pspec);
void       sfi_rec_set_seq	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiSeq		 *seq);
void       sfi_rec_set_rec	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiRec		 *v_rec);
void       sfi_rec_set_proxy	(SfiRec          *rec,
				 const gchar     *field_name,
				 SfiProxy	  proxy);
SfiBool	     sfi_rec_get_bool	(SfiRec		*rec,
				 const gchar	*field_name);
SfiInt	     sfi_rec_get_int	(SfiRec		*rec,
				 const gchar	*field_name);
SfiNum	     sfi_rec_get_num	(SfiRec		*rec,
				 const gchar	*field_name);
SfiReal	     sfi_rec_get_real	(SfiRec		*rec,
				 const gchar	*field_name);
const gchar* sfi_rec_get_string (SfiRec		*rec,
				 const gchar	*field_name);
const gchar* sfi_rec_get_choice (SfiRec		*rec,
				 const gchar	*field_name);
SfiBBlock*   sfi_rec_get_bblock	(SfiRec		*rec,
				 const gchar	*field_name);
SfiFBlock*   sfi_rec_get_fblock	(SfiRec		*rec,
				 const gchar	*field_name);
GParamSpec*  sfi_rec_get_pspec	(SfiRec		*rec,
				 const gchar	*field_name);
SfiSeq*	     sfi_rec_get_seq	(SfiRec		*rec,
				 const gchar	*field_name);
SfiRec*	     sfi_rec_get_rec	(SfiRec		*rec,
				 const gchar	*field_name);
SfiProxy     sfi_rec_get_proxy	(SfiRec		*rec,
				 const gchar	*field_name);

G_END_DECLS

#endif /* __SFI_PRIMITIVES_H__ */

/* vim:set ts=8 sts=2 sw=2: */
