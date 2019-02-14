# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Escapes ==
,::=,

# == $Q $V ==
Q       ::= $(if $(findstring 1, $(V)),, @)
QSKIP   ::= $(if $(findstring s,$(MAKEFLAGS)),: )
QSTDOUT ::= $(if $(findstring 1, $(V)),, 1>/dev/null)
QSTDERR ::= $(if $(findstring 1, $(V)),, 2>/dev/null)
QGEN	  = @$(QSKIP)echo '  GEN     ' $@
QECHO	  = @QECHO() { Q1="$$1"; shift; QR="$$*"; QOUT=$$(printf '  %-8s ' "$$Q1" ; echo "$$QR") && $(QSKIP) echo "$$QOUT"; }; QECHO
QDIE	  = bash -c 'echo "  ERROR    $@: $$@" >&2 ; exit 127' _

# == DOTGIT ==
# Toplevel .git/ directory or empty if this is not a git repository
DOTGIT ::= $(abspath $(shell git rev-parse --git-dir 2>/dev/null))
# Dependencies that are updated with each Git commit
GITCOMMITDEPS ::= $(DOTGIT:%=%/logs/HEAD)

# == LINKER ==
# $(call LINKER, EXECUTABLE, OBJECTS, DEPS, LIBS, RELPATHS)
define LINKER
$1: $2	$3
	$$(QECHO) LD $$@
	$$Q $$(CXX) $$(CXXSTD) -fPIC -o $$@ $$(LDFLAGS) $$($$@.LDFLAGS) $2 $4 $(foreach P, $5, -Wl$(,)-rpath='$$$$ORIGIN/$P' -Wl$(,)-L'$$(@D)/$P')
endef
