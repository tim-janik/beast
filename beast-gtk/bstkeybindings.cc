// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "topconfig.h"  /* BST_PATH_KEYS */
#include "bstkeybindings.hh"
#include "bstauxdialogs.hh"
#include "bstpatternctrl.hh"
#include "bstfiledialog.hh"
#include <string.h>

enum {
  ACTION_ADD,
  ACTION_EDIT,
  ACTION_DELETE,
  ACTION_LOWER,
  ACTION_RAISE,
  ACTION_LOAD
};

enum {
  BCOL_KEY,
  BCOL_FUNCTION,
  BCOL_PARAM,
  N_BCOLS
};

enum {
  FCOL_NAME,
  FCOL_BLURB,
  N_FCOLS
};


/* --- variables --- */
static const GxkStockAction key_bindings_actions[] = {
  { N_("Add..."),               "",             N_("Bind a new key combination to a program function"),
    ACTION_ADD,                 BST_STOCK_ADD, },
  { N_("Change..."),            "",             N_("Change the currently selected key binding"),
    ACTION_EDIT,                BST_STOCK_EDIT, },
  { N_("Raise"),                "",             N_("Raise the currently selected key binding, relative to its neighbours"),
    ACTION_RAISE,               BST_STOCK_ARROW_UP, },
  { N_("Lower"),                "",             N_("Lower the currently selected key binding, relative to its neighbours"),
    ACTION_LOWER,               BST_STOCK_ARROW_DOWN, },
  { N_("Delete"),               "",             N_("Delete the currently selected key binding"),
    ACTION_DELETE,              BST_STOCK_REMOVE, },
  { N_("Load"),                 "",             N_("Load a key binding set"),
    ACTION_LOAD,                BST_STOCK_LOAD, },
};


/* --- functions --- */
static inline guint
key_binding_find_key (BstKeyBinding  *kbinding,
                      guint           keyval,
                      GdkModifierType modifier,
                      guint           collision_group,
                      guint           exception)
{
  guint i;
  for (i = 0; i < kbinding->n_keys; i++)
    if (kbinding->keys[i].keyval == keyval && kbinding->keys[i].modifier == modifier && i != exception &&
        kbinding->funcs[kbinding->keys[i].func_index].collision_group == collision_group)
      return i;
  return G_MAXINT;
}

static void
key_bindings_load_file (GtkWidget   *dialog,
                        const gchar *file,
                        gpointer     data)
{
  GtkWidget *self = GTK_WIDGET (data);
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  GtkTreeView *btview = (GtkTreeView*) gxk_radget_find (self, "binding-tree-view");
  GtkTreeModel *model = gtk_tree_view_get_model (btview);
  GSList slist = { kbinding, NULL };
  BseErrorType error = bst_key_binding_parse (file, &slist);
  gxk_list_wrapper_notify_clear (GXK_LIST_WRAPPER (model));
  gxk_list_wrapper_notify_append (GXK_LIST_WRAPPER (model), kbinding->n_keys);
  if (error)
    g_message ("failed to load \"%s\": %s", file, bse_error_blurb (error));
}

