/* BSE - Better Sound Engine
 * Copyright (C) 2002, 2003 Tim Janik
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
#include "bseglue.hh"
#include "bseparam.hh"
#include "bseitem.hh"
#include "bseprocedure.hh"
#include "bsecategories.hh"
#include "bsemain.hh"
#include <string.h>


/* --- structures --- */
typedef struct {
  uint             id;
  union {
    GSList       *list;
    uint          next_nref; /* index+1 */
  } data;
} NotifyRef;
typedef struct {
  SfiGlueContext context;
  char          *user;
  SfiUStore     *bproxies;
  SfiRing	*events;
  uint        	 n_nrefs;
  NotifyRef	*nrefs;
  uint        	 free_nref; /* index+1 */
} BContext;
typedef struct {
  GSList        *closures;
  unsigned long  release_id;
  uint           remote_watch : 1;
} BProxy;
typedef struct {
  GClosure      closure;
  GQuark        qsignal;
  unsigned long handler_id;
} BClosure;
typedef struct {
  GSource         source;
  SfiGlueDecoder *decoder;
} BSource;


/* --- prototypes --- */
static SfiGlueIFace*    bglue_describe_iface            (SfiGlueContext *context,
							 const char     *iface);
static SfiGlueProc*     bglue_describe_proc             (SfiGlueContext *context,
							 const char     *proc_name);
static char**           bglue_list_proc_names           (SfiGlueContext *context);
static char**           bglue_list_method_names         (SfiGlueContext *context,
							 const char     *iface_name);
static char*            bglue_base_iface                (SfiGlueContext *context);
static char**           bglue_iface_children            (SfiGlueContext *context,
							 const char     *iface_name);
static GValue*          bglue_exec_proc                 (SfiGlueContext *context,
							 const char     *proc_name,
							 SfiSeq         *params);
static char*            bglue_proxy_iface               (SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean         bglue_proxy_is_a                (SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *iface);
static char**		bglue_proxy_list_properties	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *first_ancestor,
							 const char     *last_ancestor);
static GParamSpec*	bglue_proxy_get_pspec		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *prop_name);
static SfiSCategory	bglue_proxy_get_pspec_scategory	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *prop_name);
static void		bglue_proxy_set_property	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *prop,
							 const GValue   *value);
static GValue*		bglue_proxy_get_property	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *prop);
static gboolean		bglue_proxy_watch_release	(SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean		bglue_proxy_request_notify	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const char     *signal,
							 gboolean        enable_notify);
static void		bglue_proxy_processed_notify	(SfiGlueContext	*context,
							 uint        	 notify_id);
static GValue*          bglue_client_msg                (SfiGlueContext *context,
							 const char     *msg,
							 GValue         *value);
static SfiRing*		bglue_fetch_events		(SfiGlueContext *context);
static SfiRing*		bglue_list_poll_fds		(SfiGlueContext *context);
static void		bglue_destroy			(SfiGlueContext *context);


/* --- variables --- */
static GQuark quark_original_enum = 0;
static GQuark quark_property_notify = 0;
static GQuark quark_notify = 0;


/* --- functions --- */
SfiGlueContext*
bse_glue_context_intern (const char *user)
{
  static const SfiGlueContextTable bse_glue_table = {
    bglue_describe_iface,
    bglue_describe_proc,
    bglue_list_proc_names,
    bglue_list_method_names,
    bglue_base_iface,
    bglue_iface_children,
    bglue_exec_proc,
    bglue_proxy_iface,
    bglue_proxy_is_a,
    bglue_proxy_list_properties,
    bglue_proxy_get_pspec,
    bglue_proxy_get_pspec_scategory,
    bglue_proxy_set_property,
    bglue_proxy_get_property,
    bglue_proxy_watch_release,
    bglue_proxy_request_notify,
    bglue_proxy_processed_notify,
    bglue_client_msg,
    bglue_fetch_events,
    bglue_list_poll_fds,
    bglue_destroy,
  };
  BContext *bcontext;

  g_return_val_if_fail (user != NULL, NULL);
  if (!quark_original_enum)
    {
      quark_original_enum = g_quark_from_static_string ("bse-glue-original-enum");
      quark_property_notify = g_quark_from_static_string ("property-notify");
      quark_notify = g_quark_from_static_string ("notify");
    }

  /* create server-side glue context */
  bcontext = g_new0 (BContext, 1);
  sfi_glue_context_common_init (&bcontext->context, &bse_glue_table);
  bcontext->user = g_strdup (user);
  bcontext->bproxies = sfi_ustore_new ();
  bcontext->events = NULL;
  bcontext->n_nrefs = 0;
  bcontext->nrefs = NULL;
  bcontext->free_nref = 0;

  return &bcontext->context;
}

