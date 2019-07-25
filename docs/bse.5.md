% BSE(5) Beast-@BUILDVERSION@ | Beast/Bse Manual
%
% @FILE_REVISION@

# NAME
BSE - Better Sound Engine File Format

# SYNOPSIS
*filename*.**bse**

# DESCRIPTION

The **Bse** file format is used by the **Bse** library and dependent programs to save
projects, songs, instruments and sample collections.

# FORMAT

Bse files start out with a special magic string `"; BseProject\n"` and then contain nested
expressions in scheme syntax using the ASCII charset.
Binary data may be appended to a `*.bse` file if it is separated from the preceeding
ASCII text by one or more literal NUL characters (`'\0'`).
This mechanism is used to store arbitrary binary data like *WAV* or *Ogg/Vorbis* files
in Bse projects, while keeping the actual content user editable - text editors that
preserve binary sections have to be used, such as **vi(1)** or **emacs(1)**.

# COMPATIBILITY

The exact format and sets of objects and properties used in a Bse file depend highly
on the library version that was used to save the file. Compatibility functions are supplied
by the library itself, so old Bse files can be converted when the file is loaded.
To enable this mechanism, all Bse files contain a `"bse-version"` directive which
indicates the  Bse file format version of the supplied content.

# SEE ALSO

[**beast(1)**](beast.1.html),
[**bsewavetool(1)**](bsewavetool.1.html),
[**BSE Reference**](https://testbit.eu/pub/docs/beast/latest/namespaceBse.html){.external}
