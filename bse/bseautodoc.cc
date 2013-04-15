// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemain.hh"
#include "bsecategories.hh"
#include "bseprocedure.hh"
#include "bsesource.hh"
#include "topconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


static const gchar*
beauty_float (gdouble f)
{
  gchar *s;
  if (ABS (f) <= 18446744073709551616.)
    s = g_strdup_printf ("%.7f", f);
  else
    s = g_strdup_printf ("%.7g", f);
  const gchar *c;
  if (strchr (s, '.'))
    {
      guint l = strlen (s) - 1;
      while (s[l] == '0')
        s[l--] = 0;
      if (s[l] == '.')
        s[l--] = 0;
    }
  c = g_intern_string (s);
  g_free (s);
  return c;
}

static GQuark boxed_type_tag = 0;

static void
tag_all_boxed_pspecs (void)
{
  GType *children;
  guint i;

  children = g_type_children (G_TYPE_BOXED, NULL);
  for (i = 0; children[i]; i++)
    {
      SfiRecFields rfields = sfi_boxed_type_get_rec_fields (children[i]);
      GParamSpec *element = sfi_boxed_type_get_seq_element (children[i]);

      if (element)
	{
	  g_param_spec_ref (element);
	  g_param_spec_set_qdata (element, boxed_type_tag, const_cast<char*> (g_type_name (children[i])));
	}
      else if (rfields.n_fields)
	{
	  guint j;
	  for (j = 0; j < rfields.n_fields; j++)
	    {
	      g_param_spec_ref (rfields.fields[j]);
	      g_param_spec_set_qdata (rfields.fields[j], boxed_type_tag, const_cast<char*> (g_type_name (children[i])));
	    }
	}
    }
  g_free (children);
}

static const gchar*
lookup_boxed_tag (GParamSpec *pspec)
{
  if (pspec)
    return (const gchar*) g_param_spec_get_qdata (pspec, boxed_type_tag);
  return NULL;
}

static gchar*
type_name (GParamSpec *pspec)
{
  switch (sfi_categorize_pspec (pspec))
    {
      const gchar *btag;
      SfiRecFields rfields;
    case SFI_SCAT_BOOL:		return g_strdup ("SfiBool");
    case SFI_SCAT_INT:		return g_strdup ("SfiInt");
    case SFI_SCAT_NUM:		return g_strdup ("SfiNum");
    case SFI_SCAT_REAL:		return g_strdup ("SfiReal");
    case SFI_SCAT_STRING:	return g_strdup ("const gchar*");
    case SFI_SCAT_CHOICE:	return g_strdup ("SfiChoice");
    case SFI_SCAT_BBLOCK:	return g_strdup ("SfiBBlock*");
    case SFI_SCAT_FBLOCK:	return g_strdup ("SfiFBlock*");
    case SFI_SCAT_PSPEC:	return g_strdup ("GParamSpec*");
    case SFI_SCAT_SEQ:
      btag = lookup_boxed_tag (sfi_pspec_get_seq_element (pspec));
      if (btag)
	return g_strdup_printf ("%s*", btag);
      else
	return g_strdup ("SfiSeq*");
    case SFI_SCAT_REC:
      rfields = sfi_pspec_get_rec_fields (pspec);
      btag = rfields.n_fields ? lookup_boxed_tag (rfields.fields[0]) : NULL;
      if (btag)
	return g_strdup_printf ("%s*", btag);
      else
	return g_strdup ("SfiRec*");
    case SFI_SCAT_PROXY:	return g_strdup ("SfiProxy");
    case SFI_SCAT_NOTE:		return g_strdup ("SfiInt");
    case SFI_SCAT_TIME:		return g_strdup ("SfiNum");
    case 0:
      if (G_IS_PARAM_SPEC_ENUM (pspec))
	{
	  GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);
	  return g_strdup (G_OBJECT_CLASS_NAME (espec->enum_class));
	}
      else if (G_IS_PARAM_SPEC_BOXED (pspec))
	return g_strconcat (g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)), "*", NULL);
      else if (G_IS_PARAM_SPEC_OBJECT (pspec))
	return g_strconcat (g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)), "*", NULL);
      /* fall through */
    default:
      g_error ("unhandled pspec type: %s", g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      break;
    }
  return NULL;
}

static GSList *all_strings = NULL;

