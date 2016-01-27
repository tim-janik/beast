# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0   -*-mode:python;-*-
include "Aida.pyx"

# Generated Bse bindings (via PyxxStub.py)
include "bseidlapi.pyx"

# provide libbse setup functions
cdef extern from "pysupport.hh" namespace "Bse":
  Bse__Server Bse__init_server_instance   "Bse::init_server_instance" ()
  bool        Bse__py_init_async          "Bse::py_init_async" () except *

# initialize libbse from sys.argv if init_needed()
Bse__py_init_async()

# provide main libbse server singleton
server = Bse__Object__wrap (Bse__init_server_instance())
