# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == conftest_header_symbol ==
conftest_header_symbol =  { { : \
  && echo '\#include <$1>' \
  && echo 'int main() {' \
  && echo '\#ifndef $2' \
  && echo '($2);' \
  && echo '\#endif' \
  && echo 'return 0; }' ; } > "conftest_header_symbol-$1.c" \
  && { V=$$(echo '$2' | sed -e 's/[^a-z0-9A-Z]/_/g' -e 's/\(.*\)/\U\1/') \
       && $(CXX) -fpermissive "conftest_header_symbol-$1.c" -o "conftest_header_symbol-$1" 2>/dev/null \
       && echo "\#define HAVE_$$V			1 // $1" \
       || echo "\#undef  HAVE_$$V" ; } \
  && rm -f "conftest_header_symbol-$1.c" "conftest_header_symbol-$1" ; }

# == conftest_lib & conftest_require_lib ==
# $(call conftest_lib, header, symbol, lib) -> $CONFTEST
conftest_lib = { { echo '\#include <$(strip $1)>' \
                && echo 'int main() { return 0 == (int) (long) (void*) &($2); }' ; } > "conftest_lib-$$$$.cc" \
		&& { CONFTEST_LOG=$$($(CXX) -fpermissive "conftest_lib-$$$$.cc" -o "conftest_lib-$$$$" $(LDFLAGS) $3 2>&1) \
		     && CONFTEST=true || CONFTEST=false ; } \
		&& rm -f "conftest_lib-$$$$.cc" "conftest_lib-$$$$" ; }
conftest_lib.makefile ::= $(lastword $(MAKEFILE_LIST))
# $(call conftest_require_lib, header, symbol, lib) -> errors if $CONFTEST != true
conftest_require_lib = { $(call conftest_lib,$1,$2,$3) && $$CONFTEST \
	|| { echo "$$CONFTEST_LOG" | sed 's/^/> /'; \
	     echo '$(conftest_lib.makefile):' "ERROR: Failed to link with '$$LIBMAD_LIBS' against symbol:" '$2'; \
	     exit 7; } >&2 ; }

# == config-cache.mk ==
-include $>/config-cache.mk
$>/config-cache.mk: config-checks.mk version.sh $(GITCOMMITDEPS) | $>/.
	$(QGEN)
	$(Q) echo '# make $@'					> $@.tmp
	$(Q) GLIB_CFLAGS=$$(pkg-config --cflags glib-2.0) \
	  && echo "GLIB_CFLAGS ::= $$GLIB_CFLAGS"		>>$@.tmp
	$(Q) GLIB_LIBS=$$(pkg-config --libs glib-2.0) \
	  && echo "GLIB_LIBS ::= $$GLIB_LIBS"			>>$@.tmp
	$(Q) LIBMAD_LIBS='-lmad -lm' \
	  && echo "LIBMAD_LIBS ::= $$LIBMAD_LIBS"		>>$@.tmp \
	  && $(call conftest_require_lib, mad.h, mad_stream_errorstr, $$LIBMAD_LIBS)
	$(Q) V=$$(./version.sh -l) \
	&& echo "BUILDID ::= $$V"			>>$@.tmp
	$(Q) V=$$(./version.sh -d) \
	&& echo "VERSION_DATE ::= $$V"			>>$@.tmp
	$(Q) set -Eeuo pipefail \
	&& ./version.sh -s | sed 's/[^0-9]/ /g'		> $@.tmpv \
	&& read MAJOR MINOR MICRO REST			< $@.tmpv \
	&& rm -f $@.tmpv \
	&& echo "VERSION_MAJOR ::= $$MAJOR"		>>$@.tmp \
	&& echo "VERSION_MINOR ::= $$MINOR"		>>$@.tmp \
	&& echo "VERSION_MICRO ::= $$MICRO"		>>$@.tmp \
	&& echo "VERSION_REST  ::= $$REST"		>>$@.tmp
	$(Q) mv $@.tmp $@
CLEANFILES += $>/config-cache.mk