static const gchar*
qescape (const gchar *name)
{
  char *str = g_strescape (name, NULL);
  char *s = g_strconcat ("\"", str, "\"", NULL);
  g_free (str);
  all_strings = g_slist_prepend (all_strings, s);
  return s;
}

static gchar*
pspec_construct_description (GParamSpec  *pspec,
                             const gchar *cname,
                             const gchar *sname)
{
  const gchar *blurb = g_param_spec_get_blurb (pspec);
  const gchar *nick = g_param_spec_get_nick (pspec);

  if (blurb && (strcmp (blurb, cname) == 0 || strcmp (blurb, sname) == 0))
    blurb = NULL;
  if (nick && (strcmp (nick, cname) == 0 || strcmp (nick, sname) == 0))
    nick = NULL;

  GString *gstring = g_string_new ("");

  if (blurb || nick)
    {
      if (gstring->str[0])
        g_string_append (gstring, "\n");
      if (blurb)
        g_string_append (gstring, blurb);
      else
        g_string_append (gstring, nick);
    }
  while (gstring->len && gstring->str[gstring->len - 1] == '\n')
    gstring->str[--gstring->len] = 0;

  if (SFI_IS_PSPEC_INT (pspec))
    {
      SfiInt imin, imax;
      sfi_pspec_get_int_range (pspec, &imin, &imax, NULL);
      g_string_append_printf (gstring, " (%s %d .. %d)\n", _("Range:"), imin, imax);
    }
  else if (SFI_IS_PSPEC_REAL (pspec))
    {
      SfiReal fmin, fmax;
      sfi_pspec_get_real_range (pspec, &fmin, &fmax, NULL);
      g_string_append_printf (gstring, " (%s %s .. %s)\n", _("Range:"), beauty_float (fmin), beauty_float (fmax));
    }
  while (gstring->len && gstring->str[gstring->len - 1] == '\n')
    gstring->str[--gstring->len] = 0;

  if (gstring->len)
    return g_string_free (gstring, FALSE);
  g_string_free (gstring, TRUE);
  return NULL;
}

static void
print_pspec (GParamSpec  *pspec,
             const gchar *indent,
             const gchar *type_postfix)
{
  gchar *cname = g_type_name_to_cname (pspec->name);
  gchar *sname = g_type_name_to_sname (pspec->name);
  gchar *tname = type_name (pspec);
  const gchar *argstrig = "";
  const gchar *initializer = "";
  const gchar *group = sfi_pspec_get_group (pspec);
  gchar *blurb = pspec_construct_description (pspec, cname, sname);
  const gchar *label = g_param_spec_get_nick (pspec);
  if (label && (strcmp (label, cname) == 0 || strcmp (label, sname) == 0))
    label = NULL;
  g_print ("%s('%s', '%s%s', '%s', '%s',\n", indent, cname, tname, type_postfix, argstrig, initializer);
  g_print ("%s (%s, '', 0),\n", indent, qescape (blurb ? blurb : ""));
  g_print ("%s '%s', '%s',\n", indent, label ? label : "", group ? group : "");
  g_print ("),\n");
  g_free (cname);
  g_free (sname);
  g_free (tname);
  g_free (blurb);
}

