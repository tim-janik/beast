/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2001 and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
static char *bsw_log_domain_bsw = "BSW-MkApi";
#include        "../bse/bse.h"
#include        "../PKG_config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<string.h>

static FILE       *f_out = NULL;
static gchar      *prefix = "bsw";
static gboolean    gen_header = 0;
static gboolean    gen_body = 0;
static GHashTable *type_wrapper_ht = NULL;

#define to_lower(c)                             ( \
        (guchar) (                                                      \
          ( (((guchar)(c))>='A' && ((guchar)(c))<='Z') * ('a'-'A') ) |  \
          ( (((guchar)(c))>=192 && ((guchar)(c))<=214) * (224-192) ) |  \
          ( (((guchar)(c))>=216 && ((guchar)(c))<=222) * (248-216) ) |  \
          ((guchar)(c))                                                 \
        )                                                               \
)
#define to_upper(c) (to_lower (c) + 'A' - 'a')
#define	is_lower(c) ((c) == to_lower (c))
#define	is_upper(c) ((c) == to_upper (c))

static gchar*	tmacro_from_type (GType type);
     
static const gchar*
indent (guint n_spaces)
{
  static gchar *buffer = NULL;
  static guint blength = 0;

  if (blength <= n_spaces)
    {
      blength = n_spaces + 1;
      g_free (buffer);
      buffer = g_new (gchar, blength);
    }
  memset (buffer, ' ', n_spaces);
  buffer[n_spaces] = 0;

  return buffer;
}

static void
add_type_wrapper (GType type)
{
  if (g_type_is_a (type, BSE_TYPE_OBJECT) && !g_hash_table_lookup (type_wrapper_ht, (gpointer) type))
    {
      g_hash_table_insert (type_wrapper_ht, (gpointer) type, GUINT_TO_POINTER (TRUE));
      add_type_wrapper (g_type_parent (type));
    }
}

static void
type_wrapper_foreach (gpointer key,
		      gpointer dummy1,
		      gpointer dummy2)
{
  GType type = (GType) key;
  gchar *macro = tmacro_from_type (type);
  gchar *delim = strchr (macro, '_'); /* skip namespace prefix */

  *delim = 0;
  delim = strchr (delim + 1, '_');	/* skip _TYPE portion */
  *delim = 0;
  if (macro[0] == 'B' && macro[1] == 'S' && macro[2] == 'E')
    macro[2] = 'W';
  
  fprintf (f_out, "#define %s_IS_%s(proxy)\t(bsw_object_is_a ((proxy), \"%s\"))\n",
	   macro, delim + 1,
	   g_type_name (type));
  g_free (macro);
}

static void
print_type_wrappers (void)
{
  g_hash_table_foreach (type_wrapper_ht, type_wrapper_foreach, NULL);
}

static void
print_enums (void)
{
  GType *children;
  guint n, i;

  children = g_type_children (G_TYPE_ENUM, &n);
  for (i = 0; i < n; i++)
    {
      const gchar *name = g_type_name (children[i]);

      if (name[0] == 'B' && name[1] == 's' && name[2] == 'e')
	{
	  GEnumClass *eclass = g_type_class_ref (children[i]);
	  GEnumValue *val;

	  fprintf (f_out, "typedef enum\n{\n");
	  for (val = eclass->values; val->value_name; val++)
	    {
	      fprintf (f_out, "  BSW%s = %d,", val->value_name + 3, val->value);
	      if (children[i] == BSE_TYPE_ERROR_TYPE)
		fprintf (f_out, "       \t/* \"%s\" */\n", bse_error_blurb (val->value));
	      else
		fprintf (f_out, "\n");
	    }
	  fprintf (f_out, "} Bsw%s;\n", g_type_name (children[i]) + 3);
	}
    }
  g_free (children);
  children = g_type_children (G_TYPE_FLAGS, &n);
  for (i = 0; i < n; i++)
    {
      const gchar *name = g_type_name (children[i]);

      if (name[0] == 'B' && name[1] == 's' && name[2] == 'e')
	{
	  GFlagsClass *fclass = g_type_class_ref (children[i]);
	  GFlagsValue *val;

	  fprintf (f_out, "typedef enum\n{\n");
	  for (val = fclass->values; val->value_name; val++)
	    fprintf (f_out, "  BSW%s = 0x%x,\n", val->value_name + 3, val->value);
	  fprintf (f_out, "} Bsw%s;\n", g_type_name (children[i]) + 3);
	}
    }
  g_free (children);
  fprintf (f_out, "\n");
}

