/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include "bstsampleshell.h"

#include "bstparamview.h"
#include "bstapp.h"


/* --- prototypes --- */
static void	bst_sample_shell_class_init	(BstSampleShellClass	*klass);
static void	bst_sample_shell_init		(BstSampleShell		*pe);
static void	bst_sample_shell_rebuild	(BstSuperShell		*super_shell);
static void	bst_sample_shell_update		(BstSuperShell		*super_shell);
static void	bst_sample_shell_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);
static gboolean	bst_sample_shell_can_operate	(BstSuperShell		*super_shell,
						 BstOps			 sop);


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
  BstSuperShellClass *super_shell_class;

  super_shell_class = BST_SUPER_SHELL_CLASS (class);

  bst_sample_shell_class = class;
  parent_class = g_type_class_peek_parent (class);

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
  BstSampleShell *sample_shell = BST_SAMPLE_SHELL (super_shell);
  BseSample *sample = bse_object_from_id (super_shell->super);

  g_return_if_fail (sample_shell->param_view == NULL);

  sample_shell->param_view = (BstParamView*) bst_param_view_new (BSE_OBJECT_ID (sample));
  g_object_set (GTK_WIDGET (sample_shell->param_view),
		"visible", TRUE,
		"parent", sample_shell,
		NULL);
  g_object_connect (GTK_WIDGET (sample_shell->param_view),
		    "signal::destroy", gtk_widget_destroyed, &sample_shell->param_view,
		    NULL);
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
