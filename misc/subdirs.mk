# Forward make $(MAKECMDGOALS) to the project root, add subdir prefix

# Figure topdir from ../misc/subdirs.mk/..
misc/subdirs_mk/topdir ::= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

# extract subdir name from $(misc/subdirs_mk/topdir)/sub/dir as sub-dir
misc/subdirs_mk/subdir_prefix ::= $(patsubst $(misc/subdirs_mk/topdir)/%, %, $(abspath .))

MAKECMDGOALS ?= all
${MAKECMDGOALS}:
	@exec $(MAKE) -C $(misc/subdirs_mk/topdir) $(misc/subdirs_mk/subdir_prefix)/$@
