/* BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
 * Copyright (C) 2002 Tim Janik
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
#define G_LOG_DOMAIN "BswSCM-MkGlue"

#include <string.h>
#include <errno.h>

#include <bse/bse.h>
#include <bsw/bsw.h>
#include <bsw/bswglue.h>

/* --- typedefs & structures --- */
typedef struct {
  GType        type;
  const gchar *ctype;		/* C decl */
  const gchar *wname;		/* wrapper variants postfix */
  const gchar *iconvert;	/* scm2c function */
  const gchar *icleanup;	/* scm2c_free */
  const gchar *oconvert;	/* c2scm function */
  guint	       converter_type : 1;
} BswSCMGlue;


/* --- prototypes --- */
static void	bsw_scm_register_glue_types	(void);
static void	bsw_scm_generate_glue		(void);


/* --- variables --- */
static GSList *bsw_scm_iglue_list = NULL;
static GSList *bsw_scm_oglue_list = NULL;


/* --- functions --- */
int
main (gint   argc,
      gchar *argv[])
{
  /* initialization
   */
  g_thread_init (NULL);
  // g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
  bsw_init (&argc, &argv, NULL);

  /* scheme specific glue data
   */
  bsw_scm_register_glue_types ();

  /* generate the code
   */
  bsw_scm_generate_glue ();

  return 0;
}

/* input argument glue types */
static void
bsw_scm_register_iglue_type (GType        type,
			     const gchar *ctype,
			     const gchar *wname,
			     const gchar *iconvert,
			     const gchar *icleanup,
			     gboolean     converter_type)
{
  BswSCMGlue *glue = g_new (BswSCMGlue, 1);

  glue->type = type;
  glue->ctype = ctype;
  glue->wname = wname;
  glue->iconvert = iconvert;
  glue->icleanup = icleanup;
  glue->oconvert = NULL;
  glue->converter_type = converter_type != FALSE;
  bsw_scm_iglue_list = g_slist_prepend (bsw_scm_iglue_list, glue);
}

/* output argument glue types */
static void
bsw_scm_register_oglue_type (GType        type,
			     const gchar *ctype,
			     const gchar *wname,
			     const gchar *oconvert,
			     gboolean     converter_type)
{
  BswSCMGlue *glue = g_new (BswSCMGlue, 1);

  glue->type = type;
  glue->ctype = ctype;
  glue->wname = wname;
  glue->iconvert = NULL;
  glue->icleanup = NULL;
  glue->oconvert = oconvert;
  glue->converter_type = converter_type != FALSE;
  bsw_scm_oglue_list = g_slist_prepend (bsw_scm_oglue_list, glue);
}

/* input & output argument glue types */
static void
bsw_scm_register_glue_type (GType        type,
			    const gchar *ctype,
			    const gchar *wname,
			    const gchar *iconvert,
			    const gchar *icleanup,
			    const gchar *oconvert)
{
  BswSCMGlue *glue = g_new (BswSCMGlue, 1);

  glue->type = type;
  glue->ctype = ctype;
  glue->wname = wname;
  glue->iconvert = iconvert;
  glue->icleanup = icleanup;
  glue->oconvert = oconvert;
  glue->converter_type = FALSE;
  bsw_scm_iglue_list = g_slist_prepend (bsw_scm_iglue_list, glue);
  bsw_scm_oglue_list = g_slist_prepend (bsw_scm_oglue_list, glue);
}

