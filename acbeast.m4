dnl Setup useful string and variable macros.


dnl MC_IF_VAR_EQ(environment-variable, value [, equals-action] [, else-action])
AC_DEFUN(MC_IF_VAR_EQ,[
	case "$[$1]" in
	"[$2]"[)]
		[$3]
		;;
	*[)]
		[$4]
		;;
	esac
])


dnl MC_STR_CONTAINS(src-string, sub-string [, contains-action] [, else-action])
AC_DEFUN(MC_STR_CONTAINS,[
	case "[$1]" in
	*"[$2]"*[)]
		[$3]
		;;
	*[)]
		[$4]
		;;
	esac
])

dnl MC_EVAR_ADD(environment-variable, check-string, add-string)
AC_DEFUN(MC_EVAR_ADD,[
	MC_STR_CONTAINS($[$1], [$2], [$1]="$[$1]", [$1]="$[$1] [$3]")
])
dnl MC_EVAR_SUPPLEMENT(environment-variable, check-string, add-string)
AC_DEFUN(MC_EVAR_SUPPLEMENT,[
	MC_STR_CONTAINS($[$1], [$2], [$1]="$[$1] [$3]", [$1]="$[$1]")
])


dnl MC_CHECK_VERSION(given-version, required-version [, match-action] [, else-action])
AC_DEFUN(MC_CHECK_VERSION,[
[eval `echo "$1:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g' \
 -e 's/\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):.*/ac_v1=\1 ac_v2=\2 ac_v3=\3 ac_v4=\4 ac_v5=\5 ac_v6=\6/' \
`]
[eval `echo "$2:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g' \
 -e 's/\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):.*/ac_r1=\1 ac_r2=\2 ac_r3=\3 ac_r4=\4 ac_r5=\5 ac_r6=\6/' \
`]
ac_vm=[`expr \( $ac_v1 \> $ac_r1 \) \| \( \( $ac_v1 \= $ac_r1 \) \& \(		\
	      \( $ac_v2 \> $ac_r2 \) \| \( \( $ac_v2 \= $ac_r2 \) \& \(		\
	       \( $ac_v3 \> $ac_r3 \) \| \( \( $ac_v3 \= $ac_r3 \) \& \(	\
	        \( $ac_v4 \> $ac_r4 \) \| \( \( $ac_v4 \= $ac_r4 \) \& \(	\
	         \( $ac_v5 \> $ac_r5 \) \| \( \( $ac_v5 \= $ac_r5 \) \& \(	\
	          \( $ac_v6 \>= $ac_r6 \)					\
		 \) \)	\
		\) \)	\
	       \) \)	\
	      \) \)	\
	     \) \)	`]
case $ac_vm in
[1)]
	[$3]
	;;
*[)]
	[$4]
	;;
esac
])


