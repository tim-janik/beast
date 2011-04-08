/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#include        "bsecategories.h"
#include        "bsesource.h"
#include        "bseprocedure.h"
#include        "bsemain.h"
#include        "bsestandardsynths.h"
#include        "topconfig.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>

static gchar *indent_inc = NULL;
static guint spacing = 1;
static FILE *f_out = NULL;
static GType root = 0;
static gboolean recursion = TRUE;
static gboolean feature_blurb = FALSE;
static gboolean feature_channels = FALSE;

/*
  #define	O_SPACE	"\\as"
  #define	O_ESPACE " "
  #define	O_BRANCH "\\aE"
  #define	O_VLINE "\\al"
  #define	O_LLEAF	"\\aL"
  #define	O_KEY_FILL "_"
*/

#define	O_SPACE	" "
#define	O_ESPACE ""
#define	O_BRANCH "+"
#define	O_VLINE "|"
#define	O_LLEAF	"`"
#define	O_KEY_FILL "_"

static void
show_nodes (GType        type,
	    GType        sibling,
	    const gchar *indent)
{
  GType   *children;
  guint i;

  if (!type)
    return;

  children = g_type_children (type, NULL);

  if (type != root)
    for (i = 0; i < spacing; i++)
      fprintf (f_out, "%s%s\n", indent, O_VLINE);
  
  fprintf (f_out, "%s%s%s%s",
	   indent,
	   sibling ? O_BRANCH : (type != root ? O_LLEAF : O_SPACE),
	   O_ESPACE,
	   g_type_name (type));

  for (i = strlen (g_type_name (type)); i <= strlen (indent_inc); i++)
    fputs (O_KEY_FILL, f_out);

  if (feature_blurb && bse_type_get_blurb (type))
    {
      fputs ("\t[", f_out);
      fputs (bse_type_get_blurb (type), f_out);
      fputs ("]", f_out);
    }

  if (G_TYPE_IS_ABSTRACT (type))
    fputs ("\t(abstract)", f_out);

  if (feature_channels && g_type_is_a (type, BSE_TYPE_SOURCE))
    {
      BseSourceClass *class = g_type_class_ref (type);
      gchar buffer[1024];

      sprintf (buffer,
	       "\t(ichannels %u) (ochannels %u)",
	       class->channel_defs.n_ichannels,
	       class->channel_defs.n_ochannels);
      fputs (buffer, f_out);
      g_type_class_unref (class);
    }

  fputc ('\n', f_out);
  
  if (children && recursion)
    {
      gchar *new_indent;
      GType   *child;

      if (sibling)
	new_indent = g_strconcat (indent, O_VLINE, indent_inc, NULL);
      else
	new_indent = g_strconcat (indent, O_SPACE, indent_inc, NULL);

      for (child = children; *child; child++)
	show_nodes (child[0], child[1], new_indent);

      g_free (new_indent);
    }

  g_free (children);
}

static void
show_cats (void)
{
  BseCategorySeq *cseq;
  guint i;

  cseq = bse_categories_match_typed ("*", 0);
  for (i = 0; i < cseq->n_cats; i++)
    fprintf (f_out, "%s\t(%s)\n",
	     cseq->cats[i]->category,
	     cseq->cats[i]->type);
  bse_category_seq_free (cseq);
}

static void
show_procdoc (void)
{
  BseCategorySeq *cseq;
  guint i;
  const gchar *nullstr = ""; // "???";

  cseq = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE);
  for (i = 0; i < cseq->n_cats; i++)
    {
      GType type = g_type_from_name (cseq->cats[i]->type);
      BseProcedureClass *class = g_type_class_ref (type);
      gchar *pname = g_type_name_to_sname (cseq->cats[i]->type);
      const gchar *blurb = bse_type_get_blurb (type);
      guint j;

      fprintf (f_out, "/**\n * %s\n", pname);
      for (j = 0; j < class->n_in_pspecs; j++)
	{
	  GParamSpec *pspec = G_PARAM_SPEC (class->in_pspecs[j]);

	  fprintf (f_out, " * @%s: %s\n",
		   pspec->name,
		   g_param_spec_get_blurb (pspec) ? g_param_spec_get_blurb (pspec) : nullstr);
	}
      for (j = 0; j < class->n_out_pspecs; j++)
	{
	  GParamSpec *pspec = G_PARAM_SPEC (class->out_pspecs[j]);

	  fprintf (f_out, " * @Returns: %s: %s\n",
		   pspec->name,
		   g_param_spec_get_blurb (pspec) ? g_param_spec_get_blurb (pspec) : nullstr);
	}
      if (blurb)
	fprintf (f_out, " * %s\n", blurb);
      fprintf (f_out, " **/\n");
      g_type_class_unref (class);
      g_free (pname);
    }
  bse_category_seq_free (cseq);
}