/* BseSomeObject        - type name (tname)
 * BSE_TYPE_SOME_OBJECT - type macro (tmacro)
 * do-funky-stuff	- proc name (pname)
 * BseSomeObject+do-it	- method name (mname)
 * some_identifier	- c identifier (cident)
 * some_function()	- c function (cfunc)
 */
static gchar*
tmacro_from_type (GType type)
{
  gchar *result, *p;
  const gchar *s;
  guint was_upper;
  
  switch (type)
    {
    case G_TYPE_CHAR:		return "G_TYPE_CHAR";
    case G_TYPE_UCHAR:		return "G_TYPE_UCHAR";
    case G_TYPE_INT:		return "G_TYPE_INT";
    case G_TYPE_UINT:		return "G_TYPE_UINT";
    case G_TYPE_FLOAT:		return "G_TYPE_FLOAT";
    case G_TYPE_DOUBLE:		return "G_TYPE_DOUBLE";
    case G_TYPE_BOOLEAN:	return "G_TYPE_BOOLEAN";
    case G_TYPE_STRING:		return "G_TYPE_STRING";
    }

  s = g_type_name (type);
  if (!is_upper (s[0]))
    g_error ("unable to handle type \"%s\"", g_type_name (type));
  result = g_new (gchar, strlen (s) * 2 + 12);
  p = result;

  *p++ = *s++;
  while (!is_upper (*s))
    {
      *p++ = to_upper (*s);
      s++;
    }
  *p++ = '_';
  *p++ = 'T';
  *p++ = 'Y';
  *p++ = 'P';
  *p++ = 'E';
  was_upper = 0;
  while (*s)
    {
      if (is_upper (*s))
	{
	  if (!was_upper || (s[1] && is_lower (s[1]) && was_upper >= 2))
	    *p++ = '_';
	  was_upper ++;
	}
      else
	was_upper = 0;
      *p++ = to_upper (*s);
      s++;
    }
  *p++ = 0;

  return result;
}

static gchar*
cfunc_from_proc_name (const gchar *string)
{
  const gchar *s;
  gchar *result = g_new (gchar, strlen (string) * 2 + 12), *p = result;
  guint was_upper;
  
  /* strip procedure namespace */
  if (strchr (string, '+'))	/* methods: BseObject+get-type */
    {
      s = string + 1;
      while (*s && (*s < 'A' || *s > 'Z'))
	s++;
      string = s;
    }
  else				/* procedures: bse-error-blurb */
    {
      s = strchr (string, '-');
      if (!s)
	s = strchr (string, '_');
      if (s && s[1] != 0)
	string = s + 1;
      else
	g_warning ("procedure without namespace prefix: \"%s\"", string);
    }
  g_assert (string[0] != 0);

  was_upper = 0;
  for (s = string; *s; s++)
    switch (*s)
      {
      case '-':
      case '+':
	*p++ = '_';
	break;
      default:
	if (is_upper (*s))
	  {
	    if (!was_upper || (s[1] && is_lower (s[1]) && was_upper >= 2))
	      {
		if (s > string)	/* don't prefix '_' proc name start */
		  *p++ = '_';
	      }
	    *p++ = to_lower (*s);
	    was_upper++;
	  }
	else
	  {
	    *p++ = *s;
	    was_upper = 0;
	  }
	break;
      }
  *p++ = 0;

  return result;
}

static gchar*
cident_canonify (const gchar *string)
{
  const gchar *s;
  gchar *result = g_new (gchar, strlen (string) * 2), *p = result;

  for (s = string; *s; s++)
    switch (*s)
      {
      case '-':
      case '+':
	*p++ = '_';
	break;
      default:
	*p++ = *s;
	break;
      }
  *p++ = 0;

  return result;
}

typedef struct {
  GType	 type;
  gchar *ctype;
  gchar *set_func;
  gchar *get_func;
} MarshalType;
static MarshalType marshal_types[] = {
  { G_TYPE_NONE,      "void     ",	NULL,			NULL,			},
  { G_TYPE_STRING,    "gchar*   ",	"bsw_value_initset_string",	"bsw_proxy_collector_get_string", },
  { G_TYPE_INT,	      "gint     ",	"bsw_value_initset_int",	"g_value_get_int", },
  { G_TYPE_UINT,      "guint    ",	"bsw_value_initset_uint",	"g_value_get_uint", },
  { G_TYPE_ENUM,      "gint     ",	"bsw_value_initset_enum",	"g_value_get_enum", },
  { G_TYPE_BOOLEAN,   "gboolean ",	"bsw_value_initset_boolean",	"g_value_get_boolean", },
  { G_TYPE_CHAR,      "gchar    ",	"bsw_value_initset_char",	"g_value_get_char", },
  { G_TYPE_UCHAR,     "guchar   ",	"bsw_value_initset_uchar",	"g_value_get_uchar", },
  { G_TYPE_FLOAT,     "gfloat   ",	"bsw_value_initset_float",	"g_value_get_float", },
  { G_TYPE_DOUBLE,    "gdouble  ",	"bsw_value_initset_double",	"g_value_get_double", },
  { G_TYPE_OBJECT,    "BswProxy ",	"bsw_value_initset_proxy",	"(BswProxy) g_value_get_pointer", },
};