static void
show_procdoc (void)
{
  BseCategorySeq *cseq;
  guint i;

  g_print ("functions = (\n");

  cseq = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE);
  for (i = 0; i < cseq->n_cats; i++)
    {
      GType type = g_type_from_name (cseq->cats[i]->type);
      BseProcedureClass *pclass = (BseProcedureClass*) g_type_class_ref (type);
      const gchar *blurb = bse_type_get_blurb (type);
      gchar *cname = g_type_name_to_cname (cseq->cats[i]->type);
      gchar *sname = g_type_name_to_sname (cseq->cats[i]->type);
      guint j;

      g_print ("{\n");
      g_print ("  'name': '%s',\n", cname);
      g_print ("  'aliases': [ ('%s', 'scheme'), ], # aliases\n", sname);
      g_print ("  'args': [ # input arguments\n");
      for (j = 0; j < pclass->n_in_pspecs; j++)
	{
          GParamSpec *pspec = G_PARAM_SPEC (pclass->in_pspecs[j]);
          print_pspec (pspec, "    ", "");
        }
      g_print ("  ],\n");

      if (pclass->n_out_pspecs == 1)
        {
          g_print ("  'return': \n");
          GParamSpec *pspec = G_PARAM_SPEC (pclass->out_pspecs[0]);
          print_pspec (pspec, "    ", "");
        }
      else if (pclass->n_out_pspecs > 1)
        g_print ("  'return': ('RETURNS', 'MultiReturn', '', '', ('%s', '', 0), ),\n", _("This procedure has multiple return values."));

      if (blurb)
        g_print ("  'description': (%s, '', 0),\n", qescape (blurb));

      /* procedures/%s:0 is a lame substitute for the real file/line location */
      if (bse_type_get_file (type))
        {
          /* advance file to ->lastdir/basename.ext */
          const gchar *last = bse_type_get_file (type), *slash = strchr (last, '/');
          while (slash)
            {
              const gchar *base = slash + 1;
              slash = strchr (base, '/');
              if (slash)
                last = base;
            }
          g_print ("  'location': ('%s', %u),\n", last, bse_type_get_line (type));
        }
      else
        g_print ("  'location': ('procedures/%s', 0),\n", cseq->cats[i]->type);

      g_print ("},\n");
      g_type_class_unref (pclass);
      g_free (cname);
      g_free (sname);
    }
  g_print ("); # end of functions\n");
  bse_category_seq_free (cseq);
}

static void
show_structdoc (void)
{
  GType *children;
  guint i;

  g_print ("structures = (\n");

  children = g_type_children (G_TYPE_BOXED, NULL);
  for (i = 0; children[i]; i++)
    {
      GType type = children[i];
      SfiRecFields rfields = sfi_boxed_type_get_rec_fields (type);
      GParamSpec *element = sfi_boxed_type_get_seq_element (type);

      if (element || rfields.n_fields)
	{
	  const gchar *name = g_type_name (type);
	  gchar *cname = g_type_name_to_cname (name);
	  gchar *sname = g_type_name_to_sname (name);
	  const gchar *dname = rfields.n_fields ? "record" : "sequence";
	  const gchar *cstring;
	  SfiRing *ring, *pspecs = NULL;
	  guint j;

          g_print ("{\n");
          g_print ("  'name': '%s',\n", name);
          g_print ("  'hint': '%s',\n", dname);
          g_print ("  'aliases': [ ('%s', 'scheme'), ], # aliases\n", sname);
          g_print ("  'fields': [\n");
	  g_print ("\n");

          for (j = 0; j < rfields.n_fields; j++)
            pspecs = sfi_ring_append (pspecs, rfields.fields[j]);
          if (element)
	    pspecs = sfi_ring_append (pspecs, element);

	  if (element)
	    {
	      GParamSpec *pspec = (GParamSpec*) pspecs->data;
	      gchar *cname = g_type_name_to_cname (pspec->name);
              g_print ("  ('n_%s', 'guint', '', '', (%s, '', 0), ),\n", cname, qescape (_("Number of elements (C specific)")));
              g_free (cname);
	    }
	  for (ring = pspecs; ring; ring = sfi_ring_walk (ring, pspecs))
	    {
	      GParamSpec *pspec = (GParamSpec*) ring->data;
              print_pspec (pspec, "    ", element ? "*" : "");
	    }
          g_print ("  ],\n");

          GString *full_description = g_string_new ("");
	  cstring = bse_type_get_blurb (type);
          if (cstring)
            g_string_append (full_description, cstring);
          cstring = bse_type_get_authors (type);
          if (cstring)
            {
              if (full_description->str[0])
                g_string_append (full_description, "\n\n");
              g_string_append (full_description, _("Authors:"));
              g_string_append (full_description, " ");
              g_string_append (full_description, cstring);
            }
          cstring = bse_type_get_license (type);
          if (cstring)
            {
              if (full_description->str[0])
                g_string_append (full_description, "\n\n");
              g_string_append (full_description, _("License:"));
              g_string_append (full_description, " ");
              g_string_append (full_description, cstring);
            }

          if (full_description->str[0])
            g_print ("  'description': (%s, '', 0),\n", qescape (full_description->str));

          /* structures/%s:0 is a lame substitute for the real file/line location */
          if (bse_type_get_file (type))
            {
              /* advance file to ->lastdir/basename.ext */
              const gchar *last = bse_type_get_file (type), *slash = strchr (last, '/');
              while (slash)
                {
                  const gchar *base = slash + 1;
                  slash = strchr (base, '/');
                  if (slash)
                    last = base;
                }
              g_print ("  'location': ('%s', %u),\n", last, bse_type_get_line (type));
            }
          else
            g_print ("  'location': ('structures/%s', 0),\n", g_type_name (type));

          g_print ("},\n");
          g_string_free (full_description, TRUE);

	  g_free (cname);
	  g_free (sname);
	  sfi_ring_free (pspecs);
	}
    }
  g_print ("); # end of structures\n");
  g_free (children);
}

