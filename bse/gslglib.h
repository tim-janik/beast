/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GSL_GLIB_H__
#define __GSL_GLIB_H__

#include <limits.h>
#include <float.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	GSL_ASSERT(foo)		do { if (!(foo)) g_error ("assertion failed `%s'", #foo); } while (0)

/* --- GLib typedefs --- */
typedef void*		gpointer;
typedef const void*	gconstpointer;
typedef char		gchar;
typedef unsigned char	guchar;
typedef signed short	gshort;
typedef unsigned short	gushort;
typedef signed int	gint;
typedef unsigned int	guint;
typedef signed long	glong;
typedef unsigned long	gulong;
typedef float		gfloat;
typedef double		gdouble;
typedef size_t		gsize;
typedef gchar		gint8;
typedef guchar		guint8;
typedef gshort		gint16;
typedef gushort		guint16;
typedef gint		gint32;
typedef guint		guint32;
typedef gint		gboolean;
typedef gint32		GTime;
#ifdef __alpha
typedef long int		gint64;
typedef unsigned long int	guint64;
#else
typedef long long int	gint64;
typedef unsigned long long int  guint64;
#endif
typedef struct _GString GString;


/* --- standard macros --- */
#ifndef ABS
#define ABS(a)		((a) > 0 ? (a) : -(a))
#endif
#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(v,l,h)    ((v) < (l) ? (l) : (v) > (h) ? (h) : (v))
#endif
#ifndef FALSE
#define FALSE           0
#endif
#ifndef TRUE
#define TRUE            (!FALSE)
#endif
#ifndef NULL
#define NULL            ((void*) 0)
#endif


/* --- glib macros --- */
#define G_MINFLOAT      FLT_MIN
#define G_MAXFLOAT      FLT_MAX
#define G_MINDOUBLE     DBL_MIN
#define G_MAXDOUBLE     DBL_MAX
#define G_MINSHORT      SHRT_MIN
#define G_MAXSHORT      SHRT_MAX
#define G_MAXUSHORT     USHRT_MAX
#define G_MININT        INT_MIN
#define G_MAXINT        INT_MAX
#define G_MAXUINT       UINT_MAX
#define G_MINLONG       LONG_MIN
#define G_MAXLONG       LONG_MAX
#define G_MAXULONG      ULONG_MAX
#define	G_USEC_PER_SEC	1000000
#define G_LITTLE_ENDIAN 1234
#define G_BIG_ENDIAN    4321
#define G_STRINGIFY(macro_or_string)    G_STRINGIFY_ARG (macro_or_string)
#define G_STRINGIFY_ARG(contents)       #contents
#if  defined __GNUC__ && !defined __cplusplus
#  define G_STRLOC      __FILE__ ":" G_STRINGIFY (__LINE__) ":" __PRETTY_FUNCTION__ "()"
#else
#  define G_STRLOC      __FILE__ ":" G_STRINGIFY (__LINE__)
#endif
#if !(defined (G_STMT_START) && defined (G_STMT_END))
#  if defined (__GNUC__) && !defined (__STRICT_ANSI__) && !defined (__cplusplus)
#    define G_STMT_START        (void)(
#    define G_STMT_END          )
#  else
#    if (defined (sun) || defined (__sun__))
#      define G_STMT_START      if (1)
#      define G_STMT_END        else (void)0
#    else
#      define G_STMT_START      do
#      define G_STMT_END        while (0)
#    endif
#  endif
#endif
#define G_STRUCT_OFFSET(struct_type, member)    \
    ((glong) ((guint8*) &((struct_type*) 0)->member))
#define G_STRUCT_MEMBER_P(struct_p, struct_offset)   \
    ((gpointer) ((guint8*) (struct_p) + (glong) (struct_offset)))
#define G_STRUCT_MEMBER(member_type, struct_p, struct_offset)   \
    (*(member_type*) G_STRUCT_MEMBER_P ((struct_p), (struct_offset)))
