/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bstlogadjustment.h"

#include <math.h>


/* --- prototypes --- */
static void	bst_log_adjustment_class_init		(BstLogAdjustmentClass	*klass);
static void	bst_log_adjustment_init			(BstLogAdjustment	*ladj);
static void	bst_log_adjustment_destroy		(GtkObject		*object);
static void	bst_log_adjustment_changed		(GtkAdjustment		*adj);
static void	bst_log_adjustment_value_changed	(GtkAdjustment		*adj);
static void	ladj_client_changed			(BstLogAdjustment	*ladj);
static void	ladj_client_value_changed		(BstLogAdjustment	*ladj);


/* --- static variables --- */
static gpointer		      parent_class = NULL;


/* --- functions --- */
GType
bst_log_adjustment_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstLogAdjustmentClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_log_adjustment_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstLogAdjustment),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_log_adjustment_init,
      };

      type = g_type_register_static (GTK_TYPE_ADJUSTMENT,
				     "BstLogAdjustment",
				     &type_info, 0);
    }

  return type;
}

static void
bst_log_adjustment_class_init (BstLogAdjustmentClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkAdjustmentClass *adjustment_class = GTK_ADJUSTMENT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_log_adjustment_destroy;

  adjustment_class->changed = bst_log_adjustment_changed;
  adjustment_class->value_changed = bst_log_adjustment_value_changed;
}

static gdouble
call_exp (BstLogAdjustment *ladj,
	  gdouble           x)
{
  return pow (ladj->base, x);
}

static gdouble
call_log (BstLogAdjustment *ladj,
	  gdouble           x)
{
  return log (CLAMP (x, ladj->llimit, ladj->ulimit)) / ladj->base_ln;
}

static void
bst_log_adjustment_init (BstLogAdjustment *ladj)
{
  ladj->block_client = 0;
  ladj->client_owned = FALSE;
  ladj->client = NULL;
  bst_log_adjustment_setup (ladj, 10000, 10, 4);
}

static void
bst_log_adjustment_destroy (GtkObject *object)
{
  BstLogAdjustment *ladj = BST_LOG_ADJUSTMENT (object);

  bst_log_adjustment_set_client (ladj, NULL);

  /* chain parent class handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkAdjustment*
bst_log_adjustment_from_adj (GtkAdjustment *client)
{
  BstLogAdjustment *ladj;

  g_return_val_if_fail (GTK_IS_ADJUSTMENT (client), NULL);

  ladj = g_object_new (BST_TYPE_LOG_ADJUSTMENT, NULL);
  g_object_ref (ladj);
  gtk_object_sink (GTK_OBJECT (ladj));
  bst_log_adjustment_set_client (ladj, client);
  ladj->client_owned = TRUE;
  
  return GTK_ADJUSTMENT (ladj);
}

static void
ladj_weak_notify (gpointer data,
	       GObject *where_the_object_was)
{
  BstLogAdjustment *ladj = BST_LOG_ADJUSTMENT (data);

  g_signal_handlers_disconnect_by_func (ladj->client, ladj_client_changed, ladj);
  g_signal_handlers_disconnect_by_func (ladj->client, ladj_client_value_changed, ladj);
  ladj->client = NULL;
  if (ladj->client_owned)
    {
      ladj->client_owned = FALSE;
      g_object_unref (ladj);
    }
}

void
bst_log_adjustment_set_client (BstLogAdjustment *ladj,
			       GtkAdjustment    *client)
{
  g_return_if_fail (BST_IS_LOG_ADJUSTMENT (ladj));
  if (client)
    g_return_if_fail (GTK_IS_ADJUSTMENT (client));

  g_object_ref (ladj);
  if (ladj->client)
    {
      g_object_weak_unref (G_OBJECT (ladj->client), ladj_weak_notify, ladj);
      ladj_weak_notify (ladj, NULL);
    }
  ladj->client = client;
  if (ladj->client)
    {
      g_object_weak_ref (G_OBJECT (ladj->client), ladj_weak_notify, ladj);
      g_object_connect (ladj->client,
			"swapped_signal::changed", ladj_client_changed, ladj,
			"swapped_signal::value_changed", ladj_client_value_changed, ladj,
			NULL);
      ladj_client_changed (ladj);
    }
  g_object_unref (ladj);
}

void
bst_log_adjustment_setup (BstLogAdjustment *ladj,
			  gdouble           center,
			  gdouble           base,
			  guint             n_steps)
{
  g_return_if_fail (BST_IS_LOG_ADJUSTMENT (ladj));
  g_return_if_fail (n_steps > 0);
  g_return_if_fail (base > 0);

  ladj->center = center;
  ladj->n_steps = n_steps;
  ladj->base = base;
  ladj->base_ln = log (ladj->base);
  ladj->ulimit = pow (ladj->base, ladj->n_steps);
  ladj->llimit = 1.0 / ladj->ulimit;
}

static void
bst_log_adjustment_changed (GtkAdjustment *adj)
{
  BstLogAdjustment *ladj = BST_LOG_ADJUSTMENT (adj);
  GtkAdjustment *client = ladj->client;

  if (client)
    if (!ladj->block_client)
      {
        ladj->block_client++;
	gtk_adjustment_changed (client);
        ladj->block_client--;
      }
}

static void
bst_log_adjustment_value_changed (GtkAdjustment *adj)
{
  BstLogAdjustment *ladj = BST_LOG_ADJUSTMENT (adj);
  GtkAdjustment *client = ladj->client;

  if (client)
    if (!ladj->block_client)
      {
	client->value = call_exp (ladj, adj->value) * ladj->center;
	ladj->block_client++;
	gtk_adjustment_value_changed (client);
	ladj->block_client--;
      }
}

static void
ladj_client_changed (BstLogAdjustment *ladj)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (ladj);
  GtkAdjustment *client = ladj->client;

  adj->upper = ladj->n_steps;
  adj->lower = -ladj->n_steps;
  adj->page_increment = (adj->upper - adj->lower) / (2.0 * ladj->n_steps);
  adj->step_increment = adj->page_increment / 100.0;
  adj->page_size = 0;

  if (0)
    g_print ("SETUP: [%f %f] (%f %f %f) center: %g   CLIENT: [%f %f]\n",
	     adj->lower, adj->upper,
	     adj->step_increment, adj->page_increment, adj->page_size,
	     ladj->center,
	     client->lower, client->upper);

  if (!ladj->block_client)
    {
      ladj->block_client++;
      gtk_adjustment_changed (adj);
      ladj->block_client--;
    }
}

static void
ladj_client_value_changed (BstLogAdjustment *ladj)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (ladj);
  GtkAdjustment *client = ladj->client;

  adj->value = call_log (ladj, client->value / ladj->center);

  if (0)
    g_print ("LOG: [%f %f] %g   CLIENT: [%f %f] %g\n",
	     adj->lower, adj->upper, adj->value,
	     client->lower, client->upper, client->value);
  
  if (!ladj->block_client)
    {
      ladj->block_client++;
      gtk_adjustment_value_changed (adj);
      ladj->block_client--;
    }
}
