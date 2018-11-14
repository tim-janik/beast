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
