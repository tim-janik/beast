# Background & Discussions

Space for notes about the synthesis engine, undo/redo stacks and other technical bits.


## Threading Overview

On a high level, Beast constitutes the front-end to the libbse sound engine.

The BSE sound engine spawns a separate (main) thread separate from the UI for all sound related management
and a number of digital signal processing (DSP) sub threads to facilitate parallelized synthesis.

Invocations of the libbse API are automatically relayed to the BSE thread via an IPC mechanism.

The main BSE thread is where the API facing BSE objects live. For actual sound synthesis, these objects
spawn synthesis engine modules that process audio buffers from within the DSP threads.
The number of parallel DSP threads usually corresponds to the number of cores available to libbse.

Additionally, libbse spawns a separate sequencer thread that generates `note-on`, `note-off` and related
commands from the notes, parts and tracks contained in a project. The synchronization required by the
sequencer thread with the main thread and the DSP threads is fairly complicated, which is why this is
planned to be merged into the DSP threads at some point.


## AIDA - Abstract Interface Definition Adapter

Aida is used to communicate with a regular API between threads and to cross programming language barriers.
At the heart of Aida is the IDL file which defines Enums, Records, Sequences and interfaces.
Interface methods can be called from one thread and executed on class instances living in other threads.
Additionally, callback execution can be scheduled across thread boundaries by attaching them to
Event emissions of an instance living in another thread.

Within Beast, Aida is used to bridge between the UI logic (the "Beast-GUI" or "EBeast-module" threads)
and the sound synthesis control logic (the "BseMain" thread).

For an IDL interface `Foo`, Aida generates a "client-side" C++ class `FooHandle` and a "server-side" C++
class `FooIface`.
The `Iface` classes contain pure virtual method definitions that need to be implemented by the server-side
logic.
The `Handle` classes contain corresponding method wrappers that call the `Iface` methods across thread
boundaries.
The `Handle` methods are implemented non-virtual to reduce the risk of ABI breakage for use cases where only
the client-side API is exposed.

### IDL - The interface definition files

#### Namespace {-}

At the outermost scope, an IDL file contains a namespace definition.
Interfaces, enums, records and sequences all need to be defined with namespaces in IDL files.

#### Primitive Types {-}

The primitive types supported in IDL files are enums, bool, integer, float and string types.
The supported compound types are sequences - a typed variable length array,
records - a set of named and typed variables with meta data,
Any - a variant type, and inheritable interfaces.

#### Enum {-}

Enum values are defined with integer values and allow `VALUE = Enum (N, strings...)` syntax to add meta data to enum values.

#### Interface {-}

An interface can contain methods and property definitions and inherit from other interfaces.
Properties defined on an interface are largely sugar for generating two idiomatic getter and setter access methods,
but the set of properties and associated meta data of an interface can be accessed programmatically through
the `__access__()` method.
The `Bse::Object` implementation makes use of this facility to implement the dynamic property API
`set_prop()`, `get_prop()`, `find_prop()`, `list_props()`.
The syntax for property definitions with meta data is `TYPE property_name = Auxillary (strings...);`, the
`Auxillary()` constructor can take various forms, but ultimately just constructs a list of UTF-8 `key=value`
strings that can be retrieved at runtime.

#### Any {-}

An Any is a variant type that can contain any of the other primitive types.
It is most useful to represent dynamically typed values in C++ which are common in languages like Python or Javascript.
If an Any is set to store an IDL Enum value, it also records the Enum type name, to allow later re-identification.
In general, Any should only be needed for languages that do not have dynamic variable types (like C++),
and should not normally need to be exposed in dynamic languages (like Python, Javascript).
E.g. converting an enum property value stored in an Any to a native type in a dynamic language, a pair could be
returned, like `(1, "Bse::FooEnum")`.

