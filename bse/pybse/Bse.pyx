# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0   -*-mode:python;-*-
include "Aida.pyx"

# Generated Bse bindings (via PyxxStub.py)
include "bseidlapi.pyx"

# Provide main Bse singleton
cdef extern from "bse/bse.hh" namespace "Bse":
  Bse__Server Bse__init_server_instance   "Bse::init_server_instance" ()
server = Bse__Object__wrap (Bse__init_server_instance())