static void
key_bindings_exec_action (gpointer data,
                          gulong   action)
{
  GtkWidget *self = GTK_WIDGET (data);
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  GtkTreeView *btview = (GtkTreeView*) gxk_radget_find (self, "binding-tree-view");
  GtkTreeView *ftview = (GtkTreeView*) gxk_radget_find (self, "function-tree-view");
  gxk_status_window_push (self);
  switch (action)
    {
      GtkTreeSelection *tsel;
      GtkTreeModel *model;
      GtkTreeIter iter;
      GdkModifierType modifier;
      guint           keyval;
      gboolean valid;
      guint nb, nf;
    case ACTION_ADD:
      tsel = gtk_tree_view_get_selection (ftview);
      if (gtk_tree_selection_get_selected (tsel, &model, &iter))
        {
          nf = gxk_list_wrapper_get_index (GXK_LIST_WRAPPER (model), &iter);
          if (nf >= kbinding->n_funcs)
            break;
        }
      else
        break;
      tsel = gtk_tree_view_get_selection (btview);
      if (gtk_tree_selection_get_selected (tsel, &model, &iter))
        nb = gxk_list_wrapper_get_index (GXK_LIST_WRAPPER (model), &iter);
      else
        nb = kbinding->n_keys;
      valid = bst_key_combo_popup (kbinding->funcs[nf].function_name, &keyval, &modifier);
      valid &= key_binding_find_key (kbinding, keyval, modifier, kbinding->funcs[nf].collision_group, ~0) >= kbinding->n_keys;
      if (valid)
        {
          kbinding->keys = (BstKeyBindingKey*) g_realloc (kbinding->keys, sizeof (kbinding->keys[0]) * (kbinding->n_keys + 1));
          g_memmove (kbinding->keys + nb + 1,
                     kbinding->keys + nb,
                     sizeof (kbinding->keys[0]) * (kbinding->n_keys - nb));
          kbinding->n_keys++;
          kbinding->keys[nb].keyval = keyval;
          kbinding->keys[nb].modifier = modifier;
          kbinding->keys[nb].func_index = nf;
          kbinding->keys[nb].param = 0;
          gxk_list_wrapper_notify_insert (GXK_LIST_WRAPPER (model), nb);
          gxk_tree_view_select_index (btview, nb);
#if 0     /* usefull for entering movement bindings, not notes */
          if (nf + 1 < kbinding->n_funcs)
            gxk_tree_view_select_index (ftview, nf + 1);
#endif
        }
      else
        bst_gui_error_bell (self);
      break;
    case ACTION_EDIT:
      tsel = gtk_tree_view_get_selection (btview);
      if (gtk_tree_selection_get_selected (tsel, &model, &iter))
        {
          nb = gxk_list_wrapper_get_index (GXK_LIST_WRAPPER (model), &iter);
          if (nb >= kbinding->n_keys)
            break;
        }
      else
        break;
      nf = kbinding->keys[nb].func_index;
      valid = bst_key_combo_popup (kbinding->funcs[nf].function_name, &keyval, &modifier);
      valid &= key_binding_find_key (kbinding, keyval, modifier, kbinding->funcs[nf].collision_group, nb) >= kbinding->n_keys;
      if (valid)
        {
          kbinding->keys[nb].keyval = keyval;
          kbinding->keys[nb].modifier = modifier;
          gxk_list_wrapper_notify_change (GXK_LIST_WRAPPER (model), nb);
        }
      else
        bst_gui_error_bell (self);
      break;
    case ACTION_LOWER:
      tsel = gtk_tree_view_get_selection (btview);
      if (gtk_tree_selection_get_selected (tsel, &model, &iter))
        {
          nb = gxk_list_wrapper_get_index (GXK_LIST_WRAPPER (model), &iter);
          if (nb >= kbinding->n_keys)
            break;
        }
      else
        break;
      if (nb + 1 < kbinding->n_keys)
        {
          BstKeyBindingKey bkey = kbinding->keys[nb];
          kbinding->keys[nb] = kbinding->keys[nb + 1];
          kbinding->keys[nb + 1] = bkey;
          gxk_list_wrapper_notify_change (GXK_LIST_WRAPPER (model), nb);
          gxk_list_wrapper_notify_change (GXK_LIST_WRAPPER (model), nb + 1);
          gxk_tree_view_select_index (btview, nb + 1);
        }
      break;
    case ACTION_RAISE:
      tsel = gtk_tree_view_get_selection (btview);
      if (gtk_tree_selection_get_selected (tsel, &model, &iter))
        {
          nb = gxk_list_wrapper_get_index (GXK_LIST_WRAPPER (model), &iter);
          if (nb >= kbinding->n_keys)
            break;
        }
      else
        break;
      if (nb > 0)
        {
          BstKeyBindingKey bkey = kbinding->keys[nb];
          kbinding->keys[nb] = kbinding->keys[nb - 1];
          kbinding->keys[nb - 1] = bkey;
          gxk_list_wrapper_notify_change (GXK_LIST_WRAPPER (gtk_tree_view_get_model (btview)), nb);
          gxk_list_wrapper_notify_change (GXK_LIST_WRAPPER (gtk_tree_view_get_model (btview)), nb - 1);
          gxk_tree_view_select_index (btview, nb - 1);
        }
      break;
    case ACTION_DELETE:
      tsel = gtk_tree_view_get_selection (btview);
      if (gtk_tree_selection_get_selected (tsel, &model, &iter))
        {
          guint nth = gxk_list_wrapper_get_index (GXK_LIST_WRAPPER (model), &iter);
          if (nth < kbinding->n_keys)
            {
              kbinding->n_keys--;
              g_memmove (kbinding->keys + nth,
                         kbinding->keys + nth + 1,
                         sizeof (kbinding->keys[0]) * (kbinding->n_keys - nth));
              gxk_list_wrapper_notify_delete (GXK_LIST_WRAPPER (model), nth);
            }
        }
      break;
    case ACTION_LOAD:
      {
        static GtkWidget *load_dialog = NULL;
        if (!load_dialog)
          load_dialog = bst_file_dialog_create();
        bst_file_dialog_setup (load_dialog, self, _("Load Key Binding"), BST_PATH_KEYS);
        gxk_widget_showraise (load_dialog);
        bst_file_dialog_set_handler (BST_FILE_DIALOG (load_dialog), key_bindings_load_file, self, NULL);
      }
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  gxk_status_window_pop ();
  gxk_widget_update_actions_downwards (self);
}

static gboolean
key_bindings_check_action (gpointer data,
                           gulong   action,
                           guint64  action_stamp)
{
  GtkWidget *self = GTK_WIDGET (data);
  gboolean editable = g_object_get_long (self, "editable");
  GtkTreeView *btview = (GtkTreeView*) gxk_radget_find (self, "binding-tree-view");
  GtkTreeView *ftview = (GtkTreeView*) gxk_radget_find (self, "function-tree-view");
  if (!editable)
    return FALSE;
  switch (action)
    {
      GtkTreeSelection *tsel;
    case ACTION_ADD:
      tsel = gtk_tree_view_get_selection (ftview);
      return gtk_tree_selection_count_selected_rows (tsel) > 0;
    case ACTION_EDIT:
    case ACTION_RAISE:
    case ACTION_LOWER:
    case ACTION_DELETE:
      tsel = gtk_tree_view_get_selection (btview);
      return gtk_tree_selection_count_selected_rows (tsel) > 0;
    case ACTION_LOAD:
      return TRUE;
    default:
      g_warning ("%s: unknown action: %lu", G_STRFUNC, action);
      return FALSE;
    }
}

static gdouble
key_binding_clamp_param (BstKeyBindingParam ptype,
                         gdouble            param)
{
  switch (ptype)
    {
    case BST_KEY_BINDING_PARAM_m1_p1:   return CLAMP (param, -1, 1);
    case BST_KEY_BINDING_PARAM_0_p1:    return CLAMP (param, 0, 1);
    case BST_KEY_BINDING_PARAM_m1_0:    return CLAMP (param, -1, 0);
    case BST_KEY_BINDING_PARAM_PERC:    return CLAMP (param, 0, 100);
    case BST_KEY_BINDING_PARAM_SHORT:   return (gint) CLAMP (param, -32, +32);
    case BST_KEY_BINDING_PARAM_USHORT:  return (gint) CLAMP (param, 0, +32);
    case BST_KEY_BINDING_PARAM_NOTE:    return SFI_NOTE_CLAMP ((gint) param);
    default:                            return 0;
    }
}

static void
key_binding_binding_param_edited (GtkWidget   *self,
                                  const gchar *strpath,
                                  const gchar *text)
{
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  guint nb = strpath ? gxk_tree_spath_index0 (strpath) : G_MAXUINT;
  BstKeyBindingParam ptype = kbinding->funcs[kbinding->keys[nb].func_index].ptype;
  gdouble value = 0;
  if (nb >= kbinding->n_keys)
    return;
  if (ptype == BST_KEY_BINDING_PARAM_NOTE)
    {
      gchar *error;
      value = sfi_note_from_string_err (text, &error);
      if (error)
        {
          g_free (error);
          bst_gui_error_bell (self);
          return;
        }
    }
  else
    value = g_strtod (text, NULL);
  kbinding->keys[nb].param = key_binding_clamp_param (ptype, value);
}

static void
key_binding_fill_binding_value (GtkWidget      *self,
                                guint           column,
                                guint           row,
                                GValue         *value,
                                GxkListWrapper *lwrapper)
{
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  gint nb = row;
  if (nb >= kbinding->n_keys)
    {
      sfi_value_set_string (value, "<BUG: invalid row count>");
      return;
    }
  switch (column)
    {
      gchar *str;
      gdouble param;
    case BCOL_KEY:
      sfi_value_take_string (value, gtk_accelerator_name (kbinding->keys[nb].keyval, kbinding->keys[nb].modifier));
      break;
    case BCOL_FUNCTION:
      sfi_value_set_string (value, kbinding->funcs[kbinding->keys[nb].func_index].function_name);
      break;
    case BCOL_PARAM:
      param = kbinding->keys[nb].param;
      switch (kbinding->funcs[kbinding->keys[nb].func_index].ptype)
        {
        case BST_KEY_BINDING_PARAM_m1_p1:       str = g_strdup_printf ("%+.7f", param);         break;
        case BST_KEY_BINDING_PARAM_0_p1:        str = g_strdup_printf ("% .7f", param);         break;
        case BST_KEY_BINDING_PARAM_m1_0:        str = g_strdup_printf ("%+.7f", param);         break;
        case BST_KEY_BINDING_PARAM_PERC:        str = g_strdup_printf ("% 3.2f", param);        break;
        case BST_KEY_BINDING_PARAM_SHORT:       str = g_strdup_printf ("% d", (gint) param);    break;
        case BST_KEY_BINDING_PARAM_USHORT:      str = g_strdup_printf ("% d", (gint) param);    break;
        case BST_KEY_BINDING_PARAM_NOTE:        str = sfi_note_to_string (param);               break;
        default:                                str = g_strdup ("");                            break;
        }
      sfi_value_take_string (value, str);
      break;
    }
}

static void
key_binding_fill_function_value (GtkWidget      *self,
                                 guint           column,
                                 guint           row,
                                 GValue         *value,
                                 GxkListWrapper *lwrapper)
{
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  switch (column)
    {
    case FCOL_NAME:
      sfi_value_set_string (value, kbinding->funcs[row].function_name);
      break;
    case FCOL_BLURB:
      sfi_value_set_string (value, kbinding->funcs[row].function_blurb);
      break;
    }
}

static void
key_binding_free (gpointer data)
{
  BstKeyBinding *kbinding = (BstKeyBinding*) data;
  g_free (kbinding->binding_name);
  g_free (kbinding->keys);
  g_free (kbinding);
}

GtkWidget*
bst_key_binding_box (const gchar                 *binding_name,
                     guint                        n_funcs,
                     const BstKeyBindingFunction *funcs,
                     gboolean                     editable)
{
  GxkRadget *self = gxk_radget_create ("beast", "key-bindings-box", NULL);
  GxkListWrapper *lwrapper;
  GtkTreeSelection *tsel;
  GtkTreeView *tview;
  BstKeyBinding *kbinding = g_new0 (BstKeyBinding, 1);
  kbinding->binding_name = g_strdup (binding_name);
  kbinding->n_funcs = n_funcs;
  kbinding->funcs = funcs;

  g_object_set_data_full ((GObject*) self, "BstKeyBinding", kbinding, key_binding_free);
  g_object_set_long (self, "editable", editable != FALSE);
  gxk_widget_publish_actions (self, "key-bindings-actions", G_N_ELEMENTS (key_bindings_actions), key_bindings_actions,
                              NULL, key_bindings_check_action, key_bindings_exec_action);

  /* binding list */
  lwrapper = gxk_list_wrapper_new (N_BCOLS,
                                   G_TYPE_STRING,       /* BCOL_KEY */
                                   G_TYPE_STRING,       /* BCOL_FUNCTION */
                                   G_TYPE_STRING        /* BCOL_PARAM */
                                   );
  g_signal_connect_object (lwrapper, "fill-value",
                           G_CALLBACK (key_binding_fill_binding_value),
                           self, G_CONNECT_SWAPPED);
  gxk_list_wrapper_notify_append (lwrapper, kbinding->n_keys);

  /* binding view setup */
  tview = (GtkTreeView*) gxk_radget_find (self, "binding-tree-view");
  gtk_tree_view_set_model (tview, GTK_TREE_MODEL (lwrapper));
  tsel = gtk_tree_view_get_selection (tview);
  g_signal_connect_swapped (tsel, "changed", G_CALLBACK (gxk_widget_update_actions), tview);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, GTK_TREE_MODEL (lwrapper));
  gxk_tree_view_add_text_column (tview, BCOL_KEY, "S", 0.0, _("Key Binding"),
                                 _("Key combinations used to activate a function"),
                                 NULL, NULL, GConnectFlags (0));
  gxk_tree_view_add_text_column (tview, BCOL_FUNCTION, "S", 0.0, _("Function"),
                                 _("Functions to be activated for a key binding"),
                                 NULL, NULL, GConnectFlags (0));
  gxk_tree_view_add_text_column (tview, BCOL_PARAM, "S", 0.0, _("Parameter"),
                                 _("Parameter to pass to functions upon activation"),
                                 editable ? (void*) key_binding_binding_param_edited : NULL, self, G_CONNECT_SWAPPED);

  /* function list */
  lwrapper = gxk_list_wrapper_new (N_FCOLS,
                                   G_TYPE_STRING,       /* FCOL_NAME */
                                   G_TYPE_STRING        /* FCOL_BLURB */
                                   );
  g_signal_connect_object (lwrapper, "fill-value",
                           G_CALLBACK (key_binding_fill_function_value),
                           self, G_CONNECT_SWAPPED);
  gxk_list_wrapper_notify_append (lwrapper, kbinding->n_funcs);

  /* function view setup */
  tview = (GtkTreeView*) gxk_radget_find (self, "function-tree-view");
  gtk_tree_view_set_model (tview, GTK_TREE_MODEL (lwrapper));
  tsel = gtk_tree_view_get_selection (tview);
  g_signal_connect_swapped (tsel, "changed", G_CALLBACK (gxk_widget_update_actions), tview);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, GTK_TREE_MODEL (lwrapper));
  gxk_tree_view_add_text_column (tview, FCOL_NAME, "S", 0.0, _("Function"),
                                 _("Function used to create new key bindings"),
                                 NULL, NULL, GConnectFlags (0));
  gxk_tree_view_add_text_column (tview, FCOL_BLURB, "S", 0.0, _("Description"), NULL, NULL, NULL, GConnectFlags (0));

  return (GtkWidget*) self;
}

