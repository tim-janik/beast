// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <sfi/glib-extra.hh>
#include "bstlogadjustment.hh"
#include <math.h>
/* --- prototypes --- */
static void	bst_log_adjustment_destroy		(GtkObject		*object);
static void	bst_log_adjustment_changed		(GtkAdjustment		*adj);
static void	bst_log_adjustment_value_changed	(GtkAdjustment		*adj);
static void	ladj_adjust_ranges			(BstLogAdjustment	*ladj);
static void	ladj_client_value_changed		(BstLogAdjustment	*ladj);
/* --- functions --- */
G_DEFINE_TYPE (BstLogAdjustment, bst_log_adjustment, GTK_TYPE_ADJUSTMENT);
static void
bst_log_adjustment_class_init (BstLogAdjustmentClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkAdjustmentClass *adjustment_class = GTK_ADJUSTMENT_CLASS (klass);
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
  ladj->client = NULL;
  bst_log_adjustment_setup (ladj, 10000, 10, 4);
}
static void
bst_log_adjustment_destroy (GtkObject *object)
{
  BstLogAdjustment *ladj = BST_LOG_ADJUSTMENT (object);
  bst_log_adjustment_set_client (ladj, NULL);
  /* chain parent class handler */
  GTK_OBJECT_CLASS (bst_log_adjustment_parent_class)->destroy (object);
}
GtkAdjustment*
bst_log_adjustment_from_adj (GtkAdjustment *client)
{
  BstLogAdjustment *ladj;
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (client), NULL);
  ladj = (BstLogAdjustment*) g_object_new (BST_TYPE_LOG_ADJUSTMENT, NULL);
  bst_log_adjustment_set_client (ladj, client);
  return GTK_ADJUSTMENT (ladj);
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
      g_signal_handlers_disconnect_by_func (ladj->client, (void*) ladj_adjust_ranges, ladj);
      g_signal_handlers_disconnect_by_func (ladj->client, (void*) ladj_client_value_changed, ladj);
      g_object_unref (G_OBJECT (ladj->client));
    }
  ladj->client = client;
  if (ladj->client)
    {
      g_object_ref (G_OBJECT (ladj->client));
      g_object_connect (ladj->client,
			"swapped_signal::changed", ladj_adjust_ranges, ladj,
			"swapped_signal::value_changed", ladj_client_value_changed, ladj,
			NULL);
      ladj_adjust_ranges (ladj);
    }
  g_object_unref (ladj);
}
void
bst_log_adjustment_setup (BstLogAdjustment *ladj,
			  gdouble           center,
			  gdouble           base,
			  gdouble           n_steps)
{
  GtkAdjustment *adj;
  g_return_if_fail (BST_IS_LOG_ADJUSTMENT (ladj));
  g_return_if_fail (n_steps > 0);
  g_return_if_fail (base > 0);
  adj = GTK_ADJUSTMENT (ladj);
  ladj->center = center;
  ladj->n_steps = n_steps;
  ladj->base = base;
  ladj->base_ln = log (ladj->base);
  ladj->ulimit = pow (ladj->base, ladj->n_steps);
  ladj->llimit = 1.0 / ladj->ulimit;
  adj->value = ladj->center;
  ladj_adjust_ranges (ladj);
  gtk_adjustment_value_changed (adj);
}
static void
bst_log_adjustment_changed (GtkAdjustment *adj)
{
  BstLogAdjustment *ladj = BST_LOG_ADJUSTMENT (adj);
  GtkAdjustment *client = ladj->client;
  if (client && !ladj->block_client)
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
  adj->value = CLAMP (adj->value, adj->lower, adj->upper);
  if (client && !ladj->block_client)
    {
      client->value = call_exp (ladj, adj->value) * ladj->center;
      ladj->block_client++;
      gtk_adjustment_value_changed (client);
      ladj->block_client--;
    }
}
static void
ladj_adjust_ranges (BstLogAdjustment *ladj)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (ladj);
  adj->upper = ladj->n_steps;
  adj->lower = -ladj->n_steps;
  adj->page_increment = (adj->upper - adj->lower) / (2.0 * ladj->n_steps);
  adj->step_increment = adj->page_increment / 100.0;
  adj->page_size = 0;
  if (0)
    {
      GtkAdjustment *client = ladj->client;
      g_printerr ("ladj: client-changed: [%f %f] (%f %f %f) center: %g   CLIENT: [%f %f]\n",
                  adj->lower, adj->upper,
                  adj->step_increment, adj->page_increment, adj->page_size,
                  ladj->center,
                  client ? client->lower : -99.777, client ? client->upper : -99.777);
    }
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
  if (client)
    adj->value = call_log (ladj, client->value / ladj->center);
  adj->value = CLAMP (adj->value, adj->lower, adj->upper);
  if (0)
    g_printerr ("ladj: client-value-changed: [%f %f] %g   CLIENT: [%f %f] %g\n",
                adj->lower, adj->upper, adj->value,
                client->lower, client->upper, client->value);
  if (!ladj->block_client)
    {
      ladj->block_client++;
      gtk_adjustment_value_changed (adj);
      ladj->block_client--;
    }
}