static MarshalType*
find_marshal_type (GType type)
{
  guint i;

  for (i = 0; i < sizeof (marshal_types) / sizeof (marshal_types[0]); i++)
    if (marshal_types[i].type == type)
      return marshal_types + i;
  for (i = 0; i < sizeof (marshal_types) / sizeof (marshal_types[0]); i++)
    if (marshal_types[i].type == G_TYPE_FUNDAMENTAL (type))
      return marshal_types + i;
  return NULL;
}

static const gchar*
marshal_type_name (GParamSpec *pspec,
		   gboolean    return_value)
{
  GType type = pspec->value_type;
  gchar *name = NULL;

  if (G_TYPE_IS_DERIVED (type))
    {
      switch (G_TYPE_FUNDAMENTAL (type))
	{
	case G_TYPE_ENUM:	name = g_type_name (type);
	case G_TYPE_FLAGS:	name = g_type_name (type);
	}
      if (name && name[0] == 'B' && name[1] == 's' && name[2] == 'e')
	{
	  name = g_strdup (name);
	  name[2] = 'w';
	}
    }
  else if (G_TYPE_FUNDAMENTAL (type) == G_TYPE_STRING)
    name = return_value ? "gchar*" : "const gchar *";
  return name ? name : find_marshal_type (type)->ctype;
}

static void
print_proc (GType              type,
	    BseProcedureClass *class)
{
  gchar *s, *cname = cfunc_from_proc_name (class->name);
  guint i, n;

  if (gen_body)
    {
      BseCategory *categories = bse_categories_from_type (type, &n);

      fprintf (f_out, "/* \"%s\" */\n", n ? categories[0].category : "?");
      g_free (categories);
    }

  s = g_strdup_printf ("%s %s_%s (",
		       class->n_out_params ? marshal_type_name (class->out_param_specs[0], TRUE) : "void",
		       prefix,
		       cname);
  n = strlen (s);
  fprintf (f_out, "%s", s);
  g_free (s);
  for (i = 0; i < class->n_in_params; i++)
    {
      add_type_wrapper (G_PARAM_SPEC_VALUE_TYPE (class->in_param_specs[i]));
      fprintf (f_out, "%s%s %s%s\n",
	       i ? indent (n) : "",
	       marshal_type_name (class->in_param_specs[i], FALSE),
	       cident_canonify (class->in_param_specs[i]->name),
	       i + 1 < class->n_in_params ? "," : gen_body ? ")" : ");");
    }
  if (!class->n_in_params)
    fprintf (f_out, "void%s\n", gen_body ? ")" : ");");
  if (class->n_out_params)
    add_type_wrapper (G_PARAM_SPEC_VALUE_TYPE (class->out_param_specs[0]));
  if (gen_body)
    {
      fprintf (f_out, "{\n");
      if (class->n_out_params)
	fprintf (f_out, "  %s result;\n", marshal_type_name (class->out_param_specs[0], TRUE));
      fprintf (f_out, "  BswProxyProcedureCall cl;\n");
      fprintf (f_out, "  GValue *value = cl.in_params;\n");
      for (i = 0; i < class->n_in_params; i++)
	fprintf (f_out, "  %s (value, %s, %s); value++;\n",
		 find_marshal_type (class->in_param_specs[i]->value_type)->set_func,
		 tmacro_from_type (class->in_param_specs[i]->value_type),
		 cident_canonify (class->in_param_specs[i]->name));
      fprintf (f_out, "  cl.n_in_params = value - cl.in_params;\n");
      if (class->n_out_params)
	fprintf (f_out, "  %s (&cl.out_param, %s, 0); value++;\n",
		 find_marshal_type (class->out_param_specs[0]->value_type)->set_func,
		 tmacro_from_type (class->out_param_specs[0]->value_type));
      fprintf (f_out, "  cl.proc_name = \"%s\";\n", class->name);
      fprintf (f_out, "  bsw_proxy_call_procedure (&cl);\n");
      if (class->n_out_params)
	{
	  fprintf (f_out, "  result = %s (&cl.out_param);\n",
		   find_marshal_type (class->out_param_specs[0]->value_type)->get_func);
	  fprintf (f_out, "  g_value_unset (&cl.out_param);\n");
	  fprintf (f_out, "  return result;\n");
	}
      fprintf (f_out, "}\n");
    }
  fputc ('\n', f_out);
  g_free (cname);
}