#define GINT_TO_POINTER(i)     ((gpointer) (int) (i))
#define GUINT_TO_POINTER(i)     ((gpointer) (guint) (i))
#define GPOINTER_TO_INT(p)     ((int) (p))
#define GPOINTER_TO_UINT(p)    ((guint) (p))
#define GUINT16_SWAP_LE_BE(val)        ((guint16) ( \
    (((guint16) (val) & (guint16) 0x00ffU) << 8) | \
    (((guint16) (val) & (guint16) 0xff00U) >> 8)))
#define GUINT32_SWAP_LE_BE(val)        ((guint32) ( \
    (((guint32) (val) & (guint32) 0x000000ffU) << 24) | \
    (((guint32) (val) & (guint32) 0x0000ff00U) <<  8) | \
    (((guint32) (val) & (guint32) 0x00ff0000U) >>  8) | \
    (((guint32) (val) & (guint32) 0xff000000U) >> 24)))
#define g_memmove memmove
#define g_assert  GSL_ASSERT
#define g_assert_not_reached()	g_assert(!G_STRLOC": should not be reached")
#define	g_return_if_fail(foo)		do { if (!(foo)) g_message (G_STRLOC ": assertion failed `%s'", #foo); } while (0)
#define	g_return_val_if_fail(foo,v)		do { if (!(foo)) { g_message (G_STRLOC ": assertion failed `%s'", #foo); return(v);}} while (0)

/* from galloca.h */

#ifdef  __GNUC__
/* GCC does the right thing */
# undef alloca
# define alloca(size)   __builtin_alloca (size)
#elif defined (GLIB_HAVE_ALLOCA_H)
/* a native and working alloca.h is there */ 
# include <alloca.h>
#else /* !__GNUC__ && !GLIB_HAVE_ALLOCA_H */
# ifdef _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# else /* !_MSC_VER */
#  ifdef _AIX
 #pragma alloca
#  else /* !_AIX */
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif /* !alloca */
#  endif /* !_AIX */
# endif /* !_MSC_VER */
#endif /* !__GNUC__ && !GLIB_HAVE_ALLOCA_H */

#define g_alloca(size) alloca (size)
#define g_newa(struct_type, n_structs)  ((struct_type*) g_alloca (sizeof (struct_type) * (gsize) (n_structs)))

/* needs inline configure check */
#if defined(__GNUC__)
#define inline __inline__
#else
#define inline /* no inline */
#endif


/* --- inline functions --- */
void
gsl_g_log (const gchar*msg,const char *format, va_list ap);
static inline void
g_error (const gchar *format,
	 ...)
{
  va_list args;
  va_start (args, format);
  gsl_g_log ("**ERROR**", format, args);
  va_end (args);
}
static inline void
g_message (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  gsl_g_log ("**MESSAGE**", format, args);
  va_end (args);
}
static inline void
g_warning (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  gsl_g_log ("**WARNING**", format, args);
  va_end (args);
}
static inline void
g_print (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  gsl_g_log (NULL, format, args);
  va_end (args);
}
typedef struct _GTrashStack     GTrashStack;
struct _GTrashStack
{
  GTrashStack *next;
};
static inline  guint
g_bit_storage (gulong number)
{
  register guint n_bits = 0;

  do
    {
      n_bits++;
      number >>= 1;
    }
  while (number);
  return n_bits;
}
static inline void
g_trash_stack_push (GTrashStack **stack_p,
		    gpointer      data_p)
{
  GTrashStack *data = (GTrashStack *) data_p;

  data->next = *stack_p;
  *stack_p = data;
}
static inline gpointer
g_trash_stack_pop (GTrashStack **stack_p)
{
  GTrashStack *data;

  data = *stack_p;
  if (data)
    {
      *stack_p = data->next;
      /* NULLify private pointer here, most platforms store NULL as
       * subsequent 0 bytes
       */
      data->next = NULL;
    }

  return data;
}
static inline gpointer
g_trash_stack_peek (GTrashStack **stack_p)
{
  GTrashStack *data;

  data = *stack_p;

  return data;
}
static inline guint
g_trash_stack_height (GTrashStack **stack_p)
{
  GTrashStack *data;
  guint i = 0;

  for (data = *stack_p; data; data = data->next)
    i++;

  return i;
}