static void
bsw_scm_register_glue_types (void)
{
  bsw_scm_register_glue_type (G_TYPE_BOOLEAN, "gboolean", "boolean", "gh_scm2bool", NULL, "gh_bool2scm");
  bsw_scm_register_glue_type (G_TYPE_UINT, "guint", "uint", "gh_scm2ulong", NULL, "gh_ulong2scm");
  bsw_scm_register_glue_type (G_TYPE_INT, "gint", "int", "gh_scm2long", NULL, "gh_long2scm");
  bsw_scm_register_glue_type (BSE_TYPE_ITEM, "BswProxy", "proxy", "gh_scm2ulong", NULL, "gh_ulong2scm");
  bsw_scm_register_iglue_type (G_TYPE_STRING, "gchar*", "string", "bsw_scm_to_str", "free", FALSE);
  bsw_scm_register_oglue_type (G_TYPE_STRING, "const gchar*", "string", "gh_str02scm", FALSE);
  bsw_scm_register_glue_type (G_TYPE_FLOAT, "gdouble", "double", "gh_scm2double", NULL, "gh_double2scm");
  bsw_scm_register_glue_type (G_TYPE_DOUBLE, "gdouble", "double", "gh_scm2double", NULL, "gh_double2scm");
  // bsw_scm_register_iglue_type (G_TYPE_ENUM, "gint", "enum", "bsw_scm_to_enum", NULL, TRUE);
  bsw_scm_register_oglue_type (G_TYPE_ENUM, "gint", "enum", "bsw_scm_from_enum", TRUE);
}

static BswSCMGlue*
bsw_scm_lookup_iglue (GType type)
{
  GSList *slist;

  for (slist = bsw_scm_iglue_list; slist; slist = slist->next)
    {
      BswSCMGlue *glue = slist->data;

      if (g_type_is_a (type, glue->type))
	return glue;
    }
  return NULL;
}

static BswSCMGlue*
bsw_scm_lookup_oglue (GType type)
{
  GSList *slist;

  for (slist = bsw_scm_oglue_list; slist; slist = slist->next)
    {
      BswSCMGlue *glue = slist->data;

      if (g_type_is_a (type, glue->type))
	return glue;
    }
  return NULL;
}