static uint
bcontext_new_notify_ref (BContext *bcontext)
{
  static unsigned char rand_counter = 0;
  uint i = bcontext->free_nref;
  if (i)
    i -= 1; /* id -> index */
  else
    {
      i = bcontext->n_nrefs++;
      bcontext->nrefs = g_renew (NotifyRef, bcontext->nrefs, bcontext->n_nrefs);
      bcontext->nrefs[i].data.next_nref = 0;
    }
  bcontext->free_nref = bcontext->nrefs[i].data.next_nref;
  if (++rand_counter == 0)
    rand_counter = 1;
  bcontext->nrefs[i].id = (rand_counter << 24) | (i + 1);
  bcontext->nrefs[i].data.list = NULL;
  return bcontext->nrefs[i].id;
}

static void
bcontext_notify_ref_add_item (BContext     *bcontext,
			      uint          id,
			      BseItem      *item)
{
  uint i = (id & 0xffffff) - 1;
  if (item)
    bcontext->nrefs[i].data.list = g_slist_prepend (bcontext->nrefs[i].data.list,
						    bse_item_use (item));
}

static gboolean
bcontext_release_notify_ref (BContext     *bcontext,
			     uint          id)
{
  uint i = (id & 0xffffff) - 1;
  if (i < bcontext->n_nrefs &&
      bcontext->nrefs[i].id == id)
    {
      while (bcontext->nrefs[i].data.list)
	{
	  GSList *slist = bcontext->nrefs[i].data.list;
	  bcontext->nrefs[i].data.list = slist->next;
	  bse_item_unuse (BSE_ITEM (slist->data));
	  g_slist_free_1 (slist);
	}
      bcontext->nrefs[i].id = 0;
      bcontext->nrefs[i].data.next_nref = bcontext->free_nref;
      bcontext->free_nref = i + 1;
      return TRUE;
    }
  else
    return FALSE;	/* no such nref */
}

static void
bcontext_queue_release (BContext *bcontext,
			SfiProxy  proxy)
{
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_EVENT_RELEASE);
  sfi_seq_append_proxy (seq, proxy);
  bcontext->events = sfi_ring_append (bcontext->events, seq);
}

static void
bcontext_queue_signal (BContext     *bcontext,
		       uint          nref_id,
		       const char   *signal,
		       SfiSeq       *args)
{
  SfiSeq *seq;

  g_return_if_fail (args != NULL && args->n_elements > 0 && SFI_VALUE_HOLDS_PROXY (args->elements));

  seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_EVENT_NOTIFY);
  sfi_seq_append_string (seq, signal);
  sfi_seq_append_int (seq, nref_id);
  sfi_seq_append_seq (seq, args);
  bcontext->events = sfi_ring_append (bcontext->events, seq);
}

static GParamSpec*
bglue_pspec_to_serializable (GParamSpec *pspec)
{
  if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      GType etype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      pspec = sfi_pspec_choice_from_enum (pspec);
      g_param_spec_set_qdata (pspec, quark_original_enum, (void *) etype);
    }
  else
    pspec = sfi_pspec_to_serializable (pspec);
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  return pspec;
}

static GValue*
bglue_value_from_serializable (const GValue *svalue,
			       GParamSpec   *pspec)
{
  GType dtype = 0, stype = G_VALUE_TYPE (svalue);
  GValue *value = NULL;
  /* this corresponds with the conversions in sfi_pspec_to_serializable() */
  if (sfi_categorize_pspec (pspec))
    return NULL;
  if (SFI_VALUE_HOLDS_CHOICE (svalue) && G_IS_PARAM_SPEC_ENUM (pspec))
    {
      value = sfi_value_empty ();
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      sfi_value_choice2enum (svalue, value, pspec);
      return value;
    }
  else if (G_IS_PARAM_SPEC_BOXED (pspec) && (SFI_VALUE_HOLDS_SEQ (svalue) ||
					     SFI_VALUE_HOLDS_REC (svalue)))
    dtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  else if (SFI_VALUE_HOLDS_PROXY (svalue) && BSE_IS_PARAM_SPEC_OBJECT (pspec))
    {
      SfiProxy proxy = sfi_value_get_proxy (svalue);
      value = sfi_value_empty ();
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      bse_value_set_object (value, bse_object_from_id (proxy));
      return value;
    }
  if (dtype)
    {
      value = sfi_value_empty ();
      g_value_init (value, dtype);
    }
  if (!dtype || !sfi_value_transform (svalue, value))
    {
      if (0)
        g_printerr ("from=%s to=%s, transformable=%u\n",
                    g_type_name (G_VALUE_TYPE (svalue)),
                    g_type_name (dtype),
                    g_value_type_transformable (G_VALUE_TYPE (svalue), dtype));
      g_warning ("unable to convert to value type `%s' from serializable (`%s')",
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                 g_type_name (stype));
    }
  return value;
}

