// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#pragma once
#include <bse/bsemain.hh>
#include <bse/bseserver.hh>
#include <bse/bsemathsignal.hh>
#include <bse/bsecategories.hh>
#include <bse/bsestandardsynths.hh>
#include <unordered_map>

namespace BseTool {
using namespace Bse;

extern bool verbose;
#define printq(...)     do { if (BseTool::verbose) printout (__VA_ARGS__); } while (0)

struct ArgDescription {
  const char *arg_name, *value_name, *arg_blurb;
  String value;
};
using ArgDescriptions = std::vector<ArgDescription>;

class ArgParser {
  const size_t                                n_args_;
  ArgDescription                             *const args_;
  std::unordered_map<String, ArgDescription*> names_;
  StringVector                                dynamics_;
  void                parse_args (const size_t N, const ArgDescription *adescs);
public:
  template<size_t N>
  explicit            ArgParser  (ArgDescription (&adescs) [N]) : n_args_ (N), args_ (adescs) {}
  String              parse_args (const uint argc, char *const argv[]); // returns error message
  ArgDescriptions     list_args  () const;
  String              operator[] (const String &arg_name) const;
  const StringVector& dynamics () const { return dynamics_; }
};

class CommandRegistry {
  ArgParser        arg_parser_;
  String         (*cmd_) (const ArgParser&);
  String           name_, blurb_;
  CommandRegistry *next_;
  static CommandRegistry *command_registry_chain_;
public:
  template<size_t N>
  explicit              CommandRegistry (ArgDescription (&adescs) [N], String (*cmd) (const ArgParser&),
                                         const String &name, const String &blurb) :
    arg_parser_ (adescs), cmd_ (cmd), name_ (name), blurb_ (blurb), next_ (command_registry_chain_)
  {
    command_registry_chain_ = this;
  }
  virtual              ~CommandRegistry ();
  CommandRegistry*      next            ()                                    { return next_; }
  String                name            () const                              { return name_; }
  String                blurb           () const                              { return blurb_; }
  ArgDescriptions       list_args       () const                              { return arg_parser_.list_args(); }
  String                run             ()                                    { return cmd_ (arg_parser_); }
  String                parse_args      (const uint argc, char *const argv[]) { return arg_parser_.parse_args (argc, argv); }
  static CommandRegistry* chain_start   ()                                    { return command_registry_chain_; }
};

} // BseTool
