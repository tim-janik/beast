


# ignore apostrophes
: apostrophe
s@^\(\([^";]\|"\([^"\]\|\\.\)*"\)*\)'@\1.@
t apostrophe ; # check/fix next apostrophe on same line

# extract gettext function calls, ignore strings and comments
s/^\(\([^";]\|"\([^"\]\|\\.\)*"\)*\)(\([NUQ]\?\)_/__INTLFIX__: \1\2_(/
: intlfunc
s/^\(\([^";]\|"\([^"\]\|\\.\)*"\)*\)(\([NUQ]\?\)_/\1\2_(/
t intlfunc ; # next intlfunc on same line (omitting marker)

# extract comments, ignore strings
s@^\(\([^";]\|"\([^"\]\|\\.\)*"\)*\);\+@\1//@