static gboolean
strequals (const gchar *s1,
           const gchar *s2)
{
  if (!s1 || !s2)
    return s1 == s2;
  return strcmp (s1, s2) == 0;
}

static void
showdoc_print_type (GObjectClass *oclass,
                    gboolean      show_channels)
{
  GType btype, type = G_OBJECT_CLASS_TYPE (oclass);
  guint j;
  const gchar *string;
  g_print ("{\n");
  g_print ("  'name': '%s',\n", g_type_name (type));

  GString *full_description = g_string_new ("");
  const gchar *cstring = bse_type_get_blurb (type);
  if (cstring)
    g_string_append (full_description, cstring);
  cstring = bse_type_get_authors (type);
  if (cstring)
    {
      if (full_description->str[0])
        g_string_append (full_description, "\n\n");
      g_string_append (full_description, _("Authors:"));
      g_string_append (full_description, " ");
      g_string_append (full_description, cstring);
    }
  cstring = bse_type_get_license (type);
  if (cstring)
    {
      if (full_description->str[0])
        g_string_append (full_description, "\n\n");
      g_string_append (full_description, _("License:"));
      g_string_append (full_description, " ");
      g_string_append (full_description, cstring);
    }
  if (full_description->str[0])
    g_print ("  'description': (%s, '', 0),\n", qescape (full_description->str));
  g_string_free (full_description, TRUE);

  g_print ("  'properties': [\n");
  btype = G_TYPE_OBJECT;
  do
    {
      GParamSpec **pspecs;
      btype = g_type_next_base (type, btype);
      pspecs = g_object_class_list_properties ((GObjectClass*) g_type_class_peek (btype), NULL);
      /* show GUI properties */
      if (type == btype || FALSE) /* always show all properties? */
        for (j = 0; pspecs[j]; j++)
          {
            GParamSpec *pspec = g_object_class_find_property (oclass, pspecs[j]->name);
            if (pspec->owner_type != btype || !sfi_pspec_check_option (pspec, "G"))
              continue;
            print_pspec (pspec, "    ", "");
          }
      g_free (pspecs);
    }
  while (btype != type);
  g_print ("  ],\n");

  /* show signals */
  guint n = 0, ns, *sigs = g_signal_list_ids (type, &n);
  g_print ("  'signals': [\n");
  for (ns = 0; ns < n; ns++)
    {
      GSignalQuery query;
      g_signal_query (sigs[ns], &query);
      g_print ("  ('%s',\n", query.signal_name);
      g_print ("   '%s',\n", g_type_name (query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE));
      g_print ("   (");
      guint np;
      for (np = 0; np < query.n_params; np++)
        g_print ("'%s',", g_type_name (query.param_types[np] & ~G_SIGNAL_TYPE_STATIC_SCOPE));
      g_print ("),\n");
      g_print ("  ),\n");
    }
  g_free (sigs);
  g_print ("  ],\n");

  /* show input and output channels */
  g_print ("  'channels': [\n");
  if (show_channels &&
      !G_TYPE_IS_ABSTRACT (type) &&
      g_type_is_a (type, BSE_TYPE_SOURCE) &&
      !strequals ("BseServer", g_type_name (type)))
    {
      BseSource *source = (BseSource*) g_object_new (type, NULL);
      for (j = 0; j < BSE_SOURCE_N_ICHANNELS (source); j++)
        {
          g_print ("  {\n");
          g_print ("    'id': %u,\n", j + 1);
          g_print ("    'name': '%s',\n", BSE_SOURCE_ICHANNEL_IDENT (source, j));
          g_print ("    'kind': '%s',\n", BSE_SOURCE_IS_JOINT_ICHANNEL (source, j) ? "JointInput" : "Input");
          g_print ("    'label': '%s',\n", BSE_SOURCE_ICHANNEL_LABEL (source, j));
          string = BSE_SOURCE_ICHANNEL_BLURB (source, j);
          if (string)
            g_print ("    'description': (%s, '', 0),\n", qescape (string));
          g_print ("  },\n");
        }
      for (j = 0; j < BSE_SOURCE_N_OCHANNELS (source); j++)
        {
          g_print ("  {\n");
          g_print ("    'id': %u,\n", j + 1);
          g_print ("    'name': '%s',\n", BSE_SOURCE_OCHANNEL_IDENT (source, j));
          g_print ("    'kind': '%s',\n", "Output");
          g_print ("    'label': '%s',\n", BSE_SOURCE_OCHANNEL_LABEL (source, j));
          string = BSE_SOURCE_OCHANNEL_BLURB (source, j);
          if (string)
            g_print ("    'description': (%s, '', 0),\n", qescape (string));
          g_print ("  },\n");
        }
      g_object_unref (source);
    }
  g_print ("  ],\n");

  g_print ("},\n");
}

