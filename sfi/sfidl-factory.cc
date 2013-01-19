// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfidl-factory.hh"
#include "glib-extra.hh"
#include <algorithm>
using namespace Sfidl;
namespace {
  list<Factory *> *factories = 0;
}
Factory::Factory()
{
  if (!factories)
    factories = new list<Factory *>();
  factories->push_back (this);
}
Factory::~Factory()
{
  list<Factory *>::iterator fi = find (factories->begin(), factories->end(), this);
  g_assert (fi != factories->end());
  factories->erase (fi);
}
list<Factory *> Factory::listFactories()
{
  return *factories;
}
/* vim:set ts=8 sts=2 sw=2: */
