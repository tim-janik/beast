/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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


/* --- item sequence editors --- */
#include "bstitemseqdialog.h"
static void
param_item_seq_changed (gpointer             data,
                        BseItemSeq          *iseq,
                        BstItemSeqDialog    *isdialog)
{
  GxkParam *param = data;
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      SfiSeq *seq = bse_item_seq_to_seq (iseq);
      GValue *value = sfi_value_seq (seq);
      bse_proxy_set_property (proxy, "inputs", value);
      sfi_value_free (value);
    }
}

static void
param_item_seq_popup_editor (GtkWidget *widget,
                             GxkParam  *param)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      BsePropertyCandidates *pc = bse_item_get_property_candidates (proxy, "inputs");
      const GValue *value = bse_proxy_get_property (proxy, "inputs");
      SfiSeq *seq = g_value_get_boxed (value);
      BseItemSeq *iseq = bse_item_seq_from_seq (seq);
      bst_item_seq_dialog_popup (widget, proxy,
                                 pc->nick, pc->tooltip, pc->items,
                                 g_param_spec_get_nick (param->pspec), g_param_spec_get_blurb (param->pspec), iseq,
                                 param_item_seq_changed,
                                 param);
      bse_item_seq_free (iseq);
    }
}

static GtkWidget*
param_item_seq_create (GxkParam    *param,
                       const gchar *tooltip,
                       guint        variant)
{
  /* create entry-look-alike dialog-popup button with "..." indicator */
  GtkWidget *widget = g_object_new (GTK_TYPE_BUTTON,
                                    "can-focus", 1,
                                    NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  GtkWidget *box = g_object_new (GTK_TYPE_HBOX,
                                 "parent", widget,
                                 "spacing", 3,
                                 NULL);
  GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
                                   "label", _("..."),
                                   NULL);
  gtk_box_pack_end (GTK_BOX (box), label, FALSE, TRUE, 0);
  GtkWidget *frame = g_object_new (GTK_TYPE_FRAME,
                                   "shadow-type", GTK_SHADOW_IN,
                                   "border-width", 1,
                                   "parent", box,
                                   NULL);
  gxk_widget_modify_normal_bg_as_base (frame);
  GtkWidget *ebox = g_object_new (GTK_TYPE_EVENT_BOX,
                                  "parent", frame,
                                  NULL);
  gxk_widget_modify_normal_bg_as_base (ebox);
  label = g_object_new (GXK_TYPE_SIMPLE_LABEL, "parent", ebox, "auto-cut", TRUE, "xpad", 2, NULL);
  gxk_widget_modify_normal_bg_as_base (label);
  /* store handles */
  g_object_set_data (widget, "beast-GxkParam", param);
  g_object_set_data (widget, "beast-GxkParam-label", label);
  /* connections */
  g_object_connect (widget, "signal::clicked", param_item_seq_popup_editor, param, NULL);
  gtk_widget_show_all (widget);
  /* gxk_widget_add_option (box, "hexpand", "+"); */
  return widget;
}

static void
param_item_seq_update (GxkParam  *param,
                       GtkWidget *widget)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  gchar *content = NULL;
  if (proxy)
    {
      BsePropertyCandidates *pc = bse_item_get_property_candidates (proxy, "inputs");
      const GValue *value = bse_proxy_get_property (proxy, "inputs");
      SfiSeq *seq = g_value_get_boxed (value);
      BseItemSeq *iseq = bse_item_seq_from_seq (seq);
      if (iseq)
        {
          if (iseq->n_items == 1)
            content = g_strdup_printf ("%s", bse_item_get_name_or_type (iseq->items[0]));
          else if (iseq->n_items > 1 && (!pc->partitions || pc->partitions->n_types == 0))
            content = g_strdup_printf ("#%u", iseq->n_items);
          else if (iseq->n_items > 1) /* && partitions->n_types */
            {
              guint i, j, other = 0, *partitions = g_newa (guint, pc->partitions->n_types);
              memset (partitions, 0, pc->partitions->n_types * sizeof (partitions[0]));
              for (i = 0; i < iseq->n_items; i++)
                {
                  for (j = 0; j < pc->partitions->n_types; j++)
                    if (bse_item_check_is_a (iseq->items[i], pc->partitions->types[j]))
                      {
                        partitions[j]++;
                        break;
                      }
                  if (j >= pc->partitions->n_types)
                    other++;
                }
              GString *gstring = g_string_new ("");
              for (j = 0; j < pc->partitions->n_types; j++)
                g_string_append_printf (gstring, "%s#%u", j ? " & " : "", partitions[j]);
              if (other)
                g_string_append_printf (gstring, " & #%u", other);
              content = g_string_free (gstring, FALSE);
            }
          bse_item_seq_free (iseq);
        }
    }
  GtkWidget *label = g_object_get_data (widget, "beast-GxkParam-label");
  g_object_set (label,
                "label", content ? content : "--",
                NULL);
  g_free (content);
}

static GxkParamEditor param_item_seq = {
  { "item-list",        N_("Item List"), },
  { G_TYPE_BOXED,       "SfiSeq", },
  { "item-sequence",    +5,     TRUE, },        /* options, rating, editing */
  param_item_seq_create, param_item_seq_update,
};
