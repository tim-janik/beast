/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstfreeradiobutton.h"

static void bst_free_radio_button_class_init (BstFreeRadioButtonClass *klass);
static void bst_free_radio_button_clicked    (GtkButton		      *button);


GtkType
bst_free_radio_button_get_type (void)
{
  static GtkType free_radio_button_type = 0;

  if (!free_radio_button_type)
    {
      static const GtkTypeInfo free_radio_button_info =
      {
	"BstFreeRadioButton",
	sizeof (BstFreeRadioButton),
	sizeof (BstFreeRadioButtonClass),
	(GtkClassInitFunc) bst_free_radio_button_class_init,
	(GtkObjectInitFunc) NULL,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      free_radio_button_type = gtk_type_unique (GTK_TYPE_RADIO_BUTTON, &free_radio_button_info);
    }

  return free_radio_button_type;
}

static void
bst_free_radio_button_class_init (BstFreeRadioButtonClass *class)
{
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (class);

  button_class->clicked = bst_free_radio_button_clicked;
}

static void
bst_free_radio_button_clicked (GtkButton *button)
{
  GtkToggleButton *toggle_button;
  GtkStateType new_state;
  
  g_return_if_fail (BST_IS_FREE_RADIO_BUTTON (button));

  toggle_button = GTK_TOGGLE_BUTTON (button);
  toggle_button->active = !toggle_button->active;
  new_state = button->in_button ? GTK_STATE_PRELIGHT : toggle_button->active ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL;

  if (GTK_WIDGET_STATE (button) != new_state)
    gtk_widget_set_state (GTK_WIDGET (button), new_state);
  
  gtk_toggle_button_toggled (toggle_button);
  gtk_widget_queue_draw (GTK_WIDGET (button));
}
