# Forward make $(MAKECMDGOALS) to the project root, add subdir prefix

# Figure topdir from ../misc/subdirs.mk/..
misc/subdirs_mk/topdir ::= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

# extract subdir name from $(misc/subdirs_mk/topdir)/sub/dir as sub-dir
misc/subdirs_mk/subdir_prefix ::= $(patsubst $(misc/subdirs_mk/topdir)/%,%, $(abspath .))

# Read default config, in particular $(builddir)
-include $(misc/subdirs_mk/topdir)/config-defaults.mk
builddir ?= $(O)
builddir ?= out

# Fallback to 'make all' in topdir if MAKECMDGOALS is empty
MAKECMDGOALS ?= all

# Prefix filenames in MAKECMDGOALS
sed_subdirprefix ::= 's! \([^./ ]\+\.[^/ ]\+\)! $(builddir)/$(misc/subdirs_mk/subdir_prefix)/\1!g'
PREFIXED_MAKECMDGOALS != echo " $(MAKECMDGOALS)" | sed $(sed_subdirprefix)

# Trigger a single recursive $(MAKE) call
.PHONY: $(MAKECMDGOALS) .._todir_MAKECMDGOALS
$(MAKECMDGOALS): .._todir_MAKECMDGOALS ; @:
.._todir_MAKECMDGOALS:
	@exec $(MAKE) -C $(misc/subdirs_mk/topdir) $(PREFIXED_MAKECMDGOALS)
