/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_TYPE_H__
#define __BSE_TYPE_H__

#include	<bse/bsedefs.h>

G_BEGIN_DECLS

/* --- typedefs --- */
#define BSE_TYPE_PROCEDURE	G_TYPE_MAKE_FUNDAMENTAL (G_TYPE_RESERVED_BSE_FIRST + 3)

/* type macros
 */
#define	BSE_TYPE_IS_PROCEDURE(type)	(G_TYPE_FUNDAMENTAL (type) == BSE_TYPE_PROCEDURE)
#define	BSE_CLASS_NAME(class)		(g_type_name (G_TYPE_FROM_CLASS (class)))
#define	BSE_CLASS_TYPE(class)		(G_TYPE_FROM_CLASS (class))
#define	BSE_TYPE_IS_OBJECT(type)	(g_type_is_a ((type), BSE_TYPE_OBJECT))

/* --- extra types --- */
extern GType bse_type_id_packed_pointer;
#define BSE_TYPE_PACKED_POINTER (bse_type_id_packed_pointer)


/* --- prototypes --- */
void         bse_type_init                      (void);
void         bse_type_add_options               (GType               type,
                                                 const gchar        *options);
const gchar* bse_type_get_options               (GType               type);
void         bse_type_add_blurb                 (GType               type,
                                                 const gchar        *blurb,
						 const gchar        *file,
						 guint         	     line);
const gchar* bse_type_get_blurb                 (GType               type);
const gchar* bse_type_get_file                  (GType               type);
guint        bse_type_get_line                  (GType               type);
void         bse_type_add_authors               (GType               type,
                                                 const gchar        *authors);
const gchar* bse_type_get_authors               (GType               type);
void         bse_type_add_license               (GType               type,
                                                 const gchar        *license);
const gchar* bse_type_get_license               (GType               type);
GType        bse_type_register_static           (GType               parent_type,
                                                 const gchar        *type_name,
                                                 const gchar        *type_blurb,
						 const gchar        *file,
						 guint         	     line,
                                                 const GTypeInfo    *info);
GType        bse_type_register_abstract         (GType               parent_type,
                                                 const gchar        *type_name,
                                                 const gchar        *type_blurb,
						 const gchar        *file,
						 guint         	     line,
                                                 const GTypeInfo    *info);
GType        bse_type_register_dynamic          (GType               parent_type,
                                                 const gchar        *type_name,
                                                 GTypePlugin        *plugin);
GType        bse_type_register_loadable_boxed   (BseExportNodeBoxed *bnode,
                                                 GTypePlugin        *plugin);
void         bse_type_reinit_boxed              (BseExportNodeBoxed *bnode);
void         bse_type_uninit_boxed              (BseExportNodeBoxed *bnode);


/* --- implementation details --- */

/* magic macros to define type initialization function within
 * .c files. they identify builtin type functions for magic post
 * processing and help resolving runtime type id retrival.
 */
#define	BSE_TYPE_ID(BseTypeName)	(bse_type_builtin_id_##BseTypeName)
#ifdef BSE_COMPILATION
#  define BSE_BUILTIN_PROTO(BseTypeName) GType bse_type_builtin_register_##BseTypeName (void)
#  define BSE_BUILTIN_TYPE(BseTypeName)	 EXTERN_C BSE_BUILTIN_PROTO (BseTypeName); \
                                         EXTERN_C GType bse_type_builtin_register_##BseTypeName (void)
#  define BSE_DUMMY_TYPE(BseTypeName)	 EXTERN_C BSE_BUILTIN_PROTO (BseTypeName) { return 0; } \
                                         EXTERN_C BSE_BUILTIN_PROTO (BseTypeName)
#endif /* BSE_COMPILATION */


/* --- customized pspec constructors --- */
GParamSpec*     bse_param_spec_enum (const gchar    *name,
                                     const gchar    *nick,
                                     const gchar    *blurb,
                                     gint            default_value, /* can always be 0 */
                                     GType           enum_type,
                                     const gchar    *hints);


/* -- auto generated type ids --- */
#include        <bse/bsegentypes.h>