dnl Setup CC with default CFLAGS value.
AC_DEFUN(MC_PROG_CC_WITH_CFLAGS,[
	MC_IF_VAR_EQ(CFLAGS, "", CFLAGS="-g")
	CFLAGS_saved="$CFLAGS"
	unset CFLAGS
	dnl Checks for compiler characteristics, CFLAGS.
	AC_PROG_CC
	MC_STR_CONTAINS($CFLAGS, -g, CFLAGS_include_g=yes)
	MC_STR_CONTAINS($CFLAGS, -O, CFLAGS_include_O=yes)
	CFLAGS="$CFLAGS_saved"

	dnl Setup CFLAGS for debugging.
	MC_IF_VAR_EQ(enable_debug, yes,
		MC_IF_VAR_EQ(CFLAGS_include_g, yes,
			MC_EVAR_ADD(CFLAGS, -g, -g)
		)
	
		MC_IF_VAR_EQ(GCC, yes,
			dnl MC_EVAR_ADD(CFLAGS, -fvolatile-global, -fvolatile-global)
			dnl MC_EVAR_ADD(CFLAGS, -fverbose-asm, -fverbose-asm)
		)
	)

	dnl Further setup CFLAGS for GCC.
	MC_IF_VAR_EQ(GCC, yes,

		dnl Debugging
		MC_EVAR_SUPPLEMENT(CFLAGS, -g, -ggdb3)

		dnl Sane Behaviour
		dnl dflt: MC_EVAR_ADD(CFLAGS, -fsigned-char, -fsigned-char)
		dnl dflt: MC_EVAR_ADD(CFLAGS, -fsigned-bitfields, -fsigned-bitfields)
		dnl dflt: MC_EVAR_ADD(CFLAGS, -fno-writable-strings, -fno-writable-strings)
		MC_EVAR_ADD(CFLAGS, -fno-strict-aliasing, -fno-strict-aliasing)
		MC_EVAR_ADD(CFLAGS, -fno-cond-mismatch, -fno-cond-mismatch)
		MC_EVAR_ADD(CFLAGS, -ffor-scope, -ffor-scope)
		MC_EVAR_ADD(CFLAGS, -Wno-cast-qual, -Wno-cast-qual)
        	
		dnl Warnings.
		MC_EVAR_ADD(CFLAGS, -Wall, -Wall)
		MC_EVAR_ADD(CFLAGS, -Wmissing-prototypes, -Wmissing-prototypes)
		dnl MC_EVAR_ADD(CFLAGS, -Wstrict-prototypes, -Wstrict-prototypes)
		MC_EVAR_ADD(CFLAGS, -Winline, -Winline)
		dnl glibc breakage: MC_EVAR_ADD(CFLAGS, -Wpointer-arith, -Wpointer-arith)
		MC_IF_VAR_EQ(enable_pedantic_ansi, yes,
			MC_EVAR_ADD(CFLAGS, -ansi, -ansi)
			MC_EVAR_ADD(CFLAGS, -pedantic, -pedantic)
		)
		dnl junk, warns on prototype arguments:
		dnl MC_EVAR_ADD(CFLAGS, -Wshadow, -Wshadow)
		dnl junk, warns on pure returntype casts as well:
		dnl MC_EVAR_ADD(CFLAGS, -Wbad-function-cast, -Wbad-function-cast)
		dnl bogus: MC_EVAR_ADD(CFLAGS, -Wconversion, -Wconversion)
		dnl bogus: MC_EVAR_ADD(CFLAGS, -Wsign-compare, -Wsign-compare)
		MC_EVAR_ADD(CFLAGS, -Wmissing-noreturn, -Wmissing-noreturn)
		dnl glibc breakage: MC_EVAR_ADD(CFLAGS, -Wredundant-decls, -Wredundant-decls)
		
	
		dnl Optimizations
		MC_EVAR_ADD(CFLAGS, -O, -O6)
		MC_EVAR_ADD(CFLAGS, -pipe, -pipe)
		MC_EVAR_ADD(CFLAGS, -fstrength-reduce, -fstrength-reduce)
		MC_EVAR_ADD(CFLAGS, -fexpensive-optimizations, -fexpensive-optimizations)
		MC_EVAR_ADD(CFLAGS, -finline-functions, -finline-functions)
		MC_EVAR_ADD(CFLAGS, -frerun-cse-after-loop, -frerun-cse-after-loop)
		MC_EVAR_ADD(CFLAGS, -freg-struct-return, -freg-struct-return)
		MC_EVAR_ADD(CFLAGS, -funroll-loops, -funroll-loops)
		MC_EVAR_ADD(CFLAGS, -frerun-loop-opt, -frerun-loop-opt)
		MC_EVAR_ADD(CFLAGS, -fgcse, -fgcse)
		MC_EVAR_ADD(CFLAGS, -fno-keep-static-consts, -fno-keep-static-consts)
	,	
		MC_IF_VAR_EQ(CFLAGS_include_O, yes,
			MC_EVAR_ADD(CFLAGS, -O, -O2)
		)
	)
])


dnl Setup CXX with default CXXFLAGS value.
AC_DEFUN(MC_PROG_CXX_WITH_CXXFLAGS,[
	MC_IF_VAR_EQ(CXXFLAGS, "", CXXFLAGS="-g")
	CXXFLAGS_saved="$CXXFLAGS"
	unset CXXFLAGS
	dnl Checks for compiler characteristics, CXXFLAGS.
	AC_PROG_CXX
	MC_STR_CONTAINS($CXXFLAGS, -g, CXXFLAGS_include_g=yes)
	MC_STR_CONTAINS($CXXFLAGS, -O, CXXFLAGS_include_O=yes)
	CXXFLAGS="$CXXFLAGS_saved"

	dnl Setup CXXFLAGS for debugging.
	MC_IF_VAR_EQ(enable_debug, yes,
		MC_IF_VAR_EQ(CXXFLAGS_include_g, yes,
			MC_EVAR_ADD(CXXFLAGS, -g, -g)
		)
	
		MC_IF_VAR_EQ(GCC, yes,
			dnl MC_EVAR_ADD(CXXFLAGS, -fvolatile-global, -fvolatile-global)
			dnl MC_EVAR_ADD(CXXFLAGS, -fverbose-asm, -fverbose-asm)
		)
	)

	dnl Further setup CXXFLAGS for GXX.
	MC_IF_VAR_EQ(GXX, yes,
        	
		dnl Warnings.
		MC_EVAR_ADD(CXXFLAGS, -Wctor-dtor-privacy, -Wctor-dtor-privacy)
		MC_EVAR_ADD(CXXFLAGS, -Wreorder, -Wreorder)
		MC_EVAR_ADD(CXXFLAGS, -Wdeprecated, -Wdeprecated)
	
		dnl Optimizations
		MC_EVAR_ADD(CXXFLAGS, -fnonnull-objects, -fnonnull-objects)
		dnl -funroll-loops gives problems with -O and templates (see Rep-CppBug_1.C)
		dnl MC_EVAR_ADD(CXXFLAGS, -funroll-loops, -funroll-loops)
		dnl MC_EVAR_ADD(CXXFLAGS, -fhandle-signatures, -fhandle-signatures)
		dnl MC_EVAR_ADD(CXXFLAGS, -fhandle-exceptions, -fhandle-exceptions)
		dnl MC_EVAR_ADD(CXXFLAGS, -frtti, -frtti)
	,	
		MC_IF_VAR_EQ(CXXFLAGS_include_O, yes,
			MC_EVAR_ADD(CXXFLAGS, -O, -O2)
		)
	)
])