GValue*
bse_value_from_sfi (const GValue *value,
		    GParamSpec   *pspec)
{
  GValue *rvalue;

  g_return_val_if_fail (SFI_IS_VALUE (value), NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  rvalue = bglue_value_from_serializable (value, pspec);
  return rvalue ? rvalue : sfi_value_clone_shallow (value);
}

static GValue*
bglue_value_to_serializable (const GValue *svalue)
{
  GValue *value = NULL;
  GType dtype = 0, vtype = G_VALUE_TYPE (svalue);
  /* this corresponds with the conversions in sfi_pspec_to_serializable() */
  if (sfi_categorize_type (vtype))
    return sfi_value_clone_shallow (svalue);

  switch (G_TYPE_FUNDAMENTAL (vtype))
    {
      BseObject *object;
    case G_TYPE_FLOAT:
      dtype = SFI_TYPE_REAL;
      break;
    case G_TYPE_BOXED:
      {
        SfiRecFields rfields = sfi_boxed_type_get_rec_fields (vtype);
        GParamSpec *element = sfi_boxed_type_get_seq_element (vtype);
        if (rfields.n_fields)
          dtype = SFI_TYPE_REC;
        else if (element)
          dtype = SFI_TYPE_SEQ;
      }
      break;
    case G_TYPE_ENUM:
      dtype = SFI_TYPE_CHOICE;
      break;
    case G_TYPE_OBJECT:
      object = (BseObject*) bse_value_get_object (svalue);
      return sfi_value_proxy (BSE_IS_OBJECT (object) ? BSE_OBJECT_ID (object) : 0);
    }
  if (dtype)
    {
      value = sfi_value_empty ();
      g_value_init (value, dtype);
    }
  if (!dtype)
    g_warning ("unable to convert value type `%s' to serializable type",
	       g_type_name (vtype));
  else if (!sfi_value_transform (svalue, value))
    g_warning ("unable to convert value type `%s' to serializable (`%s')",
	       g_type_name (vtype), g_type_name (dtype));
  return value;
}

GValue*
bse_value_to_sfi (const GValue *value)
{
  g_return_val_if_fail (G_IS_VALUE (value), NULL);

  return bglue_value_to_serializable (value);
}

GValue*
bse_glue_boxed_to_value (GType    boxed_type,
			 void * boxed)
{
  BseGlueBoxedToRec b2rec;
  BseGlueBoxedToSeq b2seq;
  GValue *value;
  
  /* Convert a boxed value into a #SfiGlueValue (usually holding
   * either a sequence or a record). The returned value is owned
   * by the GC.
   */

  g_return_val_if_fail (G_TYPE_IS_BOXED (boxed_type) && G_TYPE_IS_DERIVED (boxed_type), NULL);
  g_return_val_if_fail (boxed != NULL, NULL);
  
  b2rec = (BseGlueBoxedToRec) g_type_get_qdata (boxed_type, g_quark_from_string ("BseGlueBoxedToRec"));
  b2seq = (BseGlueBoxedToSeq) g_type_get_qdata (boxed_type, g_quark_from_string ("BseGlueBoxedToSeq"));
  if (b2rec)
    {
      SfiRec *rec = b2rec (boxed);
      value = sfi_value_rec (rec);
      sfi_rec_unref (rec);
    }
  else if (b2seq)
    {
      SfiSeq *seq = b2seq (boxed);
      value = sfi_value_seq (seq);
      sfi_seq_unref (seq);
    }
  else /* urm, bad */
    {
      g_warning ("unable to convert boxed type `%s' to record or sequence", g_type_name (boxed_type));
      value = NULL;
    }
  return value;
}

GType
bse_glue_pspec_get_original_enum (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  return (GType) g_param_spec_get_qdata (pspec, quark_original_enum);
}

static SfiGlueIFace*
bglue_describe_iface (SfiGlueContext *context,
                      const char     *iface)
{
  GType xtype, type = g_type_from_name (iface);
  SfiGlueIFace *f;
  GObjectClass *oclass;
  GParamSpec **pspecs;
  GSList *plist = NULL;
  uint i, n;
  
  if (!G_TYPE_IS_OBJECT (type) || !g_type_is_a (type, BSE_TYPE_ITEM))
    return NULL;
  
  f = sfi_glue_iface_new (g_type_name (type));
  f->n_ifaces = g_type_depth (type) - g_type_depth (BSE_TYPE_ITEM) + 1;
  f->ifaces = g_new (char*, f->n_ifaces + 1);
  xtype = type;
  for (i = 0; i < f->n_ifaces; i++)
    {
      f->ifaces[i] = g_strdup (g_type_name (xtype));
      xtype = g_type_parent (xtype);
    }
  f->ifaces[i] = NULL;
  
  oclass = (GObjectClass*) g_type_class_ref (type);
  xtype = BSE_TYPE_ITEM;
  pspecs = g_object_class_list_properties (oclass, &n);
  f->n_props = 0;
  for (i = 0; i < n; i++)
    {
      GParamSpec *pspec = pspecs[i];
      
      if (g_type_is_a (pspec->owner_type, xtype))
        {
          plist = g_slist_prepend (plist, g_strdup (pspec->name));
          f->n_props++;
        }
    }
  g_free (pspecs);
  g_type_class_unref (oclass);
  
  i = f->n_props;
  f->props = g_new (char*, i + 1);
  f->props[i] = NULL;
  while (i--)
    {
      GSList *tmp = plist->next;
      
      f->props[i] = (char*) plist->data;
      g_slist_free_1 (plist);
      plist = tmp;
    }
  
  return f;
}

uint
bse_glue_enum_index (GType enum_type,
		     int   enum_value)
{
  GEnumClass *eclass;
  GEnumValue *ev;
  uint index;
  
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), G_MAXINT);
  g_return_val_if_fail (G_TYPE_IS_DERIVED (enum_type), G_MAXINT);
  
  eclass = (GEnumClass*) g_type_class_ref (enum_type);
  ev = g_enum_get_value (eclass, enum_value);
  if (!ev)
    sfi_diag ("%s: enum \"%s\" has no value %u", G_STRLOC, g_type_name (enum_type), enum_value);
  index = ev ? ev - eclass->values : G_MAXINT;
  g_type_class_unref (eclass);
  
  return index;
}