/* --- dynamic config --- */
#define BSE_GCONFIG(cfg) (bse_global_config->cfg)
extern BseGConfig        *bse_global_config;    /* from bsegconfig.[hc] */


/* --- provide IDL pspec initializers --- */
#define sfidl_pspec_Bool(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_bool (name, nick, blurb, dflt, options), group)
#define sfidl_pspec_Bool_default(group, locfile, locline, name) \
  sfi_pspec_set_group (sfi_pspec_bool (name, NULL, NULL, FALSE, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_Trigger(group, locfile, locline, name, nick, blurb, options) \
  sfi_pspec_set_group (sfi_pspec_bool (name, nick, blurb, FALSE, "trigger:skip-undo:" options), group)
#define sfidl_pspec_Int(group, locfile, locline, name, nick, blurb, dflt, min, max, step, options) \
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, min, max, step, options), group)
#define sfidl_pspec_Int_default(group, locfile, locline, name) \
  sfi_pspec_set_group (sfi_pspec_int (name, NULL, NULL, 0, G_MININT, G_MAXINT, 256, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_Num(group, locfile, locline, name, nick, blurb, dflt, min, max, step, options) \
  sfi_pspec_set_group (sfi_pspec_num (name, nick, blurb, dflt, min, max, step, options), group)
#define sfidl_pspec_Num_default(group, locfile, locline, name) \
  sfi_pspec_set_group (sfi_pspec_num (name, NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1000, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_UInt(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, 0, G_MAXINT, 1, options), group)
#define sfidl_pspec_Real(group, locfile, locline, name, nick, blurb, dflt, min, max, step, options) \
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, min, max, step, options), group)
#define sfidl_pspec_Real_default(group, locfile, locline, name) \
  sfi_pspec_set_group (sfi_pspec_real (name, NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 10, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_Perc(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, 0.0, 100.0, 5.0, "scale:" options), group)
#define sfidl_pspec_DBVolume(group, locfile, locline, name, nick, blurb, dfltdb, mindb, maxdb, options) \
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, bse_db_to_factor (dfltdb), bse_db_to_factor (mindb), bse_db_to_factor (maxdb), \
                                       bse_db_to_factor (MIN (0.5, ABS (maxdb - mindb) / 10.0)), "scale:db-volume:" options), group)
#define sfidl_pspec_Balance(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, -100.0, +100.0, 5.0, "scale:" options), group)
#define sfidl_pspec_Note(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_note (name, nick, blurb, dflt, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, options), group)
#define sfidl_pspec_Octave(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, dflt, BSE_MIN_OCTAVE, BSE_MAX_OCTAVE, 4, options), group)
#define sfidl_pspec_Frequency(group, locfile, locline, name, nick, blurb, dflt, min, max, options) \
  sfi_pspec_set_group (bse_param_spec_freq (name, nick, blurb, dflt, min, max, "scale:" options), group)
#define sfidl_pspec_Freq(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfidl_pspec_Frequency (group, locfile, locline, name, nick, blurb, dflt, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY, options)
#define sfidl_pspec_Gain(group, locfile, locline, name, nick, blurb, dflt, min, max, step, options) \
  sfi_pspec_set_group (sfi_pspec_real (name, nick, blurb, dflt, min, max, step, options), group)
#define sfidl_pspec_FineTune(group, locfile, locline, name, nick, blurb, options) \
  sfi_pspec_set_group (sfi_pspec_int (name, nick, blurb, 0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10, options), group)
#define sfidl_pspec_LogScale(group, locfile, locline, name, nick, blurb, dflt, min, max, step, center, base, n_steps, options) \
  sfi_pspec_set_group (sfi_pspec_log_scale (name, nick, blurb, dflt, min, max, step, center, base, n_steps, "scale:" options), group)
