// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsetool.hh"
#include <bse/bse.hh>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>

using namespace Bse;
using namespace BseTool;

// == arg parsing ==
String
ArgParser::operator[] (const String &arg_name) const
{
  auto it = names_.find (arg_name);
  if (it == names_.end())
    return "";
  ArgDescription *adesc = it->second;
  return adesc->value;
}

ArgDescriptions
ArgParser::list_args () const
{
  ArgDescriptions args;
  for (size_t i = 0; i < n_args_; i++)
    if (args_[i].arg_name && args_[i].arg_name[0])
      args.push_back (args_[i]);
  return args;
}

String
ArgParser::parse_args (const uint argc, char *const argv[])
{
  std::unordered_map<String, String> aliases;
  std::vector<String> fixed; // fixed arguments, like <OBLIGATION> or [OPTIONAL]
  bool with_dynamics = false;
  size_t need_obligatory = 0;
  // fill names_ from args_
  for (size_t i = 0; i < n_args_; i++)
    {
      if (!args_[i].arg_name)
        continue;
      StringVector names = string_split (args_[i].arg_name, ",");
      for (size_t j = 0; j < names.size(); j++)
        {
          String name = string_canonify (names[j], string_set_a2z() + string_set_A2Z() + "0123456789_-", "");
          while (name[0] == '-')
            name = name.substr (1);
          if (name.empty())
            continue;
          if (j == 0)
            {
              names_[name] = &args_[i];
              if (names[j][0] == '<')
                {
                  need_obligatory++;
                  fixed.push_back (name);
                }
              else if (names[j][0] == '[')
                {
                  if (string_endswith (names[j], "...]"))
                    with_dynamics = true;
                  else
                    fixed.push_back (name);
                }
            }
          else // found an alias after ','
            names_[name] = &args_[i];
        }
    }
  // parse and assign command line arguments
  bool seen_dashdash = false;
  size_t fixed_index = 0;
  for (size_t i = 0; i < argc; i++)
    if (!argv[i])
      continue;
    else if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == 0)
      seen_dashdash = true;
    else if (!seen_dashdash && argv[i][0] == '-')
      {
        const char *arg = argv[i] + 1 + (argv[i][1] == '-');
        const char *eq = strchr (arg, '=');
        String arg_name = !eq ? arg : String (arg, eq - arg);
        auto it = names_.find (arg_name);
        if (it == names_.end())
          return string_format ("invalid argument: %s", argv[i]);
        ArgDescription *adesc = it->second;
        if (adesc->value_name && adesc->value_name[0])
          {
            if (eq)
              adesc->value = eq + 1;
            else
              {
                if (i + 1 >= argc)
                  return string_format ("incomplete argument: %s", argv[i]); // argument lacks value assignment
                i++;
                adesc->value = argv[i];
              }
          }
        else
          adesc->value = "1"; // value means 'option is present'
      }
    else // non-option arguments
      {
        if (fixed_index < fixed.size())
          {
            ArgDescription *adesc = names_[fixed[fixed_index]];
            adesc->value = argv[i];
            fixed_index++;
          }
        else if (with_dynamics)
          dynamics_.push_back (argv[i]);
        else
          return string_format ("invalid extra argument: %s", argv[i]);
      }
  if (fixed_index < need_obligatory)
    return string_format ("missing mandatory argument <%s>", fixed[fixed_index]);
  return ""; // success
}

// == CommandRegistry ==
CommandRegistry *CommandRegistry::command_registry_chain_ = NULL;
CommandRegistry::~CommandRegistry()
{
  command_registry_chain_ = NULL;
}

// == crawl ==
static ArgDescription crawl_options[] = {
  { "<glob>",   "",     "Glob pattern to list matching files", "" },
};

static String
crawl (const ArgParser &ap)
{
  const String glob = ap["glob"];
  // plist = bse_search_path_list_files (argv[1], NULL);
  SfiRing *ring = sfi_file_crawler_list_files (glob.c_str(), NULL, GFileTest (0));
  while (ring)
    {
      const char *name = (const char*) sfi_ring_pop_head (&ring);
      printout ("%s\n", name);
    }
  sfi_ring_free_deep (ring, g_free);
  return "";
}

static CommandRegistry crawl_cmd (crawl_options, crawl, "crawl", "Test filesystem crawling");

// == dump-info ==
static ArgDescription dump_info_options[] = {
  { "", "", "", "" }, // dummy, no options currently
};

#define	PREC_SHIFT      16
#define	FLF	        "26.20"

