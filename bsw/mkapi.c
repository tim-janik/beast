/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2001 and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <bse/bsemain.h>
#include <bse/bsetype.h>
#include <bse/bseglue.h>
#include "topconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

static gchar      *gen_prefix = "foo_prefix";
static gboolean    gen_header = 0;
static gboolean    gen_body = 0;


/* FIXME: special casing */
typedef struct {
  gchar *tname;
  gchar *oname;
  gchar *to_type;
  gchar *from_type;
} HackType;

static HackType*
pspec_hack_type (GParamSpec *pspec)
{
  HackType ht = { NULL, };

  if (SFI_IS_PSPEC_SEQ (pspec))
    {
      ht.oname = "seq";
      switch (sfi_categorize_pspec (sfi_pspec_get_seq_element (pspec)))
	{
	  HackType *rt;
	case SFI_SCAT_INT:	ht.tname = "BseIntSeq*";	break;
	case SFI_SCAT_NOTE:	ht.tname = "BseNoteSeq*";	break;
	case SFI_SCAT_STRING:	ht.tname = "BseStringSeq*";	break;
	case SFI_SCAT_PROXY:	ht.tname = "BseProxySeq*";	break;
	case SFI_SCAT_REC:
	  rt = pspec_hack_type (sfi_pspec_get_seq_element (pspec));
	  if (rt)
	    {
	      ht.tname = g_new0 (gchar, strlen (rt->tname) + 3 + 1);
	      strcat (ht.tname, rt->tname);
	      ht.tname[strlen (ht.tname) - 1] = 0;
	      strcat (ht.tname, "Seq*");
	      break;
	    }
	  /* fall through */
	default:
	  g_warning ("%s: can't figure sequence type for pspec \"%s\"", G_STRLOC, pspec->name);
	  break;
	}
    }
  else if (SFI_IS_PSPEC_REC (pspec))
    {
      SfiRecFields rf = sfi_pspec_get_rec_fields (pspec);
      GParamSpec **farray = rf.fields;
      ht.oname = "rec";
      if (farray == bse_part_note_fields.fields)
	ht.tname = "BsePartNote*";
      else if (farray == bse_note_description_fields.fields)
	ht.tname = "BseNoteDescription*";
      else if (farray == bse_note_sequence_fields.fields)
	ht.tname = "BseNoteSequence*";
      else if (farray == bse_icon_fields.fields)
	ht.tname = "BseIcon*";
      else if (farray == bse_category_fields.fields)
	ht.tname = "BseCategory*";
      else
	g_warning ("%s: can't figure record type for pspec \"%s\"", G_STRLOC, pspec->name);
    }
  else if (SFI_IS_PSPEC_CHOICE (pspec))
    {
      GType etype = bse_glue_pspec_get_original_enum (pspec);
      ht.oname = "choice";
      if (etype)
	ht.tname = g_type_name (etype);
    }
  if (ht.tname)
    {
      gchar *cname = g_type_name_to_cname (ht.tname);
      if (!SFI_IS_PSPEC_CHOICE (pspec))
	cname[strlen (cname) - 1] = 0;
      ht.to_type = g_strdup_printf ("%s_to_%s", cname, ht.oname);
      ht.from_type = g_strdup_printf ("%s_from_%s", cname, ht.oname);
      g_free (cname);
      return g_memdup (&ht, sizeof (ht));
    }
  return NULL;
}

static gchar*
sname2cname (const gchar *str)
{
  return g_type_name_to_cname (str);
}

