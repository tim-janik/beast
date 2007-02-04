/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2000,2002 Stefan Westerfeld
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
#include <string.h>
#include <stdio.h>
#include <list>
#include <string>
#include <map>
#include "sfidl-namespace.hh"
#include <sfi/glib-extra.h>

using namespace std;

/* generic utilities */

static list<string> symbolToList(string symbol)
{
  list<string> result;
  string current;
  
  string::iterator si;
  for(si = symbol.begin(); si != symbol.end(); si++)
    {
      if(*si != ':')
	{
	  current += *si;
	}
      else
	{
	  if(current != "")
	    result.push_back(current);
	  
	  current = "";
	}
    }
  
  result.push_back(current);
  return result;
}

static string listToSymbol(list<string>& symlist)
{
  string s;
  list<string>::iterator si;
  for(si = symlist.begin(); si != symlist.end(); si++)
    {
      if(s != "") s += "::";
      s += *si;
    }
  return s;
}

/* NamespaceHelper */
NamespaceHelper::NamespaceHelper(FILE *outputfile) : out(outputfile)
{
}

NamespaceHelper::~NamespaceHelper()
{
  leaveAll();
}

void
NamespaceHelper::setFromSymbol(string symbol)
{
  list<string> symlist = symbolToList (symbol);
  symlist.pop_back();
  
  /* check that the current namespace doesn't contain wrong parts at end */
  list<string>::iterator ni,si;
  ni = currentNamespace.begin();
  si = symlist.begin();
  while (ni != currentNamespace.end() && si != symlist.end() && *ni == *si)
    {
      ni++;
      si++;
    }
  /* close unwanted namespaces */
  reverse (ni, currentNamespace.end()); /* close inner namespaces first */
  while (ni != currentNamespace.end())
    {
      fprintf (out,"} // %s\n", (*ni++).c_str());
    }
  
  /* enter new components at the end */
  while (si != symlist.end())
    {
      fprintf(out,"namespace %s {\n",(*si++).c_str());
    }
  currentNamespace = symlist;
}

void NamespaceHelper::leaveAll()
{
  setFromSymbol("unqualified");
}

string NamespaceHelper::printableForm(string symbol)
{
  list<string> symlist = symbolToList(symbol);
  list<string> current = currentNamespace;
  
  while(!current.empty())
    {
      // namespace longer than symbol?
      g_assert (!symlist.empty());
      
      if(*current.begin() == *symlist.begin())
	{
	  current.pop_front();
	  symlist.pop_front();
	}
      else
	{
	  return "::"+symbol;
	}
    }
  
  return listToSymbol(symlist);
}
const char*
NamespaceHelper::printable_form (std::string symbol)
{
  return g_intern_string (printableForm (symbol).c_str());
}

string NamespaceHelper::nameOf(string symbol)
{
  if(symbol == "") return "";
  
  list<string> symlist = symbolToList(symbol);
  return symlist.back();
}

string NamespaceHelper::namespaceOf(string symbol)
{
  list<string> symlist = symbolToList(symbol);
  if(symlist.size() < 2) return "";
  
  symlist.pop_back();
  return listToSymbol(symlist);
}

/* vim:set ts=8 sts=2 sw=2: */
