% BEAST/BSE HACKING

The Beast + BSE code base origins date back to the 90ties, so hacking on it requires some knowledge about ancient aspects and mixings with newer technologies. The following gives a high-level overview of the pitfalls involved, this file can be discussed at beast@gnome.org.

Migrations
==========

The code base is currently undergoing several migrations and new developments:

IDL-Migration
-------------
**[ONGOING]** (For BSE) Move from sfidl (and the old PROC files) to AIDA IDL (aidacc, distributed with Rapicorn). BSE already contains an AIDA style IDL file with C++11 objects that can be used in Beast. Eventually, all sfidl files need to be ported to AIDA IDL.

C++-Migration
-------------
**[ONGOING]** Move from GObject (BseObject) to AIDA C++11 objects, derived from Aida::ImplicitBase and specified in IDL files. The following steps are planned:
* **[DONE]** For object creation, `bse_object_new_valist()` is used everywhere instead of `g_object_new()` and its variants.
* **[DONE]** `bse_object_new_valist()` creates the GObject based BseObject and then creates and attaches an IDL based C++11 object (derived from Bse::Object). Back pointers are installed to link the two together and to convert back and forth. This way, features can be migrated incrementally from BseObject to Bse::Object and derived types. A template method as<>() is provided to convert between object types. This scheme requires no particular order and allows attaching C++ objects to abstract or concrete GObject types, whatever is easier for migration efforts.
* **[ONGOING]** Create as much IDL C++ shims as possible, to ease procedure migrations.
* **[ONGOING]** Move procedures from PROC files into interface methods in IDL files. Initially procedures were moved into sfidl files, but now procedures are being moved to AIDA IDL. Note that during build time, bsehack.idl is generated, that already contains IDL formatting for all procedures.
* Next, signals should be migrated, as signals are the the main tie to libgobject.so.
* Once all signals are converted, all g_signal_ related code is eliminated from BSE and SFI, bse_proxy_connect can be removed.
* Also, migrate all properties from GObject based to AIDA IDL, this might mean more Rapicorn AIDA support for records, sequences and Any than currently available.
* Develop a suitable replacement for the `sfidl --plugin` mode, which generates very customized code to implement BSE objects and related synthesis engine modules.
* Last, all g_object_ and g_type_ calls can be replaced.

Plugin-Merging
--------------
**[STARTED]** The BSE plugin API has almost no uses, but plugin loading causes major slowdowns in the startup phase. As a consequence, all plugins shipped together with BSE should be linked into the same ELF library.

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

Bstrecords-Migration
--------------------
**[COMPLETED]** Beast used to generate some structures from bstrecords.idl (sfidl), these are all migrated into bstapi.idl (ADIA IDL).

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
  ServerIface *server_iface = server->as<ServerIface*>();
  ServerIfaceP server_iface_p = shared_ptr_cast<ServerIface> (server_iface);
  // In beast-gtk/
  ServerIface *server_iface;
  ObjectIfaceP server_iface->from_proxy (proxyid);
  SfiProxy proxy = server_iface->proxy_id();