void
bst_key_binding_box_set (GtkWidget                   *self,
                         BstKeyBindingItemSeq        *kbseq)
{
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  GtkTreeView *btview = (GtkTreeView*) gxk_radget_find (self, "binding-tree-view");
  GtkTreeModel *model = gtk_tree_view_get_model (btview);
  bst_key_binding_set_item_seq (kbinding, kbseq);
  gxk_list_wrapper_notify_clear (GXK_LIST_WRAPPER (model));
  gxk_list_wrapper_notify_append (GXK_LIST_WRAPPER (model), kbinding->n_keys);
}

BstKeyBindingItemSeq*
bst_key_binding_box_get (GtkWidget *self)
{
  BstKeyBinding *kbinding = (BstKeyBinding*) g_object_get_data ((GObject*) self, "BstKeyBinding");
  return bst_key_binding_get_item_seq (kbinding);
}

BstKeyBindingKey*
bst_key_binding_lookup_key (BstKeyBinding  *kbinding,
                            guint           keyval,
                            GdkModifierType modifier,
                            guint           collision_group)
{
  guint i;
  keyval = gdk_keyval_to_lower (keyval);
  modifier &= GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK;
  for (i = 0; i < kbinding->n_keys; i++)
    if (kbinding->keys[i].keyval == keyval &&
        kbinding->keys[i].modifier == modifier &&
        kbinding->funcs[kbinding->keys[i].func_index].collision_group == collision_group)
      return &kbinding->keys[i];
  return NULL;
}

