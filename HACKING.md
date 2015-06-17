% BEAST/BSE HACKING

The Beast + BSE code base origins date back to the 90ties, so hacking on it
requires some knowledge about ancient aspects and mixings with newer
technologies. The following gives a high-level overview of the pitfalls
involved, this file can be discussed at beast@gnome.org.

Migrations
==========

The code base is currently undergoing several migrations and new developments:

IDL-Migration
-------------
**[STARTED]** (For BSE) Move from sfidl (and the old PROC files) to AIDA IDL (aidacc, distributed with Rapicorn). BSE already contains an AIDA style IDL file with C++11 objects that can be used in Beast. Eventually, all sfidl files need to be ported to AIDA IDL.

PROC-Migration
--------------
**[STARTED]** Move from PROC files to IDL files. Initially procedures were moved into sfidl files, but now procedures need to be moved to AIDA IDL. Note that during build time, bsehack.idl is generated, that already contains IDL formatting for all procedures.

Enum-Migration
--------------
**[STARTED]** Beast enums are being migrated from scanned C headers into IDL files. Note that some have been migrated into bstapi.idl (AIDA IDL) while others are still remaining in bstrecords.idl (sfidl).

Bstrecords-Migration
--------------------
**[STARTED]** Beast contains some generated structures and enums in bstrecords.idl (sfidl), these all need to be moved into bstapi.idl (ADIA IDL).

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

C++11-Compilation
-----------------
**[COMPLETE]** Turn all .c files into .cc files so C++11 can be used everywhere.
