# Historic Artefacts

Historic notes and writeups that are likely outdated or obsolete.

## Ancient type system description

Description of the BSE types and its type system around 1999, before the migration to GObject.

### BSE Structures

#### BseChunk

A BseChunk contains header informations for sample value blocks passed
around during sound processing, and a pointer to the associated sample
data itself. The pure sample data blocks are referred to as "Hunk"s.
A hunk's size is determined by the size of each sample value:
`sizeof (BseSampleValue)`, the number of tracks contained in a hunk (for instance
a stereo hunk contains 2 tracks) and a global variable that designates
the block (hunk's) size for the current sound processing run:
`BSE_TRACK_LENGTH` (`bse_globals->track_length`).
The sample values themselves are layed out interleaved within the hunk,
e.g. a 2 track stereo hunk with a `hunk_length` of 16 would contain:

~~~yaml
L - left track (1) value
R - right track (2) value
LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLR
~~~

Each chunk is associated with a certain output channel of a specific source,
and applies to a distinct time frame. The actuall time frame length is
determined through `BSE_TRACK_LENGTH` and the current mixing frequency
`BSE_MIX_FREQ`.

#### BseParam

A parameter that can be get/set on an object.
Each parameter has a parameter specification associated with it.

#### BseParamSpec\*

A parameter specification similar to an object's class or to
procedure classes, but for a given parameter type multiple
parameter specifications may exist (e.g. to implement integer
parameters with different ranges).
Thus the type system actually implements parameters as "unclassed" types.

#### BseObject

BseObject implements basic structure operations and provides the
necessary cruft for objects to have classes.

#### BseObjectClass

A class associated with each objects instance.
A class is referenced by its object instances.
introduces per-object parameters.

#### BseProcedure

A BseProcedure is a a special classed-only (i.e. not-object) type that
holds parameter specifications in it's calss structure and can be called.
Work is in progress to map certain procedures to certain object types, so
they can be thought of as special object-methods to do perform certain
object specific modifications.

#### BsePlugin

A plugin handle that holds information about existing plugins, such as
where the actuall plugin's implementation is stored on disk and what
types of objects are implemented by the plugin.

Plugin can provide:

- object and class implementations derived from BseEffect or BseSource
- procedure classes
- enum and flags definitions (currently lacks implementation)
- sample/song file loading and saving routines (currently lacks
  implementation)

#### BseType

type ids are provided for
objects & classes,
object interfaces,
parameters,
procedures,
enum & flags definitions.


### The BSE type system

BSE features a single inheritance object hierarchy with multiple interfaces.
It therefore contains a type system that registers several object and
non-object types. The type system features unclassed types (parameters),
classed types (enumerations and procedures) and object types (also classed).
The type system implemented by BSE has several analogies to the Gtk+ type
system and several type system related code portions have been traded between
the two projects. However, the BSE type system got furtherly enhanced (read:
complicated ;) in some regards by featuring dynamic types to allow certain
implementations to reside in dynamic plugins.

Usually, all types get registered with their name, description and inheritance
information upon startup and for static types the implementation related type
information is also stored right away. This information cannot be stored right
away for dynamic types since the implementation related type information
contains function pointers that will not remaind valid across the life time
of plugins (a plugin being loaded, unloaded and the reloaded again will most
certainly end up in a different memory location than before).
When class or object instances of a dynamic type need to be created, the
corresponding plugin will be loaded into memory and the required implementation
related type information will be retrieved.

Upon destruction of the instance, the type info is released again and the
plugin will be unloaded.
So for dynamic types, the only things that stays persistent across plugin life
times are the type's name, description, inheritance information and type id,
as well as additional information about what plugin this type is implemented
by.

Parameter types are unclassed and do not feature inheritance, they are merely
variable type ids to distinguish between e.g. strings and integers.
Enumeration types are classed, with flags being a certain kind of enumerations
in that their values only feature certain bits. For enumertaions, a flat
hierarchy is supported, where every enumeration or flags type derives from
`BSE_TYPE_ENUM` or `BSE_TYPE_FLAGS` respectively.

Object types are classed as well, and may exist in multiple instances per type
id. They feature single inheritance and multiple interfaces, where an interface
type denotes a certain alternate class-based vtable that is required to adress
similar object methods from different heirarchy branches with the same API.
Type inheritance can be examined by testing their `is_a` relationships, e.g.
for a type `parent` and a type `child` deriving from `parent`, the following
holds true: `child is_a parent`, while the reverse: `parent is_a child`
would fail, because `child` inherits all of `parent`'s facilities, but not
the other way around.

Whether a certain object type implements a specific interface, can be tested
through the `conforms_to` relationship, e.g. for an interface `I` requirering
a method implementation `foo` and a base type `B`, not implementing `foo` with
it's two child types `C` and `D`, both carrying different imlpementations of
`foo`, the following applies:

- `B conforms_to I`: `FALSE`
- `C conforms_to I`: `TRUE`
- `D conforms_to I`: `TRUE`

We will now outline the several steps that are taken by the type system to
create/destroy classes and objects. This is actually the guide line for the
type system's implementation, so feel free to skip this over ;)
In the below, unreferencing of certain objects may automatically cause their
destruction, info structure retrival and releasing may (for dynamic types)
accompany plugin loading/unloading.

#### initialization

- all static type ids + type infos get registered
- all dynamic type ids get registered
  (implies: all interface type ids are registered)
- static type infos are persistant

#### class creation

- type info is enforced
- parent class is referenced
- class gets allocated and initialized

#### object creation

- class is referenced
- object is allocated and initialized

#### object destruction

- object gets deallocated (destruction has already been handled by the object system)
- class gets unreferenced

#### class destruction

- class gets destructed and deallocated
- type info is released
- parent class is unreferenced

#### interface registration

- interface id gets embeded in the host type's interface entry list
- the interface is registered for all children as well

#### interface class creation

- class is given
- class is referenced
- interface info is enforced
- interface is referenced
- interface entry info is enforced
- interface class is allocated and initialized (and propagated to children)

#### interface class destruction

- interface class gets destructed and deallocated (and propagated to children)
- interface entry info is released
- interface gets unreferenced
- interface info is released
- class gets unreferenced

#### BSE Sources

Each source can have an (conceptually) infinite amount of input channels and
serve a source network (BseSNet) with multiple output channels.
Per source, a list of input channels is kept (source pointer and output
channel id, along with an associated history hint). Sources also keep back
links to other sources using its output channels, this is required to maintain
automated history updates and purging.

#### BSE Source Networks

A BSE Source Network is an accumulation of BseSources that are interconnected
to controll sample data flow.
The net as a whole is maintained by a BseSNet object which keeps
references of all sources contained in the net and provides means
to store/retrieve undo/redo information for the whole net and implements
the corresponding requirements to save and load the complete net at a
certain configuration.

Implementation details:
- a BseSNet keeps a list and references of all sources contained
  in the net.
- each source can have a (conceptually) infinite amount of input
  channels and serve the net with multiple output channels.
- per source, a list of input channels is kept (source pointer plus
  output channel id, along with an associated history hint).
- sources also keep back links to sources using its output channels,
  this is required to maintain automated history updates.


## BSE category description

Description of the BSE categories around 2003, before the removal of procedures.

### BSE Categories

The purpose of BSE Categories is to provide a mapping of functionality
provided by BSE and its plugins to GUI frontends. Categories are used to
provide a hirarchical structure, to group certain services and to aid
the user in finding required functionality.

#### Object Categories

BSE objects and procedures are "categorized" to easily integrate them
into menu or similar operation structures in GUIs.

Objects that are derived from the BseSource class with the main intend
to be used as synthesis modules in a synthesis network are categorized
under the `/Modules/` toplevel category, ordered by operational
categories.

#### Core Procedures

Procedures provided by the BSE core are categorized under a
subcategory of the `/Methods/` toplevel category.
The object type a procedure operates on is appended to
the toplevel category to form a category name starting with
`/Methods/<ObjectType>/`.
Procedures implementing object methods need to take the
object as first argument and follow a special naming convention
(for the procedure name). To construct the procedure type name,
the object type and method named are concatenated to form
`<ObjectType>+<MethodName>`. This ensure proper namespacing
of method names to avoid clashes with equally named methods
of distinct object types.
Some special internal procedures may also be entered under
the `/Proc/` toplevel category.

#### Utility Procedures

Procedures implementing utility functionality, usually implemented
as scripts, are entered into one of the below listed categories.
Depending on the category, some procedure arguments are automatically
provided by the GUI, the name and type of automatic arguments are
listed with the category in the following table:

Category      | Automatic arguments as (name:type) pairs
--------------+---------------------------------------------------------------
/Project/     | project:BseProject
/SNet/        | snet:BseSNet
/Song/        | song:BseSong
/Part/        | part:BsePart
/CSynth/      | csynth:BseCSynth
/WaveRepo/    | wave_repo:BseWaveRepo
/Wave/        | wave:BseWave

Table: Set of toplevel categories used by libbse.

The set of automatic arguments provided upon invocation is not restricted
by te above lists, so caution should be taken when naming multiple arguments
to a procedure `auto_*`.

#### Aliasing/Mapping

A certain BSE type (procedure or object) may be entered under several
categories simultaneously, thus allowing multiple categories to
refer (alias) to the same type (functionality).


#### Examples

&nbsp;

Category                        | TypeName             | Comment
--------------------------------+----------------------+----------------------
/Methods/BseProject/File/Store  | BseProject+store-bse | BseProject procedure
/Modules/Spatial/Balance        | BseBalance           | synthesis module
/SNet Utility/Utils/Grid Align  | modules2grid         | scheme script

Table: Category examples from the Beast source code.


## BseHeart - Synthesis Loop, Dec 1999

BSE's heart is a unique globally existing object of type BseHeart which
takes care of glueing the PCM devices and the internal network structure
together and handle syncronization issues.

BseHeart brings the internal network to work by incrementing a global
BSE index variable, cycling all active BseSources to that new index,
collecting their output chunks and writing them into the output PCM devices.
As of now there are still some unsolved syncronization problems, such
as having multiple PCM devices that run at different speeds. While this
could be solved for multiples/fractions of their mixing frequencies,
such as one device (card) running at 44100Hz and another one at 22050Hz,
problems still remain for slight alterations in their oscilaltors if
a device e.g. runs at 44099Hz (such as the es1371 from newer PCI SB
cards).

For the time being, we do not handle different output/input frequencies
at all and use our own syncronization within BSE, bound to GLib's main
loop mechanism by BseHeart attaching a GSource.

To support user defined latencies, PCM Devices implement their own output
buffer queue which is used for temporary storage of output buffers if
the specified latency forces a delay greater than what can be achived through
the device driver's queue. The devices also maintain an input queue for
recorded data to avoid overruns in the PCM drivers, ideally these queues
will immediatedly be emptied again because output is also required.

*FIXME: A check may be necessary to catch indefinitely growing input
queues due to input->output frequency differences. We will also
need to catch input underruns here, though handling that is somwhat
easier in that we can simply slide in a zero pad chunk there.*

The fundamental BSE cycling (block-wise processing of all currently active
sources) is bound to the PCM devices output requirements, according to the
current latency setting.
Integrated into GLib's main loop, we perform the following steps:

prepare():
:	We walk all input devices until one is found that reports requirement
	to process incoming data.
	We walk all output devices until one is found that reports requirement
	to process outgoing data or needs refilling of its output queue,
	according to the latency settiongs.
	If none is found, the devices report an estimated timeout value for
	how long it takes until they need to process further data.
	*[Implementation note: as an optimization, we walk all PCM devices in
	one run and leave the distinction between input and output up to them]*
	The minimum of the reported timeout values is used for GLib's main
	loop to determine when to check for dispatching again.

check():
:	We simply do the same thing as in prepare() to figure whether
	some PCM devices need processing and if necessary request dispatching.

dispatch():
: We walk the output devices in order to flush remaining queued chunks,
	and collect reports on whether they need new input blocks according to
	the latency setting.
	We walk the input devices so they can process incoming data (queue it
	up in chunks).
	*[Implementation note: as an optimization, we currently do these two
	walks in one step, leaving the details to the PCM devices]*
	Now here's the tweak: if *any* output device reports the need for
	further input data, we perform a complete BSE cycle.
	If no output devices require further input data, we walk the input
	devices and check whether they have more than one chunk in their
	queue (and if necessary, fix that) this is required to catch overruns
	and constrain output latency.

cycle():
:	For input devices that have empty queues, we report underruns and pad
	their queues with zero chunks, i.e. we ensure all input PCM devices
	have at least one usable chunk in their input queue.
	We increment the BseIndex of all currently active BseSources, calc
	their output chunks and queue them up for all output sources connected
	to them.
	We walk the input devices and remove exactly one chunk from
	their input queues.
