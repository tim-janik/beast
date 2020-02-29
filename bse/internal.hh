// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_INTERNAL_HH__
#define __BSE_INTERNAL_HH__

// Import simple BSE types into global scope
using Bse::uint8;
using Bse::uint16;
using Bse::uint32;
using Bse::uint64;
using Bse::int8;
using Bse::int16;
using Bse::int32;
using Bse::int64;
using Bse::unichar;
using Bse::String;

/// Retrieve the translation of a C or C++ string.
#define _(...)          ::Bse::_ (__VA_ARGS__)
/// Mark a string for translation, passed through verbatim by the preprocessor.
#define N_(str)         (str)

/// Constrain, apply, notify and implement a property change, the property name must equal `__func__`.
#define APPLY_IDL_PROPERTY(lvalue, rvalue)      BSE_OBJECT_APPLY_IDL_PROPERTY(lvalue, rvalue)

/// Yield the number of C @a array elements.
#define ARRAY_SIZE(array)               BSE_ARRAY_SIZE (array)

/// Return from the current function if @a cond is unmet and issue an assertion warning.
#define assert_return(cond, ...)        BSE_ASSERT_RETURN (cond, __VA_ARGS__)
/// Return from the current function and issue an assertion warning.
#define assert_return_unreached(...)    BSE_ASSERT_RETURN_UNREACHED (__VA_ARGS__)

/// Indentation helper for editors that cannot (yet) decipher `if constexpr`
#define	if_constexpr	if constexpr

/// Produce a const char* string, wrapping @a str into C-style double quotes.
#define CQUOTE(str)                                     BSE_CQUOTE(str)

/// Hint to the compiler to optimize for @a cond == TRUE.
#define ISLIKELY(cond)  BSE_ISLIKELY (cond)
/// Hint to the compiler to optimize for @a cond == FALSE.
#define UNLIKELY(cond)  BSE_UNLIKELY (cond)

/// Return silently if @a cond does not evaluate to true with return value @a ...
#define return_unless(cond, ...)        BSE_RETURN_UNLESS (cond, __VA_ARGS__)

/// Create a Bse::StringVector, from a const char* C-style array.
#define STRING_VECTOR_FROM_ARRAY(ConstCharArray)        BSE_STRING_VECTOR_FROM_ARRAY(ConstCharArray)

#endif  // __BSE_INTERNAL_HH__
