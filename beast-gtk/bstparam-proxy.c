/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bstcluehunter.h"
#include <string.h>


/* --- proxy parameters --- */
typedef struct {
  BseProxySeq *pseq;
  gchar      **paths;
  gchar       *prefix;
} ParamProxyPopulation;
static void
param_proxy_free_population (gpointer p)
{
  ParamProxyPopulation *pop = p;
  g_strfreev (pop->paths);
  g_free (pop->prefix);
  bse_proxy_seq_free (pop->pseq);
  g_free (pop);
}

static void
param_proxy_populate (GtkWidget *chunter,
		      BstParam  *bparam)
{
  BstClueHunter *ch = BST_CLUE_HUNTER (chunter);
  ParamProxyPopulation *pop = NULL;
  BseProxySeq *pseq;
  gchar *p;
  guint i, l;

  /* clear current list */
  bst_clue_hunter_remove_matches (ch, "*");

  /* list candidates */
  pseq = bparam->binding->list_proxies (bparam);
  if (pseq)
    {
      pop = g_new (ParamProxyPopulation, 1);
      pop->pseq = pseq;
      pop->paths = NULL;
      pop->prefix = NULL;
      /* go from object to path name */
      for (i = 0; i < pseq->n_proxies; i++)
	pop->paths = g_straddv (pop->paths, g_strdup (bse_item_get_uname_path (pseq->proxies[i])));
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
  for (i = 0; pop->paths[i]; i++)
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
  for (i = 0; pop->paths[i]; i++)
    bst_clue_hunter_add_string (ch, pop->paths[i] + l);
}

static void
param_proxy_change_value (GtkWidget *action,
			  BstParam  *bparam)
{
  if (!bparam->updating)
    {
      GtkWidget *chunter = bst_clue_hunter_from_entry (action);
      ParamProxyPopulation *pop = g_object_get_data (G_OBJECT (chunter), "pop");
      gchar *string = g_strdup_stripped (gtk_entry_get_text (GTK_ENTRY (action)));
      SfiProxy item = 0;
      if (pop)
	{
	  guint i, l = strlen (string);
	  if (pop->prefix && l)			/* try exact match with prefix */
	    {
	      guint j = strlen (pop->prefix);
	      for (i = 0; pop->paths[i]; i++)
		if (strcmp (string, pop->paths[i] + j) == 0)
		  {
		    item = pop->pseq->proxies[i];
		    break;
		  }
	    }
	  if (!item && l)
	    for (i = 0; pop->paths[i]; i++)	/* try tail matching path */
	      {
		guint j = strlen (pop->paths[i]);
		if (j >= l && strcmp (string, pop->paths[i] + j - l) == 0)
		  {
		    item = pop->pseq->proxies[i];
		    break;
		  }
	      }
	  if (!item && pop->prefix && l)        /* try case insensitive exact match with prefix */
	    {
	      guint j = strlen (pop->prefix);
	      for (i = 0; pop->paths[i]; i++)
		if (g_ascii_strcasecmp (string, pop->paths[i] + j) == 0)
		  {
		    item = pop->pseq->proxies[i];
		    break;
		  }
	    }
	  if (!item && l)
	    for (i = 0; pop->paths[i]; i++)	/* try case insensitive tail matching path */
	      {
		guint j = strlen (pop->paths[i]);
		if (j >= l && g_ascii_strcasecmp (string, pop->paths[i] + j - l) == 0)
		  {
		    item = pop->pseq->proxies[i];
		    break;
		  }
	      }
	}
      /* we get lots of notifications from focus-out, so try to optimize */
      if (sfi_value_get_proxy (&bparam->value) != item)
	{
	  sfi_value_set_proxy (&bparam->value, item);
	  bst_param_apply_value (bparam);
	}
      else if (!item && string[0]) /* make sure the entry is correctly updated */
        bst_param_update (bparam);
      g_free (string);
    }
}

SfiProxy
bst_proxy_seq_list_match (GSList      *proxy_seq_slist,
			  const gchar *text)
{
  SfiProxy cmatch = 0, tmatch = 0, tcmatch = 0;
  GSList *slist;
  guint l, i;
  if (!text || !text[0])
    return 0;
  l = strlen (text);
  for (slist = proxy_seq_slist; slist; slist = slist->next)
    {
      BseProxySeq *pseq = slist->data;
      for (i = 0; i < pseq->n_proxies; i++)
	{
	  const gchar *path = bse_item_get_uname_path (pseq->proxies[i]);
	  guint j = path ? strlen (path) : 0;
	  if (j == l && strcmp (text, path) == 0)
	    return pseq->proxies[i];	/* found exact match */
	  else if (!cmatch && j == l && g_strcasecmp (text, path) == 0)
	    cmatch = pseq->proxies[i];	/* remember first case insensitive match */
	  else if (!tmatch && j > l && strcmp (text, path + j - l) == 0)
	    tmatch = pseq->proxies[i];	/* remember first tail match */
	  else if (!tcmatch && j > l && g_strcasecmp (text, path + j - l) == 0)
	    tcmatch = pseq->proxies[i];	/* remember first case insensitive tail match */
	}
    }
  /* fallback to tail match, then case insensitive matches */
  return tmatch ? tmatch : cmatch ? cmatch : tcmatch;
}

static gboolean
param_proxy_focus_out (GtkWidget     *entry,
		       GdkEventFocus *event,
		       BstParam      *bparam)
{
  param_proxy_change_value (entry, bparam);
  return FALSE;
}

static GtkWidget*
param_proxy_create_action (BstParam   *bparam,
			   GtkWidget **post_action)
{
  GtkWidget *action = g_object_new (GTK_TYPE_ENTRY,
				    "visible", TRUE,
				    "width_request", 250,
				    "activates_default", TRUE,
				    NULL);
  GtkWidget *chunter = g_object_new (BST_TYPE_CLUE_HUNTER,
				     "keep_history", FALSE,
				     "entry", action,
				     "user_data", bparam,
				     NULL);
  g_object_connect (action,
		    "signal::key_press_event", bst_param_entry_key_press, NULL,
		    "signal::activate", param_proxy_change_value, bparam,
		    "signal::focus_out_event", param_proxy_focus_out, bparam,
		    NULL);
  g_object_connect (chunter,
		    "signal::poll_refresh", param_proxy_populate, bparam,
		    NULL);

  if (post_action)
    *post_action = bst_clue_hunter_create_arrow (BST_CLUE_HUNTER (chunter));

  return action;
}

static BstGMask*
param_proxy_create_gmask (BstParam    *bparam,
			  const gchar *tooltip,
			  GtkWidget   *gmask_parent)
{
  GtkWidget *action, *prompt, *xframe, *post_action;
  BstGMask *gmask;
  
  action = param_proxy_create_action (bparam, &post_action);
  
  xframe = g_object_new (BST_TYPE_XFRAME,
			 "visible", TRUE,
			 "cover", action,
			 NULL);
  g_object_connect (xframe,
		    "swapped_signal::button_check", bst_param_xframe_check_button, bparam,
		    NULL);
  prompt = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
			 "label", g_param_spec_get_nick (bparam->pspec),
			 "xalign", 0.0,
			 "parent", xframe,
			 NULL);

  gmask = bst_gmask_form (gmask_parent, action, BST_GMASK_INTERLEAVE);
  bst_gmask_set_prompt (gmask, prompt);
  bst_gmask_set_atail (gmask, post_action);
  bst_gmask_set_tip (gmask, tooltip);
  
  return gmask;
}

