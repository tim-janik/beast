/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999, 2000 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstfreeradiobutton.h"

static void bst_free_radio_button_class_init (BstFreeRadioButtonClass *klass);

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
  GtkButtonClass *button_class;
  GtkButtonClass *radios_parent_class;

  button_class = GTK_BUTTON_CLASS (class);

  radios_parent_class = gtk_type_class (gtk_type_parent (GTK_TYPE_RADIO_BUTTON));

  button_class->clicked = radios_parent_class->clicked;
}
