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

dnl MC_ADD_TO_VAR(environment-variable, check-string, add-string)
AC_DEFUN(MC_ADD_TO_VAR,[
	MC_STR_CONTAINS($[$1], [$2], [$1]="$[$1]", [$1]="$[$1] [$3]")
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
			MC_ADD_TO_VAR(CFLAGS, -g, -g)
		)
	
		MC_IF_VAR_EQ(GCC, yes,
			dnl MC_ADD_TO_VAR(CFLAGS, -fvolatile-global, -fvolatile-global)
			dnl MC_ADD_TO_VAR(CFLAGS, -fverbose-asm, -fverbose-asm)
		)
	)

	dnl Further setup CFLAGS for GCC.
	MC_IF_VAR_EQ(GCC, yes,
        	
		dnl Warnings.
		MC_ADD_TO_VAR(CFLAGS, -Wall, -Wall)
		MC_ADD_TO_VAR(CFLAGS, -Wmissing-prototypes, -Wmissing-prototypes)
		dnl MC_ADD_TO_VAR(CFLAGS, -Wstrict-prototypes, -Wstrict-prototypes)
		MC_ADD_TO_VAR(CFLAGS, -Winline, -Winline)
		MC_ADD_TO_VAR(CFLAGS, -Wpointer-arith, -Wpointer-arith)
		MC_IF_VAR_EQ(enable_pedantic_ansi, yes,
			MC_ADD_TO_VAR(CFLAGS, -ansi, -ansi)
			MC_ADD_TO_VAR(CFLAGS, -pedantic, -pedantic)
		)
	
		dnl Optimizations
		MC_ADD_TO_VAR(CFLAGS, -O, -O6)
		MC_ADD_TO_VAR(CFLAGS, -pipe, -pipe)
		MC_ADD_TO_VAR(CFLAGS, -fstrength-reduce, -fstrength-reduce)
		MC_ADD_TO_VAR(CFLAGS, -fexpensive-optimizations, -fexpensive-optimizations)
		MC_ADD_TO_VAR(CFLAGS, -finline-functions, -finline-functions)
		MC_ADD_TO_VAR(CFLAGS, -frerun-cse-after-loop, -frerun-cse-after-loop)
		MC_ADD_TO_VAR(CFLAGS, -freg-struct-return, -freg-struct-return)
		dnl MC_ADD_TO_VAR(CFLAGS, -funroll-loops, -funroll-loops)
	,	
		MC_IF_VAR_EQ(CFLAGS_include_O, yes,
			MC_ADD_TO_VAR(CFLAGS, -O, -O2)
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
			MC_ADD_TO_VAR(CXXFLAGS, -g, -g)
		)
	
		MC_IF_VAR_EQ(GCC, yes,
			dnl MC_ADD_TO_VAR(CXXFLAGS, -fvolatile-global, -fvolatile-global)
			dnl MC_ADD_TO_VAR(CXXFLAGS, -fverbose-asm, -fverbose-asm)
		)
	)

	dnl Further setup CXXFLAGS for GXX.
	MC_IF_VAR_EQ(GXX, yes,
        	
		dnl Warnings.
		MC_ADD_TO_VAR(CXXFLAGS, -Wall, -Wall)
		MC_ADD_TO_VAR(CXXFLAGS, -Wmissing-prototypes, -Wmissing-prototypes)
		MC_ADD_TO_VAR(CXXFLAGS, -Wstrict-prototypes, -Wstrict-prototypes)
		MC_ADD_TO_VAR(CXXFLAGS, -Winline, -Winline)
		MC_ADD_TO_VAR(CXXFLAGS, -Wpointer-arith, -Wpointer-arith)
		MC_IF_VAR_EQ(enable_pedantic_ansi, yes,
			MC_ADD_TO_VAR(CXXFLAGS, -ansi, -ansi)
			MC_ADD_TO_VAR(CXXFLAGS, -pedantic, -pedantic)
		)
	
		dnl Optimizations
		MC_ADD_TO_VAR(CXXFLAGS, -O, -O6)
		MC_ADD_TO_VAR(CXXFLAGS, -pipe, -pipe)
		MC_ADD_TO_VAR(CXXFLAGS, -fstrength-reduce, -fstrength-reduce)
		MC_ADD_TO_VAR(CXXFLAGS, -fexpensive-optimizations, -fexpensive-optimizations)
		MC_ADD_TO_VAR(CXXFLAGS, -finline-functions, -finline-functions)
		MC_ADD_TO_VAR(CXXFLAGS, -frerun-cse-after-loop, -frerun-cse-after-loop)
		MC_ADD_TO_VAR(CXXFLAGS, -freg-struct-return, -freg-struct-return)
		MC_ADD_TO_VAR(CXXFLAGS, -fnonnull-objects, -fnonnull-objects)
		dnl -funroll-loops gives problems with -O and templates (see Rep-CppBug_1.C)
		dnl MC_ADD_TO_VAR(CXXFLAGS, -funroll-loops, -funroll-loops)
		MC_ADD_TO_VAR(CXXFLAGS, -fhandle-signatures, -fhandle-signatures)
		dnl MC_ADD_TO_VAR(CXXFLAGS, -fhandle-exceptions, -fhandle-exceptions)
		dnl MC_ADD_TO_VAR(CXXFLAGS, -frtti, -frtti)
	,	
		MC_IF_VAR_EQ(CXXFLAGS_include_O, yes,
			MC_ADD_TO_VAR(CXXFLAGS, -O, -O2)
		)
	)
])
