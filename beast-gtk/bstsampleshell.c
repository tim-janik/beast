/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstsampleshell.h"

#include "bstparamview.h"
#include "bstapp.h"


/* --- prototypes --- */
static void	bst_sample_shell_class_init	(BstSampleShellClass	*klass);
static void	bst_sample_shell_init		(BstSampleShell		*pe);
static void	bst_sample_shell_destroy	(GtkObject		*object);
static void	bst_sample_shell_rebuild	(BstSuperShell		*super_shell);
static void	bst_sample_shell_update		(BstSuperShell		*super_shell);
static void	bst_sample_shell_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gboolean	bst_sample_shell_can_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static void	bst_sample_shell_setup_super	(BstSuperShell		*super_shell,
						 BseSuper		*super);
static void	bst_sample_shell_release_super	(BstSuperShell		*super_shell,
						 BseSuper		*super);


/* --- static variables --- */
static gpointer             parent_class = NULL;
static BstSampleShellClass *bst_sample_shell_class = NULL;


/* --- functions --- */
GtkType
bst_sample_shell_get_type (void)
{
  static GtkType sample_shell_type = 0;
  
  if (!sample_shell_type)
    {
      GtkTypeInfo sample_shell_info =
      {
	"BstSampleShell",
	sizeof (BstSampleShell),
	sizeof (BstSampleShellClass),
	(GtkClassInitFunc) bst_sample_shell_class_init,
	(GtkObjectInitFunc) bst_sample_shell_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      sample_shell_type = gtk_type_unique (BST_TYPE_SUPER_SHELL, &sample_shell_info);
    }
  
  return sample_shell_type;
}

static void
bst_sample_shell_class_init (BstSampleShellClass *class)
{
  GtkObjectClass *object_class;
  BstSuperShellClass *super_shell_class;

  object_class = GTK_OBJECT_CLASS (class);
  super_shell_class = BST_SUPER_SHELL_CLASS (class);

  bst_sample_shell_class = class;
  parent_class = gtk_type_class (BST_TYPE_SUPER_SHELL);

  object_class->destroy = bst_sample_shell_destroy;

  super_shell_class->setup_super = bst_sample_shell_setup_super;
  super_shell_class->release_super = bst_sample_shell_release_super;
  super_shell_class->operate = bst_sample_shell_operate;
  super_shell_class->can_operate = bst_sample_shell_can_operate;
  super_shell_class->rebuild = bst_sample_shell_rebuild;
  super_shell_class->update = bst_sample_shell_update;

  class->factories_path = "<BstSampleShell>";
}

static void
bst_sample_shell_init (BstSampleShell *sample_shell)
{
  sample_shell->param_view = NULL;
  sample_shell->tooltips = gtk_tooltips_new ();
  gtk_object_ref (GTK_OBJECT (sample_shell->tooltips));
  gtk_object_sink (GTK_OBJECT (sample_shell->tooltips));
}

static void
bst_sample_shell_destroy (GtkObject *object)
{
  BstSampleShell *sample_shell = BST_SAMPLE_SHELL (object);
  BseSample *sample = BSE_SAMPLE (BST_SUPER_SHELL (sample_shell)->super);
  
  bse_source_clear_ochannels (BSE_SOURCE (sample));
  
  gtk_container_foreach (GTK_CONTAINER (sample_shell), (GtkCallback) gtk_widget_destroy, NULL);
  
  gtk_object_unref (GTK_OBJECT (sample_shell->tooltips));
  sample_shell->tooltips = NULL;
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget*
bst_sample_shell_new (BseSample *sample)
{
  GtkWidget *sample_shell;
  
  g_return_val_if_fail (BSE_IS_SAMPLE (sample), NULL);
  g_return_val_if_fail (bst_super_shell_from_super (BSE_SUPER (sample)) == NULL, NULL);
  
  sample_shell = gtk_widget_new (BST_TYPE_SAMPLE_SHELL,
				 "super", sample,
				 NULL);
  
  return sample_shell;
}

static void
bst_sample_shell_build (BstSampleShell *sample_shell)
{
  BseSample *sample = BSE_SAMPLE (BST_SUPER_SHELL (sample_shell)->super);

  sample_shell->param_view = (BstParamView*) bst_param_view_new (BSE_OBJECT (sample));
  gtk_widget_set (GTK_WIDGET (sample_shell->param_view),
		  "signal::destroy", gtk_widget_destroyed, &sample_shell->param_view,
		  "visible", TRUE,
		  "parent", sample_shell,
		  NULL);
}

static void
bst_sample_shell_setup_super (BstSuperShell *super_shell,
			      BseSuper      *super)
{
  BstSampleShell *sample_shell;
  
  sample_shell = BST_SAMPLE_SHELL (super_shell);
  
  BST_SUPER_SHELL_CLASS (parent_class)->setup_super (super_shell, super);

  if (super)
    bst_sample_shell_build (sample_shell);
}

static void
bst_sample_shell_release_super (BstSuperShell *super_shell,
				BseSuper      *super)
{
  BstSampleShell *sample_shell;
  
  sample_shell = BST_SAMPLE_SHELL (super_shell);
  
  gtk_container_foreach (GTK_CONTAINER (sample_shell), (GtkCallback) gtk_widget_destroy, NULL);
  
  BST_SUPER_SHELL_CLASS (parent_class)->release_super (super_shell, super);
}

static void
bst_sample_shell_update (BstSuperShell *super_shell)
{
  BstSampleShell *sample_shell;
  
  sample_shell = BST_SAMPLE_SHELL (super_shell);
  
  bst_param_view_update (sample_shell->param_view);
}

static void
bst_sample_shell_rebuild (BstSuperShell *super_shell)
{
  BstSampleShell *sample_shell;
  
  sample_shell = BST_SAMPLE_SHELL (super_shell);
  
  bst_param_view_rebuild (sample_shell->param_view);
}

static void
bst_sample_shell_operate (BstSuperShell *super_shell,
			BstOps         op)
{
  // BseSample *sample = BSE_SAMPLE (super_shell->super);
  BstSampleShell *sample_shell = BST_SAMPLE_SHELL (super_shell);

  g_return_if_fail (bst_sample_shell_can_operate (super_shell, op));
  
  switch (op)
    {
    default:
      break;
    }

  bst_update_can_operate (GTK_WIDGET (sample_shell));
}

static gboolean
bst_sample_shell_can_operate (BstSuperShell *super_shell,
			    BstOps	   op)
{
  // BstSampleShell *sample_shell = BST_SAMPLE_SHELL (super_shell);
  // BseSample *sample = BSE_SAMPLE (super_shell->super);

  switch (op)
    {
    default:
      return FALSE;
    }
}
