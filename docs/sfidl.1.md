% SFIDL(1) Beast-@BUILDID@ | Beast/Bse Manual
%
% @FILE_REVISION@

# NAME
sfidl - SFI IDL Compiler (Beast internal)

# SYNOPSIS

**sfidl** \[*OPTIONS*\] *input.idl*

# DESCRIPTION

**Sfidl** generates glue code for Bse objects and plugins from interface definition language files.

# OPTIONS

**--help** \[*BINDING*\]
:   Print general usage information. Or, if *BINDING* was specified, print usage information for this language binding.

**--version**
:   Print program version.

**-I** *DIRECTORY*
:   Add *DIRECTORY* to include path.

**--print-include-path**
:   Print include path.

**--nostdinc**
:   Prevents standard include path from being used.

### Language bindings:

**--client-c**
:   Generate C client language binding.

**--client-c**
:   Generate C core language binding.

**--host-c**
:   Generate C host language binding.

**--client-cxx**
:   Generate C++ client language binding.

**--core-cxx**
:   Generate C++ core language binding.

**--plugin**
:   Generate C++ plugin language binding.

**--list-types**
:   Print all types defined in the idlfile. This option is used only for BSE internally to ease transition from C to C++ types.

### Language binding options:

**--header**
:   Generate header file, this is the default.

**--source**
:   Generate source file.

**--prefix** *prefix*
:   C host/client language binding option, sets the prefix for C functions. The prefix ensures that no symbol clashes will occur between different programs/libraries which are using a binding, so it is important to set it to something unique to your application/library.

**--init** *name*
:   Set the name of the init function for C host/core bindings.

**--namespace** *namespace*
:   C++ client language binding, sets the namespace to use for the code. The namespace ensures that no symbol clashes will occur between different programs/libraries which are using a binding, so it is important to set it to something unique to your application/library.

**--lower**
:   Select lower case identifiers in the C++ client language binding (create\_midi\_synth), this is the default.

**--mixed**
:   Select mixed case identifiers in the C++ client language binding (createMidiSynth).

# SEE ALSO

[**BSE Reference**](https://testbit.eu/pub/docs/beast/latest/namespaceBse.html){.external},
[**Sfidl Manual**](https://testbit.eu/wiki/Sfidl){.external}
