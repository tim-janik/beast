/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2003 Tim Janik
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
#include "gxklogadjustment.h"
#include <math.h>

/* --- prototypes --- */
static void log_adjustment_class_init           (GxkLogAdjustmentClass *klass);
static void log_adjustment_init                 (GxkLogAdjustment      *self);
static void log_adjustment_destroy              (GtkObject             *object);
static void log_adjustment_changed              (GtkAdjustment         *adj);
static void log_adjustment_value_changed        (GtkAdjustment         *adj);
static void log_adjustment_adjust_ranges        (GxkLogAdjustment      *self);
static void log_adjustment_client_value_changed (GxkLogAdjustment      *self);

/* --- static variables --- */
static gpointer		      parent_class = NULL;

/* --- functions --- */
GType
gxk_log_adjustment_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (GxkLogAdjustmentClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) log_adjustment_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (GxkLogAdjustment),
	0,      /* n_preallocs */
	(GInstanceInitFunc) log_adjustment_init,
      };
      type = g_type_register_static (GTK_TYPE_ADJUSTMENT, "GxkLogAdjustment", &type_info, 0);
    }
  return type;
}

static void
log_adjustment_class_init (GxkLogAdjustmentClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkAdjustmentClass *adjustment_class = GTK_ADJUSTMENT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = log_adjustment_destroy;
  
  adjustment_class->changed = log_adjustment_changed;
  adjustment_class->value_changed = log_adjustment_value_changed;
}

static gdouble
call_exp (GxkLogAdjustment *self,
	  gdouble           x)
{
  return pow (self->base, x);
}

static gdouble
call_log (GxkLogAdjustment *self,
	  gdouble           x)
{
  return log (CLAMP (x, self->llimit, self->ulimit)) / self->base_ln;
}

static void
log_adjustment_init (GxkLogAdjustment *self)
{
  self->block_client = 0;
  self->client = NULL;
  gxk_log_adjustment_setup (self, 10000, 10, 4);
}

static void
log_adjustment_destroy (GtkObject *object)
{
  GxkLogAdjustment *self = GXK_LOG_ADJUSTMENT (object);
  
  gxk_log_adjustment_set_client (self, NULL);
  
  /* chain parent class handler */
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkAdjustment*
gxk_log_adjustment_from_adj (GtkAdjustment *client)
{
  GxkLogAdjustment *self;
  
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (client), NULL);
  
  self = g_object_new (GXK_TYPE_LOG_ADJUSTMENT, NULL);
  gxk_log_adjustment_set_client (self, client);
  
  return GTK_ADJUSTMENT (self);
}

void
gxk_log_adjustment_set_client (GxkLogAdjustment *self,
			       GtkAdjustment    *client)
{
  g_return_if_fail (GXK_IS_LOG_ADJUSTMENT (self));
  if (client)
    g_return_if_fail (GTK_IS_ADJUSTMENT (client));
  
  g_object_ref (self);
  if (self->client)
    {
      g_signal_handlers_disconnect_by_func (self->client, log_adjustment_adjust_ranges, self);
      g_signal_handlers_disconnect_by_func (self->client, log_adjustment_client_value_changed, self);
      g_object_unref (G_OBJECT (self->client));
    }
  self->client = client;
  if (self->client)
    {
      g_object_ref (G_OBJECT (self->client));
      g_object_connect (self->client,
			"swapped_signal::changed", log_adjustment_adjust_ranges, self,
			"swapped_signal::value_changed", log_adjustment_client_value_changed, self,
			NULL);
      log_adjustment_adjust_ranges (self);
    }
  g_object_unref (self);
}

void
gxk_log_adjustment_setup (GxkLogAdjustment *self,
			  gdouble           center,
			  gdouble           base,
			  gdouble           n_steps)
{
  GtkAdjustment *adj;
  
  g_return_if_fail (GXK_IS_LOG_ADJUSTMENT (self));
  g_return_if_fail (n_steps > 0);
  g_return_if_fail (base > 0);
  
  adj = GTK_ADJUSTMENT (self);
  self->center = center;
  self->n_steps = n_steps;
  self->base = base;
  self->base_ln = log (self->base);
  self->ulimit = pow (self->base, self->n_steps);
  self->llimit = 1.0 / self->ulimit;
  
  adj->value = self->center;
  log_adjustment_adjust_ranges (self);
  gtk_adjustment_value_changed (adj);
}

static void
log_adjustment_changed (GtkAdjustment *adj)
{
  GxkLogAdjustment *self = GXK_LOG_ADJUSTMENT (adj);
  GtkAdjustment *client = self->client;
  
  if (client && !self->block_client)
    {
      self->block_client++;
      gtk_adjustment_changed (client);
      self->block_client--;
    }
}

static void
log_adjustment_value_changed (GtkAdjustment *adj)
{
  GxkLogAdjustment *self = GXK_LOG_ADJUSTMENT (adj);
  GtkAdjustment *client = self->client;
  
  adj->value = CLAMP (adj->value, adj->lower, adj->upper);
  if (client && !self->block_client)
    {
      client->value = call_exp (self, adj->value) * self->center;
      self->block_client++;
      gtk_adjustment_value_changed (client);
      self->block_client--;
    }
}

static void
log_adjustment_adjust_ranges (GxkLogAdjustment *self)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (self);
  
  adj->upper = self->n_steps;
  adj->lower = -self->n_steps;
  adj->page_increment = (adj->upper - adj->lower) / (2.0 * self->n_steps);
  adj->step_increment = adj->page_increment / 100.0;
  adj->page_size = 0;
  
  if (0)
    {
      GtkAdjustment *client = self->client;
      g_printerr ("ladj: client-changed: [%f %f] (%f %f %f) center: %g   CLIENT: [%f %f]\n",
                  adj->lower, adj->upper,
                  adj->step_increment, adj->page_increment, adj->page_size,
                  self->center,
                  client ? client->lower : -99.777, client ? client->upper : -99.777);
    }
  
  if (!self->block_client)
    {
      self->block_client++;
      gtk_adjustment_changed (adj);
      self->block_client--;
    }
}

static void
log_adjustment_client_value_changed (GxkLogAdjustment *self)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (self);
  GtkAdjustment *client = self->client;
  
  if (client)
    adj->value = call_log (self, client->value / self->center);
  adj->value = CLAMP (adj->value, adj->lower, adj->upper);
  
  if (0)
    g_printerr ("ladj: client-value-changed: [%f %f] %g   CLIENT: [%f %f] %g\n",
                adj->lower, adj->upper, adj->value,
                client->lower, client->upper, client->value);
  
  if (!self->block_client)
    {
      self->block_client++;
      gtk_adjustment_value_changed (adj);
      self->block_client--;
    }
}