/* --- GCC features --- */
#if     __GNUC__ >= 2 && __GNUC_MINOR__ > 95
#define G_GNUC_PRINTF( format_idx, arg_idx )    \
  __attribute__((format (printf, format_idx, arg_idx)))
#define G_GNUC_SCANF( format_idx, arg_idx )     \
  __attribute__((format (scanf, format_idx, arg_idx)))
#define G_GNUC_FORMAT( arg_idx )                \
  __attribute__((format_arg (arg_idx)))
#define G_GNUC_NORETURN                         \
  __attribute__((noreturn))
#define G_GNUC_CONST                            \
  __attribute__((const))
#define G_GNUC_UNUSED                           \
  __attribute__((unused))
#define G_GNUC_NO_INSTRUMENT                    \
  __attribute__((no_instrument_function))
#else   /* !__GNUC__ */
#define G_GNUC_PRINTF( format_idx, arg_idx )
#define G_GNUC_SCANF( format_idx, arg_idx )
#define G_GNUC_FORMAT( arg_idx )
#define G_GNUC_NORETURN
#define G_GNUC_CONST
#define G_GNUC_UNUSED
#define G_GNUC_NO_INSTRUMENT
#endif  /* !__GNUC__ */



/* --- sick defines --- */
typedef struct { int fd; short events, revents; } GPollFD;



/* --- functions --- */
#define	g_malloc		gsl_g_malloc
#define	g_malloc0		gsl_g_malloc0
#define	g_realloc		gsl_g_realloc
#define	g_free			gsl_g_free
#define g_strdup		gsl_g_strdup
#define g_strndup		gsl_g_strndup
#define g_memdup		gsl_g_memdup
#define	g_strdup_printf		gsl_g_strdup_printf
#define	g_strdup_vprintf	gsl_g_strdup_vprintf
#define	g_strndup		gsl_g_strndup
#define	g_strconcat		gsl_g_strconcat
#define	g_usleep		gsl_g_usleep
#define	g_strerror		gsl_g_strerror
#define g_direct_hash 	gsl_g_direct_hash 
#define g_direct_equal 	gsl_g_direct_equal 
#define g_str_equal 	gsl_g_str_equal 
#define g_str_hash 	gsl_g_str_hash 
#define g_strtod	gsl_g_strtod
#define g_stpcpy	gsl_g_stpcpy
#define g_printf_string_upper_bound gsl_g_printf_string_upper_bound
gpointer g_malloc         (gulong        n_bytes);
gpointer g_malloc0        (gulong        n_bytes);
gpointer g_realloc        (gpointer      mem,
			   gulong        n_bytes);
void     g_free           (gpointer      mem);
gpointer              g_memdup         (gconstpointer mem,
					guint          byte_size);
gchar*                g_strdup         (const gchar *str);
gchar*		      g_strndup (const gchar *str,
				 gsize        n);
gchar*                g_strdup_printf  (const gchar *format,
					...) G_GNUC_PRINTF (1, 2);
gchar*                g_strdup_vprintf (const gchar *format,
					va_list      args);
gchar*                g_strndup        (const gchar *str,
					gsize        n);
gchar*                g_strconcat      (const gchar *string1,
					...); /* NULL terminated */
void g_usleep(unsigned long usec);
char* g_strerror(int e);
guint g_direct_hash (gconstpointer v);
gboolean g_direct_equal (gconstpointer v1, gconstpointer v2);
gboolean g_str_equal (gconstpointer v1,   gconstpointer v2);
guint g_str_hash (gconstpointer key);
gdouble	g_strtod (const gchar *nptr, 	  gchar **endptr);
gsize g_printf_string_upper_bound (const gchar *format,  va_list      args);
gchar * g_stpcpy (gchar       *dest, 	  const gchar *src);
     