static GtkWidget*
param_proxy_create_widget (BstParam    *bparam,
			   const gchar *tooltip)
{
  GtkWidget *action = param_proxy_create_action (bparam, NULL);
  
  return action;
}

static void
param_proxy_update (BstParam  *bparam,
		    GtkWidget *action)
{
  SfiProxy item = sfi_value_get_proxy (&bparam->value);
  const gchar *cstring = item ? bse_item_get_uname_path (item) : NULL;
  GtkWidget *chunter = bst_clue_hunter_from_entry (action);

  if (cstring && chunter)
    {
      ParamProxyPopulation *pop = g_object_get_data (G_OBJECT (chunter), "pop");
      if (!pop)
	{
	  /* try populating now */
	  param_proxy_populate (chunter, bparam);
	  pop = g_object_get_data (G_OBJECT (chunter), "pop");
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
		  param_proxy_populate (chunter, bparam);
		  pop = g_object_get_data (G_OBJECT (chunter), "pop");
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
      if (strcmp (gtk_entry_get_text (GTK_ENTRY (action)), cstring))
	gtk_entry_set_text (GTK_ENTRY (action), cstring);
    }
  else if (strcmp (gtk_entry_get_text (GTK_ENTRY (action)), ""))
    gtk_entry_set_text (GTK_ENTRY (action), "");
}

struct _BstParamImpl param_proxy = {
  "Proxy",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE | BST_PARAM_PROXY_LIST,
  SFI_SCAT_PROXY,	NULL /* hints */,
  param_proxy_create_gmask,
  NULL, /* create_widget */
  param_proxy_update,
};

struct _BstParamImpl rack_proxy = {
  "Proxy",		+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE | BST_PARAM_PROXY_LIST,
  SFI_SCAT_PROXY,	NULL /* hints */,
  NULL, /* create_gmask */
  param_proxy_create_widget,
  param_proxy_update,
};