static SfiGlueProc*
bglue_describe_proc (SfiGlueContext *context,
                     const char     *proc_name)
{
  GType type = g_type_from_name (proc_name);
  BseProcedureClass *proc;
  SfiGlueProc *p = NULL;
  
  if (!BSE_TYPE_IS_PROCEDURE (type))
    return NULL;
  
  proc = (BseProcedureClass*) g_type_class_ref (type);
  if (proc->n_out_pspecs < 2)
    {
      uint i;
      
      p = sfi_glue_proc_new (g_type_name (type));
      p->help = g_strdup (bse_type_get_blurb (type));
      p->authors = g_strdup (bse_type_get_authors (type));
      p->license = g_strdup (bse_type_get_license (type));
      if (proc->n_out_pspecs)
	{
	  GParamSpec *pspec = bglue_pspec_to_serializable (proc->out_pspecs[0]);
	  sfi_glue_proc_add_ret_param (p, pspec);
	  g_param_spec_unref (pspec);
	}
      for (i = 0; i < proc->n_in_pspecs; i++)
	{
	  GParamSpec *pspec = bglue_pspec_to_serializable (proc->in_pspecs[i]);
	  sfi_glue_proc_add_param (p, pspec);
	  g_param_spec_unref (pspec);
	}
    }
  g_type_class_unref (proc);
  
  return p;
}

static char**
bglue_list_proc_names (SfiGlueContext *context)
{
  BseCategorySeq *cseq = bse_categories_match_typed ("/Proc/""*", BSE_TYPE_PROCEDURE);
  char **p;
  uint i;
  
  p = g_new (char*, cseq->n_cats + 1);
  for (i = 0; i < cseq->n_cats; i++)
    p[i] = g_strdup (cseq->cats[i]->type);
  p[i] = NULL;
  bse_category_seq_free (cseq);
  
  return p;
}

static char**
bglue_list_method_names (SfiGlueContext *context,
                         const char     *iface_name)
{
  GType type = g_type_from_name (iface_name);
  BseCategorySeq *cseq;
  char **p, *prefix;
  uint i, l, n_procs;
  
  if (!g_type_is_a (type, BSE_TYPE_ITEM))
    return NULL;
  
  prefix = g_strdup_printf ("%s+", g_type_name (type));
  l = strlen (prefix);
  
  cseq = bse_categories_match_typed ("/Methods/" "*", BSE_TYPE_PROCEDURE);
  p = g_new (char*, cseq->n_cats + 1);
  n_procs = 0;
  for (i = 0; i < cseq->n_cats; i++)
    if (strncmp (cseq->cats[i]->type, prefix, l) == 0)
      p[n_procs++] = g_strdup (cseq->cats[i]->type + l);
  p[n_procs] = NULL;
  bse_category_seq_free (cseq);
  g_free (prefix);
  
  return p;
}

static char*
bglue_base_iface (SfiGlueContext *context)
{
  return g_strdup ("BseItem");
}

static char**
bglue_iface_children (SfiGlueContext *context,
                      const char     *iface_name)
{
  GType type = g_type_from_name (iface_name);
  char **childnames = NULL;
  
  if (g_type_is_a (type, BSE_TYPE_ITEM))
    {
      GType *children;
      uint n;
      
      children = g_type_children (type, &n);
      childnames = g_new (char*, n + 1);
      childnames[n] = NULL;
      while (n--)
        childnames[n] = g_strdup (g_type_name (children[n]));
      g_free (children);
    }
  return childnames;
}