#define sfidl_pspec_Choice(group, locfile, locline, name, nick, blurb, dval, options, cvalues) \
  sfi_pspec_set_group (sfi_pspec_choice (name, nick, blurb, #dval, cvalues, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_Choice_default(group, locfile, locline, name, cvalues) \
  sfidl_pspec_Choice (group, locfile, locline, name, NULL, NULL, NULL, SFI_PARAM_STANDARD, cvalues)
#define sfidl_pspec_String(group, locfile, locline, name, nick, blurb, dflt, options) \
  sfi_pspec_set_group (sfi_pspec_string (name, nick, blurb, dflt, options), group)
#define sfidl_pspec_String_default(group, locfile, locline, name) \
  sfidl_pspec_String (group, locfile, locline, name, NULL, NULL, NULL, SFI_PARAM_STANDARD)
#define sfidl_pspec_BBlock(group, locfile, locline, name, nick, blurb, options) \
  sfi_pspec_set_group (sfi_pspec_bblock (name, nick, blurb, options), group)
#define sfidl_pspec_BBlock_default(group, locfile, locline, name) \
  sfidl_pspec_BBlock (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD)
#define sfidl_pspec_FBlock(group, locfile, locline, name, nick, blurb, options) \
  sfi_pspec_set_group (sfi_pspec_fblock (name, nick, blurb, options), group)
#define sfidl_pspec_FBlock_default(group, locfile, locline, name) \
  sfidl_pspec_FBlock (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD)
#define sfidl_pspec_Rec(group, locfile, locline, name, nick, blurb, options) \
  sfi_pspec_set_group (sfi_pspec_rec_generic (name, nick, blurb, options), group)
#define sfidl_pspec_Rec_default(group, locfile, locline, name, fields) \
  sfidl_pspec_Rec (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD)
#define sfidl_pspec_Record(group, locfile, locline, name, nick, blurb, options, fields) \
  sfi_pspec_set_group (sfi_pspec_rec (name, nick, blurb, fields, options), group)
#define sfidl_pspec_Record_default(group, locfile, locline, name, fields) \
  sfidl_pspec_Record (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD, fields)
#define sfidl_pspec_Sequence(group, locfile, locline, name, nick, blurb, options, element) \
  sfi_pspec_set_group (sfi_pspec_seq (name, nick, blurb, element, options), group)
#define sfidl_pspec_Sequence_default(group, locfile, locline, name, element) \
  sfidl_pspec_Sequence (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD, element)
#define sfidl_pspec_Object_default(group, locfile, locline, name) \
  sfi_pspec_set_group (sfi_pspec_proxy (name, NULL, NULL, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_Object(group, locfile, locline, name, nick, blurb, options) \
  sfi_pspec_set_group (sfi_pspec_proxy (name, nick, blurb, options), group)
/* pspecs with GType */
#define sfidl_pspec_GEnum(group, locfile, locline, name, nick, blurb, dval, options, etype) \
  sfi_pspec_set_group (bse_param_spec_genum (name, nick, blurb, etype, dval, options), group)
#define sfidl_pspec_GEnum_default(group, locfile, locline, name, etype) \
  sfi_pspec_set_group (bse_param_spec_genum (name, NULL, NULL, etype, 0, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_BoxedRec(group, locfile, locline, name, nick, blurb, options, rectype) \
  sfi_pspec_set_group (bse_param_spec_boxed (name, nick, blurb, rectype, options), group)
#define sfidl_pspec_BoxedRec_default(group, locfile, locline, name, rectype) \
  sfidl_pspec_BoxedRec (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD, rectype)
#define sfidl_pspec_BoxedSeq(group, locfile, locline, name, nick, blurb, options, seqtype) \
  sfi_pspec_set_group (bse_param_spec_boxed (name, nick, blurb, seqtype, options), group)
#define sfidl_pspec_BoxedSeq_default(group, locfile, locline, name, seqtype) \
  sfidl_pspec_BoxedSeq (group, locfile, locline, name, NULL, NULL, SFI_PARAM_STANDARD, seqtype)
#define sfidl_pspec_TypedObject_default(group, locfile, locline, name, otype) \
  sfi_pspec_set_group (bse_param_spec_object (name, NULL, NULL, otype, SFI_PARAM_STANDARD), group)
#define sfidl_pspec_TypedObject(group, locfile, locline, name, nick, blurb, options, otype) \
  sfi_pspec_set_group (bse_param_spec_object (name, nick, blurb, otype, options), group)


G_END_DECLS

#endif /* __BSE_TYPE_H__ */