const BstKeyBindingFunction*
bst_key_binding_lookup (BstKeyBinding   *kbinding,
                        guint            keyval,
                        GdkModifierType  modifier,
                        guint            collision_group,
                        gdouble         *param)
{
  BstKeyBindingKey *key = bst_key_binding_lookup_key (kbinding, keyval, modifier, collision_group);
  if (param)
    *param = key ? key->param : 0;
  return key ? &kbinding->funcs[key->func_index] : NULL;
}

guint
bst_key_binding_lookup_id (BstKeyBinding   *kbinding,
                           guint            keyval,
                           GdkModifierType  modifier,
                           guint            collision_group,
                           gdouble         *param)
{
  const BstKeyBindingFunction *func = bst_key_binding_lookup (kbinding, keyval, modifier, collision_group, param);
  return func ? func->id : 0;
}

static inline guint
key_binding_find_function (BstKeyBinding *kbinding,
                           const gchar   *func_name)
{
  guint i;
  for (i = 0; i < kbinding->n_funcs; i++)
    if (strcmp (func_name, kbinding->funcs[i].function_name) == 0)
      return i;
  return G_MAXINT;
}

void
bst_key_binding_set_item_seq (BstKeyBinding        *kbinding,
                              BstKeyBindingItemSeq *seq)
{
  BstKeyBindingKey *key;
  guint i;
  /* reset */
  kbinding->n_keys = 0;
  /* raise capacity */
  kbinding->keys = (BstKeyBindingKey*) g_realloc (kbinding->keys, sizeof (kbinding->keys[0]) * seq->n_items);
  /* convert picewise */
  key = kbinding->keys;
  for (i = 0; i < seq->n_items; i++)
    {
      gtk_accelerator_parse (seq->items[i]->key_name, &key->keyval, &key->modifier);
      key->func_index = key_binding_find_function (kbinding, seq->items[i]->func_name);
      if (key->func_index < kbinding->n_funcs && bst_key_combo_valid (key->keyval, key->modifier))
        {
          key->param = key_binding_clamp_param (kbinding->funcs[key->func_index].ptype, seq->items[i]->func_param);
          key++;
        }
      else
        g_message ("ignoring unknown key-binding function: %s", seq->items[i]->func_name);
    }
  /* admit registration */
  kbinding->n_keys = key - kbinding->keys;
  /* shrink capacity */
  kbinding->keys = (BstKeyBindingKey*) g_realloc (kbinding->keys, sizeof (kbinding->keys[0]) * kbinding->n_keys);
}