static gint
help (gchar *arg)
{
  fprintf (stderr, "usage: query <qualifier> [-r <type>] [-{i|b} \"\"] [-s #] [-{h|x|y}]\n");
  fprintf (stderr, "       -r       specifiy root type\n");
  fprintf (stderr, "       -n       don't descend type tree\n");
  fprintf (stderr, "       -p       include plugins\n");
  fprintf (stderr, "       -x       show type blurbs\n");
  fprintf (stderr, "       -y       show source channels\n");
  fprintf (stderr, "       -h       guess what ;)\n");
  fprintf (stderr, "       -b       specifiy indent string\n");
  fprintf (stderr, "       -i       specifiy incremental indent string\n");
  fprintf (stderr, "       -s       specifiy line spacing\n");
  fprintf (stderr, "       -:f      make all warnings fatal\n");
  fprintf (stderr, "qualifiers:\n");
  fprintf (stderr, "       froots     iterate over fundamental roots\n");
  fprintf (stderr, "       tree       print BSE type tree\n");
  fprintf (stderr, "       cats       print categories\n");
  fprintf (stderr, "       procdoc    print procedure documentation\n");
  fprintf (stderr, "       synthlist  list standard synths\n");
  fprintf (stderr, "       synth <x>  dump standard synth <x> definition\n");

  return arg != NULL;
}

int
main (gint   argc,
      gchar *argv[])
{
  gboolean gen_froots = 0;
  gboolean gen_tree = 0;
  gboolean gen_cats = 0;
  gboolean gen_procdoc = 0;
  gboolean list_synths = 0;
  gchar *show_synth = NULL;
  gchar *root_name = NULL;
  gchar *iindent = "";
  char pluginbool[2] = "0";
  char scriptbool[2] = "0";
  SfiInitValue config[] = {
    { "load-core-plugins", pluginbool },
    { "load-core-scripts", scriptbool },
    { NULL },
  };

  f_out = stdout;

  g_thread_init (NULL);

  sfi_init (&argc, &argv, "BseQuery", NULL);
  
  guint i;
  for (i = 1; i < argc; i++)
    {
      if (strcmp ("-s", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    spacing = atoi (argv[i]);
	}
      else if (strcmp ("-i", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    {
	      char *p;
	      guint n;
	      
	      p = argv[i];
	      while (*p)
		p++;
	      n = p - argv[i];
	      indent_inc = g_new (gchar, n * strlen (O_SPACE) + 1);
	      *indent_inc = 0;
	      while (n)
		{
		  n--;
		  strcpy (indent_inc, O_SPACE);
		}
	    }
	}
      else if (strcmp ("-b", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    iindent = argv[i];
	}
      else if (strcmp ("-r", argv[i]) == 0)
	{
	  i++;
	  if (i < argc)
	    root_name = argv[i];
	}
      else if (strcmp ("-n", argv[i]) == 0)
	{
	  recursion = FALSE;
	}
      else if (strcmp ("-x", argv[i]) == 0)
	{
	  feature_blurb = TRUE;
	}
      else if (strcmp ("-y", argv[i]) == 0)
	{
	  feature_channels = TRUE;
	}
      else if (strcmp ("froots", argv[i]) == 0)
	{
	  gen_froots = 1;
	}
      else if (strcmp ("synthlist", argv[i]) == 0)
	{
	  list_synths = 1;
	}
      else if (strcmp ("synth", argv[i]) == 0 && i + 1 < argc)
	{
	  show_synth = argv[++i];
	}
      else if (strcmp ("tree", argv[i]) == 0)
	{
	  gen_tree = 1;
	}
      else if (strcmp ("cats", argv[i]) == 0)
	{
	  gen_cats = 1;
	}
      else if (strcmp ("procdoc", argv[i]) == 0)
	{
	  gen_procdoc = 1;
	}
      else if (strcmp ("-p", argv[i]) == 0)
        pluginbool[1] = '1';
      else if (strcmp ("-:f", argv[i]) == 0)
	{
	  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else if (strcmp ("--help", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	return help (argv[i]);
    }

  bse_init_inprocess (&argc, &argv, "BseQuery", config);

  if (root_name)
    root = g_type_from_name (root_name);
  else
    root = BSE_TYPE_OBJECT;

  if (!gen_froots && !gen_tree && !gen_cats && !gen_procdoc && !list_synths && !show_synth)
    return help (argv[i-1]);

  if (!indent_inc)
    {
      indent_inc = g_new (gchar, strlen (O_SPACE) + 1);
      *indent_inc = 0;
      strcpy (indent_inc, O_SPACE);
      strcpy (indent_inc, O_SPACE);
      strcpy (indent_inc, O_SPACE);
    }

  if (gen_tree)
    show_nodes (root, 0, iindent);
  if (gen_froots)
    {
      root = ~0;
      for (i = 0; i <= G_TYPE_FUNDAMENTAL_MAX; i += G_TYPE_MAKE_FUNDAMENTAL (1))
	{
	  gchar *name = g_type_name (i);
	  
	  if (name)
	    show_nodes (i, 0, iindent);
	}
    }
  if (gen_cats)
    show_cats ();
  if (gen_procdoc)
    show_procdoc ();
  if (list_synths)
    {
      GSList *node = bse_standard_synth_get_list ();
      for (; node; node = node->next)
	g_print ("  %s\n", (gchar*) node->data);
    }
  if (show_synth)
    {
      gchar *text = bse_standard_synth_inflate (show_synth, NULL);
      g_print ("%s", text);
      g_free (text);
    }

  return 0;
}
