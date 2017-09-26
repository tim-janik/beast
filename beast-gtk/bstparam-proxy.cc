// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstcluehunter.hh"
#include <string.h>

/* --- SfiProxy parameter editors --- */
typedef struct {
  Bse::ItemSeq iseq;
  gchar      **paths = NULL;
  gchar       *prefix = NULL;
} ParamProxyPopulation;
static void
param_proxy_free_population (gpointer p)
{
  ParamProxyPopulation *pop = (ParamProxyPopulation*) p;
  g_strfreev (pop->paths);
  g_free (pop->prefix);
  delete pop;
}

static void
param_proxy_populate (GtkWidget *chunter,
		      GxkParam  *param)
{
  BstClueHunter *ch = BST_CLUE_HUNTER (chunter);
  ParamProxyPopulation *pop = NULL;
  SfiProxy proxy;
  gchar *p;
  guint l;

  /* clear current list */
  bst_clue_hunter_remove_matches (ch, "*");

  /* list candidates */
  Bse::PropertyCandidates pc;
  proxy = bst_param_get_proxy (param);
  if (proxy)
    pc = Bse::ItemH::down_cast (bse_server.from_proxy (proxy)).get_property_candidates (param->pspec->name);
  if (pc.items.size())
    {
      pop = new ParamProxyPopulation();
      pop->iseq = pc.items;
      pop->paths = NULL;
      pop->prefix = NULL;
      /* go from object to path name */
      for (size_t i = 0; i < pop->iseq.size(); i++)
        {
          Bse::ItemH item = pop->iseq[i];
          pop->paths = g_straddv (pop->paths, g_strdup (item.get_uname_path().c_str()));
        }
      if (!pop->paths || !pop->paths[0])
	{
	  param_proxy_free_population (pop);
	  pop = NULL;
	}
    }
  g_object_set_data_full (G_OBJECT (ch), "pop", pop, param_proxy_free_population);
  if (!pop)
    return;

  /* figure common prefix, aligned to path segment boundaries (':') */
  pop->prefix = g_strdup (pop->paths[0]);
  /* intersect */
  for (size_t i = 0; pop->paths[i]; i++)
    {
      const gchar *m = pop->paths[i];
      /* strdiff prefix against current path */
      p = pop->prefix;
      while (*p && *m)
	{
	  if (*p != *m)
	    break;
	  p++;
	  m++;
	}
      *p = 0; /* tail cut prefix at mismatch */
    }
  /* cut at object boundary */
  p = strrchr (pop->prefix, ':');
  if (p)
    {
      *(++p) = 0;
      p = g_strdup (pop->prefix);
    }
  g_free (pop->prefix);
  pop->prefix = p;
  l = pop->prefix ? strlen (pop->prefix) : 0;

  /* add unprefixed names to clue hunter */
  for (size_t i = 0; pop->paths[i]; i++)
    bst_clue_hunter_add_string (ch, pop->paths[i] + l);
}

static void
param_proxy_changed (GtkWidget *entry,
                     GxkParam  *param)
{
  if (!param->updating)
    {
      GtkWidget *chunter = (GtkWidget*) bst_clue_hunter_from_entry (entry);
      ParamProxyPopulation *pop = (ParamProxyPopulation*) g_object_get_data (G_OBJECT (chunter), "pop");
      gchar *string = g_strdup_stripped (gtk_entry_get_text (GTK_ENTRY (entry)));
      Bse::ItemH item;
      if (pop)
	{
	  guint i, l = strlen (string);
	  if (pop->prefix && l)			/* try exact match with prefix */
	    {
	      guint j = strlen (pop->prefix);
	      for (i = 0; pop->paths[i]; i++)
		if (strcmp (string, pop->paths[i] + j) == 0)
		  {
		    item = pop->iseq[i];
		    break;
		  }
	    }
	  if (!item && l)
	    for (i = 0; pop->paths[i]; i++)	/* try tail matching path */
	      {
		guint j = strlen (pop->paths[i]);
		if (j >= l && strcmp (string, pop->paths[i] + j - l) == 0)
		  {
		    item = pop->iseq[i];
		    break;
		  }
	      }
	  if (!item && pop->prefix && l)        /* try case insensitive exact match with prefix */
	    {
	      guint j = strlen (pop->prefix);
	      for (i = 0; pop->paths[i]; i++)
		if (g_ascii_strcasecmp (string, pop->paths[i] + j) == 0)
		  {
		    item = pop->iseq[i];
		    break;
		  }
	    }
	  if (!item && l)
	    for (i = 0; pop->paths[i]; i++)	/* try case insensitive tail matching path */
	      {
		guint j = strlen (pop->paths[i]);
		if (j >= l && g_ascii_strcasecmp (string, pop->paths[i] + j - l) == 0)
		  {
		    item = pop->iseq[i];
		    break;
		  }
	      }
	}
      /* we get lots of notifications from focus-out, so try to optimize */
      if (sfi_value_get_proxy (&param->value) != item.proxy_id())
	{
	  sfi_value_set_proxy (&param->value, item.proxy_id());
	  gxk_param_apply_value (param);
	}
      else if (!item && string[0]) /* make sure the entry is correctly updated */
        gxk_param_update (param);
      g_free (string);
    }
}

