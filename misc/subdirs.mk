# forward make $(MAKECMDGOALS) to the project root

# figure topdir from ../misc/subdirs.mk/..
misc_subdirs_mk_topdir ::= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

# extract subdir name from $(misc_subdirs_mk_topdir)/sub/dir as sub-dir
misc_subdirs_mk_subdir_prefix ::= $(subst /,-, $(patsubst $(misc_subdirs_mk_topdir)/%, %, $(abspath .)))

MAKECMDGOALS ?= all
${MAKECMDGOALS}:
	@$(MAKE) -C $(misc_subdirs_mk_topdir) $(misc_subdirs_mk_subdir_prefix)-$@
