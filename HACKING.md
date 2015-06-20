% BEAST/BSE HACKING

The Beast + BSE code base origins date back to the 90ties, so hacking on it requires some knowledge about ancient aspects and mixings with newer technologies. The following gives a high-level overview of the pitfalls involved, this file can be discussed at beast@gnome.org.

Migrations
==========

The code base is currently undergoing several migrations and new developments:

IDL-Migration
-------------
**[STARTED]** (For BSE) Move from sfidl (and the old PROC files) to AIDA IDL (aidacc, distributed with Rapicorn). BSE already contains an AIDA style IDL file with C++11 objects that can be used in Beast. Eventually, all sfidl files need to be ported to AIDA IDL.

PROC-Migration
--------------
**[STARTED]** Move from PROC files to IDL files. Initially procedures were moved into sfidl files, but now procedures need to be moved to AIDA IDL. Note that during build time, bsehack.idl is generated, that already contains IDL formatting for all procedures.

Bstrecords-Migration
--------------------
**[STARTED]** Beast generates some structures from bstrecords.idl (sfidl), these all need to be moved into bstapi.idl (ADIA IDL).

CXX-Migration
-------------
**[STARTED]** Move from GObject (BseObject) to AIDA C++11 objects, derived from Aida::ImplicitBase and specified in IDL files. The following steps are planned:
* bse_object_new_valist is used everywhere instead of g_object_new() and its variants.
* *bse_object_new_valist* creates the BseObject/GObject and then the IDL based C++ object. Back pointers are installed to link the two together. That way features can be migrated incrementally from BseObject to CxxObject. The C++ objects auto-convert to their BseObject counter parts. This allows *base* object types to be replaced by CxxObjects quickly.
    * First, signals should be migrated, as signals are the the main tie to libgobject.so.
    * Second, once all signals are converted, all g_signal_ related code is eliminated from BSE and SFI, bse_proxy_connect will be removed.
    * Last, all g_object_ and g_type_ calls can be replaced.

Rapicorn-Migration
------------------
**[PLANNED]** Migrate the use of the Gtk+ toolkti to use Rapicorn widgets instead.

Python-REPL
-----------
**[PLANNED]** Add interactive REPL loop via Python interpreter, requires
integration of multiple main loops.

Python-Scripting
----------------
**[PLANNED]** Use Python scripts instead of Scheme scripts. As soon as it's
possible to implement basic Python scripts, SCM scripts may be broken and
shall be replaced.

Plugin-Merging
--------------
**[STARTED]** The BSE plugin API has allmost no uses, but pluging loading
causes major slowdowns in the startup phase. As a consequence, all plugins
shipped together with BSE should be linked into the same ELF library.

Enum-Migration
--------------
**[COMPLETE]** Beast enums have all been migrated from scanned C headers into bstapi.idl.

C++11-Compilation
-----------------
**[COMPLETE]** All .c files have been turned into .cc files, so all sources are now compiled as C++11.


Conversions
===========

Conversion between object handles works as follows:
  // In bse/
  BseServer *server = server_impl->as<BseObject*>();
  ServerImpl *server_impl = server->as<ServerImpl*>();
  // In beast-gtk/
  ServerIface *server_iface;
  ObjectIfaceP server_iface->from_proxy (proxyid);
  SfiProxy proxy = server_iface->proxy_id();
