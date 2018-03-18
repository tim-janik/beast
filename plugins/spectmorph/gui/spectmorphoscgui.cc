// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "spectmorphglui.hh"

using namespace SpectMorph;

using std::string;
using std::vector;

class OscGui : public SignalReceiver
{
  MorphPlanPtr      morph_plan;
  MorphPlanWindow   window;

public:
  OscGui (MorphPlanPtr plan, const std::string& title) :
    morph_plan (plan),
    window (title, /* win_id */ 0, /* resize */ false, morph_plan, MorphPlanControl::NO_VOLUME)
  {
    connect (morph_plan->signal_plan_changed, this, &OscGui::on_plan_changed);

    window.show();
  }

  void
  on_plan_changed()
  {
    vector<unsigned char> data;
    MemOut mo (&data);
    morph_plan->save (&mo);
    printf ("%s\n", HexString::encode (data).c_str());
    fflush (stdout);
  }

  void
  run()
  {
    bool quit = false;

    window.set_close_callback ([&]() { quit = true; });

    while (!quit)
      {
        window.wait_for_event();
        window.process_events();
      }
  }
};

int
main (int argc, char **argv)
{
  sm_init (&argc, &argv);

  if (argc != 2)
    {
      printf ("usage: %s <title>\n", argv[0]);
      exit (1);
    }

  // read initial plan
  string plan_str;
  int ch;
  while ((ch = fgetc (stdin)) != '\n')
    plan_str += (char) ch;

  // give parent our pid (for killing the gui)
  printf ("pid %d\n", getpid());
  fflush (stdout);

  MorphPlanPtr morph_plan = new MorphPlan();
  morph_plan->set_plan_str (plan_str);

  // show window, run mainloop
  OscGui gui (morph_plan, argv[1]);
  gui.run();

  // let parent know that the user closed the gui window
  printf ("quit\n");
  fflush (stdout);

  return 0;
}
