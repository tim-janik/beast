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
      const char *starting;
      Bse::Error error;

      if (self->project.is_playing())
	starting = _("Restarting Playback");
      else
	starting = _("Starting Playback");
      self->project.auto_deactivate (0);
      error = self->project.play();
      bst_status_eprintf (error, "%s", starting);
    }
}

void
bst_project_ctrl_stop (BstProjectCtrl *self)
{
  if (self && self->project)
    {
      self->project.stop();
      gxk_status_set (GXK_STATUS_DONE, _("Stopping Playback"), NULL);
    }
}

static void
bst_project_ctrl_finalize (GObject *object)
{
  BstProjectCtrl *self = BST_PROJECT_CTRL (object);
  bst_project_ctrl_set_project (self, Bse::ProjectH());
  G_OBJECT_CLASS (bst_project_ctrl_parent_class)->finalize (object);
  using namespace Bse;
  self->project.~ProjectS();
}

static void
project_state_changed (BstProjectCtrl *self, Bse::ProjectState state)
{
  if (self->led)
    switch (state)
      {
      case Bse::ProjectState::ACTIVE:
	gxk_led_set_color (self->led, GXK_LED_BLUE);
	break;
      case Bse::ProjectState::PLAYING:
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
  bst_project_ctrl_set_project (self, Bse::ProjectH());
}

void
bst_project_ctrl_set_project (BstProjectCtrl *self, Bse::ProjectH project)
{
  assert_return (BST_IS_PROJECT_CTRL (self));

  if (self->project)
    {
      bse_proxy_disconnect (self->project.proxy_id(),
                            "any_signal::release", project_release, self,
                            NULL);
      self->project = NULL;
    }
  self->project = project;
  if (self->project)
    {
      bse_proxy_connect (self->project.proxy_id(),
			 "swapped_signal::release", project_release, self,
			 NULL);
      self->project.on ("statechanged", [self] (const Aida::Event &ev) {
          const Bse::ProjectState state = ev["state"].get<Bse::ProjectState>();
          project_state_changed (self, state);
        });
      project_state_changed (self, self->project.get_state());
    }
  else if (self->led)
    gxk_led_set_color (self->led, GXK_LED_OFF);
}

static void
bst_project_ctrl_init (BstProjectCtrl *self)
{
  new (&self->project) Bse::ProjectS();
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