static String
dump_info (const ArgParser &ap)
{
  auto print_int = [] (const char *name, int integer) {
    printout ("%s =%-4d\n", name, integer);
  };
  auto print_note = [] (const char *note_name, int note) {
    char *string = bse_note_to_string (note);
    printout ("%s =%-4d \tfactor=%" FLF "f [%-5s] (freq=%" FLF "f)\n",
              note_name, note,
              bse_transpose_factor (Bse::MusicalTuning::OD_12_TET, note - BSE_KAMMER_NOTE),
              string, bse_note_to_freq (Bse::MusicalTuning::OD_12_TET, note));
    g_free (string);
  };
  auto print_fine_tune = [] (const char *tune_name, int tune) {
    printout ("%s =%-4d \tfactor=%" FLF "f\n",
              tune_name, tune,
              bse_cent_tune_fast (tune));
  };
  printout ("Version: %s\n", Bse::version());
  printout ("Rate relevant limits:\n");
  print_int       ("BSE_MIN_OCTAVE   ", BSE_MIN_OCTAVE);
  print_int       ("BSE_MAX_OCTAVE   ", BSE_MAX_OCTAVE);
  print_note      ("BSE_MIN_NOTE     ", BSE_MIN_NOTE);
  print_note      ("BSE_KAMMER_NOTE  ", BSE_KAMMER_NOTE);
  print_note      ("BSE_MAX_NOTE     ", BSE_MAX_NOTE);
  print_note      ("BSE_KAMMER_NOTE-1", BSE_KAMMER_NOTE - 1);
  print_fine_tune ("BSE_MIN_FINE_TUNE", BSE_MIN_FINE_TUNE);
  print_fine_tune ("bse-mid-fine-tune", (BSE_MIN_FINE_TUNE + BSE_MAX_FINE_TUNE) / 2);
  print_fine_tune ("BSE_MAX_FINE_TUNE", BSE_MAX_FINE_TUNE);
  print_note      ("BSE_KAMMER_NOTE+1", BSE_KAMMER_NOTE + 1);

  int j, k;
  if (0)
    for (j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j += 3)
      print_note (":", j);
  if (0)
    for (j = BSE_MIN_FINE_TUNE; j <= BSE_MAX_FINE_TUNE; j += 10)
      print_fine_tune (":", j);

  if (0)
    for (j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j += 3)
      for (k = BSE_MIN_FINE_TUNE / 2; k <= BSE_MAX_FINE_TUNE / 2; k += 10)
	{
	  double f, freq = bse_note_to_tuned_freq (Bse::MusicalTuning::OD_12_TET, j, k);
	  int note, fine_tune;
	  printout ("compose  : note=%4d fine_tune=%4d freq=%" FLF "f\n", j, k, freq);
	  f = freq;
	  note = bse_note_from_freq (Bse::MusicalTuning::OD_12_TET, freq);
	  fine_tune = bse_note_fine_tune_from_note_freq (Bse::MusicalTuning::OD_12_TET, note, freq);
	  freq = bse_note_to_tuned_freq (Bse::MusicalTuning::OD_12_TET, note, fine_tune);
	  printout ("decompose: note=%4d fine_tune=%4d freq=%" FLF "f   (diff=%g)\n", note, fine_tune, freq, freq - f);
	}
  if (0)
    for (j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j += 1)
      {
	int octave = SFI_NOTE_OCTAVE (j);
	int semitone = SFI_NOTE_SEMITONE (j);
	int note = BSE_NOTE_GENERIC (octave, semitone);
	char *name = bse_note_to_string (j);

	printout ("note[%3d]: name=%-8s octave=%3d semitone=%3d note=%3d match=%u\n",
                  j, name, octave, semitone, note, j == note);
	g_free (name);
      }
  return "";
}

static CommandRegistry dump_info_cmd (dump_info_options, dump_info, "dump-info", "Printout common constants");


// == dump-categories ==
static ArgDescription dump_categories_options[] = {
  { "", "", "", "" }, // dummy, no options currently
};

static String
dump_categories (const ArgParser &ap)
{
  Bse::CategorySeq cseq = bse_categories_match_typed ("*", 0);
  for (size_t i = 0; i < cseq.size(); i++)
    printout ("%s\t(%s)\n", cseq[i].category, cseq[i].otype);
  return "";
}

static CommandRegistry dump_categories_cmd (dump_categories_options, dump_categories, "dump-categories",
                                            "Printout the BSE category registry");


// == standard-synth ==
static ArgDescription standard_synth_options[] = {
  { "<synth-name>", "", "The BSE synth to show (e.g. 'adsr-wave-1')", "" },
};

static String
standard_synth (const ArgParser &ap)
{
  const String bsefile = ap["synth-name"];
  gchar *text = bse_standard_synth_inflate (bsefile.c_str(), NULL);
  if (!text)
    return string_format ("no such synth: %s", bsefile);
  printout ("%s", text);
  g_free (text);
  return ""; // success
}

