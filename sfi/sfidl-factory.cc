/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2004 Stefan Westerfeld
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
#include "sfidl-factory.h"
#include "glib-extra.h"

using namespace Sfidl;
using namespace std;

namespace {
  list<Factory *> factories;
}

Factory::Factory()
{
  factories.push_back (this);
}

Factory::~Factory()
{
  list<Factory *>::iterator fi = find (factories.begin(), factories.end(), this);
  g_assert (fi != factories.end());
  factories.erase (fi);
}

list<Factory *> Factory::listFactories()
{
  return factories;
}

/* vim:set ts=8 sts=2 sw=2: */