static BseErrorType
bglue_marshal_proc (void              *marshal_data,
		    BseProcedureClass *proc,
		    const GValue      *ivalues,
		    GValue            *ovalues)
{
  return proc->execute (proc, ivalues, ovalues);
}

static GValue*
bglue_exec_proc (SfiGlueContext *context,
		 const char     *proc_name,
		 SfiSeq         *params)
{
  GValue *retval = NULL;
  GType ptype = bse_procedure_lookup (proc_name);
  
  if (BSE_TYPE_IS_PROCEDURE (ptype) && G_TYPE_IS_DERIVED (ptype))
    {
      BseProcedureClass *proc = (BseProcedureClass*) g_type_class_ref (ptype);
      GValue *ovalues = g_new0 (GValue, proc->n_out_pspecs);
      GSList *ilist = NULL, *olist = NULL, *clearlist = NULL;
      uint i, sl = sfi_seq_length (params);
      BseErrorType error;
      
      for (i = 0; i < proc->n_in_pspecs; i++)
	{
	  GParamSpec *pspec = proc->in_pspecs[i];
	  if (i < sl)
	    {
	      GValue *sfivalue = sfi_seq_get (params, i);
	      GValue *bsevalue = bglue_value_from_serializable (sfivalue, pspec);
	      ilist = g_slist_prepend (ilist, bsevalue ? bsevalue : sfivalue);
	      if (bsevalue)
		clearlist = g_slist_prepend (clearlist, bsevalue);
	    }
	  else
	    {
	      GValue *value = sfi_value_empty ();
	      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	      g_param_value_set_default (pspec, value);
	      ilist = g_slist_prepend (ilist, value);
	      clearlist = g_slist_prepend (clearlist, value);
	    }
	}
      for (i = 0; i < proc->n_out_pspecs; i++)
	{
	  g_value_init (ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
	  olist = g_slist_prepend (olist, ovalues + i);
	}
      
      ilist = g_slist_reverse (ilist);
      olist = g_slist_reverse (olist);
      error = bse_procedure_execvl (proc, ilist, olist, bglue_marshal_proc, NULL);
      g_slist_free (ilist);
      g_slist_free (olist);
      for (ilist = clearlist; ilist; ilist = ilist->next)
	sfi_value_free ((GValue*) ilist->data);
      g_slist_free (clearlist);
      
      if (error)
        g_warning ("while executing \"%s\": %s", BSE_PROCEDURE_NAME (proc), bse_error_blurb (error));
      if (proc->n_out_pspecs)
	retval = bglue_value_to_serializable (ovalues + 0);
      for (i = 0; i < proc->n_out_pspecs; i++)
        g_value_unset (ovalues + i);
      g_free (ovalues);
      g_type_class_unref (proc);
    }
  else
    sfi_diag ("failed to execute \"%s\": no such procedure", proc_name);
  
  return retval;
}

static char*
bglue_proxy_iface (SfiGlueContext *context,
                   SfiProxy        proxy)
{
  BseObject *object = (BseObject*) bse_object_from_id (proxy);
  
  if (BSE_IS_ITEM (object))
    return g_strdup (G_OBJECT_TYPE_NAME (object));
  else
    return NULL;
}

static gboolean
bglue_proxy_is_a (SfiGlueContext *context,
		  SfiProxy        proxy,
		  const char     *iface)
{
  BseObject *object = (BseObject*) bse_object_from_id (proxy);
  GType itype = iface ? g_type_from_name (iface) : 0;

  return object && itype && g_type_is_a (G_OBJECT_TYPE (object), itype);
}

static char**
bglue_proxy_list_properties (SfiGlueContext *context,
			     SfiProxy        proxy,
			     const char     *first_ancestor,
			     const char     *last_ancestor)
{
  BseObject *object = (BseObject*) bse_object_from_id (proxy);
  char **names = NULL;

  if (BSE_IS_ITEM (object))
    {
      GType first_base_type = first_ancestor ? g_type_from_name (first_ancestor) : 0;
      GType last_base_type = last_ancestor ? g_type_from_name (last_ancestor) : 0;
      uint i, n;
      GParamSpec **pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n);
      char **p = g_new (char*, n + 1);
      names = p;
      for (i = 0; i < n; i++)
	{
	  GParamSpec *pspec = pspecs[i];
	  
	  if ((!first_base_type || g_type_is_a (pspec->owner_type, first_base_type)) &&
	      (!last_base_type || g_type_is_a (last_base_type, pspec->owner_type)))
	    *p++ = g_strdup (pspec->name);
	}
      g_free (pspecs);
      *p++ = NULL;
      names = g_renew (char*, names, p - names);
    }
  return names;
}