static CommandRegistry standard_synth_cmd (standard_synth_options, standard_synth, "standard-synth", "Display definition of standard synthesizers");


// == render2wav ==
static ArgDescription render2wav_options[] = {
  { "-s, --seconds", "<seconds>", "Number of seconds to record", "0" },
  { "<bse-file>",    "",          "The BSE file for audio rendering", "" },
  { "<wav-file>",    "",          "The WAV file to use for audio output", "" },
};

static String
render2wav (const ArgParser &ap)
{
  const String bsefile = ap["bse-file"];
  const String wavfile = ap["wav-file"];
  const double n_seconds = string_to_double (ap["seconds"]);
  auto project = BSE_SERVER.create_project (bsefile);
  project->auto_deactivate (0);
  auto err = project->restore_from_file (bsefile);
  if (err != 0)
    return bse_error_blurb (err);
  BSE_SERVER.start_recording (wavfile, n_seconds);
  err = project->play();
  printq ("Recording %s to %s...\n", bsefile, wavfile);
  printq (".");
  int counter = 0;
  while (project->is_playing())
    {
      if (counter == 0)
        printq ("\b*");
      else if (counter == 25)
        printq ("\bo");
      if (g_main_context_pending (bse_main_context))
        g_main_context_iteration (bse_main_context, false);
      else
        usleep (10 * 1000);
      counter = (counter + 1) % 50;
    }
  printq ("\n");

  return "";
}

static CommandRegistry render2wav_cmd (render2wav_options, render2wav, "render2wav", "Render audio from a .bse file into a WAV file");


// == check-load ==
static ArgDescription check_load_options[] = {
  { "<bse-file>",    "",          "The BSE file to load and check for validity", "" },
};

static String
check_load (const ArgParser &ap)
{
  const String bsefile = ap["bse-file"];
  auto project = BSE_SERVER.create_project (bsefile);
  project->auto_deactivate (0);
  auto err = project->restore_from_file (bsefile);
  if (err != 0)
    return string_format ("%s: loading failed: %s", bsefile, bse_error_blurb (err));
  // success
  return "";
}

static CommandRegistry check_load_cmd (check_load_options, check_load, "check-load", "Test if a .bse file can be successfully loaded");


// == type-tree ==
static void
show_nodes (GType root, GType type, GType sibling, const String &indent, const ArgParser &ap)
{
  constexpr const char O_KEY_FILL[] = "_";
  constexpr const char O_SPACE[] = " ";
  constexpr const char O_ESPACE[] = "";
  constexpr const char O_BRANCH[] = "+";
  constexpr const char O_VLINE[] = "|";
  constexpr const char O_LLEAF[] = "`";
  uint spacing = 1;
  bool feature_blurb = string_to_bool (ap["blurb"]);
  bool feature_channels = string_to_bool (ap["channels"]);
  bool recursion = true;
  String indent_inc;

  if (!type)
    return;

  if (indent_inc.empty())
    {
      indent_inc += O_SPACE;
      indent_inc += O_SPACE;
      indent_inc += O_SPACE;
    }

  GType *children = g_type_children (type, NULL);

  if (type != root)
    for (uint i = 0; i < spacing; i++)
      printout ("%s%s\n", indent, O_VLINE);

  printout ("%s%s%s%s",
            indent,
            sibling ? O_BRANCH : (type != root ? O_LLEAF : O_SPACE),
            O_ESPACE,
            g_type_name (type));

  for (uint i = strlen (g_type_name (type)); i <= indent_inc.size(); i++)
    printout ("%s", O_KEY_FILL);

  if (feature_blurb && bse_type_get_blurb (type))
    {
      printout ("\t[");
      printout ("%s", bse_type_get_blurb (type));
      printout ("]");
    }

  if (G_TYPE_IS_ABSTRACT (type))
    printout ("\t(abstract)");

  if (feature_channels && g_type_is_a (type, BSE_TYPE_SOURCE))
    {
      BseSourceClass *klass = (BseSourceClass*) g_type_class_ref (type);

      printout ("\t(ichannels %u) (ochannels %u)", klass->channel_defs.n_ichannels, klass->channel_defs.n_ochannels);
      g_type_class_unref (klass);
    }

  printout ("\n");

  if (children && recursion)
    {
      String new_indent;

      if (sibling)
	new_indent = indent + O_VLINE + indent_inc;
      else
	new_indent = indent + O_SPACE + indent_inc;

      for (GType *child = children; *child; child++)
	show_nodes (root, child[0], child[1], new_indent, ap);
    }

  g_free (children);
}