BstKeyBindingItemSeq*
bst_key_binding_get_item_seq (BstKeyBinding *kbinding)
{
  BstKeyBindingItemSeq *iseq = bst_key_binding_item_seq_new ();
  guint i;
  for (i = 0; i < kbinding->n_keys; i++)
    {
      BstKeyBindingKey *key = kbinding->keys + i;
      BstKeyBindingItem item;
      item.key_name = gtk_accelerator_name (key->keyval, key->modifier);
      item.func_name = (char*) kbinding->funcs[key->func_index].function_name;
      item.func_param = key->param;
      bst_key_binding_item_seq_append (iseq, &item);
      g_free (item.key_name);
    }
  return iseq;
}

GParamSpec*
bst_key_binding_item_pspec (void)
{
  static GParamSpec *pspec = NULL;
  if (!pspec)
    {
      pspec = sfi_pspec_rec ("key", NULL, NULL, bst_key_binding_item_fields, SFI_PARAM_STANDARD);
      g_param_spec_ref (pspec);
      g_param_spec_sink (pspec);
    }
  return pspec;
}


/* --- keyrc file --- */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "topconfig.h"          /* BST_VERSION */
#include <sfi/sfistore.hh>       /* we rely on internal API here */
const gchar*
bst_key_binding_rcfile (void)
{
  static gchar *key_binding_rc = NULL;
  if (!key_binding_rc)
    key_binding_rc = sfi_path_get_filename (".beast/keyrc", "~");
  return key_binding_rc;
}