/* --- function defines --- */
#define g_new(struct_type, n_structs)           \
    ((struct_type *) g_malloc (((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))
#define g_new0(struct_type, n_structs)          \
    ((struct_type *) g_malloc0 (((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))
#define g_renew(struct_type, mem, n_structs)    \
    ((struct_type *) g_realloc ((mem), ((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))
#define	g_try_malloc		malloc
#define	g_try_realloc		realloc



/* --- configure stuff!!! --- */
#define	G_BYTE_ORDER G_LITTLE_ENDIAN
/* #define	GLIB_HAVE_STPCPY	1 */
/* Define G_VA_COPY() to do the right thing for copying va_list variables.
 * glibconfig.h may have already defined G_VA_COPY as va_copy or __va_copy.
 */
#if !defined (G_VA_COPY)
#  if defined (__GNUC__) && defined (__PPC__) && (defined (_CALL_SYSV) || defined (_WIN32))
#    define G_VA_COPY(ap1, ap2)   (*(ap1) = *(ap2))
#  elif defined (G_VA_COPY_AS_ARRAY)
#    define G_VA_COPY(ap1, ap2)   g_memmove ((ap1), (ap2), sizeof (va_list))
#  else /* va_list is a pointer */
#    define G_VA_COPY(ap1, ap2)   ((ap1) = (ap2))
#  endif /* va_list is a pointer */
#endif /* !G_VA_COPY */




/* subtract from biased_exponent to form base2 exponent (normal numbers) */
typedef union  _GDoubleIEEE754  GDoubleIEEE754;
typedef union  _GFloatIEEE754   GFloatIEEE754;
#define G_IEEE754_FLOAT_BIAS    (127)
#define G_IEEE754_DOUBLE_BIAS   (1023)
/* multiply with base2 exponent to get base10 exponent (nomal numbers) */
#define G_LOG_2_BASE_10         (0.30102999566398119521)
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
union _GFloatIEEE754
{
  gfloat v_float;
  struct {
    guint mantissa : 23;
    guint biased_exponent : 8;
    guint sign : 1;
  } mpn;
};
union _GDoubleIEEE754
{
  gdouble v_double;
  struct {
    guint mantissa_low : 32;
    guint mantissa_high : 20;
    guint biased_exponent : 11;
    guint sign : 1;
  } mpn;
};
#elif G_BYTE_ORDER == G_BIG_ENDIAN
union _GFloatIEEE754
{
  gfloat v_float;
  struct {
    guint sign : 1;
    guint biased_exponent : 8;
    guint mantissa : 23;
  } mpn;
};
union _GDoubleIEEE754
{
  gdouble v_double;
  struct {
    guint sign : 1;
    guint biased_exponent : 11;
    guint mantissa_high : 20;
    guint mantissa_low : 32;
  } mpn;
};
#else /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */
#error unknown ENDIAN type
#endif /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */



/* --- GHashTable --- */
typedef struct _GHashTable  GHashTable;
typedef gboolean  (*GHRFunc)  (gpointer  key,
			       gpointer  value,
			       gpointer  user_data);
typedef void            (*GHFunc)               (gpointer       key,
						 gpointer       value,
						 gpointer       user_data);
typedef guint           (*GHashFunc)            (gconstpointer  key);
typedef gboolean        (*GEqualFunc)           (gconstpointer  a,
						 gconstpointer  b);
typedef void            (*GDestroyNotify)       (gpointer       data);
#define g_hash_table_new               	gsl_g_hash_table_new               
#define g_hash_table_new_full          	gsl_g_hash_table_new_full          
#define g_hash_table_destroy           	gsl_g_hash_table_destroy           
#define g_hash_table_insert            	gsl_g_hash_table_insert            
#define g_hash_table_replace           	gsl_g_hash_table_replace           
#define g_hash_table_remove            	gsl_g_hash_table_remove            
#define g_hash_table_steal             	gsl_g_hash_table_steal             
#define g_hash_table_lookup            	gsl_g_hash_table_lookup            
#define g_hash_table_lookup_extended   	gsl_g_hash_table_lookup_extended   
#define g_hash_table_foreach           	gsl_g_hash_table_foreach           
#define g_hash_table_foreach_remove    	gsl_g_hash_table_foreach_remove    
#define g_hash_table_foreach_steal     	gsl_g_hash_table_foreach_steal     
#define g_hash_table_size              	gsl_g_hash_table_size              
GHashTable* g_hash_table_new               (GHashFunc       hash_func,
					    GEqualFunc      key_equal_func);
GHashTable* g_hash_table_new_full          (GHashFunc       hash_func,
					    GEqualFunc      key_equal_func,
					    GDestroyNotify  key_destroy_func,
					    GDestroyNotify  value_destroy_func);
void        g_hash_table_destroy           (GHashTable     *hash_table);
void        g_hash_table_insert            (GHashTable     *hash_table,
					    gpointer        key,
					    gpointer        value);
void        g_hash_table_replace           (GHashTable     *hash_table,
					    gpointer        key,
					    gpointer        value);
gboolean    g_hash_table_remove            (GHashTable     *hash_table,
					    gconstpointer   key);
gboolean    g_hash_table_steal             (GHashTable     *hash_table,
					    gconstpointer   key);
gpointer    g_hash_table_lookup            (GHashTable     *hash_table,
					    gconstpointer   key);
gboolean    g_hash_table_lookup_extended   (GHashTable     *hash_table,
					    gconstpointer   lookup_key,
					    gpointer       *orig_key,
					    gpointer       *value);
void        g_hash_table_foreach           (GHashTable     *hash_table,
					    GHFunc          func,
					    gpointer        user_data);
guint       g_hash_table_foreach_remove    (GHashTable     *hash_table,
					    GHRFunc         func,
					    gpointer        user_data);
guint       g_hash_table_foreach_steal     (GHashTable     *hash_table,
					    GHRFunc         func,
					    gpointer        user_data);
guint       g_hash_table_size              (GHashTable     *hash_table);


/* --- GScanner --- */
typedef struct _GScanner	GScanner;
typedef struct _GScannerConfig	GScannerConfig;
typedef union  _GTokenValue     GTokenValue;
typedef void		(*GScannerMsgFunc)	(GScanner      *scanner,
						 gchar	       *message,
						 gint		error);
#define G_CSET_A_2_Z	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define G_CSET_a_2_z	"abcdefghijklmnopqrstuvwxyz"
#define G_CSET_DIGITS	"0123456789"
#define G_CSET_LATINC	"\300\301\302\303\304\305\306"\
			"\307\310\311\312\313\314\315\316\317\320"\
			"\321\322\323\324\325\326"\
			"\330\331\332\333\334\335\336"
#define G_CSET_LATINS	"\337\340\341\342\343\344\345\346"\
			"\347\350\351\352\353\354\355\356\357\360"\
			"\361\362\363\364\365\366"\
			"\370\371\372\373\374\375\376\377"
typedef enum
{
  G_ERR_UNKNOWN,
  G_ERR_UNEXP_EOF,
  G_ERR_UNEXP_EOF_IN_STRING,
  G_ERR_UNEXP_EOF_IN_COMMENT,
  G_ERR_NON_DIGIT_IN_CONST,
  G_ERR_DIGIT_RADIX,
  G_ERR_FLOAT_RADIX,
  G_ERR_FLOAT_MALFORMED
} GErrorType;
typedef enum
{
  G_TOKEN_EOF			=   0,
  
  G_TOKEN_LEFT_PAREN		= '(',
  G_TOKEN_RIGHT_PAREN		= ')',
  G_TOKEN_LEFT_CURLY		= '{',
  G_TOKEN_RIGHT_CURLY		= '}',
  G_TOKEN_LEFT_BRACE		= '[',
  G_TOKEN_RIGHT_BRACE		= ']',
  G_TOKEN_EQUAL_SIGN		= '=',
  G_TOKEN_COMMA			= ',',
  
  G_TOKEN_NONE			= 256,
  
  G_TOKEN_ERROR,
  
  G_TOKEN_CHAR,
  G_TOKEN_BINARY,
  G_TOKEN_OCTAL,
  G_TOKEN_INT,
  G_TOKEN_HEX,
  G_TOKEN_FLOAT,
  G_TOKEN_STRING,
  
  G_TOKEN_SYMBOL,
  G_TOKEN_IDENTIFIER,
  G_TOKEN_IDENTIFIER_NULL,
  
  G_TOKEN_COMMENT_SINGLE,
  G_TOKEN_COMMENT_MULTI,
  G_TOKEN_LAST
} GTokenType;
union	_GTokenValue
{
  gpointer	v_symbol;
  gchar		*v_identifier;
  gulong	v_binary;
  gulong	v_octal;
  gulong	v_int;
  gdouble	v_float;
  gulong	v_hex;
  gchar		*v_string;
  gchar		*v_comment;
  guchar	v_char;
  guint		v_error;
};
struct	_GScannerConfig
{
  gchar		*cset_skip_characters;		/* default: " \t\n" */
  gchar		*cset_identifier_first;
  gchar		*cset_identifier_nth;
  gchar		*cpair_comment_single;		/* default: "#\n" */
  guint		case_sensitive : 1;
  guint		skip_comment_multi : 1;		/* C like comment */
  guint		skip_comment_single : 1;	/* single line comment */
  guint		scan_comment_multi : 1;		/* scan multi line comments? */
  guint		scan_identifier : 1;
  guint		scan_identifier_1char : 1;
  guint		scan_identifier_NULL : 1;
  guint		scan_symbols : 1;
  guint		scan_binary : 1;
  guint		scan_octal : 1;
  guint		scan_float : 1;
  guint		scan_hex : 1;			/* `0x0ff0' */
  guint		scan_hex_dollar : 1;		/* `$0ff0' */
  guint		scan_string_sq : 1;		/* string: 'anything' */
  guint		scan_string_dq : 1;		/* string: "\\-escapes!\n" */
  guint		numbers_2_int : 1;		/* bin, octal, hex => int */
  guint		int_2_float : 1;		/* int => G_TOKEN_FLOAT? */
  guint		identifier_2_string : 1;
  guint		char_2_token : 1;		/* return G_TOKEN_CHAR? */
  guint		symbol_2_token : 1;
  guint		scope_0_fallback : 1;		/* try scope 0 on lookups? */
};
struct	_GScanner
{
  gpointer		user_data;
  guint			max_parse_errors;
  guint			parse_errors;
  const gchar		*input_name;
  /* GData			*qdata; */
  GScannerConfig	*config;
  GTokenType		token;
  GTokenValue		value;
  guint			line;
  guint			position;
  GTokenType		next_token;
  GTokenValue		next_value;
  guint			next_line;
  guint			next_position;
  GHashTable		*symbol_table;
  gint			input_fd;
  const gchar		*text;
  const gchar		*text_end;
  gchar			*buffer;
  guint			scope_id;
  GScannerMsgFunc	msg_handler;
};
#define g_scanner_new			   gsl_g_scanner_new			
#define g_scanner_destroy		   gsl_g_scanner_destroy		
#define g_scanner_input_file		   gsl_g_scanner_input_file		
#define g_scanner_sync_file_offset	   gsl_g_scanner_sync_file_offset	
#define g_scanner_input_text		   gsl_g_scanner_input_text		
#define g_scanner_get_next_token	   gsl_g_scanner_get_next_token	
#define g_scanner_peek_next_token	   gsl_g_scanner_peek_next_token	
#define g_scanner_cur_token		   gsl_g_scanner_cur_token		
#define g_scanner_cur_value		   gsl_g_scanner_cur_value		
#define g_scanner_cur_line		   gsl_g_scanner_cur_line		
#define g_scanner_cur_position		   gsl_g_scanner_cur_position		
#define g_scanner_eof			   gsl_g_scanner_eof			
#define g_scanner_set_scope		   gsl_g_scanner_set_scope		
#define g_scanner_scope_add_symbol	   gsl_g_scanner_scope_add_symbol	
#define g_scanner_scope_remove_symbol	   gsl_g_scanner_scope_remove_symbol	
#define g_scanner_scope_lookup_symbol	   gsl_g_scanner_scope_lookup_symbol	
#define g_scanner_scope_foreach_symbol	   gsl_g_scanner_scope_foreach_symbol	
#define g_scanner_lookup_symbol		   gsl_g_scanner_lookup_symbol		
#define g_scanner_unexp_token		   gsl_g_scanner_unexp_token		
#define g_scanner_error			   gsl_g_scanner_error			
#define g_scanner_warn			   gsl_g_scanner_warn			
GScanner*	g_scanner_new			(const GScannerConfig *config_templ);
void		g_scanner_destroy		(GScanner	*scanner);
void		g_scanner_input_file		(GScanner	*scanner,
						 gint		input_fd);
void		g_scanner_sync_file_offset	(GScanner	*scanner);
void		g_scanner_input_text		(GScanner	*scanner,
						 const	gchar	*text,
						 guint		text_len);
GTokenType	g_scanner_get_next_token	(GScanner	*scanner);
GTokenType	g_scanner_peek_next_token	(GScanner	*scanner);
GTokenType	g_scanner_cur_token		(GScanner	*scanner);
GTokenValue	g_scanner_cur_value		(GScanner	*scanner);
guint		g_scanner_cur_line		(GScanner	*scanner);
guint		g_scanner_cur_position		(GScanner	*scanner);
gboolean	g_scanner_eof			(GScanner	*scanner);
guint		g_scanner_set_scope		(GScanner	*scanner,
						 guint		 scope_id);
void		g_scanner_scope_add_symbol	(GScanner	*scanner,
						 guint		 scope_id,
						 const gchar	*symbol,
						 gpointer	value);
void		g_scanner_scope_remove_symbol	(GScanner	*scanner,
						 guint		 scope_id,
						 const gchar	*symbol);
gpointer	g_scanner_scope_lookup_symbol	(GScanner	*scanner,
						 guint		 scope_id,
						 const gchar	*symbol);
void		g_scanner_scope_foreach_symbol	(GScanner	*scanner,
						 guint		 scope_id,
						 GHFunc		 func,
						 gpointer	 user_data);
gpointer	g_scanner_lookup_symbol		(GScanner	*scanner,
						 const gchar	*symbol);
void		g_scanner_unexp_token		(GScanner	*scanner,
						 GTokenType	expected_token,
						 const gchar	*identifier_spec,
						 const gchar	*symbol_spec,
						 const gchar	*symbol_name,
						 const gchar	*message,
						 gint		 is_error);
void		g_scanner_error			(GScanner	*scanner,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
void		g_scanner_warn			(GScanner	*scanner,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
#define		g_scanner_add_symbol( scanner, symbol, value )	G_STMT_START { \
  g_scanner_scope_add_symbol ((scanner), 0, (symbol), (value)); \
} G_STMT_END
#define		g_scanner_remove_symbol( scanner, symbol )	G_STMT_START { \
  g_scanner_scope_remove_symbol ((scanner), 0, (symbol)); \
} G_STMT_END
#define		g_scanner_foreach_symbol( scanner, func, data )	G_STMT_START { \
  g_scanner_scope_foreach_symbol ((scanner), 0, (func), (data)); \
} G_STMT_END




#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef GSL_WANT_ARTS_THREADS
#include "gslartsthreads.h"
#endif


#endif /* __GSL_GLIB_H__ */ /* vim: set ts=8 sw=2 sts=2: */

