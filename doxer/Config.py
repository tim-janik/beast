#!/usr/bin/env python2.4
#
# Doxer - Software documentation system
# Copyright (C) 2005-2006 Tim Janik
# Copyright (C) 2007 Stefan Westerfeld
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
import sys

# --- constants ---
init_dict = {
  'CONFIG_VERSION'      : 0.6,
  'TMP_DIR'		: '/tmp',
}

# --- constant dictionary ---
class ConstDict:
  class AccessError (TypeError): pass
  def __init__ (self, init_dict = {}):
    self.__dict__.update (init_dict)
  def __setattr__ (self, name, value):
    if self.__dict__.has_key (name):
      raise self.AccessError ("Attempt to rebind constant:", name)
    self.__dict__[name] = value
  def __delattr__ (self, name):
    raise self.AccessError ("Attempt to unbind constant:", name)
  def __getattr__ (self, name):
    try:
      return self.__dict__[name]
    except:
      return None
  def update (self, dict):
    self.__dict__.update (dict)
  def MutableDict (self):
    class MutableDict:
      def __getattr__ (self, name):
        try:
          return self.__dict__[name]
        except:
          return None
      def __setattr__ (self, name, value):
        if value == None:
          del self.__dict__[name]
        else:
          self.__dict__[name] = value
      def __delattr__ (self, name):
        try:
          del self.__dict__[name]
        except: pass
      def keys (self):
        return self.__dict__.keys()
      def __getitem__ (self, index):
        return self.__dict__.__getitem__ (index)
    return MutableDict()
  def debug_print (self, *args):
    import sys
    if self.__dict__.has_key ("debug"):
      line = ""
      for s in args:
        if line:
          line += " "
        line += str (s)
      sys.stderr.write ("|> " + line + "\n")
  # doxer warnings
  def doxer_warn (self, msg, stacklevel=1):
    class DoxerWarning (UserWarning):
      """
        Doxer's own warnings (we subclass to not conflict with installed warning filters)
      """
    import warnings
    if self.__dict__.has_key ("fatal_warnings"):
      raise msg
    else:
      warnings.warn (msg + ": ", DoxerWarning, stacklevel=stacklevel+1)
  def doxer_warn_if_reached (self):
    import inspect
    frame_info = inspect.getframeinfo (inspect.currentframe (1))
    self.doxer_warn ("doxer_warn_if_reached() encountered in function %s" % (frame_info[2]), 2)
  def doxer_warn_if_fail (self, condition):
    import inspect
    if not condition:
      frame_info = inspect.getframeinfo (inspect.currentframe (1))
      self.doxer_warn ("doxer_warn_if_fail() failed in function %s" % (frame_info[2]), 2)

# --- module interface ---
sys.modules[__name__] = ConstDict (init_dict)