BseErrorType
bst_key_binding_dump (const gchar *file_name,
                      GSList      *kbindings)
{
  SfiWStore *wstore;
  GSList *slist;
  gint fd;

  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  sfi_make_dirname_path (file_name);
  fd = open (file_name,
             O_WRONLY | O_CREAT | O_TRUNC, /* O_EXCL, */
             0666);
  if (fd < 0)
    return errno == EEXIST ? BSE_ERROR_FILE_EXISTS : BSE_ERROR_IO;

  wstore = sfi_wstore_new ();

  sfi_wstore_printf (wstore, "; key-binding-file for BEAST v%s\n", BST_VERSION);

  /* store BstSkinConfig */
  sfi_wstore_puts (wstore, "\n");
  for (slist = kbindings; slist; slist = slist->next)
    {
      BstKeyBinding *kbinding = (BstKeyBinding*) slist->data;
      BstKeyBindingItemSeq *iseq = bst_key_binding_get_item_seq (kbinding);
      GParamSpec *pspec = sfi_pspec_seq (kbinding->binding_name, NULL, NULL, bst_key_binding_item_pspec(), SFI_PARAM_STANDARD);
      SfiSeq *seq = bst_key_binding_item_seq_to_seq (iseq);
      GValue *value = sfi_value_seq (seq);
      sfi_wstore_put_param (wstore, value, pspec);
      g_param_spec_unref (pspec);
      bst_key_binding_item_seq_free (iseq);
      sfi_value_free (value);
      sfi_seq_unref (seq);
      sfi_wstore_puts (wstore, "\n");
    }

  /* flush buffers to file */
  sfi_wstore_flush_fd (wstore, fd);
  sfi_wstore_destroy (wstore);

  return close (fd) < 0 ? BSE_ERROR_IO : BSE_ERROR_NONE;
}

