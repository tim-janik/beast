/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2001, 2003 Tim Janik
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
#include "bstwavereposhell.h"

#include "bstparamview.h"
#include "bstactivatable.h"
#include "bstapp.h"


/* --- prototypes --- */
static void     bst_wave_repo_shell_class_init          (BstWaveRepoShellClass  *klass);
static void     bst_wave_repo_shell_init_activatable    (BstActivatableIface    *iface,
                                                         gpointer                iface_data);
static void     bst_wave_repo_shell_init                (BstWaveRepoShell       *wshell);
static void     bst_wave_repo_shell_rebuild             (BstSuperShell          *super_shell);
static gchar*   bst_wave_repo_shell_get_title           (BstSuperShell          *super_shell);
static void     bst_wave_repo_shell_activate            (BstActivatable         *activatable,
                                                         gulong                  action);
static gboolean bst_wave_repo_shell_can_activate        (BstActivatable         *activatable,
                                                         gulong                  action);
static void     bst_wave_repo_shell_request_update      (BstActivatable         *activatable);
static void     bst_wave_repo_shell_update_activatable  (BstActivatable         *activatable);



/* --- static variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
bst_wave_repo_shell_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstWaveRepoShellClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_wave_repo_shell_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstWaveRepoShell),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_wave_repo_shell_init,
      };
      static const GInterfaceInfo activatable_info = {
        (GInterfaceInitFunc) bst_wave_repo_shell_init_activatable,      /* interface_init */
        NULL,                                                           /* interface_finalize */
        NULL                                                            /* interface_data */
      };
      type = g_type_register_static (BST_TYPE_SUPER_SHELL, "BstWaveRepoShell", &type_info, 0);
      g_type_add_interface_static (type, BST_TYPE_ACTIVATABLE, &activatable_info);
    }
  return type;
}

static void
bst_wave_repo_shell_class_init (BstWaveRepoShellClass *class)
{
  BstSuperShellClass *super_shell_class = BST_SUPER_SHELL_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  super_shell_class->get_title = bst_wave_repo_shell_get_title;
  super_shell_class->rebuild = bst_wave_repo_shell_rebuild;
}

static void
bst_wave_repo_shell_init_activatable (BstActivatableIface *iface,
                                      gpointer             iface_data)
{
  iface->activate = bst_wave_repo_shell_activate;
  iface->can_activate = bst_wave_repo_shell_can_activate;
  iface->request_update = bst_wave_repo_shell_request_update;
  iface->update = bst_wave_repo_shell_update_activatable;
}

static void
bst_wave_repo_shell_init (BstWaveRepoShell *wshell)
{
  wshell->param_view = NULL;
  wshell->wave_view = NULL;
}

static void
bst_wave_repo_shell_rebuild (BstSuperShell *super_shell)
{
  BstWaveRepoShell *wshell = BST_WAVE_REPO_SHELL (super_shell);
  SfiProxy wrepo = super_shell->super;
  GtkWidget *notebook;

  g_return_if_fail (wshell->param_view == NULL);

  wshell->param_view = (BstParamView*) bst_param_view_new (wrepo);
  g_object_connect (GTK_WIDGET (wshell->param_view),
                    "signal::destroy", gtk_widget_destroyed, &wshell->param_view,
                    NULL);
  wshell->wave_view = (BstItemView*) bst_wave_view_new (wrepo);
  g_object_set (GTK_WIDGET (wshell->wave_view),
                "visible", TRUE,
                NULL);
  g_object_connect (GTK_WIDGET (wshell->wave_view),
                    "signal::destroy", gtk_widget_destroyed, &wshell->wave_view,
                    NULL);
  
  notebook = g_object_connect (g_object_new (GTK_TYPE_NOTEBOOK,
                                             "scrollable", FALSE,
                                             "tab_border", 0,
                                             "show_border", TRUE,
                                             "enable_popup", FALSE,
                                             "show_tabs", TRUE,
                                             "tab_pos", GTK_POS_TOP,
                                             "border_width", 5,
                                             "parent", wshell,
                                             "visible", TRUE,
                                             NULL),
                               "signal_after::switch-page", gxk_widget_viewable_changed, NULL,
                               NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (wshell->wave_view),
                            gtk_widget_new (GTK_TYPE_LABEL,
                                            "label", _("Waves"),
                                            "visible", TRUE,
                                            NULL));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (wshell->param_view),
                            gtk_widget_new (GTK_TYPE_LABEL,
                                            "label", _("Parameters"),
                                            "visible", TRUE,
                                            NULL));
}

static gchar*
bst_wave_repo_shell_get_title (BstSuperShell *super_shell)
{
  // BstWaveRepoShell *self = BST_WAVE_REPO_SHELL (super_shell);

  return g_strdup (_("Waves"));
}

static void
bst_wave_repo_shell_activate (BstActivatable *activatable,
                              gulong          action)
{
  BstWaveRepoShell *self = BST_WAVE_REPO_SHELL (activatable);
  if (self->wave_view)
    bst_activatable_activate (BST_ACTIVATABLE (self->wave_view), action);
  bst_widget_update_activatable (activatable);
}

static gboolean
bst_wave_repo_shell_can_activate (BstActivatable *activatable,
                                  gulong          action)
{
  BstWaveRepoShell *self = BST_WAVE_REPO_SHELL (activatable);
  return self->wave_view && bst_activatable_can_activate (BST_ACTIVATABLE (self->wave_view), action);
}

static void
bst_wave_repo_shell_request_update (BstActivatable *activatable)
{
  BstWaveRepoShell *self = BST_WAVE_REPO_SHELL (activatable);
  /* chain to normal handler */
  bst_activatable_default_request_update (activatable);
  /* add activatable children */
  if (self->wave_view)
    bst_activatable_update_enqueue (BST_ACTIVATABLE (self->wave_view));
}

static void
bst_wave_repo_shell_update_activatable (BstActivatable *activatable)
{
  // BstWaveRepoShell *self = BST_WAVE_REPO_SHELL (activatable);

  /* no original actions to update */
}