static GParamSpec*
bglue_proxy_get_pspec (SfiGlueContext *context,
		       SfiProxy        proxy,
		       const char     *prop_name)
{
  BseObject *object = (BseObject*) bse_object_from_id (proxy);
  GParamSpec *pspec;
  
  if (!BSE_IS_ITEM (object))
    {
      sfi_diag ("property lookup: no such object (proxy=%lu)", proxy);
      return NULL;
    }
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop_name);
  if (!pspec)
    return NULL;
  
  pspec = bglue_pspec_to_serializable (pspec);
  
  return pspec;
}

static SfiSCategory
bglue_proxy_get_pspec_scategory (SfiGlueContext *context,
				 SfiProxy        proxy,
				 const char     *prop_name)
{
  GParamSpec *pspec = bglue_proxy_get_pspec (context, proxy, prop_name);
  SfiSCategory scat = SFI_SCAT_INVAL;
  if (pspec)
    {
      scat = sfi_categorize_pspec (pspec);
      g_param_spec_unref (pspec);
    }
  return scat;
}

static void
bglue_proxy_set_property (SfiGlueContext *context,
			  SfiProxy        proxy,
			  const char     *prop,
			  const GValue   *value)
{
  void *object = bse_object_from_id (proxy);
  
  if (BSE_IS_OBJECT (object) && G_IS_VALUE (value))
    {
      GParamSpec *pspec = prop ? g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop) : NULL;
      if (pspec)
	{
	  GValue *pvalue = bglue_value_from_serializable (value, pspec);
	  GValue tmp_value = { 0, };
	  /* we do conversion and validation here, so we can roll our own warnings */
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  if (!sfi_value_transform (pvalue ? pvalue : value, &tmp_value))
	    sfi_diag ("property `%s' (%s) of \"%s\" cannot be set from value of type `%s'",
		      pspec->name,
		      g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		      bse_object_debug_name (object),
		      G_VALUE_TYPE_NAME (value));
	  else
	    {
	      /* silent validation */
	      g_param_value_validate (pspec, &tmp_value);
              if (BSE_IS_ITEM (object))
                {
                  BseUndoStack *ustack = bse_item_undo_open (object, "set-property %s", prop);
                  bse_item_set_property_undoable (BSE_ITEM (object), prop, &tmp_value);
                  bse_item_undo_close (ustack);
                }
              else
                g_object_set_property (G_OBJECT (object), prop, &tmp_value);
	    }
	  g_value_unset (&tmp_value);
	  if (pvalue)
	    sfi_value_free (pvalue);
	}
      else
	sfi_diag ("object %s has no property `%s'",
		  bse_object_debug_name (object), prop ? prop : "<NULL>");
    }
}

static GValue*
bglue_proxy_get_property (SfiGlueContext *context,
			  SfiProxy        proxy,
			  const char     *prop)
{
  GObject *object = (GObject*) bse_object_from_id (proxy);
  GValue *rvalue = NULL;
  
  if (BSE_IS_OBJECT (object) && prop)
    {
      GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop);
      
      if (pspec)
	{
	  GValue *value = sfi_value_empty ();
	  g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  g_object_get_property (object, prop, value);
	  rvalue = bglue_value_to_serializable (value);
	  sfi_value_free (value);
	}
      else
        sfi_diag ("object %s has no such property: %s", bse_object_debug_name (object), prop);
    }
  return rvalue;
}

static void
bcontext_destroy_bproxy (BContext *bcontext,
			 BProxy   *p,
			 SfiProxy  proxy,
			 BseItem  *item)
{
  sfi_ustore_remove (bcontext->bproxies, proxy); /* early unlink */
  while (p->closures)
    {
      GSList *slist = p->closures;
      GClosure *closure = (GClosure*) slist->data;
      BClosure *bclosure = (BClosure*) closure;
      p->closures = slist->next;
      g_slist_free_1 (slist);
      g_closure_invalidate (closure);
      g_signal_handler_disconnect (item, bclosure->handler_id);
      g_closure_unref (closure);
    }
  g_signal_handler_disconnect (item, p->release_id);
  g_free (p);
}

static void
bglue_bproxy_release (BseItem  *item,
		      BContext *bcontext)
{
  SfiProxy proxy = BSE_OBJECT_ID (item);
  BProxy *p = (BProxy*) sfi_ustore_lookup (bcontext->bproxies, proxy);
  // FIXME: remove only, if use_count through this context is 0
  if (p->remote_watch)
    {
      bcontext_queue_release (bcontext, proxy);
      p->remote_watch = FALSE;
    }
  bcontext_destroy_bproxy (bcontext, p, proxy, item);
}