static ArgDescription type_tree_options[] = {
  { "[root-node]",    "", "Name of the type tree root node (or \"\" for all fundamentals)", "BseObject" },
  { "-b, --blurb",    "", "Show type blurb", "" },
  { "-c, --channels", "", "Show type channels", "" },
};

static String
type_tree (const ArgParser &ap)
{
  const String root_name = ap["root-node"];
  const char *iindent = "";
  if (root_name.empty())
    for (uint i = 0; i <= G_TYPE_FUNDAMENTAL_MAX; i += G_TYPE_MAKE_FUNDAMENTAL (1))
      {
        const char *name = g_type_name (i);
        if (name)
          show_nodes (~0, i, 0, iindent, ap);
      }
  else
    {
      GType root = g_type_from_name (root_name.c_str());
      if (root == 0)
        return string_format ("unknown type name: %s", root_name);
      show_nodes (root, root, 0, iindent, ap);
    }
  return ""; // success
}

static CommandRegistry type_tree_cmd (type_tree_options, type_tree, "type-tree", "Printout the BSE type tree");

// == help ==
static ArgDescription help_options[] = {
  { "",         "",     "",     "" },
};

static String
print_help (const ArgParser &ap)
{
  printout ("bsetool version %s\n", Bse::version());
  printout ("Usage: bsetool <command> [args...]\n");
  printout ("Commands:\n");
  std::vector<CommandRegistry*> cmds;
  for (CommandRegistry *cmd = CommandRegistry::chain_start(); cmd; cmd = cmd->next())
    cmds.push_back (cmd);
  auto cmp_cmd = [] (const CommandRegistry *a, const CommandRegistry *b) -> bool {
    return a->name() < b->name();
  };
  std::stable_sort (cmds.begin(), cmds.end(), cmp_cmd);
  for (const auto *cmd : cmds)
    {
      printout ("  %-16s %s\n", cmd->name(), cmd->blurb());
      for (const auto &arg : cmd->list_args())
        {
          String aname = String (arg.arg_name);
          if (arg.value_name && arg.value_name[0])
            {
              aname += " ";
              aname += arg.value_name;
            }
          if (aname.size() <= 14)
            printout ("    %-14s %s\n", aname, arg.arg_blurb);
          else
            printout ("    %s\n%18s %s\n", aname, "", arg.arg_blurb);
        }
    }
  return ""; // success
}

static CommandRegistry help_cmd (help_options, print_help, "help", "Print commands and options");

// == bse tool ==
static ArgDescription bsetool_options[] = {
  { "--bse-no-load", "", "Prevent automated plugin and script registration", "" },
  { "--quiet",       "", "Prevent progress output", "" },
};

bool BseTool::verbose = true;

static int
bsetool_main (int argc_int, char *argv[])
{
  const unsigned int argc = argc_int;
  // now that the BSE thread runs, drop scheduling priorities if we have any
  setpriority (PRIO_PROCESS, getpid(), 0);
  // pre-command option argument parsing
  size_t option_argc = 1; // skip argv[0]
  while (option_argc < argc && argv[option_argc][0] == '-')
    {
      if (argv[option_argc] == String ("-h") || argv[option_argc] == String ("--help"))
        {
          help_cmd.run();
          return 0;
        }
      option_argc++;
    }
  ArgParser toolap (bsetool_options);
  String error = toolap.parse_args (option_argc - 1, argv + 1); // skip argv[0]
  if (!error.empty())
    {
      printerr ("%s: %s\n", argv[0], error);
      return 127;
    }
  verbose = !string_to_bool (toolap["quiet"]);
  // load BSE plugins, scripts, ladspa plugins, etc
  if (!string_to_bool (toolap["bse-no-load"]))
    {
      BSE_SERVER.load_assets();
      while (g_main_context_pending (bse_main_context))
        g_main_context_iteration (bse_main_context, false);
    }
  // command parsing
  if (option_argc < argc)
    for (CommandRegistry *cmd = CommandRegistry::chain_start(); cmd; cmd = cmd->next())
      if (cmd->name() == argv[option_argc])
        {
          String error = cmd->parse_args (argc - option_argc - 1, argv + option_argc + 1);
          if (!error.empty())
            {
              printerr ("%s: %s: %s\n", argv[0], cmd->name(), error);
              return 127;
            }
          error = cmd->run();
          if (!error.empty())
            {
              printerr ("%s: %s\n", cmd->name(), error);
              return 127;
            }
          return 0; // success
        }

  printerr ("%s: %s\n", argv[0], "missing command");
  return 127;
}

int
main (int argc, char *argv[])
{
  Bse::StringVector args = Bse::init_args (&argc, argv);
  Bse::init_async ("bsetool", args);
  const auto ret = Bse::jobs += [argc, argv] () {
    return bsetool_main (argc, argv);
  };
  return ret;
}