static void
showdoc_descendants (GType type)
{
  GObjectClass *oclass = (GObjectClass*) g_type_class_ref (type);
  showdoc_print_type (oclass, TRUE);
  GType *child, *children = g_type_children (type, NULL);
  for (child = children; *child; child++)
    showdoc_descendants (*child);
  g_free (children);
  g_type_class_unref (oclass);
}

static gint
help (const gchar *name,
      const gchar *arg)
{
  if (arg)
    fprintf (stderr, "%s: unknown argument: %s\n", name, arg);
  fprintf (stderr, "usage: %s [-h] [-p] {procs|structs|objects}\n", name);
  fprintf (stderr, "  -p                  include plugins\n");
  fprintf (stderr, "  -s                  include scripts\n");
  fprintf (stderr, "  -h                  show help\n");
  fprintf (stderr, "  --seealso <link>    add a SEE ALSO section link\n");

  return arg != NULL;
}

int
main (gint   argc,
      gchar *argv[])
{
  GSList *seealso = NULL;
  gboolean gen_procs = FALSE;
  gboolean gen_structs = FALSE;
  gboolean gen_objects = FALSE;
  const char *pluginbool = "load-core-plugins=0";
  const char *scriptbool = "load-core-scripts=0";
  g_thread_init (NULL);
  bse_init_test (&argc, argv);
  boxed_type_tag = g_quark_from_static_string ("bse-auto-doc-boxed-type-tag");
  for (int i = 1; i < argc; i++)
    {
      if (strcmp ("-p", argv[i]) == 0)
        pluginbool = "load-core-plugins=1";
      else if (strcmp ("-s", argv[i]) == 0)
        scriptbool = "load-core-scripts=1";
      else if (strcmp ("--bse-rcfile", argv[i]) == 0 && i + 1 < argc)
        {
          /* ignore, BSE handles this */
          i++;
        }
      else if (strcmp ("procs", argv[i]) == 0)
	{
	  gen_procs = TRUE;
	}
      else if (strcmp ("structs", argv[i]) == 0)
	{
	  gen_structs = TRUE;
	}
      else if (strcmp ("objects", argv[i]) == 0)
	{
	  gen_objects = TRUE;
	}
      else if (strcmp ("--seealso", argv[i]) == 0)
	{
	  if (i + 1 < argc)
	    seealso = g_slist_append (seealso, argv[++i]);
	  else
	    return help (argv[0], argv[i]);
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (argv[0], NULL);
	}
      else if (strcmp ("--help", argv[i]) == 0)
	{
	  return help (argv[0], NULL);
	}
      else
	return help (argv[0], argv[i]);
    }
  bse_init_inprocess (&argc, argv, "BseAutoDoc", Bse::cstrings_to_vector (pluginbool, scriptbool, NULL));
  tag_all_boxed_pspecs ();
  if (gen_procs)
    show_procdoc ();
  else if (gen_structs)
    show_structdoc ();
  else if (gen_objects)
    {
      g_print ("objects = (\n");
      showdoc_descendants (BSE_TYPE_OBJECT);
      g_print (");\n");
    }
  else
    return help (argv[0], NULL);

  return 0;
}