static BProxy*
bglue_fetch_bproxy (BContext *bcontext,
		    SfiProxy  proxy,
		    BseItem  *item)
{
  BProxy *p;

  p = (BProxy*) sfi_ustore_lookup (bcontext->bproxies, proxy);
  if (!p && (item->use_count > 0 || item->parent))
    {
      p = g_new0 (BProxy, 1);
      p->release_id = g_signal_connect_data (item, "release", G_CALLBACK (bglue_bproxy_release), bcontext, NULL, G_CONNECT_AFTER);
      p->remote_watch = FALSE;
      sfi_ustore_insert (bcontext->bproxies, proxy, p);
    }
  return p;
}

static gboolean
bglue_proxy_watch_release (SfiGlueContext *context,
			   SfiProxy        proxy)
{
  BContext *bcontext = (BContext*) context;
  BseItem *item = (BseItem*) bse_object_from_id (proxy);
  BProxy *p;

  if (!BSE_IS_ITEM (item))
    return FALSE;
  p = bglue_fetch_bproxy (bcontext, proxy, item);
  if (!p)
    return FALSE;
  if (p->remote_watch)
    g_warning ("%s: redundant watch request on proxy (%lu)", bcontext->user, proxy);
  p->remote_watch = TRUE;
  return TRUE;
}

static void
bclosure_marshal (GClosure       *closure,
		  GValue         *return_value,
		  uint            n_param_values,
		  const GValue   *param_values,
		  void           *invocation_hint,
		  void           *marshal_data)
{
  BClosure *bclosure = (BClosure*) closure;
  BContext *bcontext = (BContext*) closure->data;
  const char *signal = g_quark_to_string (bclosure->qsignal);
  SfiSeq *args = sfi_seq_new ();
  uint i, nref_id = bcontext_new_notify_ref (bcontext);

  for (i = 0; i < n_param_values; i++)
    {
      GValue *value = bglue_value_to_serializable (param_values + i);
      sfi_seq_append (args, value);
      if (SFI_VALUE_HOLDS_PROXY (value))
	bcontext_notify_ref_add_item (bcontext, nref_id, (BseItem*) g_value_get_object (param_values + i));
      sfi_value_free (value);
    }
  bcontext_queue_signal (bcontext, nref_id, signal, args);
  sfi_seq_unref (args);
}

static void
bclosure_notify_marshal (GClosure       *closure,
			 GValue         *return_value,
			 uint            n_param_values,
			 const GValue   *param_values,
			 void           *invocation_hint,
			 void           *marshal_data)
{
  BClosure *bclosure = (BClosure*) closure;
  BContext *bcontext = (BContext*) closure->data;
  const char *csignal = g_quark_to_string (bclosure->qsignal);
  SfiSeq *args = sfi_seq_new ();
  BseItem *item;
  uint nref_id = bcontext_new_notify_ref (bcontext);
  GParamSpec *pspec;

  /* here we handle aliasing of ::notify to ::property_notify,
   * and provide pspec->name instead of pspec as signal argument
   */
  item = (BseItem*) g_value_get_object (param_values + 0);
  sfi_seq_append_proxy (args, BSE_OBJECT_ID (item));
  bcontext_notify_ref_add_item (bcontext, nref_id, item);
  pspec = sfi_value_get_pspec (param_values + 1);
  sfi_seq_append_string (args, pspec->name);
  char *signal = g_strconcat ("property-", csignal, NULL);
  bcontext_queue_signal (bcontext, nref_id, signal, args);
  g_free (signal);
  sfi_seq_unref (args);
}