static gboolean
foreach_glue_proc (gpointer    data,
		   GTypeClass *klass)
{
  BseProcedureClass *proc = (BseProcedureClass*) klass;
  gchar *cname, *sname;
  guint i, need_newline;

  /* check proc args */
  for (i = 0; i < proc->n_in_pspecs; i++)
    if (!bsw_scm_lookup_iglue (G_PARAM_SPEC_VALUE_TYPE (proc->in_pspecs[i])))
      {
	g_message ("unknown input type: %s (%s)", g_type_name (G_PARAM_SPEC_VALUE_TYPE (proc->in_pspecs[i])), proc->name);
	return FALSE;
      }
  if (proc->n_out_pspecs > 1)
    return FALSE;
  if (proc->n_out_pspecs && !bsw_scm_lookup_oglue (G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[0])))
    {
      g_message ("unknown output type: %s (%s)", g_type_name (G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[0])), proc->name);
      return FALSE;
    }

  /* canonicalize proc name for c and scheme */
  cname = bsw_type_name_to_cname (proc->name);
  sname = bsw_type_name_to_sname (proc->name);

  /* prototype */
  g_print ("static SCM\nbsw_scm_wrap_%s (\n", cname);
  for (i = 0; i < proc->n_in_pspecs; i++)
    g_print ("  SCM s_arg%u%c\n", i + 1, i + 1 < proc->n_in_pspecs ? ',' : ' ');
  if (!proc->n_in_pspecs)
    g_print ("  void\n");
  g_print ("  )\n{\n");

  /* body boilerplate */
  g_print ("  BswSCMHandle *handle;\n");
  g_print ("  BswErrorType berror;\n");
  g_print ("  SCM s_retval;\n");

  /* argument decls */
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      BswSCMGlue *glue = bsw_scm_lookup_iglue (vtype);

      g_print ("  %s arg%u; /* %s */\n", glue->ctype, i + 1, pspec->name);
    }
  if (proc->n_out_pspecs)
    {
      GParamSpec *pspec = proc->out_pspecs[0];
      GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      BswSCMGlue *glue = bsw_scm_lookup_oglue (vtype);

      g_print ("  %s retval; /* %s */\n", glue->ctype, pspec->name);
    }
  g_print ("\n");

  /* assertions */
  if (0)
    {
      need_newline = FALSE;
      for (i = 0; i < proc->n_in_pspecs; i++)
	{
	  GParamSpec *pspec = proc->in_pspecs[i];
	  GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
	  BswSCMGlue *glue = bsw_scm_lookup_iglue (vtype);
	  
	  g_print ("  bsw_scm_arg_assert (\"%s\", \"%s\", s_arg%u, %u, %s);\n",
		   sname, pspec->name, i + 1, i + 1, glue->wname);
	  need_newline = TRUE;
	}
      if (need_newline)
	g_print ("\n");
    }

  /* conversions */
  g_print ("  BSW_SCM_DEFER_INTS ();\n");
  need_newline = FALSE;
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      BswSCMGlue *glue = bsw_scm_lookup_iglue (vtype);

      g_print ("  arg%u = %s (s_arg%u);\n", i + 1, glue->iconvert, i + 1);
      need_newline = TRUE;
    }
  if (need_newline)
    g_print ("\n");

  /* open execution handle */
  g_print ("  handle = bsw_scm_handle_fetch (\"%s\");\n", proc->name);
  g_print ("\n");

  /* put execution args */
  need_newline = FALSE;
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      BswSCMGlue *glue = bsw_scm_lookup_iglue (vtype);
      gchar *tmacro = bsw_type_name_to_type_macro (g_type_name (vtype));

      g_print ("  { GValue v = { 0, }; bsw_value_initset_%s (&v, %s, arg%u); bsw_scm_handle_putunset (handle, &v); }\n",
	       glue->wname, tmacro, i + 1);
      g_free (tmacro);
      if (glue->icleanup)
	g_print ("  %s (arg%u);\n", glue->icleanup, i + 1);
      need_newline = TRUE;
    }
  if (need_newline)
    g_print ("\n");

  /* execute */
  g_print ("  berror = bsw_scm_handle_eval (handle);\n");

  /* get execution return value and convert */
  if (proc->n_out_pspecs)
    {
      GParamSpec *pspec = proc->out_pspecs[0];
      GType vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      BswSCMGlue *glue = bsw_scm_lookup_oglue (vtype);
      gchar *tmacro;

      if (G_TYPE_IS_OBJECT (vtype))
	tmacro = g_strdup ("BSW_TYPE_PROXY");
      else
	tmacro = bsw_type_name_to_type_macro (g_type_name (vtype));
      g_print ("  retval = g_value_get_%s (bsw_scm_handle_peekret (handle, %s));\n",
	       glue->wname, tmacro);
      if (glue->converter_type)
	g_print ("  s_retval = %s (retval, %s);\n", glue->oconvert, tmacro);
      else
	g_print ("  s_retval = %s (retval);\n", glue->oconvert);
      g_free (tmacro);
    }
  else
    g_print ("  s_retval = gh_bool2scm (FALSE);\n");
  g_print ("\n");

  /* close execution handle */
  g_print ("  bsw_scm_handle_clean (handle);\n");
  g_print ("  BSW_SCM_ALLOW_INTS ();\n");
  g_print ("\n");

  /* finish */
  g_print ("  return s_retval;\n");
  g_print ("}\n");
  g_free (cname);
  g_free (sname);

  return TRUE;
}

static void
foreach_glue_table (gpointer    data,
		    GTypeClass *klass)
{
  BseProcedureClass *proc = (BseProcedureClass*) klass;
  gchar *cname = bsw_type_name_to_cname (proc->name);
  gchar *sname = bsw_type_name_to_sname (proc->name);
  
  /* wrapper function */
  g_print ("  { \"%s\", bsw_scm_wrap_%s, %u, },\n", sname, cname, proc->n_in_pspecs);

  g_free (cname);
  g_free (sname);
}

static void
bsw_scm_generate_glue (void)
{
  BseCategory *categories;
  GSList *slist, *class_list = NULL;
  guint i, n_cats;

  categories = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE, &n_cats);
  for (i = 0; i < n_cats; i++)
    {
      GTypeClass *klass = g_type_class_ref (categories[i].type);

      if (foreach_glue_proc (NULL, klass))
	class_list = g_slist_prepend (class_list, klass);
      else
	g_type_class_unref (klass);
    }

  g_print ("static struct { gchar *fname; SCM (*func) (); guint rargs; } bsw_scm_wrap_table[] = {\n");

  for (slist = class_list; slist; slist = slist->next)
    {
      GTypeClass *klass = slist->data;
      foreach_glue_table (NULL, klass);
      g_type_class_unref (klass);
    }

  g_print ("};\n");

  g_slist_free (class_list);
  g_free (categories);
}
