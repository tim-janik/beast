# BEAST & BSE
include $(top_srcdir)/Makefile.decl

# need -I$(top_srcdir) for <bse/bsedefs.hh>
# need -I$(top_builddir) for <sfi/sficonfig.h>
# need -I$(srcdir) for "bseserver.hh" in .genprc.cc
# need -I. (builddir) for "bsebasics.genidl.hh" in bsebasics.cc
AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ $(patsubst %, -DG_LOG_DOMAIN=\"BSE\" -DBSE_COMPILATION, $(filter $(<F), $(bse_sources) $(bse_sources))) 
AM_CXXFLAGS  += $(BSE_CFLAGS) $(RAPICORN_CFLAGS) -DRAPICORN_CONVENIENCE -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS
AIDACC_VDEBUG = $(AIDACC) $(if $(findstring 1, $(V)), --aida-debug)

# == Pyx ==
AIDASTDINC = $(shell ${AIDACC} -x PyxxStub.py -G --print-include-path /dev/null)
# Python 'Bse' module build via Cython
bseidlapi.pyx: ../bseapi.idl
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x PyxxStub.py $< -o $@ -G strip-path=$(abs_top_srcdir)/
# pythondir = ${prefix}/lib/pythonX.Y/site-packages
pymodulesdir          = ${bselibdir}/pymodules
# Pyx Bse module
CLEANFILES           += bseidlapi.pyx

# call python with uninstalled or fully installed modules
FPYTHON      = PYTHONPATH="$$PYTHONPATH:${pymodulesdir}" $(PYTHON)
UPYTHON      = PYTHONPATH="$(abs_builddir)/${objdir}:$$PYTHONPATH" $(PYTHON)
# Loading newly built Python modules requires accesss to libtool objdir
objdir       = .libs
python-call-info:
	@echo '${FPYTHON} -ic "import Bse"'
	@echo '${UPYTHON} -ic "import Bse"'