static void
print_procs (const gchar *pattern)
{
  BseCategory *categories;
  guint i, n_cats;

  categories = bse_categories_match_typed (pattern, BSE_TYPE_PROCEDURE, &n_cats);

  for (i = 0; i < n_cats; i++)
    {
      BseProcedureClass *class = g_type_class_ref (categories[i].type);
      guint j, can_wrap = TRUE;

      if (class->n_out_params > 1)
	{
	  g_message ("ignoring procedure `%s' with %u output args",
		     class->name, class->n_out_params);
	  can_wrap = FALSE;
	}
      for (j = 0; j < class->n_out_params; j++)
	if (!find_marshal_type (G_PARAM_SPEC_VALUE_TYPE (class->out_param_specs[j])))
	  {
	    g_message ("ignoring procedure `%s' with unwrappable output arg \"%s\" of type `%s'",
		       class->name,
		       class->out_param_specs[j]->name,
		       g_type_name (G_PARAM_SPEC_VALUE_TYPE (class->out_param_specs[j])));
	    can_wrap = FALSE;
	    break;
	  }
      for (j = 0; j < class->n_in_params; j++)
	if (!find_marshal_type (G_PARAM_SPEC_VALUE_TYPE (class->in_param_specs[j])))
	  {
	    g_message ("ignoring procedure `%s' with unwrappable input arg \"%s\" of type `%s'",
		       class->name,
		       class->in_param_specs[j]->name,
		       g_type_name (G_PARAM_SPEC_VALUE_TYPE (class->in_param_specs[j])));
	    can_wrap = FALSE;
	    break;
	  }
      if (can_wrap)
	print_proc (categories[i].type, class);
      g_type_class_unref (class);
    }
  g_free (categories);
}

static const gchar *cfile_header =
("#include \"bswproxy.h\"\n"
 "#include \"bswgenapi.h\"\n"
 "#include <bse/bse.h>\n"
 "#define bsw_value_initset_string(val,t,string)  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_string ((val), (string)); }\n"
 "#define bsw_value_initset_int(val,t,vint)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_int ((val), (vint)); }\n"
 "#define bsw_value_initset_uint(val,t,vuint)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_uint ((val), (vuint)); }\n"
 "#define bsw_value_initset_enum(val,t,vuint)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_enum ((val), (vuint)); }\n"
 "#define bsw_value_initset_boolean(val,t,vbool)  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_boolean ((val), (vbool)); }\n"
 "#define bsw_value_initset_char(val,t,vchar)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_char ((val), (vchar)); }\n"
 "#define bsw_value_initset_uchar(val,t,vuchar)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_uchar ((val), (vuchar)); }\n"
 "#define bsw_value_initset_float(val,t,vfloat)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_uchar ((val), (vfloat)); }\n"
 "#define bsw_value_initset_double(val,t,vdouble) { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_uchar ((val), (vdouble)); }\n"
 "#define bsw_value_initset_proxy(val,t,vproxy)	  { (val)->g_type = 0; g_value_init ((val), BSW_TYPE_PROXY); g_value_set_pointer ((val), (gpointer) (vproxy)); }\n"
 );

int
main (gint   argc,
      gchar *argv[])
{
  static gint help (gchar *arg);
  guint i;
  
  f_out = stdout;
  
  g_thread_init (NULL);

  bse_init (&argc, &argv, NULL);

  type_wrapper_ht = g_hash_table_new (NULL, NULL);

  for (i = 1; i < argc; i++)
    {
      if (strcmp ("--header", argv[i]) == 0)
	{
	  gen_header = TRUE;
	}
      else if (strcmp ("--body", argv[i]) == 0)
	{
	  gen_body = TRUE;
	}
      else if (strcmp ("-h", argv[i]) == 0 ||
	  strcmp ("--help", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	return help (argv[i]);
    }

  if (gen_body)
    fprintf (f_out, "%s\n", cfile_header);
  if (gen_header)
    print_enums ();
  print_procs ("*");
  if (gen_header)
    print_type_wrappers ();
  
  return 0;
}

static gint
help (gchar *arg)
{
  fprintf (stderr, "usage: mkapi <qualifier> [-r <type>] [-{i|b} \"\"] [-s #] [-{h|x|y}]\n");
  fprintf (stderr, "       -h       guess what ;)\n");

  return arg != NULL;
}