static gchar*
pspec_ctype (GParamSpec *pspec)
{
  switch (pspec ? sfi_categorize_pspec (pspec) & SFI_SCAT_TYPE_MASK : -1)
    {
    case -1:			return "void";
    case SFI_SCAT_BOOL:		return "SfiBool";
    case SFI_SCAT_INT:		return "SfiInt";
    case SFI_SCAT_NUM:		return "SfiNum";
    case SFI_SCAT_REAL:		return "SfiReal";
    case SFI_SCAT_STRING:	return "const gchar*";
    case SFI_SCAT_CHOICE:	return "SfiChoice";
    case SFI_SCAT_FBLOCK:	return "SfiFBlock*";
    case SFI_SCAT_SEQ:		return "SfiSeq*";
    case SFI_SCAT_REC:		return "SfiRec*";
    case SFI_SCAT_PROXY:	return "SfiProxy";
    default:
      g_warning ("unhandled pspec (\"%s\") type: %s", pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      return "UNKNOWN";
    }
}

static gchar*
pspec_rtype (GParamSpec *pspec)
{
  HackType *ht = pspec_hack_type (pspec);
  return ht ? ht->tname : pspec_ctype (pspec);
}

static gchar*
pspec_fname (GParamSpec *pspec)
{
  switch (pspec ? sfi_categorize_pspec (pspec) & SFI_SCAT_TYPE_MASK : -1)
    {
    case -1:			return "void";
    case SFI_SCAT_BOOL:		return "bool";
    case SFI_SCAT_INT:		return "int";
    case SFI_SCAT_NUM:		return "num";
    case SFI_SCAT_REAL:		return "real";
    case SFI_SCAT_STRING:	return "string";
    case SFI_SCAT_CHOICE:	return "choice";
    case SFI_SCAT_FBLOCK:	return "fblock";
    case SFI_SCAT_SEQ:		return "seq";
    case SFI_SCAT_REC:		return "rec";
    case SFI_SCAT_PROXY:	return "proxy";
    default:
      g_warning ("unhandled pspec (\"%s\") type: %s", pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      return "UNKNOWN";
    }
}

static void
print_glue_proc (SfiGlueProc *p)
{
  guint i;
  if (gen_header)
    {
      /* alias definition */
      g_print ("#define %s %s_%s\n", sname2cname (p->name), gen_prefix, sname2cname (p->name));
      /* header prototype */
      g_print ("%s %s (", pspec_rtype (p->ret_param), sname2cname (p->name));
      for (i = 0; i < p->n_params; i++)
	{
	  GParamSpec *pspec = p->params[i];
	  HackType *ht = pspec_hack_type (pspec);
	  if (i)
	    g_print (", ");
	  g_print ("%s %s", ht ? ht->tname : pspec_ctype (pspec), sname2cname (pspec->name));
	}
      if (!i)
	g_print ("void");
      g_print (");\n");
    }
  if (gen_body)
    {
      /* body prototype */
      g_print ("%s\n%s_%s (", pspec_rtype (p->ret_param), gen_prefix, sname2cname (p->name));
      for (i = 0; i < p->n_params; i++)
	{
	  GParamSpec *pspec = p->params[i];
          HackType *ht = pspec_hack_type (pspec);
	  if (i)
	    g_print (", ");
	  g_print ("%s %s", ht ? ht->tname : pspec_ctype (pspec), sname2cname (pspec->name));
	}
      if (!i)
	g_print ("void");
      g_print (")\n");
      /* body */
      g_print ("{\n");
      if (p->ret_param)
	{
	  g_print ("  %s _retval;\n", pspec_ctype (p->ret_param));
	  g_print ("  _retval = sfi_glue_vcall_%s (\"%s\",", pspec_fname (p->ret_param), p->name);
	}
      else
	g_print ("  sfi_glue_vcall_%s (\"%s\",", pspec_fname (p->ret_param), p->name);
      for (i = 0; i < p->n_params; i++)
	{
          GParamSpec *pspec = p->params[i];
          HackType *ht = pspec_hack_type (pspec);
	  guint cat = sfi_categorize_pspec (pspec) & SFI_SCAT_TYPE_MASK;
	  gchar *cname = sname2cname (pspec->name);
	  if (ht)
	    g_print (" '%c', %s (%s),", cat, ht->to_type, cname);
	  else
	    g_print (" '%c', %s,", cat, cname);
	}
      g_print (" 0);\n");
      if (p->ret_param)
	{
	  GParamSpec *pspec = p->ret_param;
	  HackType *ht = pspec_hack_type (pspec);
	  if (ht)
	    g_print ("  return %s (_retval);\n", ht->from_type);
	  else
	    g_print ("  return _retval;\n");
	}
      g_print ("}\n");
    }
}

static void
print_methods (gchar **ifaces)
{
  while (*ifaces)
    {
      gchar **names = sfi_glue_list_method_names (*ifaces);
      gchar *uname = g_type_name_to_cupper (*ifaces);
      guint i;
      
      if (strncmp (uname, "BSE_", 4) == 0)
	g_print ("#define BSE_IS_%s(proxy) bse_proxy_is_a ((proxy), \"%s\")\n",
		 uname + 4, *ifaces);
      for (i = 0; names[i]; i++)
	{
	  gchar *mname = g_strdup_printf ("%s+%s", *ifaces, names[i]);
	  SfiGlueProc *p = sfi_glue_describe_proc (mname);
	  g_free (mname);
	  if (!p)
	    g_message ("unable to retrieve description of \"%s\"", names[i]);
	  else
	    print_glue_proc (p);
	}

      names = sfi_glue_iface_children (*ifaces);
      print_methods (names);

      ifaces++;
    }
}

static void
print_procs (void)
{
  gchar **procs = sfi_glue_list_proc_names ();
  gchar *ifaces[2];
  guint i = 0;

  while (procs[i])
    {
      SfiGlueProc *p = sfi_glue_describe_proc (procs[i]);
      if (!p)
	g_message ("unable to retrieve description of \"%s\"", procs[i]);
      else
	print_glue_proc (p);
      i++;
    }

  ifaces[0] = sfi_glue_base_iface ();
  ifaces[1] = NULL;
  print_methods (ifaces);
}

static gint
evals_rcmp (gconstpointer v1,
	    gconstpointer v2)
{
  const GEnumValue *val1 = v1;
  const GEnumValue *val2 = v2;
  gchar *s1 = sfi_strdup_canon (val1->value_name);
  gchar *s2 = sfi_strdup_canon (val2->value_name);
  gint cmp = sfi_constants_rcmp (s1, s2);
  g_free (s1);
  g_free (s2);
  return cmp;
}

static void
print_enums (void)
{
  GType *children;
  guint n, i;

  if (gen_header)
    {
      children = g_type_children (G_TYPE_ENUM, &n);
      for (i = 0; i < n; i++)
	{
	  const gchar *name = g_type_name (children[i]);
	  const gchar *cname = g_type_name_to_cname (name);
	  GEnumClass *eclass = g_type_class_ref (children[i]);
          gboolean regular_choice = strcmp (name, "BseErrorType") != 0;
	  GEnumValue *val;
	  /* enum definition */
	  g_print ("typedef enum {\n");
	  for (val = eclass->values; val->value_name; val++)
	    {
              guint vnum = val - eclass->values + regular_choice;
	      g_print ("  %s = %d,\n", val->value_name, vnum);
	    }
	  g_print ("} %s;\n", name);
	  /* conversion aliases */
	  g_print ("#define %s_to_choice %s_%s_to_choice\n", cname, gen_prefix, cname);
	  g_print ("#define %s_from_choice %s_%s_from_choice\n", cname, gen_prefix, cname);
	  /* conversion prototypes */
	  g_print ("const gchar* %s_to_choice (%s value);\n", cname, name);
	  g_print ("%s %s_from_choice (const gchar *choice);\n", name, cname);
	  /* cleanup */
	  g_type_class_unref (eclass);
	}
      g_free (children);
    }
  if (gen_body)
    {
      children = g_type_children (G_TYPE_ENUM, &n);
      for (i = 0; i < n; i++)
	{
	  const gchar *name = g_type_name (children[i]);
          const gchar *cname = g_type_name_to_cname (name);
	  GEnumClass *eclass = g_type_class_ref (children[i]);
	  GEnumValue *val;
	  GSList *node, *slist = NULL;
	  guint vnum, min = G_MAXINT, max = 0;
          gboolean regular_choice = strcmp (name, "BseErrorType") != 0;
	  gchar *str;
	  /* value constants decl */
	  g_print ("static const SfiConstants %s_vals[] = {", name);
	  /* sort enum values */
	  for (val = eclass->values; val->value_name; val++)
	    slist = g_slist_prepend (slist, val);
	  slist = g_slist_sort (slist, evals_rcmp);
	  /* print out enum values */
	  for (node = slist; node; node = node->next)
	    {
	      val = node->data;
	      vnum = val - eclass->values + regular_choice;
	      str = sfi_strdup_canon (val->value_name);
	      g_print ("\n  { \"%s\", %u, %u },", str, strlen (str), vnum);
	      min = MIN (vnum, min);
	      max = MAX (vnum, max);
	      g_free (str);
	    }
	  g_print ("\n}; /* %u */\n", val - eclass->values);
	  /* to_choice conversion */
	  g_print ("const gchar*\n%s_%s_to_choice (%s value)\n", gen_prefix, cname, name);
	  g_print ("{\n");
	  g_print ("  g_return_val_if_fail (value >= %u && value <= %u, NULL);\n", min, max);
	  g_print ("  return sfi_constants_get_name (G_N_ELEMENTS (%s_vals), %s_vals, value);\n", name, name);
	  g_print ("}\n");
	  /* from_choice conversion */
	  g_print ("%s\n%s_%s_from_choice (const gchar *choice)\n", name, gen_prefix, cname);
	  g_print ("{\n");
	  g_print ("  return choice ? sfi_constants_get_index (G_N_ELEMENTS (%s_vals), %s_vals, choice) : 0;\n", name, name);
	  g_print ("}\n");
	  /* cleanup */
	  g_slist_free (slist);
	  g_type_class_unref (eclass);
	}
      g_free (children);
    }
  g_print ("\n");
}

int
main (gint   argc,
      gchar *argv[])
{
  static gint help (gchar *arg);
  guint i;
  
  g_thread_init (NULL);

  bse_init_intern (&argc, &argv, NULL);

  for (i = 1; i < argc; i++)
    {
      if (strcmp ("--header", argv[i]) == 0)
	{
	  gen_header = TRUE;
	}
      else if (strcmp ("--body", argv[i]) == 0)
	{
	  gen_body = TRUE;
	}
      else if (strcmp ("-p", argv[i]) == 0)
	{
	  // FIXME: bsw_register_plugins (NULL, TRUE, NULL, NULL, NULL);
	}
      else if (strcmp ("-h", argv[i]) == 0 ||
	  strcmp ("--help", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	return help (argv[i]);
    }

  sfi_glue_context_push (bse_glue_context_intern ("BSW-MkAPI"));

  print_enums ();
  print_procs ();

  sfi_glue_context_pop ();
  
  return 0;
}

static gint
help (gchar *arg)
{
  fprintf (stderr, "usage: mkapi <qualifier> [-r <type>] [-{i|b} \"\"] [-s #] [-{h|p|x|y}]\n");
  fprintf (stderr, "       -p       include plugins\n");
  fprintf (stderr, "       -h       guess what ;)\n");

  return arg != NULL;
}