static GTokenType
key_binding_try_statement (gpointer   context_data,
                           SfiRStore *rstore,
                           GScanner  *scanner,
                           gpointer   user_data)
{
  GSList *slist, *kbindings = (GSList*) context_data;
  g_assert (scanner->next_token == G_TOKEN_IDENTIFIER);
  for (slist = kbindings; slist; slist = slist->next)
    {
      BstKeyBinding *kbinding = (BstKeyBinding*) slist->data;
      if (strcmp (kbinding->binding_name, scanner->next_value.v_identifier) == 0)
        {
          GParamSpec *pspec = sfi_pspec_seq (kbinding->binding_name, NULL, NULL, bst_key_binding_item_pspec(), SFI_PARAM_STANDARD);
          GValue *value = sfi_value_seq (NULL);
          GTokenType token;
          SfiSeq *seq;
          g_scanner_get_next_token (rstore->scanner);
          token = sfi_rstore_parse_param (rstore, value, pspec);
          g_param_spec_unref (pspec);
          seq = sfi_value_get_seq (value);
          if (token == G_TOKEN_NONE && seq)
            {
              BstKeyBindingItemSeq *iseq = bst_key_binding_item_seq_from_seq (seq);
              bst_key_binding_set_item_seq (kbinding, iseq);
              bst_key_binding_item_seq_free (iseq);
            }
          sfi_value_free (value);
          return token;
        }
    }
  return SFI_TOKEN_UNMATCHED;
}

BseErrorType
bst_key_binding_parse (const gchar *file_name,
                       GSList      *kbindings)
{
  BseErrorType error = BSE_ERROR_NONE;
  SfiRStore *rstore;
  gchar *absname;
  gint fd;
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  absname = sfi_path_get_filename (file_name, NULL);
  fd = open (absname, O_RDONLY, 0);
  if (fd < 0)
    {
      g_free (absname);
      return (errno == ENOENT || errno == ENOTDIR || errno == ELOOP ?
              BSE_ERROR_FILE_NOT_FOUND : BSE_ERROR_IO);
    }

  rstore = sfi_rstore_new ();
  sfi_rstore_input_fd (rstore, fd, absname);
  if (sfi_rstore_parse_all (rstore, kbindings, key_binding_try_statement, absname) > 0)
    error = BSE_ERROR_PARSE_ERROR;
  sfi_rstore_destroy (rstore);
  close (fd);
  g_free (absname);
  return error;
}