static gboolean
bglue_proxy_request_notify (SfiGlueContext *context,
			    SfiProxy        proxy,
			    const char     *signal,
			    gboolean        enable_notify)
{
  BContext *bcontext = (BContext*) context;
  BseItem *item = (BseItem*) bse_object_from_id (proxy);
  GQuark qsignal;
  BProxy *p;
  GClosure *closure;
  BClosure *bclosure;
  GSList *slist, *last = NULL;
  GClosureMarshal sig_closure_marshal;
  char *sig_name, *c;
  uint sig_id;
  gboolean connected;

  if (!BSE_IS_ITEM (item) || !signal)
    return FALSE;
  p = bglue_fetch_bproxy (bcontext, proxy, item);
  if (!p)
    return FALSE;

  /* get canonified signal name quark */
  qsignal = sfi_glue_proxy_get_signal_quark (signal);

  /* special case ::notify, which we don't export through the glue layer */
  if (qsignal == quark_notify || strncmp (signal, "notify:", 7) == 0)
    return FALSE;
  /* special case ::property-notify, which we implement on top of ::notify */
  if (qsignal == quark_property_notify || (strncmp (signal, "property", 8) == 0 &&
					   (signal[8] == '_' || signal[8] == '-') &&
					   strncmp (signal + 9, "notify:", 7) == 0))
    {
      signal += 9; /* skip "property-" */
      qsignal = sfi_glue_proxy_get_signal_quark (signal);
      sig_closure_marshal = bclosure_notify_marshal;
    }
  else
    sig_closure_marshal = bclosure_marshal;
  
  /* canonify signal name */
  signal = g_quark_to_string (qsignal);
  
  for (slist = p->closures; slist; last = slist, slist = last->next)
    {
      bclosure = (BClosure*) slist->data;
      if (bclosure->qsignal == qsignal)
	{
	  if (enable_notify)
	    {
	      sfi_diag ("%s: redundant signal \"%s\" connection on proxy (%lu)", bcontext->user, signal, proxy);
	      return TRUE;
	    }
          /* disable notify, disconnect closure */
	  closure = (GClosure*) bclosure;
	  if (last)
	    last->next = slist->next;
	  else
	    p->closures = slist->next;
	  g_slist_free_1 (slist);
	  g_closure_invalidate (closure);
	  g_signal_handler_disconnect (item, bclosure->handler_id);
	  g_closure_unref (closure);
	  return FALSE;
	}
    }
  if (!enable_notify)
    {
#if 0
      sfi_diag ("%s: bogus disconnection for signal \"%s\" on proxy (%lu)", bcontext->user, signal, proxy);
#endif
      return FALSE;
    }
  
  /* abort early if the signal is unknown */
  sig_name = g_strdup (signal);
  c = strchr (sig_name, ':');
  if (c)
    *c = 0;
  sig_id = g_signal_lookup (sig_name, G_OBJECT_TYPE (item));
  g_free (sig_name);
  if (!sig_id)
    return FALSE;

  closure = g_closure_new_simple (sizeof (BClosure), bcontext);
  g_closure_set_marshal (closure, sig_closure_marshal);
  bclosure = (BClosure*) closure;
  bclosure->qsignal = qsignal;
  g_closure_ref (closure);
  g_closure_sink (closure);
  bclosure->handler_id = g_signal_connect_closure (item, signal, closure, FALSE);
  if (bclosure->handler_id)
    {
      p->closures = g_slist_prepend (p->closures, closure);
      connected = TRUE;
    }
  else
    {
      connected = FALSE;	/* something failed */
      g_closure_unref (closure);
    }
  return connected;
}

static void
bglue_proxy_processed_notify (SfiGlueContext *context,
			      uint            notify_id)
{
  BContext *bcontext = (BContext*) context;
  if (!bcontext_release_notify_ref (bcontext, notify_id))
    sfi_diag ("got invalid event receipt (%u)", notify_id);
}

static GValue*
bglue_client_msg (SfiGlueContext *context,
                  const char     *msg,
                  GValue         *value)
{
  GValue *retval = NULL;
  
  if (!msg)
    ;
  else
    {
      sfi_diag ("unhandled client message: %s", msg);
      retval = sfi_value_string ("Unknown client msg");
    }
  
  return retval;
}

static SfiRing*
bglue_fetch_events (SfiGlueContext *context)
{
  BContext *bcontext = (BContext*) context;
  SfiRing *events = bcontext->events;
  bcontext->events = NULL;
  return events;
}

static SfiRing*
bglue_list_poll_fds (SfiGlueContext *context)
{
  return NULL;
}

static gboolean
bproxy_foreach_slist (void          *data,
		      unsigned long  unique_id,
		      void          *value)
{
  GSList **slist_p = (GSList**) data;
  *slist_p = g_slist_prepend (*slist_p, (void *) unique_id);
  return TRUE;
}

static void
bglue_destroy (SfiGlueContext *context)
{
  BContext *bcontext = (BContext*) context;
  GSList *plist = NULL;
  SfiSeq *seq;
  uint i;
  sfi_ustore_foreach (bcontext->bproxies, bproxy_foreach_slist, &plist);
  while (plist)
    {
      GSList *slist;
      for (slist = plist; slist; slist = slist->next)
	{
	  SfiProxy proxy = (unsigned long) slist->data;
	  BProxy *p = (BProxy*) sfi_ustore_lookup (bcontext->bproxies, proxy);
	  if (p)
	    bcontext_destroy_bproxy (bcontext, p, proxy, (BseItem*) bse_object_from_id (proxy));
	}
      g_slist_free (plist);
      plist = NULL;
      sfi_ustore_foreach (bcontext->bproxies, bproxy_foreach_slist, &plist);
    }
  sfi_ustore_destroy (bcontext->bproxies);
  g_free (bcontext->user);
  seq = (SfiSeq*) sfi_ring_pop_head (&bcontext->events);
  while (seq)
    {
      sfi_seq_unref (seq);
      seq = (SfiSeq*) sfi_ring_pop_head (&bcontext->events);
    }
  for (i = 0; i < bcontext->n_nrefs; i++)
    if (bcontext->nrefs[i].id)
      bcontext_release_notify_ref (bcontext, bcontext->nrefs[i].id);
  g_free (bcontext->nrefs);
  g_free (bcontext);
}

/* vim:set ts=8 sts=2 sw=2: */