SfiProxy
bst_item_seq_list_match (GSList      *item_seq_slist,
                         const gchar *text)
{
  SfiProxy cmatch = 0, tmatch = 0, tcmatch = 0;
  GSList *slist;
  guint l, i;
  if (!text || !text[0])
    return 0;
  l = strlen (text);
  for (slist = item_seq_slist; slist; slist = slist->next)
    {
      BseIt3mSeq *iseq = (BseIt3mSeq*) slist->data;
      for (i = 0; i < iseq->n_items; i++)
	{
          Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (iseq->items[i]));
	  const String path = item.get_uname_path();
	  uint j = path.size();
	  if (j == l && Bse::string_cmp (text, path) == 0)
	    return iseq->items[i];	/* found exact match */
	  else if (!cmatch && j == l && Bse::string_casecmp (text, path) == 0)
	    cmatch = iseq->items[i];	/* remember first case insensitive match */
	  else if (!tmatch && j > l && Bse::string_cmp (text, &path[0] + j - l) == 0)
	    tmatch = iseq->items[i];	/* remember first tail match */
	  else if (!tcmatch && j > l && Bse::string_casecmp (text, &path[0] + j - l) == 0)
	    tcmatch = iseq->items[i];	/* remember first case insensitive tail match */
	}
    }
  /* fallback to tail match, then case insensitive matches */
  return tmatch ? tmatch : cmatch ? cmatch : tcmatch;
}

static GtkWidget*
param_proxy_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *box = gtk_hbox_new (FALSE, 0);
  GtkWidget *widget = (GtkWidget*) g_object_new (GTK_TYPE_ENTRY,
				    "activates_default", TRUE,
                                    "parent", box,
                                    "width_chars", 0,
				    NULL);
  GtkWidget *chunter = (GtkWidget*) g_object_new (BST_TYPE_CLUE_HUNTER,
				     "keep_history", FALSE,
				     "entry", widget,
				     "user_data", param,
                                     "align-widget", box,
				     NULL);
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (proxy));
      Bse::PropertyCandidates pc = item.get_property_candidates (param->pspec->name);
      gxk_widget_set_tooltip (chunter, pc.tooltip.c_str());
    }
  gxk_widget_add_font_requisition (widget, 16, 2);
  gxk_param_entry_connect_handlers (param, widget, param_proxy_changed);
  g_object_connect (chunter,
		    "signal::poll_refresh", param_proxy_populate, param,
		    NULL);
  GtkWidget *arrow = bst_clue_hunter_create_arrow (BST_CLUE_HUNTER (chunter), TRUE);
  gtk_box_pack_end (GTK_BOX (box), arrow, FALSE, TRUE, 0);
  gtk_widget_show_all (box);
  gxk_widget_set_tooltip (widget, tooltip);
  gxk_widget_set_tooltip (arrow, tooltip);
  gxk_widget_add_option (box, "hexpand", "+");
  return box;
}

static void
param_proxy_update (GxkParam  *param,
		    GtkWidget *box)
{
  SfiProxy proxy = sfi_value_get_proxy (&param->value);
  Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (proxy));
  const String upath = item ? item.get_uname_path() : "";
  const char *cstring = upath.c_str();
  GtkWidget *entry = ((GtkBoxChild*) GTK_BOX (box)->children->data)->widget;
  GtkWidget *chunter = (GtkWidget*) bst_clue_hunter_from_entry (entry);

  if (cstring && chunter)
    {
      ParamProxyPopulation *pop = (ParamProxyPopulation*) g_object_get_data (G_OBJECT (chunter), "pop");
      if (!pop)
	{
	  /* try populating now */
	  param_proxy_populate (chunter, param);
	  pop = (ParamProxyPopulation*) g_object_get_data (G_OBJECT (chunter), "pop");
	}
      if (pop)
	{
	  /* strip common prefix */
	  if (pop->prefix)
	    {
	      guint l = strlen (pop->prefix);
	      if (strncmp (pop->prefix, cstring, l) == 0)
		cstring += l;
	      else
		{
		  /* prefix became invalid */
		  param_proxy_populate (chunter, param);
		  pop = (ParamProxyPopulation*) g_object_get_data (G_OBJECT (chunter), "pop");
		  if (pop && pop->prefix)
		    {
		      l = strlen (pop->prefix);
		      if (strncmp (pop->prefix, cstring, l) == 0)
			cstring += l;
		      else
			{
			  /* heck, prefix became invalid _again_? */
			  g_object_set_data (G_OBJECT (chunter), "pop", NULL);
			}
		    }
		}
	    }
	}
      if (strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), cstring))
	gtk_entry_set_text (GTK_ENTRY (entry), cstring);
    }
  else if (strcmp (gtk_entry_get_text (GTK_ENTRY (entry)), ""))
    gtk_entry_set_text (GTK_ENTRY (entry), "");
  gtk_editable_set_editable (GTK_EDITABLE (entry), param->editable);
}

static GxkParamEditor param_proxy = {
  { "proxy",            N_("Object Drop Down Box"), },
  { G_TYPE_POINTER,     "SfiProxy", },
  { NULL,       +5,     TRUE, },        /* options, rating, editing */
  param_proxy_create,   param_proxy_update,
};
