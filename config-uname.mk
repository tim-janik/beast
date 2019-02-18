# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Compiler & uname ==
# Example: uname_S=Linux uname_M=x86_64 uname_R=2.6.17-44-kvm
uname_S		::= $(shell uname -s 2>/dev/null || echo None)
uname_M		::= $(shell uname -m 2>/dev/null || echo None)
uname_R		::= $(shell uname -r 2>/dev/null || echo None)
CXXKIND		 != case $$($(CXX) --version 2>&1) in *"Free Software Foundation"*) echo gcc;; *clang*) echo clang;; *) echo UNKNOWN;; esac
HAVE_GCC	::= $(if $(findstring $(CXXKIND), gcc),1,)
HAVE_CLANG	::= $(if $(findstring $(CXXKIND), clang),1,)
ifeq ($(HAVE_GCC)$(HAVE_CLANG),)		# do we HAVE_ *any* recognized compiler?
$(error Compiler '$(CXX)' not recognized, version identifier: $(CXXKIND))
endif
CCACHE		 ?= $(if $(CCACHE_DIR), ccache)

# == C/CXX/LD Flags ==
COMMONFLAGS	::= -fno-strict-overflow -fno-strict-aliasing # sane C / C++
COMMONFLAGS	 += -Wall -Wdeprecated -Werror=format-security -Wredundant-decls -Wpointer-arith -Wmissing-declarations
COMMONFLAGS	 += -Werror=incompatible-pointer-types -Werror-implicit-function-declaration
#COMMONFLAGS	 += -Wdate-time -Wconversion -Wshadow
CONLYFLAGS	::= -Wmissing-prototypes -Wnested-externs -Wno-pointer-sign
CXXONLYFLAGS	::= -Woverloaded-virtual -Wsign-promo
#CXXONLYFLAGS	 += -Wnon-virtual-dtor -Wempty-body -Wignored-qualifiers -Wunreachable-code -Wtype-limits
OPTIMIZE	::= -O3 -funroll-loops -ftree-vectorize
LDOPTIMIZE	::= -O1 -Wl,--hash-style=both -Wl,--compress-debug-sections=zlib
ifdef HAVE_CLANG  # clang++
  COMMONFLAGS	 += -Wno-tautological-compare -Wno-constant-logical-operand
  #COMMONFLAGS	 += -Wno-unused-command-line-argument
endif
ifdef HAVE_GCC    # g++
  COMMONFLAGS	 += -fno-delete-null-pointer-checks
  OPTIMIZE	 += -fdevirtualize-speculatively -ftracer -ftree-loop-distribution -ftree-loop-ivcanon -ftree-loop-im
  ifdef CCACHE
    COMMONFLAGS	 += -fdiagnostics-color=auto
  endif
endif
ifeq ($(uname_S),x86_64)
  COMMONFLAGS	 += -mcx16			# for CMPXCHG16B, in AMD64 since 2005
  OPTIMIZE	 += -minline-all-stringops
  OPTIMIZE	 += -mmmx -msse -msse2		# Intel since 2001, AMD since 2003
  OPTIMIZE	 += -msse3			# Intel since 2004, AMD since 2007
  OPTIMIZE	 += -mssse3			# Intel since 2006, AMD since 2011
  #OPTIMIZE	 += -msse4a			# AMD since 2007
  #OPTIMIZE	 += -msse4.1 -msse4.2		# Intel since 2008, AMD since 2011
  #OPTIMIZE	 += -mavx			# Intel since 2011, AMD since 2011
  #OPTIMIZE	 += -mavx2			# Intel since 2013, AMD since 2015
endif
CFLAGS		::= $(strip $(COMMONFLAGS) $(CONLYFLAGS) $(OPTIMIZE)) $(CFLAGS)
CXXFLAGS	::= $(strip $(COMMONFLAGS) $(CXXONLYFLAGS) $(OPTIMIZE)) $(CXXFLAGS)
LDFLAGS		::= $(strip $(LDOPTIMIZE)) -Wl,-export-dynamic -Wl,--as-needed -Wl,--no-undefined -Wl,-Bsymbolic-functions $(LDFLAGS)

# == LINKER ==
# $(call LINKER, EXECUTABLE, OBJECTS, DEPS, LIBS, RELPATHS)
define LINKER
$1: $2	$3
	$$(QECHO) LD $$@
	$$Q $$(CXX) $$(CXXSTD) -fPIC -o $$@ $$(LDFLAGS) $$($$@.LDFLAGS) $2 $4 $(foreach P, $5, -Wl$(,)-rpath='$$$$ORIGIN/$P' -Wl$(,)-L'$$(@D)/$P')
endef
