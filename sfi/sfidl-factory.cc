/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2004 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
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
