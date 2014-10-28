// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstprojectctrl.hh"

#include <math.h>


/* --- functions --- */
G_DEFINE_TYPE (BstProjectCtrl, bst_project_ctrl, GTK_TYPE_HBOX);

void
bst_project_ctrl_play (BstProjectCtrl *self)
{
  if (self && self->project)
    {
      gchar *starting;
      BseErrorType error;

      if (bse_project_is_playing (self->project))
	starting = _("Restarting Playback");
      else
	starting = _("Starting Playback");
      bse_project_auto_deactivate (self->project, 0);
      error = bse_project_play (self->project);
      bst_status_eprintf (error, "%s", starting);
    }
}

void
bst_project_ctrl_stop (BstProjectCtrl *self)
{
  if (self && self->project)
    {
      bse_project_stop (self->project);
      gxk_status_set (GXK_STATUS_DONE, _("Stopping Playback"), NULL);
    }
}

static void
bst_project_ctrl_finalize (GObject *object)
{
  BstProjectCtrl *self = BST_PROJECT_CTRL (object);
  bst_project_ctrl_set_project (self, 0);
  G_OBJECT_CLASS (bst_project_ctrl_parent_class)->finalize (object);
}

static void
project_state_changed (BstProjectCtrl *self,
		       SfiChoice       choice)
{
  BseProjectState state = bse_project_state_from_choice (choice);

  if (self->led)
    switch (state)
      {
      case BSE_PROJECT_ACTIVE:
	gxk_led_set_color (self->led, GXK_LED_BLUE);
	break;
      case BSE_PROJECT_PLAYING:
	gxk_led_set_color (self->led, GXK_LED_GREEN);
	break;
      default:
	gxk_led_set_color (self->led, GXK_LED_OFF);
	break;
      }
}

static void
project_release (BstProjectCtrl *self)
{
  bst_project_ctrl_set_project (self, 0);
}

void
bst_project_ctrl_set_project (BstProjectCtrl *self,
			      SfiProxy        project)
{
  g_return_if_fail (BST_IS_PROJECT_CTRL (self));
  if (project)
    g_return_if_fail (BSE_IS_PROJECT (project));

  if (self->project)
    bse_proxy_disconnect (self->project,
			  "any_signal", project_state_changed, self,
			  "any_signal::release", project_release, self,
			  NULL);
  self->project = project;
  if (self->project)
    {
      bse_proxy_connect (self->project,
			 "swapped_signal::state-changed", project_state_changed, self,
			 "swapped_signal::release", project_release, self,
			 NULL);
      project_state_changed (self, bse_project_state_to_choice (bse_project_get_state (self->project)));
    }
  else if (self->led)
    gxk_led_set_color (self->led, GXK_LED_OFF);
}

static void
bst_project_ctrl_init (BstProjectCtrl *self)
{
  GtkWidget *box = GTK_WIDGET (self);
  GtkWidget *frame;
  guint spaceing = 0;

  /* Led */
  self->led = (GxkLed*) gxk_led_new (GXK_LED_OFF);
  frame = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                     "shadow_type", GTK_SHADOW_ETCHED_OUT,
                                     "child", self->led,
                                     NULL);
  gxk_led_set_border_width (self->led, 1);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, spaceing);
  gxk_nullify_in_object (self, &self->led);

  /* Stop */
  self->stop = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
			     "child", gxk_polygon_new (&gxk_polygon_stop),
			     "can_focus", FALSE,
			     NULL);
  g_object_connect (self->stop, "swapped_signal::clicked", bst_project_ctrl_stop, self, NULL);
  gtk_box_pack_start (GTK_BOX (box), self->stop, FALSE, FALSE, spaceing);
  gxk_nullify_in_object (self, &self->stop);

  /* size-group Led's frame and Stop */
  gxk_size_group (GTK_SIZE_GROUP_BOTH, frame, self->stop, NULL);

  /* Rewind */
  self->rew = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                         "child", gxk_polygon_new (&gxk_polygon_rewind),
                                         "can_focus", FALSE,
                                         "sensitive", FALSE,
                                         NULL);
  gtk_box_pack_start (GTK_BOX (box), self->rew, FALSE, FALSE, spaceing);
  gxk_nullify_in_object (self, &self->rew);

  /* Play */
  self->play = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                          "child", gxk_polygon_new (&gxk_polygon_play),
                                          "can_focus", FALSE,
                                          NULL);
  g_object_connect (self->play, "swapped_signal::clicked", bst_project_ctrl_play, self, NULL);
  gtk_box_pack_start (GTK_BOX (box), self->play, FALSE, FALSE, spaceing);
  gxk_nullify_in_object (self, &self->play);

  /* Pause */
  self->pause = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                           "child", gxk_polygon_new (&gxk_polygon_pause),
                                           "can_focus", FALSE,
                                           "sensitive", FALSE,
                                           NULL);
  gtk_box_pack_start (GTK_BOX (box), self->pause, FALSE, FALSE, spaceing);
  gxk_nullify_in_object (self, &self->pause);

  /* Forward */
  self->fwd = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                         "child", gxk_polygon_new (&gxk_polygon_forward),
                                         "can_focus", FALSE,
                                         "sensitive", FALSE,
                                         NULL);
  gtk_box_pack_start (GTK_BOX (box), self->fwd, FALSE, FALSE, spaceing);
  gxk_nullify_in_object (self, &self->fwd);

  gtk_widget_show_all (box);
}


static void
bst_project_ctrl_class_init (BstProjectCtrlClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bst_project_ctrl_finalize;
}
