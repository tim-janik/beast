// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html


/* --- item sequence editors --- */
#include "bstitemseqdialog.hh"
static void
param_item_seq_changed (gpointer             data,
                        BseIt3mSeq          *iseq,
                        BstItemSeqDialog    *isdialog)
{
  GxkParam *param = (GxkParam*) data;
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      SfiSeq *seq = bse_it3m_seq_to_seq (iseq);
      sfi_value_take_seq (&param->value, seq);
      gxk_param_apply_value (param);
    }
}

static void
param_item_seq_popup_editor (GtkWidget *widget,
                             GxkParam  *param)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      Bse::ItemH item = Bse::ItemH::__cast__ (bse_server.from_proxy (proxy));
      Bse::PropertyCandidates pc = item.get_property_candidates (param->pspec->name);
      SfiSeq *seq = (SfiSeq*) g_value_get_boxed (&param->value);
      BseIt3mSeq *iseq = bse_it3m_seq_from_seq (seq);
      BseIt3mSeq *pc_items = bst_it3m_seq_from_item_seq (pc.items);
      bst_item_seq_dialog_popup (widget, proxy,
                                 pc.label.c_str(), pc.tooltip.c_str(), pc_items,
                                 g_param_spec_get_nick (param->pspec), g_param_spec_get_blurb (param->pspec), iseq,
                                 param_item_seq_changed,
                                 param, NULL);
      bse_it3m_seq_free (pc_items);
      bse_it3m_seq_free (iseq);
    }
}

static GtkWidget*
param_item_seq_create (GxkParam    *param,
                       const gchar *tooltip,
                       guint        variant)
{
  /* create entry-look-alike dialog-popup button with "..." indicator */
  GtkWidget *widget = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                    "can-focus", 1,
                                    NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_HBOX,
                                 "parent", widget,
                                 "spacing", 3,
                                 NULL);
  GtkWidget *label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                   "label", _("..."),
                                   NULL);
  gtk_box_pack_end (GTK_BOX (box), label, FALSE, TRUE, 0);
  GtkWidget *frame = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                   "shadow-type", GTK_SHADOW_IN,
                                   "border-width", 1,
                                   "parent", box,
                                   NULL);
  gxk_widget_modify_normal_bg_as_base (frame);
  GtkWidget *ebox = (GtkWidget*) g_object_new (GTK_TYPE_EVENT_BOX,
                                  "parent", frame,
                                  NULL);
  gxk_widget_modify_normal_bg_as_base (ebox);
  label = (GtkWidget*) g_object_new (GXK_TYPE_SIMPLE_LABEL, "parent", ebox, "auto-cut", TRUE, "xpad", 2, NULL);
  gxk_widget_modify_normal_bg_as_base (label);
  /* store handles */
  g_object_set_data ((GObject*) widget, "beast-GxkParam", param);
  g_object_set_data ((GObject*) widget, "beast-GxkParam-label", label);
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
      Bse::ItemH item = Bse::ItemH::__cast__ (bse_server.from_proxy (proxy));
      Bse::PropertyCandidates pc = item.get_property_candidates (param->pspec->name);
      SfiSeq *seq = (SfiSeq*) g_value_get_boxed (&param->value);
      BseIt3mSeq *iseq = seq ? bse_it3m_seq_from_seq (seq) : NULL;
      if (iseq)
        {
          if (iseq->n_items == 1)
            {
              Bse::ItemH item = Bse::ItemH::__cast__ (bse_server.from_proxy (iseq->items[0]));
              content = g_strdup (item.get_name_or_type().c_str());
            }
          else if (iseq->n_items > 1 && pc.partitions.empty())
            content = g_strdup_format ("#%u", iseq->n_items);
          else if (iseq->n_items > 1) /* && partitions->n_types */
            {
              guint i, other = 0, *partitions = g_newa (guint, pc.partitions.size());
              memset (partitions, 0, pc.partitions.size() * sizeof (guint));
              for (i = 0; i < iseq->n_items; i++)
                {
                  size_t j;
                  for (j = 0; j < pc.partitions.size(); j++)
                    {
                      Bse::ItemH item = Bse::ItemH::__cast__ (bse_server.from_proxy (iseq->items[i]));
                      if (item.check_is_a (pc.partitions[j]))
                        {
                          partitions[j]++;
                          break;
                        }
                    }
                  if (j >= pc.partitions.size())
                    other++;
                }
              GString *gstring = g_string_new ("");
              for (size_t j = 0; j < pc.partitions.size(); j++)
                g_string_append_printf (gstring, "%s#%u", j ? " & " : "", partitions[j]);
              if (other)
                g_string_append_printf (gstring, " & #%u", other);
              content = g_string_free (gstring, FALSE);
            }
          bse_it3m_seq_free (iseq);
        }
    }
  GtkWidget *label = (GtkWidget*) g_object_get_data ((GObject*) widget, "beast-GxkParam-label");
  g_object_set (label,
                "label", content ? content : "--",
                NULL);
  g_free (content);
}

static GxkParamEditor param_it3m_seq = {
  { "item-list",        N_("Item List"), },
  { G_TYPE_BOXED,       "SfiSeq", },
  { "item-sequence",    +5,     TRUE, },        /* options, rating, editing */
  param_item_seq_create, param_item_seq_update,
};
